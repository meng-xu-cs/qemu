# QCE Executor

## Instructions

- Build the exemplar (see the exemplar README)

Assume the exemplar is available at `<path-challenge>`

- Build the Docker

```bash
# (host)
./run.py docker build
# alternatively, ./run.py docker build --force will do a clean rebuild
```

- Get inside the Docker environment

```bash
# (host)
./run.py docker shell -v <path-challenge>
```

The rest of the steps happens inside the Docker shell

- Build QEMU

```bash
# (docker)
cd /src
./run.py build
# alternatively, ./run.py build --release for a release build
```

- Option 1: Run the challenge with the solution

```bash
# (docker)
./run.py linux --kvm --verbose \
  --kernel /work0/src/arch/x86_64/boot/bzImage \
  --harness /work0/src/test_harness/linux_test_harness.c \
  --blob /work0/exemplar_only/blobs/sample_solve.bin
```

Alternative, there is a shortcut command for this
without going through the Docker environment.

```bash
# (host)
./run.py dev -v <path-challenge> --kvm --solution
```

- Option 2: Run the challenge in fuzzing mode

```bash
# (docker)
./run.py linux --kvm --verbose \
  --kernel /work0/src/arch/x86_64/boot/bzImage \
  --harness /work0/src/test_harness/linux_test_harness.c
```

Alternative, there is a shortcut command for this
without going through the Docker environment.

```bash
# (host)
./run.py dev -v <path-challenge> --kvm
```

## Other Options

By default, QCE uses a minimal root filesystem based on busybox-static.
The exemplar uses [virtme-ng](https://github.com/arighi/virtme-ng)
which shares the host rootfs to the guest VM (read-only).

To simulate this behavior, append the `--virtme` flag to these commands.
For example:

```bash
# (docker)
./run.py linux --kvm --virtme --verbose \
  --kernel /work0/src/arch/x86_64/boot/bzImage \
  --harness /work0/src/test_harness/linux_test_harness.c \
  --blob /work0/exemplar_only/blobs/sample_solve.bin
```

or

```bash
# (host)
./run.py dev -v <path-challenge> --kvm --virtme --solution
```

## Internal Design for VM Snapshot and Reload

NOTE: this is only applicable in fuzzing mode,
i.e., with `--harness` specified but without `--blob` specified.

```text
[Process 1] Host Agent starts

[Process 2] Guest VM kernel boots
  --> /init
    --> Guest Agent `main()`
      --> Prepare environment
      --> Inform Host Agent on ready (via ivshmem)
      --> Busy wait

[Process 1] Host Agent receives guest ready signal
  --> Ask QEMU monitor to take a VM snapshot
  --> Wait for guest VM to stop (by querying QEMU monitor)

<snapshot-reloading-point>
[Process 2] Guest VM
      --> Busy wait, then resumed by Host Agnet (via ivshmem)
      --> <prepare blob !! logic not implemented>
      --> `execve(harness)`
      --> crash or finish (which causes killing /init, again a crash)

[Process 1] Host Agent receives guest VM stopped signal
  --> <prepare blob !! logic not implemented>
  --> Ask QEMU monitor to reload a VM snapshot
  --> Resume the guest VM

Go back to the <snapshot-reloading-point>
```

- For /init script, see `mk_initramfs()` in `utils.py`
- For internals of the Guest Agent, see `agent/guest/guest.c`
- For internals of the Host Agent, see `agent/host/Cargo.toml`

## Concolic Execution Mode

Dropping the `--kvm` option will enter concolic execution mode
which is under development currently. Please DO NOT try this.
