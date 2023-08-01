// SPDX-FileCopyrightText: 2023 "Everybody"
//
// SPDX-License-Identifier: MIT

#pragma once

#include "type.h"
#include <stdio.h>
#include <stdint.h>
#include <iostream>


class RandomGen{
public:
  u64 state = rand();
  void setSeed(u64 seed);
  u32 nextInt();
  void nextBytes(u8* bytes, u64 len);
};


class Memory{
public:
	u8* mem[1 << 12];
	u64 seed = 0;
	u64 randOffset = 0;

	Memory();
	~Memory();

	u8* get(u32 address);
	void read(u32 address,u32 length, u8 *data);
	void write(u32 address,u32 length, u8 *data);
	u8& operator [](u32 address);
	void loadHex(std::string path);
    void loadBin(std::string path, uint64_t offset);
};

