use std::io;
use std::os::unix::net::UnixStream;
use std::path::Path;
use std::thread::sleep;
use std::time::Duration;

use qapi::qmp::{Event, JobStatus};
use qapi::{qmp, Command, Qmp};

const SNAPSHOT_TAG: &str = "qce";
const SNAPSHOT_JOB_PREFIX: &str = "qce_job_";

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
                    JobStatus::created | JobStatus::running => (),
                    JobStatus::aborting => {
                        aborted = true;
                    }
                    JobStatus::concluded => break 'outer,
                    status => {
                        return Err(io::Error::new(
                            io::ErrorKind::Other,
                            format!("unexpected job status: {:?}", status),
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

pub fn snapshot_save(socket: &Path, job_index: usize) -> io::Result<()> {
    let job_id = format!("{}{}", SNAPSHOT_JOB_PREFIX, job_index);
    exec_async(
        socket,
        qmp::snapshot_save {
            tag: SNAPSHOT_TAG.to_string(),
            job_id: job_id.clone(),
            vmstate: "".to_string(),
            devices: vec![],
        },
        &job_id,
    )
}
