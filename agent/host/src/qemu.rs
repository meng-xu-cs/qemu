use std::io;
use std::os::unix::net::UnixStream;
use std::path::Path;
use std::thread::sleep;
use std::time::Duration;

use qapi::qmp::{Event, JobStatus};
use qapi::{qmp, Qmp};

const SNAPSHOT_TAG: &str = "qce";
const SNAPSHOT_JOB: &str = "qce_save";

pub fn snapshot_save(socket: &Path) -> io::Result<()> {
    let stream = UnixStream::connect(socket)?;
    let mut qmp = Qmp::from_stream(&stream);
    qmp.handshake()?;

    qmp.execute(&qmp::snapshot_save {
        tag: SNAPSHOT_TAG.to_string(),
        job_id: SNAPSHOT_JOB.to_string(),
        vmstate: "".to_string(),
        devices: vec![],
    })?;

    loop {
        qmp.nop()?;
        for event in qmp.events() {
            if let Event::JOB_STATUS_CHANGE { data, timestamp: _ } = event {
                if data.id != SNAPSHOT_JOB {
                    continue;
                }
                match data.status {
                    JobStatus::created | JobStatus::running => (),
                    JobStatus::aborting => {
                        return Err(io::Error::new(io::ErrorKind::Other, "job aborted"));
                    }
                    JobStatus::concluded => break,
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
}
