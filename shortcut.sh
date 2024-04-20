#!/bin/bash

./run.py linux \
    /work0 \
    --kvm \
    --harness /work0/src/test_harnesses/linux_test_harness.c \
    --blob /work0/exemplar_only/blobs/sample_solve.bin
