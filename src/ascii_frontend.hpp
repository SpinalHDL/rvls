/*
 * ascii_frontend.h
 *
 *  Created on: Aug 1, 2023
 *      Author: rawrr
 */

#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <iostream>
#include <queue>
#include <sstream>
#include <fstream>

#include "config.hpp"
#include "type.h"
#include "context.hpp"
#include "hart.hpp"

extern void checkFile(std::ifstream &lines, RvlsConfig &config);


