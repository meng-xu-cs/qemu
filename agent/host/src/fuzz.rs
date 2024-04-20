use std::collections::{BTreeMap, BTreeSet};
use std::fs::{File, OpenOptions};
use std::hash::Hasher;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::{fs, io};

use log::info;
use twox_hash::XxHash64;

/// XXHash seed synchronized with QEMU
const QEMU_XXHASH_SEED: u64 = 1;

type CovHash = u64;
type CovTrace = Vec<u64>;
type CovDatabase = BTreeMap<u64, BTreeMap<CovHash, BTreeSet<CovTrace>>>;
type TestID = usize;

/// Filename for the aggregated coverage
const CORPUS_FILE_COVERAGE: &str = "total_cov";
/// Dirname for the seed queue
const CORPUS_DIR_QUEUE: &str = "queue";
/// Dirname for seeds executed
const CORPUS_DIR_TRIED: &str = "tried";

/// Fuzzing controller
pub struct Fuzzer {
    /// path to the coverage file
    path_corpus_cov: PathBuf,
    /// path to the corpus/tried directory
    path_corpus_dir_tried: PathBuf,
    /// path to the corpus/queue directory
    path_corpus_dir_queue: PathBuf,
    /// path to the output directory
    path_output: PathBuf,

    /// seed cursor
    seed_cursor: usize,
    /// seed counter
    seed_counter: usize,

    /// coverage map
    coverage: CovDatabase,

    /// counter for the current fuzzing session
    session_counter: usize,
}

impl Fuzzer {
    pub fn new(path_corpus: PathBuf, path_output: PathBuf) -> io::Result<Self> {
        // prepare the seeds
        let path_tried = path_corpus.join(CORPUS_DIR_TRIED);
        if !path_tried.exists() {
            fs::create_dir(&path_tried)?;
        }
        let tried = Self::analyze_corpus_dir(&path_tried)?;

        let path_queue = path_corpus.join(CORPUS_DIR_QUEUE);
        if !path_queue.exists() {
            fs::create_dir(&path_queue)?;
        }
        let queue = Self::analyze_corpus_dir(&path_queue)?;

        // check 1: the two seed sets don't overlap
        if !tried.is_disjoint(&queue) {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                "overlapping seeds in ended and queue".to_string(),
            ));
        }

        // check 2: seeds are continuous
        let seed_cursor = tried.len();
        let mut seed_counter = tried.len() + queue.len();
        for i in 0..seed_cursor {
            if !tried.contains(&i) {
                return Err(io::Error::new(
                    io::ErrorKind::Other,
                    format!("missing seed #{} in tried", i),
                ));
            }
        }
        for i in seed_cursor..seed_counter {
            if !queue.contains(&i) {
                return Err(io::Error::new(
                    io::ErrorKind::Other,
                    format!("missing seed #{} in queue", i),
                ));
            }
        }
        info!(
            "found {} seeds in total with cursor at {}",
            seed_counter, seed_cursor
        );

        // deposit an initial seed if there is nothing in the queue
        if seed_counter == 0 {
            fs::write(path_queue.join("0"), "X")?;
            seed_counter += 1;
        }

        // load the coverage
        let path_cov = path_corpus.join(CORPUS_FILE_COVERAGE);
        let coverage = Self::load_coverage(&path_cov)?;
        let instance = Self {
            path_corpus_cov: path_cov,
            path_corpus_dir_tried: path_tried,
            path_corpus_dir_queue: path_queue,
            path_output,
            seed_cursor,
            seed_counter,
            coverage,
            session_counter: 0,
        };
        Ok(instance)
    }

    fn analyze_corpus_dir(path: &Path) -> io::Result<BTreeSet<TestID>> {
        let mut seeds = BTreeSet::new();
        for item in fs::read_dir(path)? {
            let item = item?;
            if !item.file_type()?.is_file() {
                return Err(io::Error::new(
                    io::ErrorKind::Other,
                    "expect only files in corpus directories".to_string(),
                ));
            }
            let id = item
                .file_name()
                .into_string()
                .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "non-ascii filename"))?
                .parse::<TestID>()
                .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "non-number filename"))?;
            seeds.insert(id);
        }
        Ok(seeds)
    }

    fn load_coverage(path: &Path) -> io::Result<CovDatabase> {
        let mut cov = CovDatabase::new();
        if !path.exists() {
            // dump an empty file here as this is needed by the guest
            OpenOptions::new().create(true).write(true).open(path)?;
            return Ok(cov);
        }

        let raw_bytes = fs::read(path)?;
        unsafe {
            let mut cursor = raw_bytes.as_ptr() as *const u64;
            let num_sizes = *cursor;
            cursor = cursor.add(1);

            for len in 1..=num_sizes {
                let l1 = cov.entry(len).or_default();

                let num_hashes = *cursor;
                cursor = cursor.add(1);
                for _ in 0..num_hashes {
                    let hash = *cursor;
                    cursor = cursor.add(1);
                    let l2 = l1.entry(hash).or_default();

                    let num_traces = *cursor;
                    cursor = cursor.add(1);
                    for _ in 0..num_traces {
                        let mut trace = CovTrace::new();
                        for _ in 0..len {
                            trace.push(*cursor);
                            cursor = cursor.add(1);
                        }
                        l2.insert(trace);
                    }
                }
            }

            // sanity check
            let offset = cursor.offset_from(raw_bytes.as_ptr() as *const u64) as usize;
            assert_eq!(offset * size_of::<u64>(), raw_bytes.len());
        }

        // done with the construction
        Ok(cov)
    }

    fn save_coverage(path: &Path, cov: &CovDatabase) -> io::Result<()> {
        let mut counter = 0;
        let mut file = File::create(path)?;
        file.write_all(&(cov.len() as u64).to_ne_bytes())?;
        for (_, l1) in cov {
            file.write_all(&(l1.len() as u64).to_ne_bytes())?;
            for (hash, l2) in l1 {
                file.write_all(&hash.to_ne_bytes())?;
                file.write_all(&(l2.len() as u64).to_ne_bytes())?;
                for trace in l2 {
                    for val in trace {
                        file.write_all(&val.to_ne_bytes())?;
                    }
                }
                counter += l2.len();
            }
        }
        file.flush()?;
        info!("traces saved into coverage database: {}", counter);
        Ok(())
    }

    fn merge_session_coverage(&mut self, path_session: &Path) -> io::Result<()> {
        // load the coverage
        let path_cov = path_session.join("cov");
        let cov_raw = fs::read(path_cov)?;

        // short-circuit if there is no coverage information generated
        if cov_raw.len() == 0 {
            let cov_hash = XxHash64::with_seed(QEMU_XXHASH_SEED).finish();
            info!("guest coverage hash: {:#016x} (no coverage)", cov_hash);
            return Ok(());
        }

        // parse the coverage
        let cov_len = cov_raw.len() / size_of::<u64>();
        if cov_len * size_of::<u64>() != cov_raw.len() {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                format!("invalid length of coverage trace {}", cov_raw.len()),
            ));
        }
        let cov = unsafe {
            let cov_ptr = cov_raw.as_ptr() as *const u64;
            std::slice::from_raw_parts(cov_ptr, cov_len)
        };

        // hash the coverage trace as well as step-wise value
        let mut tracer = CovTrace::new();
        let mut hasher = XxHash64::with_seed(QEMU_XXHASH_SEED);
        for (i, val) in cov.iter().enumerate() {
            hasher.write_u64(*val);
            tracer.push(*val);

            let step_hash = hasher.clone().finish();
            self.coverage
                .entry(i as u64 + 1)
                .or_default()
                .entry(step_hash)
                .or_default()
                .insert(tracer.clone());
        }
        let cov_hash = hasher.finish();
        info!("guest coverage hash: {:#016x}", cov_hash);

        // save the updated coverage
        Self::save_coverage(&self.path_corpus_cov, &self.coverage)?;

        // done
        Ok(())
    }

    fn merge_session_seeds(&mut self, path_session: &Path) -> io::Result<()> {
        // copy over new seeds generated
        let old_seed_counter = self.seed_counter;
        let path_seeds = path_session.join("seeds");
        for item in fs::read_dir(&path_seeds)? {
            let item = item?;
            fs::copy(
                item.path(),
                self.path_corpus_dir_queue
                    .join(self.seed_counter.to_string()),
            )?;
            self.seed_counter += 1;
        }
        info!("seeds enqueued: {}", self.seed_counter - old_seed_counter);

        // mark the old seed as done
        let seed_name = self.seed_cursor.to_string();
        fs::rename(
            self.path_corpus_dir_queue.join(&seed_name),
            self.path_corpus_dir_tried.join(&seed_name),
        )?;
        self.seed_cursor += 1;

        // done
        Ok(())
    }

    pub fn process_session_result(&mut self) -> io::Result<()> {
        let path_session = self.path_output.join(self.session_counter.to_string());
        self.merge_session_coverage(&path_session)?;
        self.merge_session_seeds(&path_session)?;
        Ok(())
    }

    pub fn has_pending_seeds(&self) -> bool {
        self.seed_cursor != self.seed_counter
    }

    pub fn current_seed(&self) -> io::Result<Vec<u8>> {
        fs::read(
            self.path_corpus_dir_queue
                .join(&self.seed_cursor.to_string()),
        )
    }

    pub fn next_session(&mut self) {
        self.session_counter += 1;
    }
}
