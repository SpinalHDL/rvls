/*
 * hart.h
 *
 *  Created on: Aug 1, 2023
 *      Author: rawrr
 */

#include "hart.hpp"
#include "snapshot.hpp"
#include "disasm.h"

#include <format>

static bool isHpmCounterCsr(u32 csr){
    return (csr >= CSR_MHPMCOUNTER3 && csr <= CSR_MHPMCOUNTER31) ||
           (csr >= CSR_HPMCOUNTER3 && csr <= CSR_HPMCOUNTER31) ||
           (csr >= CSR_MHPMCOUNTER3H && csr <= CSR_MHPMCOUNTER31H) ||
           (csr >= CSR_HPMCOUNTER3H && csr <= CSR_HPMCOUNTER31H);
}

static bool isCounterEnableCsr(u32 csr){
    return csr == CSR_MCOUNTEREN || csr == CSR_SCOUNTEREN || csr == CSR_HCOUNTEREN;
}

static bool isTopiCsr(u32 csr){
    return csr == CSR_MTOPI || csr == CSR_STOPI || csr == CSR_VSTOPI;
}

static u32 machineHpmCounterCsr(u32 csr){
    if(csr >= CSR_HPMCOUNTER3 && csr <= CSR_HPMCOUNTER31) {
        return CSR_MHPMCOUNTER3 + (csr - CSR_HPMCOUNTER3);
    }
    if(csr >= CSR_HPMCOUNTER3H && csr <= CSR_HPMCOUNTER31H) {
        return CSR_MHPMCOUNTER3H + (csr - CSR_HPMCOUNTER3H);
    }
    return csr;
}

static reg_t triggerIndexMask(u32 triggerCount){
    reg_t mask = 0;
    while(triggerCount > 1) {
        mask = (mask << 1) | 1;
        triggerCount = (triggerCount + 1) >> 1;
    }
    return mask;
}

static void syncCsrRead(csr_t_p csr, u64 value){
    if(!csr)
        return;

    auto hpm = std::dynamic_pointer_cast<RvlsHpmCounterCsr>(csr);
    if(hpm) {
        hpm->sync(value);
    } else {
        csr->unlogged_backdoor_write(value);
    }
}

static std::string formatHex(u64 value, u32 width){
    if(width <= 8) return std::format("0x{:0{}x}", static_cast<u32>(value), width);
    return std::format("0x{:0{}x}", value, width);
}

static std::string formatRaw128(u64 high, u64 low){
    return std::format("0x{:016x}{:016x}", high, low);
}

static void dumpCsr(std::stringstream &ss, const state_t *state, reg_t address, u32 width){
    if(!state)
        return;

    auto it = state->csrmap.find(address);
    if(it == state->csrmap.end() || !it->second)
        return;

    ss << std::format("  0x{:03x}=", address);
    try {
        ss << formatHex(it->second->read(), width);
    } catch (...) {
        ss << "<read failed>";
    }
    ss << "\n";
}

SpikeIf::SpikeIf(CpuMemoryView *memory, u32 hartId){
    this->memory = memory;
    this->hartId = hartId;
    debug_mmu = NULL;
}

Region* SpikeIf::getRegion(u64 address){
    for(auto &r : regions){
        if(address >= r.base && address < r.base + r.size) {
            return &r;
        }
    }
    return NULL;
}

bool SpikeIf::isMem(u64 address){
    auto r = getRegion(address);
    return r != NULL && r->type == RegionType::mem;
}

bool SpikeIf::isIo(u64 address){
    auto r = getRegion(address);
    return r != NULL && r->type == RegionType::io;
}

bool SpikeIf::isFetchable(u64 address){
    auto r = getRegion(address);
    return r != NULL;
}

// should return NULL for MMIO addresses
char* SpikeIf::addr_to_mem(reg_t addr)  {
//        if((addr & 0xE0000000) == 0x00000000) return NULL;
//        printf("addr_to_mem %lx ", addr);
//        return (char*) memory->get(addr);
    return NULL;
}

bool SpikeIf::reservable(reg_t addr)  {
    return isMem(addr);
}

// used for MMIO addresses
bool SpikeIf::mmio_fetch(reg_t addr, size_t len, u8* bytes)  {
    if(isFetchable(addr)) {
        memory->fetch(addr, len, bytes);
        return true;
    }
    return false;
}


bool SpikeIf::mmio_mmu(reg_t addr, size_t len, u8* bytes)  {
    if(isMem(addr)) {
        memory->mmu(addr, len, bytes);
        return true;
    }
    return false;
}

bool SpikeIf::mmio_load(reg_t addr, size_t len, u8* bytes)  {
    if(isMem(addr)) {
        memory->load(addr, len, bytes);
        return true;
    }
    if(isIo(addr)){
    //        printf("mmio_load %lx %ld\n", addr, len);
        assertTrue(hartId, "missing mmio\n", !ioQueue.empty());
        auto dut = ioQueue.front();
        assertEq(hartId, "mmio write\n", dut.write, false);
        assertEq(hartId, "mmio address\n", dut.address, addr);
        assertEq(hartId, "mmio len\n", dut.size, len);
        memcpy(bytes, (u8*)&dut.data, len);
        ioQueue.pop();
        return !dut.error;
    }

    return false;
}
bool SpikeIf::mmio_store(reg_t addr, size_t len, const u8* bytes)  {
    if(isMem(addr)) {
        memory->store(addr, len, (u8*) bytes);
        return true;
    }

    if(isIo(addr)){
    //        printf("mmio_store %lx %ld\n", addr, len);
        assertTrue(hartId, "missing mmio\n", !ioQueue.empty());
        auto dut = ioQueue.front();
        assertEq(hartId, "mmio write\n", dut.write, true);
        assertEq(hartId, "mmio address\n", dut.address, addr);
        assertEq(hartId, "mmio len\n", dut.size, len);
        assertTrue(hartId, "mmio data\n", !memcmp((u8*)&dut.data, bytes, len));
        ioQueue.pop();
        return !dut.error;
    }

    return false;
}
// Callback for processors to let the simulation know they were reset.
void SpikeIf::proc_reset(unsigned id)  {
//        printf("proc_reset %d\n", id);
}

const cfg_t &SpikeIf::get_cfg() const  {
    return cfg;
}

const map<size_t, processor_t*>& SpikeIf::get_harts() const  {
    return harts;
}

const char* SpikeIf::get_symbol(uint64_t addr)  {
//        printf("get_symbol %lx\n", addr);
    return NULL;
}

RvlsTselectCsr::RvlsTselectCsr(processor_t* const proc, const reg_t addr, u32 triggerCount) :
    basic_csr_t(proc, addr, 0),
    mask(triggerIndexMask(triggerCount)) {
}

bool RvlsTselectCsr::unlogged_write(const reg_t val) noexcept {
    return basic_csr_t::unlogged_write(val & mask);
}



Hart::Hart(u32 hartId, string isa, string priv, u32 physWidth, u32 pmpNum, u32 triggerCount, CpuMemoryView *memory, FILE *logs){
    this->memory = memory;
    this->hartId = hartId;
    this->physWidth = physWidth;
    this->isaStorage = isa;
    this->privStorage = priv;
    sif = new SpikeIf(memory, hartId);
    sif->cfg.isa = this->isaStorage.c_str();
    sif->cfg.priv = this->privStorage.c_str();
    sif->cfg.pmpregions = pmpNum;
    sif->cfg.pmpgranularity = 1 << 12;
    sif->cfg.trigger_count = triggerCount;
    sif->cfg.hartids = vector<size_t>({hartId});
    sif->cfg.explicit_hartids = true;
    spikeSink.open("/dev/null", std::ofstream::binary);
    proc = new processor_t(this->isaStorage.c_str(), this->privStorage.c_str(), &sif->cfg, sif, hartId, false, logs, spikeSink);
    sif->harts[hartId] = proc;
    auto xlen = proc->get_xlen();
    // asid support is VexiiRiscv is not implemented.
    proc->set_impl(IMPL_MMU_ASID, false);
    proc->set_max_vaddr_bits(xlen == 32 ? 32 : 39);
    proc->reset();
    proc->enable_commit_log_state();
    proc->paddr_bits_sim = physWidth;
    proc->set_pmp_num(pmpNum);
    state = proc->get_state();
    if(pmpNum > 0) {
        backdoorWriteCsr(CSR_PMPADDR0, ~reg_t(0));
        backdoorWriteCsr(CSR_PMPCFG0, PMP_R | PMP_W | PMP_X | PMP_NAPOT);
    }
    state->csrmap[CSR_MCYCLE] = std::make_shared<basic_csr_t>(proc, CSR_MCYCLE, 0);
    state->csrmap[CSR_MCYCLEH] = std::make_shared<basic_csr_t>(proc, CSR_MCYCLEH, 0);
    state->csrmap[CSR_CYCLE] = std::make_shared<counter_proxy_csr_t>(proc, CSR_CYCLE, state->csrmap[CSR_MCYCLE]);
    state->csrmap[CSR_CYCLEH] = std::make_shared<counter_proxy_csr_t>(proc, CSR_CYCLEH, state->csrmap[CSR_MCYCLEH]);
    backdoorWriteCsr(CSR_MCOUNTEREN, MCOUNTEREN_TIME);
    if(triggerCount == 0) {
        state->csrmap[CSR_TINFO] = std::make_shared<inaccessible_csr_t>(proc, CSR_TINFO);
    } else {
        state->tselect = std::make_shared<RvlsTselectCsr>(proc, CSR_TSELECT, triggerCount);
        state->csrmap[CSR_TSELECT] = state->tselect;
        // VexiiRiscv currently exposes only debug trigger type 2 (mcontrol).
        state->csrmap[CSR_TINFO] = std::make_shared<const_csr_t>(proc, CSR_TINFO, reg_t(1) << 2);
    }
    state->csrmap[CSR_TDATA3] = std::make_shared<inaccessible_csr_t>(proc, CSR_TDATA3);
    if(!state->csrmap.count(CSR_MTOPI)) {
        state->csrmap[CSR_MTOPI] = std::make_shared<mtopi_csr_t>(proc, CSR_MTOPI);
    }
    if(!state->csrmap.count(CSR_STOPI)) {
        state->csrmap[CSR_STOPI] = std::make_shared<nonvirtual_stopi_csr_t>(proc, CSR_STOPI);
    }
    if(!state->csrmap.count(CSR_VSTOPI)) {
        state->csrmap[CSR_VSTOPI] = std::make_shared<vstopi_csr_t>(proc, CSR_VSTOPI);
    }
    for(reg_t i = 0; i < N_HPMCOUNTERS; ++i) {
        const reg_t mcounterAddr = CSR_MHPMCOUNTER3 + i;
        const reg_t counterAddr = CSR_HPMCOUNTER3 + i;
        auto mcounter = std::make_shared<RvlsHpmCounterCsr>(proc, mcounterAddr);
        state->csrmap[mcounterAddr] = mcounter;
        state->csrmap[counterAddr] = std::make_shared<counter_proxy_csr_t>(proc, counterAddr, mcounter);

        if(xlen == 32) {
            const reg_t mcounterhAddr = CSR_MHPMCOUNTER3H + i;
            const reg_t counterhAddr = CSR_HPMCOUNTER3H + i;
            auto mcounterh = std::make_shared<RvlsHpmCounterCsr>(proc, mcounterhAddr);
            state->csrmap[mcounterhAddr] = mcounterh;
            state->csrmap[counterhAddr] = std::make_shared<counter_proxy_csr_t>(proc, counterhAddr, mcounterh);
        }
    }
    getContextHartsManager().add(hartId, *this);
}

void Hart::close() {
    getContextHartsManager().remove(hartId);
    auto f = proc->get_log_file();
    if(f) fclose(f);
}

void Hart::setPc(u64 pc){
    state->pc = pc;
}

static const reg_t dump_csrs[] = {
    CSR_SSTATUS, CSR_SIE, CSR_STVEC, CSR_SCOUNTEREN,
    CSR_SSCRATCH, CSR_SEPC, CSR_SCAUSE, CSR_STVAL, CSR_SIP, CSR_SATP,
    CSR_VSSTATUS, CSR_VSTVEC, CSR_VSEPC, CSR_VSCAUSE, CSR_VSTVAL, CSR_VSATP,
    CSR_MSTATUS, CSR_MISA, CSR_MEDELEG, CSR_MIDELEG, CSR_MIE, CSR_MTVEC,
    CSR_MCOUNTEREN, CSR_MCOUNTINHIBIT, CSR_MSCRATCH, CSR_MEPC, CSR_MCAUSE,
    CSR_MTVAL, CSR_MIP, CSR_MTINST, CSR_MTVAL2,
    CSR_HSTATUS, CSR_HEDELEG, CSR_HIDELEG, CSR_HIE, CSR_HCOUNTEREN,
    CSR_HTVAL, CSR_HIP, CSR_HVIP, CSR_HTINST, CSR_HGATP,
    CSR_MCYCLE, CSR_MINSTRET, CSR_TIME
};

std::string Hart::formatFailureContext() const {
    std::stringstream ss;
    const u32 xlen = proc ? proc->get_xlen() : 64;
    const u32 regWidth = xlen / 4;

    ss << "=== rvls failure context ===\n";

    if(state) {
        ss << "spike_pc: " << formatHex(state->pc, regWidth) << "\n";
        ss << "inst: " << formatHex(state->last_inst.bits(), 8) << "\n";
        ss << "privilege: priv=" << state->prv
           << " pre_priv=" << state->prev_prv
           << " v=" << state->v
           << " pre_v=" << state->prev_v
           << " last_inst_priv=" << state->last_inst_priv
           << " last_inst_xlen=" << state->last_inst_xlen
           << " last_inst_flen=" << state->last_inst_flen
           << "\n";
        ss << "trap: happened=" << state->trap_happened
           << " interrupt=" << state->trap_interrupt
           << " code=" << state->trap_code
           << "\n";
    }

    ss << "rvls trace state:\n";
    ss << "  integerWriteValid=" << integerWriteValid
       << " integerWriteData=" << formatHex(integerWriteData, regWidth) << "\n";
    ss << "  floatWriteValid=" << floatWriteValid
       << " floatWriteData=" << formatHex(floatWriteData, 16) << "\n";
    ss << "  csrRead=" << csrRead << " csrWrite=" << csrWrite
       << std::format(" csrAddress=0x{:x}", csrAddress)
       << " csrReadData=" << formatHex(csrReadData, regWidth)
       << " csrWriteData=" << formatHex(csrWriteData, regWidth)
       << "\n";
    ss << "  scValid=" << scValid << " scFailure=" << scFailure << "\n";
    ss << "  interruptPending=" << formatHex(interruptPending, regWidth) << "\n";

    if(state) {
        ss << "spike log_reg_write:\n";
        if(state->log_reg_write.empty()) {
            ss << "  <empty>\n";
        } else {
            for(const auto &item : state->log_reg_write) {
                const u32 address = item.first >> 4;
                const u32 kind = item.first & 0xf;
                ss << "  kind=" << kind << " address=" << address;
                ss << " value=" << formatHex(item.second.v[0], regWidth);
                if(kind == 1) {
                    ss << " raw128=" << formatRaw128(item.second.v[1], item.second.v[0]);
                }
                ss << "\n";
            }
        }

        ss << "XPR:\n";
        for(int i = 0; i < NXPR; ++i) {
            ss << std::format("  {:>4}={}", xpr_name[i], formatHex(state->XPR[i], regWidth));
            if((i + 1) % 4 == 0) ss << "\n";
        }
        if(NXPR % 4 != 0) ss << "\n";

        ss << "FPR raw128:\n";
        for(int i = 0; i < NFPR; ++i) {
            const auto value = state->FPR[i];
            ss << std::format("  {:>4}={}", fpr_name[i], formatRaw128(value.v[1], value.v[0]));
            if((i + 1) % 2 == 0) ss << "\n";
        }
        if(NFPR % 2 != 0) ss << "\n";

        ss << "CSR:\n";
        for(auto csr : dump_csrs) {
            dumpCsr(ss, state, csr, regWidth);
        }
    }

    return ss.str();
}

void Hart::writeRf(u32 rfKind, u32 address, u64 data){
    switch(rfKind){
    case 0:
        integerWriteValid = true;
        integerWriteData = data;
        break;
    case 1:
        floatWriteValid = true;
        floatWriteData = data;
        break;
    case 4:
        if((csrWrite || csrRead) && csrAddress != address){
            failure(hartId, "duplicated CSR access \n");
        }
        csrAddress = address;
        csrWrite = true;
        csrWriteData = data;
        break;
    default:
        failure(hartId, "??? unknown RF trace \n");
        break;
    }

}


void Hart::readRf(u32 rfKind, u32 address, u64 data){
    switch(rfKind){
    case 4:
        if((csrWrite || csrRead) && csrAddress != address){
            failure(hartId, "duplicated CSR access \n");
        }
        csrAddress = address;
        csrRead = true;
        csrReadData = data;
        break;
    default:
        failure(hartId, "??? unknown RF trace \n");
        break;
    }

}

void Hart::physExtends(u64 &v){
    //v = (u64)(((s64)v<<(64-physWidth)) >> (64-physWidth));
}

void Hart::trap(bool interrupt, u32 code){
    int mask = 1 << code;
    auto fromPc = state->pc;
    if(interrupt) state->mip->write_with_mask(mask, mask);
    proc->step(1);
    if(interrupt) state->mip->write_with_mask(mask, 0);
    if(!state->trap_happened){
        failure(hartId, "DUT did trap on %lx\n", fromPc);
    }

    memory->step();
    assertEq(hartId, "DUT interrupt missmatch", interrupt, state->trap_interrupt);
    assertEq(hartId, "DUT code missmatch", code, state->trap_code);
    physExtends(state->pc);
}

void Hart::commit(u64 pc){
	auto shift = 64-proc->get_xlen();
    if(pc != (state->pc << shift >> shift)){
        failure(hartId, "PC MISSMATCH dut=%lx ref=%lx\n", pc, state->pc);
    }

    //Sync CSR
    u64 csrBackup = 0;
    if(csrRead){
        switch(csrAddress){
        case CSR_MCYCLE:
        case CSR_UCYCLE:
            backdoorWriteCsr(CSR_MCYCLE, csrReadData);
            break;
        case CSR_MCYCLEH:
        case CSR_UCYCLEH:
            backdoorWriteCsr(CSR_MCYCLEH, csrReadData);
            break;
        case CSR_TIME:
            state->time->sync(csrReadData);
            break;
        case CSR_TIMEH:
            state->time->sync((state->time->read() & 0xffffffffULL) | (csrReadData << 32));
            break;
        case MIP:
        case SIP:
        case UIP:
            csrBackup = state->mie->read();
            state->mip->unlogged_write_with_mask(-1, csrReadData);
            state->mie->unlogged_write_with_mask(MIE_MTIE | MIE_MEIE |  MIE_MSIE | MIE_SEIE, 0);
//                                cout << main_time << " " << hex << robCtx.csrReadData << " " << state->mip->read()  << " " << state->csrmap[robCtx.csrAddress]->read() << dec << endl;
            break;
        }
        if(isTopiCsr(csrAddress)){
            state->mip->unlogged_write_with_mask(-1, interruptPending);
        }
        if(isHpmCounterCsr(csrAddress)){
            syncCsrRead(findCsr(machineHpmCounterCsr(csrAddress)), csrReadData);
        }
        if(isCounterEnableCsr(csrAddress)){
            backdoorWriteCsr(csrAddress, csrReadData);
        }
    }

    if(scValid){
        scValid = false;
        if(scFailure) {
            proc->get_mmu()->yield_load_reservation();
        } else {
            //Assume spike and dut match, but not necessarily true
        }
    }

    [[maybe_unused]] long long instret = 0;
    if(auto minstret = findCsr(CSR_MINSTRET)) {
        instret = minstret->read();
    }
    //Run the spike model
    proc->step(1);
    memory->step();

    //Sync back some CSR
    state->mip->unlogged_write_with_mask(-1, 0);
    if(csrRead){
        switch(csrAddress){
        case MIP:
        case SIP:
        case UIP:
            state->mie->unlogged_write_with_mask(MIE_MTIE | MIE_MEIE |  MIE_MSIE | MIE_SEIE, csrBackup);
            break;
        case MVENDORID:
        case MARCHID:
        case MIMPID:
        case MHARTID:
            for (auto &item : state->log_reg_write) {
                if (item.first == 0)
                  continue;
                u32 rd = item.first >> 4;
                switch (item.first & 0xf) {
                case 0:  //integer
                	item.second.v[0] = integerWriteData;
                	state->XPR.write(rd, integerWriteData);
					break;
                }
			}
        	break;
        }

        if(isHpmCounterCsr(csrAddress)){
            for (auto &item : state->log_reg_write) {
                if (item.first == 0)
                  continue;
                u32 rd = item.first >> 4;
                switch (item.first & 0xf) {
                case 0:  //integer
                    item.second.v[0] = integerWriteData;
                    state->XPR.write(rd, integerWriteData);
                    break;
                }
            }
        }
    }

    //Checks
//        printf("%016lx %08lx\n", pc, state->last_inst.bits());
    assertTrue(hartId, "DUT missed a trap", !state->trap_happened);
    for (auto item : state->log_reg_write) {
        if (item.first == 0)
          continue;

        u32 rd = item.first >> 4;
        switch (item.first & 0xf) {
        case 0: { //integer
            assertTrue(hartId, "INTEGER WRITE MISSING", integerWriteValid);
            assertEq(hartId, "INTEGER WRITE MISSMATCH", integerWriteData, item.second.v[0]);
            integerWriteValid = false;
        } break;
        case 1: { //float
            assertTrue(hartId, "FLOAT WRITE MISSING", floatWriteValid);
            assertEq(hartId, "FLOAT WRITE MISSMATCH", floatWriteData, item.second.v[0]);
            floatWriteValid = false;
        } break;
        case 4:{ //CSR
            u64 inst = state->last_inst.bits();
            switch(inst){
            case 0x30200073: //MRET
            case 0x10200073: //SRET
            case 0x00200073: //URET
                physExtends(state->pc);
                break;
            default:{
                    if((inst & 0x7F) == 0x73 && (inst & 0x3000) != 0){
                        if(!(csrAddress >= 1 && csrAddress <= 3)){ //avoid fcsr
                            if(csrWrite && (csrAddress == MIP || csrAddress == SIP) && rd == CSR_MVIP){
                                continue; //Spike logs mip.SEIP's mvip alias before the architectural mip write.
                            }
                            if(!csrWrite && ((csrAddress == CSR_SIE && rd == CSR_MIE) || (csrAddress == CSR_SIP && rd == CSR_MIP))){
                                continue; //Spike logs the backing machine CSR after a supervisor CSR proxy write.
                            }
                            assertTrue(hartId, "CSR WRITE MISSING", csrWrite);
                            assertEq(hartId, "CSR WRITE ADDRESS", (u32)(csrAddress & 0xCFF), (u32)(rd & 0xCFF));
                        }
    //                                                assertEq("CSR WRITE DATA", whitebox->robCtx[robId].csrWriteData, item.second.v[0]);
                    }
                    break;
                }
            }
            csrWrite = false;
        } break;
        default: {
            failure(hartId, "??? unknown spike trace %lx\n", item.first & 0xf);
        } break;
        }
    }

    csrRead = false;
    assertTrue(hartId, "CSR WRITE SPAWNED", !csrWrite || (csrAddress >= 0x3b0 && csrAddress <= 0x3b0+63) || (csrAddress >= 0xb80 && csrAddress <= 0xb80+15) || (csrAddress == CSR_MCOUNTINHIBIT));
    assertTrue(hartId, "INTEGER WRITE SPAWNED", !integerWriteValid);
    assertTrue(hartId, "FLOAT WRITE SPAWNED", !floatWriteValid);
    csrWrite = false;
}

void Hart::ioAccess(TraceIo io){
    sif->ioQueue.push(io);
}

void Hart::setInt(u32 id, bool value){
    u64 mask = 1ULL << id;
    if(value) {
        interruptPending |= mask;
    } else {
        interruptPending &= ~mask;
    }
}

void Hart::scStatus(bool failure){
    scValid = true;
    scFailure = failure;
}

void Hart::addRegion(Region r){
    sif->regions.push_back(r);
}

csr_t_p Hart::findCsr(reg_t address) const {
    if(!state)
        return nullptr;

    auto it = state->csrmap.find(address);
    if(it == state->csrmap.end())
        return nullptr;

    return it->second;
}

void Hart::backdoorWriteCsr(reg_t address, u64 value) {
    auto csr = findCsr(address);
    if(csr)
        csr->unlogged_backdoor_write(value);
}
