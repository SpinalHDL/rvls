/*
 * hart.h
 *
 *  Created on: Aug 1, 2023
 *      Author: rawrr
 */

#pragma once

#include <sys/syscall.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <vector>
#include "global.hpp"
#include "memory.hpp"
#include "processor.h"
#include "mmu.h"
#include "simif.h"
#include "coherency.hpp"


using namespace std;

class TraceIo{
public:
    bool write;
    u64 address;
    u64 data;
    u32 mask;
    u32 size;
    bool error;

    TraceIo(){}
    TraceIo(std::istringstream &f){
        f >> write >> hex >> address >> data >> mask >> dec >> size >> error;
    }
};


enum RegionType  {mem = 0, io = 1};
class Region{
public:
    RegionType type;
    u64 base, size;
};

class SpikeIf : public simif_t{
public:
    CpuMemoryView *memory;
    u32 hartId;
    queue <TraceIo> ioQueue;
    vector<Region> regions;
    cfg_t cfg;
    map<size_t, processor_t*> harts;

    SpikeIf(CpuMemoryView *memory, u32 hartId);

    virtual char* addr_to_mem(reg_t addr);
    virtual bool reservable(reg_t addr);
    virtual bool mmio_fetch(reg_t addr, size_t len, u8* bytes);
    virtual bool mmio_mmu(reg_t addr, size_t len, u8* bytes);
    virtual bool mmio_load(reg_t addr, size_t len, u8* bytes);
    virtual bool mmio_store(reg_t addr, size_t len, const u8* bytes);
    virtual void proc_reset(unsigned id);
    virtual const cfg_t &get_cfg() const;
    virtual const map<size_t, processor_t*>& get_harts() const;
    virtual const char* get_symbol(uint64_t addr);
    bool isMem(u64 address);
    bool isIo(u64 address);
    bool isFetchable(u64 address);
    Region* getRegion(u64 address);
};

class Hart{
public:
    SpikeIf *sif;
    processor_t *proc;
    state_t *state;
    CpuMemoryView *memory;
    u32 hartId;
    string isaStorage;
    string privStorage;
    ofstream spikeSink;

    u32 physWidth;

    bool integerWriteValid = false;
    u64 integerWriteData = 0;

    bool floatWriteValid = false;
    u64 floatWriteData = 0;


    u32 csrAddress = 0;
    bool csrWrite = false;
    bool csrRead = false;
    u64 csrWriteData = 0;
    u64 csrReadData = 0;

    bool scValid = false;
    bool scFailure = false;

    u64 interruptPending = 0;

    Hart(u32 hartId, string isa, string priv, u32 physWidth, u32 pmpNum, u32 triggerCount, u32 asidWidth, CpuMemoryView *memory, FILE *logs);
    void close();
    void setPc(u64 pc);
    void setRegister(s32 id, u64 value);
    void writeRf(u32 rfKind, u32 address, u64 data);
    void readRf(u32 rfKind, u32 address, u64 data);
    void physExtends(u64 &v);
    void trap(bool interrupt, u32 code);
    void commit(u64 pc);
    void ioAccess(TraceIo io);
    void setInt(u32 id, bool value);
    void scStatus(bool failure);
    void addRegion(Region r);
    std::string formatFailureContext() const;
    csr_t_p findCsr(reg_t address) const;
    void backdoorWriteCsr(reg_t address, u64 value);
};

class RvlsHpmCounterCsr : public csr_t {
public:
    RvlsHpmCounterCsr(processor_t* const proc, const reg_t addr) : csr_t(proc, addr), value(0) {}

    reg_t read() const noexcept override {
        return value;
    }

    void sync(reg_t value) noexcept {
        this->value = value;
    }

protected:
    bool unlogged_write(const reg_t) noexcept override {
        return true;
    }

private:
    reg_t value;
};

class RvlsTselectCsr : public basic_csr_t {
public:
    RvlsTselectCsr(processor_t* const proc, const reg_t addr, u32 triggerCount);

protected:
    bool unlogged_write(const reg_t val) noexcept override;

private:
    reg_t mask;
};
