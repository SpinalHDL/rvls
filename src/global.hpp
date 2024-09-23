/*
 * global.h
 *
 *  Created on: Aug 1, 2023
 *      Author: rawrr
 */

#pragma once

#include <string>
#include <sstream>

#include <cstdio>
#include <cstdarg>

#include <iostream>
#include <string>


#define CSR_UCYCLE 0xC00
#define CSR_UCYCLEH 0xC80
#define MIP 0x344
#define SIP 0x144
#define UIP  0x44
#define CAUSE_MACHINE_SOFTWARE 3
#define CAUSE_MACHINE_TIMER 7
#define CAUSE_MACHINE_EXTERNAL 11
#define CAUSE_SUPERVISOR_EXTERNAL 9
#define MIE_MTIE (1 << CAUSE_MACHINE_TIMER)
#define MIE_MEIE (1 << CAUSE_MACHINE_EXTERNAL)
#define MIE_MSIE (1 << CAUSE_MACHINE_SOFTWARE)
#define MIE_SEIE (1 << CAUSE_SUPERVISOR_EXTERNAL)
#define MVENDORID  0xF11 // MRO Vendor ID.
#define MARCHID    0xF12 // MRO Architecture ID.
#define MIMPID     0xF13 // MRO Implementation ID.
#define MHARTID    0xF14 // MRO Hardware thread ID.Machine Trap Setup


#define API __attribute__((visibility("default")))

//class FailureException : public std::exception {
//public:
//	FailureException(std::string message) : message(message){
//
//	}
//	std::string message;
//};

static std::string myvsprintf(const char* formatString, std::va_list args){
    std::va_list args2{};
    va_copy(args2, args);
    int r{std::vsnprintf(nullptr, 0, formatString, args)};

    if (r < 0) {
        // TODO: HANDLE ERROR
    }

    std::string s(r + 1, '\0');
    r = std::vsnprintf((char *)s.data(), r + 1, formatString, args2);

    if (r < 0) {
        // TODO: HANDLE ERROR
    }

    va_end(args2);
    s.pop_back();
    return s;
}

static std::string strFormat(const char* formatString, ...){
    std::va_list args{};
    va_start(args, formatString);
    std::string str = myvsprintf(formatString, args);
    va_end(args);
    return str;
}


//static void failure(){
//	throw std::runtime_error("");
//}

static void failure(std::string message){
	throw std::runtime_error(message);
}

static void failure(const char* formatString, ...){
    std::va_list args{};
    va_start(args, formatString);
    throw std::runtime_error(myvsprintf(formatString, args));
    va_end(args);
}


static void breakMe(){
    volatile int a = 0;
}


#define assertEq(message, x,ref) if((x) != (ref)) {\
	std::stringstream str; \
	str << hex << message << " DUT=" << x << " REF=" << ref << dec; \
	failure(str.str());\
}

#define assertTrue(message, x) if(!(x)) {\
	failure(message);\
}


