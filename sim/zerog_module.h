#ifndef ZEROG_MODULE_H
#define ZEROG_MODULE_H

#include "rand.h"
#include <stdint.h>

/** This is a module that will 'encapsulate' the functions we will be using in the simulation
with the actual code for the hardware. This is achieved by changing the compiling conditions in 
zerog_module.c to HARDWARE (TODO name might change). 
\see zerog_module.c */

void zerog_init(); 
void zerog_comms_send(char*);
char* zerog_comms_receive();
void zerog_distortion(void*); 
void zerog_cycle();

struct zerog_parts_t {
  uint16_t battery; /** The virtual battery. Initially set to max -> 2^16 */
  uint8_t binmode; /** Info for how our binary was compiled. */
};

#endif 