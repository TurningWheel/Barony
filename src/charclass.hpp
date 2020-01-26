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
	int characterClass = CLASS_BARBARIAN;
public:
	PlayerCharacterClassManager(Stat* myStats, int charClass)
	{
		classStats = myStats;
		characterClass = charClass;
		proficiencies.numProficiencies = NUMPROFICIENCIES;
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			proficiencies.list.push_back(classStats->PROFICIENCIES[i]);
		}
	};
	void serialize(FileInterface* file) {
		file->property("CLASS", characterClass);
		file->property("MAXHP", classStats->MAXHP);
		file->property("MAXMP", classStats->MAXMP);
		file->property("STR", classStats->STR);
		file->property("DEX", classStats->DEX);
		file->property("CON", classStats->CON);
		file->property("INT", classStats->INT);
		file->property("PER", classStats->PER);
		file->property("CHR", classStats->CHR);
		file->property("GOLD", classStats->GOLD);
		file->property("Proficiencies", proficiencies);
	}

	struct Proficiencies
	{
		int numProficiencies = NUMPROFICIENCIES;
		std::vector<Sint32> list;
		void serialize(FileInterface* file)
		{
			file->property("NumProficiencies", numProficiencies);
			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				char str[64];
				snprintf(str, 64, "%s", getSkillLangEntry(i));
				file->property(str, list[i]);
			}
		}
	} proficiencies;

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
		if ( PHYSFS_getRealDir("/data/characterclasses.json") )
		{
			std::string inputPath = PHYSFS_getRealDir("/data/characterclasses.json");
			inputPath.append("/data/characterclasses.json");
			if ( FileHelper::readObject(inputPath.c_str(), *this) )
			{
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
				setCharacterStatsAfterSerialization();
			}
		}
	}

	void setCharacterStatsAfterSerialization()
	{
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			classStats->PROFICIENCIES[i] = proficiencies.list[i];
		}
		classStats->HP = classStats->MAXHP;
		classStats->OLDHP = classStats->HP;
		classStats->MP = classStats->MAXMP;

	}
};