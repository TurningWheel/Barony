/*-------------------------------------------------------------------------------

	BARONY
	File: cppfuncs.hpp
	Desc: contains functions for random, generic, recycled code that gets used in every project under the sun for menial tasks.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/


#pragma once

#include <fstream>
#include <string>
#include <vector>
#include "prng.hpp"

template<typename T>
T randomEntryFromVector(std::vector<T> vector)
{
	if ( !vector.size() )
	{
		throw "Empty vector!";
	}

	return vector[rand() % vector.size()];
}
