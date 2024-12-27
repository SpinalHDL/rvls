# Introduction

RVLS (Risc-V Lock Step) is a CPU simulation trace checker.
- Typical usage is to check that a simulated CPU system is behaving right
- Has a human-readable text frontend to feed the traces
- Can be directly integrated into a C++ sim for direct checking
- Has a Java JNI frontend for its integration in a SpinalHDL / Chisel based testbench
- Support multi-core systems, by tracking memory coherency status across CPUs
- Use Spike's "proc" as golden reference model
- Use a lightly modified Spike version to provide more info and allow coherency checks

See [example/simple/trace.log](example/simple/trace.log) for an example of ASCII trace which can be checked by RVLS

RVLS is used to check the behavior of multicore NaxRiscv.
NaxRiscv use a Write-Back L1 data cache, with tilelink to provide memory coherency between the cores, using a MESI protocol (https://en.wikipedia.org/wiki/MESI_protocol)

Not everything is strictly kept in sync, noticeably, it assumes that :
- The software and fence.i  keep the hardware L1 instruction cache coherent
- The software and sfence keep the hardware MMU TLB coherent

# How to use

```shell
build/apps/rvls -f example/simple/trace.log
```

# Dependencies

```shell
sudo apt-get install device-tree-compiler libboost-all-dev

# Install ELFIO, used to load elf file in the sim 
git clone https://github.com/serge1/ELFIO.git
cd ELFIO
git checkout d251da09a07dff40af0b63b8f6c8ae71d2d1938d # Avoid C++17
sudo cp -R elfio /usr/include
```

# How to compile

```shell
git clone https://github.com/SpinalHDL/rvls.git
git clone https://github.com/SpinalHDL/riscv-isa-sim.git --recursive

# Compile riscv-isa-sim (spike), used as a golden model during the sim to check the dut behavior (lock-step)
cd riscv-isa-sim
mkdir build
cd build
# Optionally add CFLAGS='-g -O0' CXXFLAGS='-g -O0'
../configure --prefix=$RISCV --enable-commitlog  --without-boost --without-boost-asio --without-boost-regex
make -j$(nproc)
cd ../..

# Compile RVLS
cd rvls
make -j$(nproc)

# Demo
build/apps/rvls -f example/simple/trace.log --spike-debug --spike-log
head -10 spike.log
```
   
# JNI frontend

You can find the Java JNI interface in the [bindings/jni/rvls/jni/Frontend.java](bindings/jni/rvls/jni/Frontend.java) folder. It works in a very similar to the ASCII frontend excepted for the followings : 
- Commands a provided via JNI calls (no file involved)
- Allows to check the behavior of the SoC during the simulation itself (lock-step)

# ASCII frontend

There is a ASCII based frontend which can be used to feed the CPUs execution traces.
It consists into the simple lines of commands described bellow. 

See [example/simple/trace.log](example/simple/trace.log) for an example of trace.

## General commands

`time $value`
- Used to provide some sporadic timestamp, just for debug purposes

## Memory commands

`elf load $offset_hex $path`

`bin load $offset_hex $path`

`memview new $memoryViewId $readIds $writeIds`
- Create a new memory view
- A memory view provide a representation of how a given memory master (ex CPU) observe the global memory content/ordering
- readIds and writeIds represent the number of outstanding load/store that the CPU can have at most (LQ/SQ size)


## RISC-V commands

There are mostly 3 kinds of RISC-V related commands :
- The generals ones, to create a CPU / commit / trap
- The ones to trace a register file read / write
- The ones which are related to memory load / stores

### general commands

`rv new $hartId $isa $priv $physWidth $memoryViewId`
- Create a new CPU
- isa follow Spike, ex : RV32IMA
- priv follow Spike, ex : MSU
- physWidth specify the physical address memory width (32 bits max for now)
- memoryViewId specify which memory view will be used by the CPU to do load/store

`"rv region add $hartId $kind $base_hex $size_hex")`
- Specify the memory regions for the given hart.
- kind : 0=memory 1=io

`rv set pc $hartId $pc_hex`
- Used once after reset to specify where the CPU PC landed

`rv commit $hartId $pc_hex`
- Specify when a given hart committed an instruction

`rv trap $hartId $interrupt $code`
- Used for exception and interrupts traps

`rv int set $hartId $intId $value`
- Specify when hardware values of the input interrupts pins (external / timer interrupts)
- intId follow the privileged spec mstatus CSR 

### Register file commands 

`rv rf w $hartId $rfKind $address $data_hex`
- rfKind : 0 = int, 1 = float, 4 = csr 
- If address == 32 => don't check address

`rv rf r $hartId $rfKind $address $data_hex`
- rfKind : 0 = int, 1 = float, 4 = csr 
- If address == 32 => don't check address
- Note that currently, only the reads to CSR should be logged

### Load/Store commands
`rv load exe $hartId $id $size $addr_hex $data_hex`
- load exe meaning "the moment at which the CPU read a load value from the cache for a given LQ id"
- This is used by the cpu memory view to precisely figure out the memory ordering against other CPUs.

`rv load com $hartId $id`
- load com meaning "the moment at which a given memory load is committed"
- should precede the related `rv commit` command

`rv load flu $hartId`
- load flu meaning "Remove all the outstanding memory load, as the CPU flushed the LQ"

`rv store com $hartId $id $size $addr_hex $data_hex`
- store com meaning "A memory store commit"
- should precede the related `rv commit` command

`rv store bro $hartId $id`
- store bro meaning "A memory store broadcast"
- Make the related store visible to all other memory views (used for memory ordering across CPUs)

`rv store sc $hartId $failure`
- Specify if a given "store conditional" succeeded
- should precede the related `rv commit` command

`rv io $hartId $write $address_hex $data_hex $mask_hex $size $error`
- Log the memory IO load/store accesses
