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

#include "type.h"
#include "context.hpp"
#include "hart.hpp"
#include "config.h"

extern void checkFile(std::ifstream &lines, RvlsConfig &config);


