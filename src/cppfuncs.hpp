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

	return vector[prng_get_uint() % vector.size()];
}

inline std::vector<std::string> getLinesFromFile(std::string filename)
{
	std::vector<std::string> lines;
	std::ifstream file(filename);
	if ( !file )
	{
		printlog("Error: Failed to open file \"%s\"", filename.c_str());
		return lines;
	}
	std::string line;
	while ( std::getline(file, line) )
	{
		if ( !line.empty() )
		{
			lines.push_back(line);
		}
	}
	file.close();

	return lines;
}
