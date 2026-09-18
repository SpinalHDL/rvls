#include "ascii_frontend.hpp"
#include <charconv>

using namespace std;

static vector<u8> parseBytes(const string &token, size_t size){
    if(size > 16 || token.size() != size*2) throw runtime_error("Bad data size");
    vector<u8> data(size);
    for(size_t i = 0; i < size; ++i){
        const char *start = token.data() + token.size() - (i+1)*2;
        unsigned value;
        auto [end, error] = from_chars(start, start+2, value, 16);
        if(error != errc() || end != start+2) throw runtime_error("Bad data value");
        data[i] = value;
    }
    return data;
}

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
                        string dataToken;
                        f >> hartId >> rfKind >> address >> dataToken;
                        auto data = parseBytes(dataToken, rfKind == 1 ? 16 : 8);
                        rv->writeRf(rfKind, address, data);
                    } else if(str == "r") {
                        u32 hartId, rfKind, address;
                        string dataToken;
                        f >> hartId >> rfKind >> address >> dataToken;
                        auto data = parseBytes(dataToken, 8);
                        rv->readRf(rfKind, address, data);
                    } else {
                        throw runtime_error(line);
                    }

                } else if (str == "load") {
                    f >> str;
                    if(str == "exe") {
                        u32 hartId, lqId;
                        u64 address, len;
                        string dataToken;
                        f >> hartId >> lqId >> len >> hex >> address >> dataToken >> dec;
                        auto data = parseBytes(dataToken, len);
                        rv->memory->loadExecute(lqId, address, len, data.data());
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
                        u64 address, len;
                        string dataToken;
                        f >> hartId >> sqId >> len >> hex >> address >> dataToken >> dec;
                        auto data = parseBytes(dataToken, len);
                        rv->memory->storeExecute(sqId, address, len, data.data());
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
                    TraceIo io;
                    string dataToken;
                    f >> io.write >> hex >> io.address >> dataToken >> io.mask >> dec >> io.size >> io.error;
                    io.data = parseBytes(dataToken, io.size);
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
                    } else if(str == "reg"){
                        u32 hartId;
                        s32 id;
                        u64 value;
                        char prefix;
                        f >> hartId >> prefix >> id >> hex >> value >> dec;
                        if(!f || prefix != 'x'){
                            throw runtime_error(line);
                        }
                        rv->setRegister(id, value);
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
                    u32 hartId, physWidth, viewId, pmpNum, triggerCount = 0, asidWidth = 0;
                    string isa, priv;
                    f >> hartId >> isa >> priv >> physWidth >> pmpNum;
                    vector<u32> tail;
                    u32 tailValue;
                    while(f >> tailValue) tail.push_back(tailValue);
                    if(tail.size() == 1) {
                        viewId = tail[0];
                    } else if(tail.size() == 2) {
                        triggerCount = tail[0];
                        viewId = tail[1];
                    } else if(tail.size() == 3) {
                        triggerCount = tail[0];
                        asidWidth = tail[1];
                        viewId = tail[2];
                    } else {
                        throw runtime_error(line);
                    }
                    context.rvNew(hartId, isa, priv, physWidth, pmpNum, triggerCount, asidWidth, viewId, context.spikeLogs);
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
