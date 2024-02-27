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
	PlayerCharacterClassManager(Stat* const myStats, const int charClass)
	{
		classStats = myStats;
		characterClass = charClass;
	};
	bool serialize(FileInterface* const file) {
		// recommend you start with this because it makes versioning way easier down the road
		int version = 0;
		file->property("version", version);

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

		// doing a vector is automatic:
		/*std::vector<Uint32> pros;
		file->property("Proficiencies", pros);
		}*/

		// how to do an raw array:
		/*file->propertyName("Proficiencies");
		Uint32 arrSize = NUMPROFICIENCIES;
		file->beginArray(arrSize);
		assert(arrSize == NUMPROFICIENCIES); // error out if the array size in json is not correct!
		for (Uint32 c = 0; c < arrSize; ++c) {
			file->value(classStats->PROFICIENCIES[c]);
		}
		file->endArray();*/

		// how to do an inline object:
		file->propertyName("Proficiencies");
		file->beginObject();
		int numProficiencies = NUMPROFICIENCIES;
		file->property("NumProficiencies", numProficiencies);
		for ( int i = 0; i < numProficiencies; ++i )
		{
			char str[64];
			snprintf(str, 64, "%s", getSkillLangEntry(i));
			int prof = classStats->getProficiency(i);
			file->property(str, prof);
		}
		file->endObject();
		return true;
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

	void setCharacterStatsAfterSerialization() const
	{
		classStats->HP = classStats->MAXHP;
		classStats->OLDHP = classStats->HP;
		classStats->MP = classStats->MAXMP;
	}
};
