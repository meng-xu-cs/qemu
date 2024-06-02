#include "utils.h"

#define PATH_HARNESS "/root/harness"
#define PATH_BLOB "/root/blob"

struct ivshmem pack;
uint64_t *kcov_data;

void __attribute__((constructor)) guest_agent_init(void) {
  // fuzzing mode
  checked_write_or_create(PATH_BLOB, "X", 1);

  // connect to the ivshmem device
  probe_ivshmem(&pack, IVSHMEM_SIZE);
  LOG_INFO("ivshmem ready");

  // check and enable kcov
  int kcov = open("/sys/kernel/debug/kcov", O_RDWR);
  if (kcov == -1) {
    ABORT_WITH_ERRNO("open kcov");
  }
  if (ioctl(kcov, KCOV_INIT_TRACE64, KCOV_COVER_SIZE)) {
    ABORT_WITH_ERRNO("ioctl init kcov");
  }

  kcov_data = mmap(NULL, KCOV_COVER_SIZE * sizeof(kcov_data[0]),
                   PROT_READ | PROT_WRITE, MAP_SHARED, kcov, 0);
  if (kcov_data == MAP_FAILED) {
    ABORT_WITH_ERRNO("mmap kcov");
  }

  if (ioctl(kcov, KCOV_ENABLE, KCOV_TRACE_PC)) {
    ABORT_WITH_ERRNO("ioctl enable kcov");
  }
  close(kcov);
  LOG_INFO("kcov ready");

  // wait for host to be ready
  struct vmio *vmio = pack.addr;
  while (atomic_load(&vmio->flag) == 0) {
    // do nothing, this (unexpected) busy waiting is intentional
  }

  atomic_store(&vmio->spin_guest, 1);
  atomic_store(&vmio->spin_host, 0);
  LOG_INFO("notified host on ready");

  // wait for host to release us
  while (atomic_load(&vmio->spin_guest)) {
    // busy wait
  }
  LOG_INFO("operation resumed by host");

  if (vmio->size > 0) {
    // get the blob from the host
    checked_write_or_create(PATH_BLOB, vmio->buf, vmio->size);
    LOG_INFO("get blob from host");

    // debug
    LOG_INFO("blob size: %lu", vmio->size);
    for (int i = 0; i < vmio->size; i++) {
      // LOG_INFO("blob[%d]: %c", i, vmio->buf[i]);
    }
  }
  LOG_INFO("blob ready");

  

  // start kcov
  __atomic_store_n(&kcov_data[0], 0, __ATOMIC_RELAXED);
}

void __attribute__((destructor)) guest_agent_fini(void) {
  struct vmio *vmio = pack.addr;

  // get kcov data & send kcov data to the host
  uint64_t ncov = __atomic_load_n(&kcov_data[0], __ATOMIC_RELAXED);
  LOG_INFO("kcov data length: %lu", ncov);
  if (ncov >= KCOV_COVER_SIZE) {
    ABORT_WITH("too much kcov entries");
  }
  LOG_INFO("store kcov len");

  atomic_store(&vmio->size, ncov * 8);
  LOG_INFO("store kcov data");
  for (uint64_t i = 0; i < ncov; i++) {
    uint64_t pc = __atomic_load_n(&kcov_data[i + 1], __ATOMIC_RELAXED);
    atomic_store((uint64_t*)(&vmio->buf[i * 8]), pc);
    // LOG_INFO("kcov data[%llu]: %llu", i, pc);
  }

  // debug: test panic
  // exit(1);

  // let the host know we are done
  atomic_store(&vmio->spin_guest, 2);

  // unmap the memory as clean up
  unmap_ivshmem(&pack);
}
