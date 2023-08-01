#pragma once

#include "type.h"
#include "memory.hpp"
#include "hart.hpp"
#include "elf.hpp"
#include <vector>
#include <string>

class Context{
public:
    Memory memory;
    std::vector<Hart*> harts;

    void loadElf(std::string path, u64 offset);
    void loadBin(std::string path, u64 offset);
    void rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth);
    void close();
};

