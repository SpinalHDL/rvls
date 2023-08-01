// SPDX-FileCopyrightText: 2023 "Everybody"
//
// SPDX-License-Identifier: MIT

#pragma once

#include "stdio.h"
#include "type.h"
#include <stdexcept>
#include <byteswap.h>
//
#include <iostream>
#include <elfio/elfio.hpp>
#include <elf.h>

using namespace ELFIO;

//https://chris.bracken.jp/2018/10/decoding-an-elf-binary/ <3
class Elf{
public:
    elfio reader;
    Elf(const char* path);
    void visitBytes(std::function<void(u8, u64)> func);
    u64 getSymbolAddress(char *targetName);
};
