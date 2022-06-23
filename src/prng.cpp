#include "prng.hpp"
#include "main.hpp"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <string.h>

BaronyRNG local_rng;
BaronyRNG net_rng;

static inline void swap_byte(uint8_t& a, uint8_t& b) {
    uint8_t t = a;
    a = b;
    b = t;
}

void BaronyRNG::seedImpl(const void* key, size_t size) {
	assert(key != nullptr && size > 0);
	for (int i = 0; i < 256; ++i) {
		buf[i] = i;
	}

	uint8_t b;
	auto bytes = static_cast<const uint8_t*>(key);
	for (int i = 0; i < 256; ++i) {
		b = b + buf[i] + bytes[i % size];
		swap_byte(buf[i], buf[b]);
	}

	memcpy(seed, buf, sizeof(seed));
	seed_size = size;

	i1 = i2 = 0;
	seeded = true;
}

void BaronyRNG::seedBytes(const void* key, size_t size) {
    seedImpl(key, size);
}

void BaronyRNG::seedTime() {
	time_t t = time(nullptr);
	seedImpl(&t, 4); // we only want a 32-bit seed
}

int BaronyRNG::getSeed(void* out, size_t size) {
    if (!seeded || size < seed_size) {
        return -1;
    }
    memcpy(out, seed, seed_size);
    return seed_size;
}

void BaronyRNG::getBytes(void* data_, size_t size) {
	if (!seeded) {
	    printlog("rng not seeded, seeding by unix time");
	    time_t t = time(nullptr);
	    seedImpl(&t, 4); // we only want a 32-bit seed
	}
	for (uint8_t* data = static_cast<uint8_t*>(data_); size-- > 0; ++data) {
	    i1 = i1 + 1;
	    i2 = i2 + buf[i1];
	    swap_byte(buf[i1], buf[i2]);
		*data = buf[(buf[i1] + buf[i2])];
	}
}

uint8_t BaronyRNG::getU8() {
    uint8_t result;
	getBytes(&result, sizeof(result));
	return result;
}

uint16_t BaronyRNG::getU16() {
    uint16_t result;
	getBytes(&result, sizeof(result));
	return result;
}

uint32_t BaronyRNG::getU32() {
    uint32_t result;
	getBytes(&result, sizeof(result));
	return result;
}

uint64_t BaronyRNG::getU64() {
    uint64_t result;
	getBytes(&result, sizeof(result));
	return result;
}

int8_t BaronyRNG::getI8() {
    int8_t result;
	getBytes(&result, sizeof(result));
	return result;
}

int16_t BaronyRNG::getI16() {
    int16_t result;
	getBytes(&result, sizeof(result));
	return result;
}

int32_t BaronyRNG::getI32() {
    int32_t result;
	getBytes(&result, sizeof(result));
	return result;
}

int64_t BaronyRNG::getI64() {
    int64_t result;
	getBytes(&result, sizeof(result));
	return result;
}

float BaronyRNG::getF32() {
    uint32_t u32;
	getBytes(&u32, sizeof(u32));
	return (float)u32 / ((uint64_t)1 << 32);
}

double BaronyRNG::getF64() {
    uint32_t u32;
	getBytes(&u32, sizeof(u32));
	return (double)u32 / ((uint64_t)1 << 32);
}

int BaronyRNG::distribution(const unsigned int* chances, int size) {
    if (size <= 0) {
        // list is smaller than 0
        assert(0 && "BaronyRNG::distribution() list is less-or-equal than 0");
        return -1;
    }

    unsigned int total = 0;
    for (int c = 0; c < size; ++c) {
        total += chances[c];
    }
    if (total == 0) {
        // nothing has a chance to be picked
        assert(0 && "BaronyRNG::distribution() chances of picking anything are 0");
        return -1;
    }

    unsigned int choice = getF32() * total;
    for (int c = 0; c < size; ++c) {
        if (chances[c] > choice) {
            return c;
        } else {
            choice -= chances[c];
        }
    }

    assert(0 && "BaronyRNG::distribution() nothing was picked. this should never happen");
    return -1;
}