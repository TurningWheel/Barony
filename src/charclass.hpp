/*-------------------------------------------------------------------------------

BARONY
File: charclass.hpp
Desc: defines character classes

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once
#include "main.hpp"
#include "stat.hpp"
#include "json.hpp"
#include "files.hpp"

class PlayerCharacterClassManager
{
	Stat* classStats = nullptr;
	int charClass = CLASS_BARBARIAN;
public:
	PlayerCharacterClassManager(Stat* stat)
	{
		classStats = stat;
	};
	void serialize(FileInterface* file) {
		
		file->property("MAXHP", static_cast<Sint32>(classStats->MAXHP));
		file->property("MAXMP", static_cast<Sint32>(classStats->MAXMP));
		file->property("STR", static_cast<Sint32>(classStats->STR));
		file->property("DEX", static_cast<Sint32>(classStats->DEX));
		file->property("CON", static_cast<Sint32>(classStats->CON));
		file->property("INT", static_cast<Sint32>(classStats->INT));
		file->property("PER", static_cast<Sint32>(classStats->PER));
		file->property("CHR", static_cast<Sint32>(classStats->CHR));
		file->property("GOLD", static_cast<Sint32>(classStats->GOLD));
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			char prof[64];
			snprintf(prof, 64, "Proficiency::%s", getSkillLangEntry(i));
			file->property(prof, static_cast<Sint32>(classStats->PROFICIENCIES[i]));
		}
		classStats->HP = classStats->MAXHP;
		classStats->OLDHP = classStats->HP;
		classStats->MP = classStats->MAXMP;
	}

	void writeToFile()
	{
		std::string outputPath = outputdir;
		outputPath.append("/data/characterclasses.json");
		if ( FileHelper::writeObject(outputPath.c_str(), EFileFormat::Json, *this) )
		{
			printlog("[JSON]: Successfully read json file %s", outputPath.c_str());
		}
	}

	void readFromFile()
	{
		if ( PHYSFS_getRealDir("data/characterclasses.json") )
		{
			std::string inputPath = PHYSFS_getRealDir("data/characterclasses.json");
			inputPath.append("data/characterclasses.json");
			if ( FileHelper::readObject(inputPath.c_str(), *this) )
			{
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
};