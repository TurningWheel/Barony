/*-------------------------------------------------------------------------------

BARONY
File: custom_tools.cpp
Desc: misc modding tools

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/
#include "items.hpp"
#include "custom_tools.hpp"

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
const std::vector<std::string> MonsterStatCustomManager::itemStatusStrings =
{
	"broken",
	"decrepit",
	"worn",
	"serviceable",
	"excellent"
};