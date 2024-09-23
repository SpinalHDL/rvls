#include "ascii_frontend.hpp"


using namespace std;

#include "CLI11.hpp"
#include "config.hpp"


int main(int argc, char **argv){
    RvlsConfig config;

    CLI::App app{"RISCV lock step checker"};


    app.add_option("-f,--file", config.asciiTraceFile, "ASCII trace file to check");
//    app.add_option("--spike-debug{1}", config.spikeDebug, "")->default_val(false);
    app.add_flag("--spike-debug{1}", config.spikeDebug, "")->default_val(false);
    app.add_flag("--spike-log{1}", config.spikeLogCommit, "")->default_val(false);

    CLI11_PARSE(app, argc, argv);

    if(config.asciiTraceFile.empty()){
        printf("No work was given in arguments(");
        exit(1);
    }
    std::ifstream f(config.asciiTraceFile);
    if(!f.good()){
        printf("Bad input file %s\n", config.asciiTraceFile.c_str());
        exit(1);
    }
    if(f.eof()){
        printf("Empty file %s\n", config.asciiTraceFile.c_str());
        exit(1);
    }
    checkFile(f, config);
}

