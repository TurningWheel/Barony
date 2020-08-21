#pragma once

/* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ValidateUTF8String() is based on https://stackoverflow.com/a/22135005/8552466
 */

#include <array>
#include <cstdint>

namespace UTFD
{

constexpr uint32_t UTF8_ACCEPT { 0 };
constexpr uint32_t UTF8_REJECT { 1 };

static constexpr std::array<uint8_t, 400> UTF8_DECODER
{
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	 8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	 0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
	12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
	12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
	12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
	12,36,12,12,12,12,12,12,12,12,12,12,
};

inline uint32_t DecodeUTF8(uint32_t* const codep, const uint32_t byte)
{
	if ( byte > UTF8_DECODER.size() )
	{
		return UTF8_REJECT;
	}

	const uint32_t type = UTF8_DECODER[byte];
	uint32_t state = UTF8_ACCEPT;

	*codep = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
	state = UTF8_DECODER[256 + state + type];

	return state;
}

// Returns 0 (UTF8_ACCEPT) if valid UTF8, 1 (UTF8_REJECT) if invalid, or greater than 1 if more processing is required (two byte character)
inline uint32_t ValidateUTF8String(char* const str, const size_t length)
{
	uint32_t state = UTF8_ACCEPT;

	for ( size_t charIndex = 0; charIndex < length; charIndex++ )
	{
		const uint32_t type = UTF8_DECODER[static_cast<uint8_t>(str[charIndex])];
		state = UTF8_DECODER[256 + state + type];

		if ( state == UTF8_REJECT )
		{
			break;
		}
	}

	return state;
}

// Returns 0 (UTF8_ACCEPT) if valid UTF8, 1 (UTF8_REJECT) if invalid, or greater than 1 if more processing is required (two byte character)
inline uint32_t ValidateUTF8Character(const char c)
{
	return UTF8_DECODER[256 + UTF8_ACCEPT + UTF8_DECODER[static_cast<uint8_t>(c)]];
}

}

