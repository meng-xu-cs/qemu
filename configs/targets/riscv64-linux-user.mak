TARGET_ARCH=riscv64
TARGET_BASE_ARCH=riscv
TARGET_ABI_DIR=riscv
TARGET_XML_FILES= gdb-xml/riscv-64bit-cpu.xml gdb-xml/riscv-32bit-fpu.xml gdb-xml/riscv-64bit-fpu.xml gdb-xml/riscv-64bit-virtual.xml
CONFIG_SEMIHOSTING=y
CONFIG_ARM_COMPATIBLE_SEMIHOSTING=y
TARGET_SYSTBL_ABI=64
TARGET_SYSTBL_ABI=common,64,riscv,rlimit,memfd_secret
TARGET_SYSTBL=syscall.tbl
TARGET_LONG_BITS=64
