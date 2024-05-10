use std::io;
use std::io::BufReader;
use std::os::unix::net::UnixStream;

use qapi::qmp::{Event, JobStatus};
use qapi::{qmp, Qmp, Stream};

const SNAPSHOT_TAG: &str = "qce";
const SNAPSHOT_JOB_PREFIX: &str = "qce_job_";
const SNAPSHOT_DISK: &str = "disk0";

pub struct QemuProxy<'a> {
    qmp: Qmp<Stream<BufReader<&'a UnixStream>, &'a UnixStream>>,
    job_count: usize,
}

impl<'a> QemuProxy<'a> {
    pub fn new(stream: &'a UnixStream) -> io::Result<Self> {
        let mut qmp = Qmp::from_stream(stream);
        qmp.handshake()?;
        Ok(Self { qmp, job_count: 0 })
    }

    fn next_job_id(&mut self) -> String {
        let job_id = format!("{}{}", SNAPSHOT_JOB_PREFIX, self.job_count);
        self.job_count += 1;
        job_id
    }

    fn wait_for_async_job(&mut self, job_id: &str) -> io::Result<()> {
        let mut aborted = false;

        'outer: loop {
            self.qmp.nop()?;
            for event in self.qmp.events() {
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
        }

        // early return if job completed successfully
        if !aborted {
            return Ok(());
        }

        // probe for a reason
        let mut reason = None;
        for item in self.qmp.execute(&qmp::query_jobs {})? {
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

    fn wait_for_status_change<F: Fn(Event) -> bool>(&mut self, predicate: F) -> io::Result<()> {
        loop {
            self.qmp.nop()?;
            for event in self.qmp.events() {
                if predicate(event) {
                    return Ok(());
                }
            }
        }
    }

    fn vm_stop(&mut self) -> io::Result<()> {
        self.qmp.execute(&qmp::stop {})?;
        self.wait_for_status_change(|event| matches!(event, Event::STOP { .. }))
    }

    fn vm_cont(&mut self) -> io::Result<()> {
        self.qmp.execute(&qmp::cont {})?;
        self.wait_for_status_change(|event| matches!(event, Event::RESUME { .. }))
    }

    pub fn snapshot_save(&mut self) -> io::Result<()> {
        self.vm_stop()?;

        let job_id = self.next_job_id();
        self.qmp.execute(&qmp::snapshot_save {
            tag: SNAPSHOT_TAG.to_string(),
            job_id: job_id.clone(),
            vmstate: SNAPSHOT_DISK.to_string(),
            devices: vec![SNAPSHOT_DISK.to_string()],
        })?;
        self.wait_for_async_job(&job_id)
    }

    pub fn snapshot_load(&mut self) -> io::Result<()> {
        let job_id = self.next_job_id();
        self.qmp.execute(&qmp::snapshot_load {
            tag: SNAPSHOT_TAG.to_string(),
            job_id: job_id.clone(),
            vmstate: SNAPSHOT_DISK.to_string(),
            devices: vec![SNAPSHOT_DISK.to_string()],
        })?;
        self.wait_for_async_job(&job_id)?;

        self.vm_cont()
    }

    pub fn wait_for_guest_finish(&mut self) -> io::Result<VMExitMode> {
        let mut mode = VMExitMode::Soft;
        loop {
            self.qmp.nop()?;
            for event in self.qmp.events() {
                match &event {
                    Event::STOP { .. } => return Ok(mode),
                    Event::SHUTDOWN { data, .. } => {
                        if !data.guest {
                            mode = VMExitMode::Hard;
                        }
                    }
                    Event::POWERDOWN { .. } => {
                        mode = VMExitMode::Hard;
                    }
                    Event::RESET { data, .. } => {
                        if !data.guest {
                            mode = VMExitMode::Hard;
                        }
                    }
                    Event::GUEST_PANICKED { .. } => {}
                    Event::MEMORY_FAILURE { .. }
                    | Event::BLOCK_IMAGE_CORRUPTED { .. }
                    | Event::BLOCK_IO_ERROR { .. }
                    | Event::BLOCK_JOB_ERROR { .. } => {
                        return Err(io::Error::new(
                            io::ErrorKind::Other,
                            format!("unexpected error detected in event stream: {:?}", event),
                        ));
                    }
                    _ => (),
                }
            }
        }
    }
}

pub enum VMExitMode {
    Soft,
    Hard,
}
