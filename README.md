# Introduction

RVLS (Risc-V Lock Step) is a CPU simulation trace checker.
- Typical usage is to check that a simulated CPU system is behaving right
- Has a human-readable text frontend to feed the traces
- Support multi-core systems, by tracking memory coherency status across CPUs
- Use Spike's "proc" as golden reference model
- Use a lightly modified Spike version to provide more info and allow coherency checks

See example/simple/trace.log for an example of ASCII trace which can be checked by RVLS

RVLS is used to check the behaviour of multicore NaxRiscv. 
NaxRiscv use a Write-Back L1 data cache, with tilelink to provide memory coherency between the cores, using a MESI protocol (https://en.wikipedia.org/wiki/MESI_protocol)

Not everything is strictly keept in sync, noticibly, it assumes that :
- The software and fence.i  keep the hardware L1 instruction cache coherent
- The software and sfence keep the hardware MMU TLB coherent

# How to use

```shell
build/apps/rvls -f example/simple/trace.log
```

# How to compile

```shell
git clone https://github.com/SpinalHDL/rvls.git
git clone https://github.com/SpinalHDL/riscv-isa-sim.git --recursive

# Compile riscv-isa-sim (spike), used as a golden model during the sim to check the dut behaviour (lock-step)
cd riscv-isa-sim
mkdir build
cd build
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
   
# ASCII frontend

There is a ASCII based frontend which can be used to feed the CPUs execution traces.
It consists into the simple lines of commands described bellow. 

## General commands

`time $value`
- Used to provide some sporatic timestap, just for debug purposes

## Memory commands

`elf load $offset_hex $path`

`bin load $offset_hex $path`

`memview new $memoryViewId $readIds $writeIds`
- Create a new memory view
- A memory view provide a representation of how a given memory master (ex CPU) observe the global memory content/ordering
- readIds and writeIds represent the number of outstanding load/store that the CPU can have at most (LQ/SQ size)


## RISC-V commands

There mostly 3 kind of RISC-V related commands :
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

`rv set pc $hartId $pc_hex`
- Used once after reset to specify where the CPU PC landed

`rv commit $hartId $pc_hex`
- Specify when a given hart commited an instruction

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
- load exe meaning "the moment at which the CPU readed a load value from the cache for a given LQ id"
- This is used by the cpu memory view to precisely figure out the memory ordering against other CPUs.

`rv load com $hartId $id`
- load com meaning "the moment at which a given memory load is commited"
- should precede the related `rv commit` command

`rv load flu $hartId`
- load flu meaning "Remove all the outstanding memory load, as the CPU flushed the LQ"

`rv store com $hartId $id $size $addr_hex $data_hex`
- store com meaning "A memory store commit"
- should precede the related `rv commit` command

`rv store bro $hartId $id`
- store bro meaning "A memory store broadcast"
- Make the related store visible to all other memory views (used for memory ordering accross CPUs)

`rv store sc $hartId $failure`
- Specify if a given "store conditional" succeeded
- should precede the related `rv commit` command

`rv io $hartId $write $address_hex $data_hex $mask_hex $size $error`
- Log the memory IO load/store accesses
