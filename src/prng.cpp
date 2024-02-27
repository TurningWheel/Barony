#include "prng.hpp"
#include "main.hpp"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <string.h>

BaronyRNG local_rng;
BaronyRNG net_rng;
BaronyRNG map_sequence_rng;

#ifndef EDITOR
#include "interface/consolecommand.hpp"
#include "net.hpp"
static BaronyRNG test_rng;

static ConsoleCommand test_rng_seed(
    "/test_rng_seed",
    "seed test rng",
    [](int argc, const char* argv[]){
    if (argc < 2) {
        test_rng.seedTime();
    } else {
        auto seed = strtol(argv[1], nullptr, 10);
        test_rng.seedBytes(&seed, sizeof(seed));
    }
    });

static ConsoleCommand test_rng_seed_health(
    "/test_rng_seed_health",
    "test rng seed health",
    [](int argc, const char* argv[]){
    test_rng.testSeedHealth();
    });

static ConsoleCommand test_rng_u8(
    "/test_rng_u8",
    "test rng u8",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getU8();
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%d", (int)result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_i8(
    "/test_rng_i8",
    "test rng i8",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getI8();
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%d", (int)result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_f32(
    "/test_rng_f32",
    "test rng f32",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getF32();
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%.2f", result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_f64(
    "/test_rng_f64",
    "test rng f64",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getF64();
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%.2f", result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_uniform(
    "/test_rng_uniform",
    "test rng with uniform(a, b, iterations)",
    [](int argc, const char* argv[]){
    const int a = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : -10;
    const int b = argc > 2 ? (int)strtol(argv[2], nullptr, 10) : 10;
    const int i = argc > 3 ? (int)strtol(argv[3], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        int result = test_rng.uniform(a, b);
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%d", result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_discrete(
    "/test_rng_discrete",
    "test rng with discrete({chances}, iterations)",
    [](int argc, const char* argv[]){
    if (argc < 3) {
        messagePlayer(clientnum, MESSAGE_MISC, "args: {chances} iterations");
        return;
    }
    std::vector<unsigned int> chances;
    for (int c = 1; c < argc - 1; ++c) {
        unsigned int chance = (int)strtol(argv[c], nullptr, 10);
        chances.push_back(chance);
    }
    const int i = (int)strtol(argv[argc - 1], nullptr, 10);
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        int result = test_rng.discrete(chances.data(), chances.size());
        sum += result;
        //messagePlayer(clientnum, MESSAGE_MISC, "%d", result);
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_normal(
    "/test_rng_normal",
    "test rng with normal(mean, deviation, iterations)",
    [](int argc, const char* argv[]){
    const int m = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 0;
    const int d = argc > 2 ? (int)strtol(argv[2], nullptr, 10) : 5;
    const int i = argc > 3 ? (int)strtol(argv[3], nullptr, 10) : 100000;
    std::map<int, int> hist{};
    for (int c = 0; c < i; ++c) {
        int result = test_rng.normal(m, d);
        ++hist[result];
        //messagePlayer(clientnum, MESSAGE_MISC, "%d", result);
    }
    for (auto p : hist) {
        int value = p.second / 200;
        if (value) {
            std::string s(value, '*');
            messagePlayer(clientnum, MESSAGE_MISC, "%5d %s", p.first, s.c_str());
        }
    }
    });
#endif

void BaronyRNG::testSeedHealth() const {
	std::string seed_str;
	seed_str.reserve(2049);
	real_t sum = 0.0;
	for (int c = 0; c < 256; ++c) {
	    for (int b = 0; b < 8; ++b) {
	        if (buf[c] & (1 << b)) {
	            sum += 1.0;
	            seed_str.append("1");
	        } else {
	            seed_str.append("0");
	        }
	    }
	}
	sum /= 2048.0;
	printlog("rng seed bits are %.2f%% on", sum * 100.0);
	printlog("seed: %s", seed_str.c_str());
}

size_t BaronyRNG::bytesRead() const {
    return bytes_read;
}

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

	uint8_t b = 0;
	auto bytes = static_cast<const uint8_t*>(key);
	for (int i = 0; i < 256; ++i) {
		b = b + buf[i] + bytes[i % size];
		swap_byte(buf[i], buf[b]);
	}

	memcpy(seed, key, size);
	seed_size = size;

	i1 = i2 = 0;
	bytes_read = 0;
	seeded = true;
}

void BaronyRNG::seedBytes(const void* key, size_t size) {
    seedImpl(key, size);
}

void BaronyRNG::seedTime() {
    // we only want a 32-bit seed
    uint32_t t = (uint32_t)getTime();
	seedImpl(&t, sizeof(t));
}

int BaronyRNG::getSeed(void* out, size_t size) const {
    if (!seeded || size < seed_size) {
        assert(0 && "wtf are you doin");
        return -1;
    }
    memcpy(out, seed, seed_size);
    return seed_size;
}

void BaronyRNG::getBytes(void* data_, size_t size) {
#ifndef NDEBUG
    /*if (this == &local_rng) {
        std::string str = stackTrace();
        if (!str.empty() && str.find("gameLogic") == std::string::npos) {
            printlog(str.c_str());
        }
    }*/
#endif
	if (!seeded) {
	    printlog("rng not seeded, seeding by unix time");
        // we only want a 32-bit seed
        uint32_t t = (uint32_t)getTime();
	    seedImpl(&t, sizeof(t));
	}
	for (uint8_t* data = static_cast<uint8_t*>(data_); size-- > 0; ++data) {
	    i1 = ((int)i1 + 1) & 255;
	    i2 = ((int)i2 + buf[i1]) & 255;
	    swap_byte(buf[i1], buf[i2]);
		*data = buf[(buf[i1] + buf[i2]) & 255];
		++bytes_read;
	}
#ifdef NDEBUG
	//checkMarker();
#endif
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
	constexpr uint64_t div = (uint64_t)1 << 32;
	return (float)u32 / div;
}

double BaronyRNG::getF64() {
    uint32_t u32;
	getBytes(&u32, sizeof(u32));
	constexpr uint64_t div = (uint64_t)1 << 32;
	return (double)u32 / div;
}

int BaronyRNG::rand() {
    int i;
	getBytes(&i, sizeof(i));
    return i & 0x7fffffff;
}

int BaronyRNG::uniform(int a, int b) {
    if (a == b) {
        return a;
    }
    int min = std::min(a, b);
    int max = std::max(a, b);
    int diff = (max - min) + 1;
    int choice = getF64() * diff;
    return min + choice;
}

int BaronyRNG::discrete(const unsigned int* chances, int size) {
    if (size <= 0) {
        // list is smaller than 0
        assert(0 && "BaronyRNG::discrete() list is less-or-equal than 0");
        return 0;
    }

    unsigned int total = 0;
    for (int c = 0; c < size; ++c) {
        total += chances[c];
    }
    if (total == 0) {
        // nothing has a chance to be picked
        assert(0 && "BaronyRNG::discrete() chances of picking anything are 0");
        return 0;
    }

    unsigned int choice = getF64() * total;
    for (int c = 0; c < size; ++c) {
        if (chances[c] > choice) {
            return c;
        } else {
            choice -= chances[c];
        }
    }

    assert(0 && "BaronyRNG::discrete() nothing was picked. this should never happen");
    return 0;
}

int BaronyRNG::normal(int mean, int deviation) {
    const real_t m = mean;
    const real_t d = deviation;
    const real_t f1 = getF64();
    const real_t f2 = getF64();
    const real_t norm = cos(2.0 * PI * f2) * sqrt(-2.0 * log(f1));
    return round(norm * d + m);
}

void BaronyRNG::setMarker() const {
#ifndef NDEBUG
    memcpy(marker, buf, sizeof(marker));
#endif
}

void BaronyRNG::checkMarker() const {
#ifndef NDEBUG
    if (!memcmp(marker, buf, sizeof(marker))) {
        printlog("reached marker");
    }
#endif
}
