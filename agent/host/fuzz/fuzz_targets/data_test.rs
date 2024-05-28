#![no_main]

use libfuzzer_sys::{fuzz_mutator, fuzz_target, fuzzz_crossover};

extern "C" { pub fn SubmitLibfuzzerCoverage(pc: u64); } 
extern "C" { pub fn ProtoFuzzerInputOne(data: *const u8, size: usize, out: *mut u8, max_out_size: usize) -> usize; }
extern "C" { pub fn ProtoFuzzerCustomMutator(data: *mut u8, size: usize, max_size: usize, seed: u32) -> usize; }
extern "C" { pub fn ProtoFuzzerCustomCrossover(data1: *const u8, size1: usize, data2: *const u8, size2: usize, out: *mut u8, max_out_size: usize, seed: u32) -> usize; }

fuzz_target!(|data: &[u8]| {
    // output fuzz data
    let mut out = vec![0u8; 1048576];
    let out_size = unsafe { ProtoFuzzerInputOne(data.as_ptr(), data.len(), out.as_mut_ptr(), out.len()) };
    // println!("out_size: {}", out_size);
    // println!("out: {:?}", &out[..out_size]);
    // submit coverage
    unsafe { SubmitLibfuzzerCoverage(out_size as u64); }
});

fuzz_mutator!(
    |data: &mut [u8], size: usize, max_size: usize, _seed: u32| {
        unsafe {
            return ProtoFuzzerCustomMutator(data.as_mut_ptr(), size, max_size, _seed);
        }
    }
);

fuzz_crossover!(|data1: &[u8], data2: &[u8], out: &mut [u8], seed: u32| {
    unsafe {
        return ProtoFuzzerCustomCrossover(data1.as_ptr(), data1.len(), data2.as_ptr(), data2.len(), out.as_mut_ptr(), out.len(), seed);
    }
});
