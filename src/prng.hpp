/*-------------------------------------------------------------------------------

	BARONY
	File: prng.hpp
	Desc: prototypes for prng.cpp, pseudo-random number generation

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <stddef.h>
#include <stdint.h>

class BaronyRNG {
public:
    void seedTime();                      // seed according to a 32-bit time value
    void seedBytes(const void*, size_t);  // seed given byte buffer (uses 256 bytes at most)
    void getBytes(void*, size_t);         // fill a buffer with pseudo-random bytes

    // fill a buffer of given size with our seed value (for instance to reseed)
    // return the size of the seed or -1 if the buffer was not large enough
    int getSeed(void*, size_t);

    uint8_t  getU8();   // get number in range [0 - 2^8)
    uint16_t getU16();  // get number in range [0 - 2^16)
    uint32_t getU32();  // get number in range [0 - 2^32)
    uint64_t getU64();  // get number in range [0 - 2^64)
    int8_t   getI8();   // get number in range [-(2^8)/2 - (2^8)/2)
    int16_t  getI16();  // get number in range [-(2^16)/2 - (2^16)/2)
    int32_t  getI32();  // get number in range [-(2^32)/2 - (2^32)/2)
    int64_t  getI64();  // get number in range [-(2^64)/2 - (2^64)/2)
    float    getF32();  // get number in range [0.0 - 1.0)
    double   getF64();  // get number in range [0.0 - 1.0)

    // pick a number using a list of chances to pick each number
    int distribution(const unsigned int* chances, int size);

private:
    bool seeded = false; // initialized or not
    uint8_t seed[256];   // seed
    uint8_t seed_size;   // seed size
    uint8_t buf[256];    // rng buffer
    uint8_t i1;          // rng index 1
    uint8_t i2;          // rng index 2

    void seedImpl(const void*, size_t);
};

extern BaronyRNG local_rng; // RNG for anything that does not require client synchronization
extern BaronyRNG net_rng;   // RNG which must always be synchronized among all clients
extern BaronyRNG map_rng;   // used strictly during map generation