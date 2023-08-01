/*
 * global.h
 *
 *  Created on: Aug 1, 2023
 *      Author: rawrr
 */

#pragma once

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


#define API __attribute__((visibility("default")))

class successException : public std::exception { };
#define failure() throw std::exception();
#define success() throw successException();
static void breakMe(){
    volatile int a = 0;
}

#define assertEq(message, x,ref) if((x) != (ref)) {\
    cout << hex << "\n*** " << message << " DUT=" << x << " REF=" << ref << " ***\n\n" << dec;\
    breakMe();\
    failure();\
}

#define assertTrue(message, x) if(!(x)) {\
    printf("\n*** %s ***\n\n",message);\
    breakMe();\
    failure();\
}



