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

    pub fn snapshot_save(&mut self) -> io::Result<()> {
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
        self.wait_for_async_job(&job_id)
    }

    pub fn reset(&mut self) -> io::Result<()> {
        self.qmp.execute(&qmp::system_reset {})?;
        loop {
            match self.check_guest_reset()? {
                None => continue,
                Some(VMExitMode::Host) => return Ok(()),
                Some(VMExitMode::Hard) | Some(VMExitMode::Soft) => {
                    return Err(io::Error::new(
                        io::ErrorKind::Other,
                        "unexpected VM exit mode".to_string(),
                    ));
                }
            }
        }
    }

    pub fn check_guest_reset(&mut self) -> io::Result<Option<VMExitMode>> {
        self.qmp.nop()?;
        for event in self.qmp.events() {
            match &event {
                Event::STOP { .. } => return Ok(Some(VMExitMode::Host)),
                Event::SHUTDOWN { data, .. } => {
                    let mode = if !data.guest {
                        VMExitMode::Host
                    } else {
                        VMExitMode::Hard
                    };
                    return Ok(Some(mode));
                }
                Event::POWERDOWN { .. } => {
                    return Ok(Some(VMExitMode::Host));
                }
                Event::RESET { data, .. } => {
                    let mode = if !data.guest {
                        VMExitMode::Host
                    } else {
                        VMExitMode::Soft
                    };
                    return Ok(Some(mode));
                }
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
        Ok(None)
    }

    #[allow(dead_code)]
    fn probe_for_events(&mut self) -> io::Result<()> {
        loop {
            self.qmp.nop()?;
            for event in self.qmp.events() {
                eprintln!("event: {:?}", event);
            }
        }
    }
}

pub enum VMExitMode {
    Soft,
    Hard,
    Host,
}
