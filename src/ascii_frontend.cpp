#include "ascii_frontend.hpp"

using namespace std;

void checkFile(std::ifstream &lines, RvlsConfig &config){
    Context context;
    context.config = config;
    #define rv context.harts[hartId]
    std::string line;
    u64 lineId = 1;
    context.spikeLogs = fopen("spike.log", "w");
    cout << "Model check started" << endl;
    try{
        while (getline(lines, line)){
            istringstream f(line);
            string str;
            f >> str;
            if(str == "rv"){
                f >> str;
                if (str == "commit") {
                    u32 hartId;
                    u64 pc;
                    f >> hartId >> hex >> pc >> dec;
                    rv->commit(pc);
                } else if (str == "rf") {
                    f >> str;
                    if(str == "w") {
                        u32 hartId, rfKind, address;
                        u64 data;
                        f >> hartId >> rfKind >> address >> hex >> data >> dec;
                        rv->writeRf(rfKind, address, data);
                    } else if(str == "r") {
                        u32 hartId, rfKind, address;
                        u64 data;
                        f >> hartId >> rfKind >> address >> hex >> data >> dec;
                        rv->readRf(rfKind, address, data);
                    } else {
                        throw runtime_error(line);
                    }

                } else if (str == "load") {
                    f >> str;
                    if(str == "exe") {
                        u32 hartId, lqId;
                        u64 address, len, data;
                        f >> hartId >> lqId >> len >> hex >> address >> data >> dec;
                        rv->memory->loadExecute(lqId, address, len, (u8*)&data);
                    } else if(str == "com") {
                        u32 hartId, lqId;
                        f >> hartId >> lqId;
                        rv->memory->loadCommit(lqId);
                    } else if(str == "flu") {
                        u32 hartId;
                        f >> hartId;
                        rv->memory->loadFlush();
                    } else {
                        throw runtime_error(line);
                    }
                } else if (str == "store") {
                    f >> str;
                    if(str == "exe") {
                        u32 hartId, sqId;
                        u64 address, len, data;
                        f >> hartId >> sqId >> len >> hex >> address >> data >> dec;
                        rv->memory->storeExecute(sqId, address, len, (u8*)&data);
                    } else if(str == "com") {
                        u32 hartId, sqId;
                        f >> hartId >> sqId;
                        rv->memory->storeCommit(sqId);
                    } else if(str == "bro") {
                        u32 hartId, sqId;
                        f >> hartId >> sqId;
                        rv->memory->storeBroadcast(sqId);
                    } else if (str == "sc") {
                        u32 hartId;
                        bool pass;
                        f >> hartId >> pass;
                        rv->scStatus(pass);
                    } else {
                        throw runtime_error(line);
                    }
                } else if (str == "io") {
                    u32 hartId;
                    f >> hartId;
                    auto io = TraceIo(f);
                    rv->ioAccess(io);
                } else if (str == "trap") {
                    u32 hartId, code;
                    bool interrupt;
                    f >> hartId >> interrupt >> code;
                    rv->trap(interrupt, code);
                } else if (str == "int") {
                    f >> str;
                    if(str == "set") {
                        u32 hartId, intId;
                        bool value;
                        f >> hartId >> intId >> value;
                        rv->setInt(intId, value);
                    } else {
                        throw runtime_error(line);
                    }
                } else if (str == "set") {
                    f >> str;
                    if(str == "pc"){
                        u32 hartId;
                        u64 pc;
                        f >> hartId >> hex >> pc >> dec;
                        rv->setPc(pc);
                    } else {
                        throw runtime_error(line);
                    }
                } else if (str == "region") {
                    f >> str;
                    if(str == "add"){
                        u32 hartId;
                        u64 type;
                        Region r;
                        f >> hartId >> type >> hex >> r.base >> r.size >> dec;
                        r.type = (RegionType)type;
                        rv->addRegion(r);
                    } else {
                        throw runtime_error(line);
                    }
                } else if(str == "new"){
                    u32 hartId, physWidth, viewId, pmpNum;
                    string isa, priv;
                    f >> hartId >> isa >> priv >> physWidth >> pmpNum >> viewId;
                    context.rvNew(hartId, isa, priv, physWidth, pmpNum, viewId, context.spikeLogs);
                } else {
                    throw runtime_error(line);
                }
            } else if (str == "time") {
                u64 time;
                f >> time;
                context.time = time;
            } else if(str == "elf"){
                f >> str;
                if(str == "load"){
                    string path;
                    u64 offset;
                    f >> hex >> offset >> dec >> path;
                    context.loadElf(path, offset);
                } else {
                    throw runtime_error(line);
                }
            } else if(str == "bin"){
                f >> str;
                if(str == "load"){
                    string path;
                    u64 offset;
                    f >> hex >> offset >> dec >> path;
                    context.loadBin(path, offset);
                } else {
                    throw runtime_error(line);
                }
            } else if(str == "bytes"){
                f >> str;
                if(str == "load"){
                    string path;
                    u64 offset;
                    f >> hex >> offset;
                    for (u64 number; f >> hex >> number;) {
                    	context.loadBytes(offset, 1, (u8*)&number);
                    	offset += 1;
                    }
                } else {
                    throw runtime_error(line);
                }
            } else if(str == "memview"){
                f >> str;
                if(str == "new"){
                    string path;
                    u64 id, readIds, writeIds;
                    f >> id >> readIds >> writeIds;
                    context.cpuMemoryViewNew(id, readIds, writeIds);
                } else {
                    throw runtime_error(line);
                }
            } else {
                throw runtime_error(line);
            }
            lineId += 1;
        }
    } catch (const std::exception &e) {
        printf("Failed at line %ld : %s\n", lineId, line.c_str());
        printf("- %s\n", e.what());
        context.print();
        context.close();
        throw e;
    }
    cout << "Model check Success <3" << endl;
    context.close();
}


