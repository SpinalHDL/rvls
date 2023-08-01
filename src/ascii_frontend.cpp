#include "ascii_frontend.hpp"

using namespace std;

void checkFile(std::ifstream &lines){
    Context context;
#define rv context.harts[hartId]
    std::string line;
    u64 lineId = 0;
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
                } else if(str == "new"){
                    u32 hartId, physWidth;
                    string isa, priv;
                    f >> hartId >> isa >> priv >> physWidth;
                    context.rvNew(hartId, isa, priv, physWidth);
                } else {
                    throw runtime_error(line);
                }
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
            }  else if(str == "bin"){
                f >> str;
                if(str == "load"){
                    string path;
                    u64 offset;
                    f >> hex >> offset >> dec >> path;
                    context.loadBin(path, offset);
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
        context.close();
        throw e;
    }
    context.close();

    cout << "Model check Success <3" << endl;
}


