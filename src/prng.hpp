#pragma once

#include <stddef.h>
#include "SDL.h"

void prng_seed_time (void);
void prng_seed_bytes (const void*, size_t);
unsigned char prng_get_octet (void);
unsigned char prng_get_byte (void);
void prng_get_bytes (void*, size_t);
unsigned long prng_get_ulong (void);
long prng_get_long (void);
Uint32 prng_get_uint (void);
int prng_get_int (void);
double prng_get_double (void);
double prng_get_double_normal (void);
