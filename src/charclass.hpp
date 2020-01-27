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
	const std::vector<std::string> kProficiencyNames =
	{
		"Tinkering",
		"Stealth",
		"Trading",
		"Appraise",
		"Swimming",
		"Leader",
		"Casting",
		"Magic",
		"Ranged",
		"Sword",
		"Mace",
		"Axe",
		"Polearm",
		"Shield",
		"Unarmed",
		"Alchemy"
	};
public:
	PlayerCharacterClassManager(Stat* myStats, int charClass)
	{
		classStats = myStats;
		characterClass = charClass;
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

		file->propertyName("Proficiencies");
		//if ( file->isReading() )
		//{
		//	for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		//	{
		//		classStats->PROFICIENCIES[i] = 0;
		//	}

		//	file->beginObject();
		//	for ( int i = 0; i < kProficiencyNames.size() && i < NUMPROFICIENCIES; ++i )
		//	{
		//		file->property(kProficiencyNames[i].c_str(), classStats->PROFICIENCIES[i]);
		//	}
		//	file->endObject();
		//}
		//else
		//{
		//	// writing out proficiencies to file
		//	Uint32 numProficiencies = NUMPROFICIENCIES;
		//	file->beginObject();
		//	for ( int i = 0; i < kProficiencyNames.size() && i < numProficiencies; ++i )
		//	{
		//		file->property(kProficiencyNames[i].c_str(), classStats->PROFICIENCIES[i]);
		//	}
		//	file->endObject();
		//}
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
		classStats->HP = classStats->MAXHP;
		classStats->OLDHP = classStats->HP;
		classStats->MP = classStats->MAXMP;

	}
};