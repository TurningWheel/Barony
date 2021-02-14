#pragma once

#include <dirent.h>
#include "main.hpp"

class Directory {
public:
	Directory(const char* name) :
		path(name)
	{
		//TODO: Use datadir. rom:/ needs to get prepended...
		DIR* dir;
		struct dirent* ent;
		if ((dir = opendir(name)) == NULL)
		{
			printlog("failed to open directory '%s'", name);
			return;
		}
		while ((ent = readdir(dir)) != NULL)
		{
			std::string entry(ent->d_name);
			if (ent->d_name[0] != '.')
			{
				list.emplace_back(ent->d_name);
			}
		}
		closedir(dir);
		std::sort(list.begin(), list.end());
	}
	std::vector<std::string> list;
	const char* path;
};