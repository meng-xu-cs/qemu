use std::io;
use std::os::unix::net::UnixStream;
use std::path::{Path, PathBuf};
use std::thread::sleep;
use std::time::Duration;

use log::error;
use qapi::qmp::{Event, JobStatus};
use qapi::{qmp, Command, Qmp};

const SNAPSHOT_TAG: &str = "qce";
const SNAPSHOT_JOB_PREFIX: &str = "qce_job_";
const SNAPSHOT_DISK: &str = "disk0";

fn exec_sync<C: Command>(socket: &Path, command: C) -> io::Result<Event> {
    let stream = UnixStream::connect(socket)?;
    let mut qmp = Qmp::from_stream(&stream);
    qmp.handshake()?;

    // send the command
    qmp.execute(&command)?;

    // wait for result
    let events: Vec<_> = qmp.events().collect();
    if events.len() != 1 {
        for event in events {
            error!("unexpected event {:?}", event);
        }
        return Err(io::Error::new(
            io::ErrorKind::Other,
            "expect one and only one event for a command in sync mode".to_string(),
        ));
    }
    Ok(events.into_iter().next().unwrap())
}

fn exec_async<C: Command>(socket: &Path, command: C, job_id: &str) -> io::Result<()> {
    let stream = UnixStream::connect(socket)?;
    let mut qmp = Qmp::from_stream(&stream);
    qmp.handshake()?;

    // send the command
    qmp.execute(&command)?;

    // wait for result
    let mut aborted = false;
    'outer: loop {
        qmp.nop()?;
        for event in qmp.events() {
            if let Event::JOB_STATUS_CHANGE { data, timestamp: _ } = event {
                if data.id != job_id {
                    continue;
                }
                match data.status {
                    JobStatus::created
                    | JobStatus::ready
                    | JobStatus::running
                    | JobStatus::waiting
                    | JobStatus::pending => (),
                    JobStatus::aborting => {
                        aborted = true;
                    }
                    JobStatus::concluded => break 'outer,
                    JobStatus::standby
                    | JobStatus::null
                    | JobStatus::undefined
                    | JobStatus::paused => {
                        return Err(io::Error::new(
                            io::ErrorKind::Other,
                            format!("unexpected job status: {:?}", data.status),
                        ));
                    }
                }
            }
        }
        sleep(Duration::from_millis(1));
    }

    // early return if job completed successfully
    if !aborted {
        return Ok(());
    }

    // probe for a reason
    let mut reason = None;
    for item in qmp.execute(&qmp::query_jobs {})? {
        if item.id != job_id {
            continue;
        }
        let error = item
            .error
            .unwrap_or_else(|| "aborted without an error message".to_string());
        reason = Some(error);
        break;
    }

    Err(io::Error::new(
        io::ErrorKind::Other,
        format!(
            "job aborted: {}",
            reason.unwrap_or_else(|| "unable to probe for a reason".to_string())
        ),
    ))
}

pub struct QemuProxy {
    socket: PathBuf,
    job_count: usize,
}

impl QemuProxy {
    pub fn new(socket: PathBuf) -> Self {
        Self {
            socket,
            job_count: 0,
        }
    }

    fn next_job_id(&mut self) -> String {
        let job_id = format!("{}{}", SNAPSHOT_JOB_PREFIX, self.job_count);
        self.job_count += 1;
        job_id
    }

    fn vm_stop(&mut self) -> io::Result<()> {
        match exec_sync(&self.socket, qmp::stop {})? {
            Event::STOP { .. } => Ok(()),
            e => Err(io::Error::new(
                io::ErrorKind::Other,
                format!("unexpected event: {:?}", e),
            )),
        }
    }

    fn vm_cont(&mut self) -> io::Result<()> {
        match exec_sync(&self.socket, qmp::cont {})? {
            Event::RESUME { .. } => Ok(()),
            e => Err(io::Error::new(
                io::ErrorKind::Other,
                format!("unexpected event: {:?}", e),
            )),
        }
    }

    pub fn snapshot_save(&mut self) -> io::Result<()> {
        self.vm_stop()?;

        let job_id = self.next_job_id();
        exec_async(
            &self.socket,
            qmp::snapshot_save {
                tag: SNAPSHOT_TAG.to_string(),
                job_id: job_id.clone(),
                vmstate: SNAPSHOT_DISK.to_string(),
                devices: vec![SNAPSHOT_DISK.to_string()],
            },
            &job_id,
        )?;

        self.vm_cont()
    }
}
