/*-------------------------------------------------------------------------------

	BARONY
	File: actmonster.cpp
	Desc: behavior function for monsters

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "shops.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "colors.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"
#include "ui/MainMenu.hpp"
#include "menu.hpp"

float limbs[NUMMONSTERS][20][3];

// determines which monsters fight which
bool swornenemies[NUMMONSTERS][NUMMONSTERS] =
{
//    N  H  R  G  S  T  B  S  G  S  S  I  C  G  D  S  M  L  M  D  S  K  S  G  I  V  S  C  I  G  A  L  L  S  S  G  D  B   
//    O  U  A  O  L  R  A  P  H  K  C  M  R  N  E  U  I  I  I  E  H  O  C  O  N  A  H  O  N  O  U  I  I  N  P  Y  U  U   
//    T  M  T  B  I  O  T  I  O  E  O  P  A  O  M  C  M  C  N  V  P  B  A  L  C  M  A  C  S  A  T  F  I  T  L  R  M  G     
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1 }, // HUMAN
	{ 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // RAT
	{ 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0 }, // GOBLIN
	{ 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1 }, // SLIME
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // TROLL
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1 }, // BAT_SMALL
	{ 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // SPIDER
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // GHOUL
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // SKELETON
	{ 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // SCORPION
	{ 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // IMP
	{ 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // CRAB
	{ 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0 }, // GNOME
	{ 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1 }, // DEMON
	{ 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // SUCCUBUS
	{ 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // MIMIC
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // LICH
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // MINOTAUR
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // DEVIL
	{ 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHOPKEEPER
	{ 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0 }, // KOBOLD
	{ 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0 }, // SCARAB
	{ 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0 }, // CRYSTALGOLEM
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // INCUBUS
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // VAMPIRE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // SHADOW
	{ 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // COCKATRICE
	{ 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // INSECTOID
	{ 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // GOATMAN
	{ 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // AUTOMATON
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // LICH_ICE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }, // LICH_FIRE
	{ 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // SENTRYBOT
	{ 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // SPELLBOT
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GYROBOT
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DUMMYBOT
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 }  // BUGBEAR
//    N  H  R  G  S  T  B  S  G  S  S  I  C  G  D  S  M  L  M  D  S  K  S  G  I  V  S  C  I  G  A  L  L  S  S  G  D  B   
//    O  U  A  O  L  R  A  P  H  K  C  M  R  N  E  U  I  I  I  E  H  O  C  O  N  A  H  O  N  O  U  I  I  N  P  Y  U  U   
//    T  M  T  B  I  O  T  I  O  E  O  P  A  O  M  C  M  C  N  V  P  B  A  L  C  M  A  C  S  A  T  F  I  T  L  R  M  G 
};

// determines which monsters come to the aid of other monsters
bool monsterally[NUMMONSTERS][NUMMONSTERS] =
{
//    N  H  R  G  S  T  B  S  G  S  S  I  C  G  D  S  M  L  M  D  S  K  S  G  I  V  S  C  I  G  A  L  L  S  S  G  D  B   
//    O  U  A  O  L  R  A  P  H  K  C  M  R  N  E  U  I  I  I  E  H  O  C  O  N  A  H  O  N  O  U  I  I  N  P  Y  U  U   
//    T  M  T  B  I  O  T  I  O  E  O  P  A  O  M  C  M  C  N  V  P  B  A  L  C  M  A  C  S  A  T  F  I  T  L  R  M  G    
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // HUMAN
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // RAT
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // GOBLIN
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // SLIME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // TROLL
	{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // BAT_SMALL
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SPIDER
	{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0 }, // GHOUL
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // SKELETON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SCORPION
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // IMP
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // CRAB
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GNOME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // DEMON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // SUCCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MIMIC
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // LICH
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MINOTAUR
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // DEVIL
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHOPKEEPER
	{ 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // KOBOLD
	{ 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SCARAB
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // CRYSTALGOLEM
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // INCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // VAMPIRE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // SHADOW
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // COCKATRICE
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // INSECTOID
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0 }, // GOATMAN
	{ 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // AUTOMATON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // LICH_ICE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // LICH_FIRE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // SENTRYBOT
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // SPELLBOT
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // GYROBOT
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 }, // DUMMYBOT
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // BUGBEAR
//    N  H  R  G  S  T  B  S  G  S  S  I  C  G  D  S  M  L  M  D  S  K  S  G  I  V  S  C  I  G  A  L  L  S  S  G  D  B   
//    O  U  A  O  L  R  A  P  H  K  C  M  R  N  E  U  I  I  I  E  H  O  C  O  N  A  H  O  N  O  U  I  I  N  P  Y  U  U   
//    T  M  T  B  I  O  T  I  O  E  O  P  A  O  M  C  M  C  N  V  P  B  A  L  C  M  A  C  S  A  T  F  I  T  L  R  M  G 
};

// monster sight ranges
double sightranges[NUMMONSTERS] =
{
	256,  // NOTHING
	256,  // HUMAN
	128,  // RAT
	256,  // GOBLIN
	80,   // SLIME
	32,   // TROLL
	128,    // BAT_SMALL
	96,   // SPIDER
	128,  // GHOUL
	192,  // SKELETON
	96,   // SCORPION
	256,  // IMP
	96,   // CRAB
	128,  // GNOME
	256,  // DEMON
	256,  // SUCCUBUS
	256,  // MIMIC
	512,  // LICH
	512,  // MINOTAUR
	1024, // DEVIL
	256,  // SHOPKEEPER
	192,  // KOBOLD
	512,  // SCARAB
	192,  // CRYSTALGOLEM
	256,  // INCUBUS
	192,  // VAMPIRE
	768,  // SHADOW
	256,  // COCKATRICE
	256,  // INSECTOID
	256,  // GOATMAN
	192,  // AUTOMATON
	512,  // LICH_ICE
	512,  // LICH_FIRE
	256,  // SENTRYBOT
	192,  // SPELLBOT
	256,  // GYROBOT
	32,   // DUMMYBOT
	128	  // BUGBEAR
};

int monsterGlobalAnimationMultiplier = 10;
int monsterGlobalAttackTimeMultiplier = 1;

std::string getMonsterLocalizedName(Monster creature)
{
	if ( creature == BUGBEAR )
	{
		return Language::get(6256);
	}
	else if ( creature < KOBOLD )
	{
	    if (creature == SPIDER && ((!intro && arachnophobia_filter) || (intro && MainMenu::arachnophobia_filter)) ) {
		    return Language::get(102);
	    } else {
		    return Language::get(90 + creature);
	    }
	}
	else
	{
		return Language::get(2000 + creature - KOBOLD);
	}
	return "nothing";
}

std::string getMonsterLocalizedPlural(Monster creature)
{
	if ( creature == BUGBEAR )
	{
		return Language::get(6257);
	}
	if ( creature < KOBOLD )
	{
	    if (creature == SPIDER && ((!intro && arachnophobia_filter) || (intro && MainMenu::arachnophobia_filter))) {
		    return Language::get(123);
	    } else {
		    return Language::get(111 + creature);
	    }
	}
	else
	{
		return Language::get(2050 + creature - KOBOLD);
	}
	return "nothings";
}
std::string getMonsterLocalizedInjury(Monster creature)
{
	if ( creature == BUGBEAR )
	{
		return Language::get(6258);
	}
	if ( creature < KOBOLD )
	{
	    if (creature == SPIDER && ((!intro && arachnophobia_filter) || (intro && MainMenu::arachnophobia_filter)) ) {
		    return Language::get(144);
	    } else {
		    return Language::get(132 + creature);
	    }
	}
	else
	{
		return Language::get(2100 + creature - KOBOLD);
	}
	return "hit type";
}

void ShopkeeperPlayerHostility_t::reset()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		playerHostility[i].clear();
	}
}
ShopkeeperPlayerHostility_t::ShopkeeperPlayerHostility_t()
{
	reset();
};

void ShopkeeperPlayerHostility_t::resetPlayerHostility(const int player, bool clearAll)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return; }

	if ( auto h = getPlayerHostility(player) )
	{
		if ( h->wantedLevel != NO_WANTED_LEVEL && h->wantedLevel != FAILURE_TO_IDENTIFY )
		{
			if ( h->equipment == 0 ) // if we had a facemask, the wanted would still be in effect
			{
				messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(4304));
			}
		}
		if ( h->wantedLevel != FAILURE_TO_IDENTIFY )
		{
			h->wantedLevel = NO_WANTED_LEVEL;
			h->bRequiresNetUpdate = true;
		}
	}
	if ( clearAll )
	{
		Uint32 type = stats[player]->type;
		Uint8 equipment = 0;
		if ( type == SUCCUBUS || type == INCUBUS
			|| type == TROLL || type == RAT || type == SPIDER || type == CREATURE_IMP )
		{
			// no sex modifier
		}
		else
		{
			type |= (stats[player]->sex == sex_t::MALE ? 0 : 1) << 8;
		}

		if ( (type & 0xFF) == TROLL || (type & 0xFF) == RAT || (type & 0xFF) == SPIDER || (type & 0xFF) == CREATURE_IMP )
		{
			// no equipment modifier
		}
		else
		{
			if ( stats[player]->mask && stats[player]->mask->type == MASK_BANDIT && !(players[player]->entity && players[player]->entity->isInvisible()) )
			{
				equipment = 1 + (stats[player]->mask->appearance % items[MASK_BANDIT].variations);
			}
			type |= (0x7F & (equipment)) << 9;
		}

		for ( auto h2 = playerHostility[player].begin(); h2 != playerHostility[player].end(); ++h2 )
		{
			if ( h2->first == type )
			{
				continue;
			}
			if ( h2->second.wantedLevel != NO_WANTED_LEVEL && h2->second.wantedLevel != FAILURE_TO_IDENTIFY )
			{
				h2->second.wantedLevel = NO_WANTED_LEVEL;
				h2->second.bRequiresNetUpdate = true;
			}
		}
	}
}

bool ShopkeeperPlayerHostility_t::isPlayerEnemy(const int player)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }

	Monster type = stats[player]->type;
	if ( !playerRaceCheckHostility(player, type) )
	{ 
		return true;
	}
	if ( auto h = getPlayerHostility(player) )
	{
		return h->wantedLevel != NO_WANTED_LEVEL;
	}
	return false;
}

bool ShopkeeperPlayerHostility_t::playerRaceCheckHostility(const int player, const Monster type) const
{
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	if ( type != HUMAN && type != AUTOMATON ) 
	{
		if ( stats[player] && stats[player]->mask && stats[player]->mask->type == MONOCLE )
		{
			if ( !stats[player]->EFFECTS[EFF_SHAPESHIFT] && !(players[player]->entity && players[player]->entity->isInvisible()) )
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

ShopkeeperPlayerHostility_t::PlayerRaceHostility_t* ShopkeeperPlayerHostility_t::getPlayerHostility(const int player, Uint32 overrideType)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return nullptr; }

	Uint32 type = stats[player]->type;
	Uint8 equipment = 0;
	if ( type == SUCCUBUS || type == INCUBUS
		|| type == TROLL || type == RAT || type == SPIDER || type == CREATURE_IMP )
	{
		// no sex modifier
	}
	else
	{
		type |= (stats[player]->sex == sex_t::MALE ? 0 : 1) << 8;
	}

	if ( (type & 0xFF) == TROLL || (type & 0xFF) == RAT || (type & 0xFF) == SPIDER || (type & 0xFF) == CREATURE_IMP )
	{
		// no equipment modifier
	}
	else
	{
		if ( stats[player]->mask && stats[player]->mask->type == MASK_BANDIT && !(players[player]->entity && players[player]->entity->isInvisible()) )
		{
			equipment = 1 + (stats[player]->mask->appearance % items[MASK_BANDIT].variations);
		}
		type |= (0x7F & (equipment)) << 9;
	}

	if ( overrideType != NOTHING )
	{
		type = overrideType;
	}

	if ( playerHostility[player].find(type) == playerHostility[player].end() )
	{
		auto wantedLevel = NO_WANTED_LEVEL;
		if ( equipment >= 1 && equipment <= 3 )
		{
			wantedLevel = FAILURE_TO_IDENTIFY;
		}
		playerHostility[player].emplace(std::make_pair(type, PlayerRaceHostility_t(type, wantedLevel, player)));
	}
	else
	{
		if ( playerHostility[player][type].wantedLevel == NO_WANTED_LEVEL )
		{
			if ( equipment >= 1 && equipment <= 3 )
			{
				playerHostility[player][type].wantedLevel = FAILURE_TO_IDENTIFY;
			}
		}
	}
	return &playerHostility[player][type];
}

ShopkeeperPlayerHostility_t::WantedLevel ShopkeeperPlayerHostility_t::getWantedLevel(const int player)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return NO_WANTED_LEVEL; }

	//if ( !playerRaceCheckHostility(player, type) ) { return NO_WANTED_LEVEL; }
	if ( auto h = getPlayerHostility(player) )
	{
		return h->wantedLevel;
	}
	return NO_WANTED_LEVEL;
}

void ShopkeeperPlayerHostility_t::setWantedLevel(ShopkeeperPlayerHostility_t::PlayerRaceHostility_t& h, 
	ShopkeeperPlayerHostility_t::WantedLevel wantedLevel, Entity* shopkeeper, bool primaryPlayerCheck)
{
	assert(h.player >= 0 && h.player < MAXPLAYERS);
	assert(multiplayer != CLIENT);
	if ( h.player < 0 || h.player >= MAXPLAYERS ) { return; }

	bool checkNearbyPlayersForAccessory = false;
	if ( wantedLevel > NO_WANTED_LEVEL && wantedLevel != WANTED_FOR_AGGRESSION_SHOPKEEP_INITIATED )
	{
		checkNearbyPlayersForAccessory = primaryPlayerCheck;
	}

	if ( h.wantedLevel == NO_WANTED_LEVEL && wantedLevel > NO_WANTED_LEVEL )
	{
		if ( wantedLevel == WANTED_FOR_ACCESSORY )
		{
			messagePlayerColor(h.player, MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(4306));
		}
		else
		{
			messagePlayerColor(h.player, MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(4305));
		}

		Compendium_t::Events_t::eventUpdateCodex(h.player, Compendium_t::CPDM_WANTED_RUNS, "wanted", 1);
		Compendium_t::Events_t::eventUpdateCodex(h.player, Compendium_t::CPDM_WANTED_TIMES_RUN, "wanted", 1);
	}
	if ( h.wantedLevel != wantedLevel )
	{
		h.bRequiresNetUpdate = true;
	}
	if ( h.wantedLevel < wantedLevel )
	{
		h.wantedLevel = wantedLevel;
	}
	//messagePlayer(0, MESSAGE_DEBUG, "Player %d wanted level: %d", h.player, wantedLevel);
	
	if ( !checkNearbyPlayersForAccessory ) { return; }

	if ( shopkeeper )
	{
		bool inshop = false;
		//if ( shopx >= 0 && shopx < map.width && shopy >= 0 && shopy < map.height )
		//{
		//	// if the crime was inside a shop
		//	if ( shoparea[shopy + shopx * map.height] )
		//	{
		//		inshop = true;
		//	}
		//}
		//if ( base_playerx >= 0 && base_playerx < map.width && base_playery >= 0 && base_playery < map.height )
		//{
		//	// if the crime was inside a shop
		//	if ( shoparea[base_playery + base_playerx * map.height] )
		//	{
		//		inshop = true;
		//	}
		//}
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( h.player == i ) { continue; }
			if ( !players[i]->entity ) { continue; }

			if ( inshop )
			{
				int secondary_playerx = static_cast<int>(players[i]->entity->x) >> 4;
				int secondary_playery = static_cast<int>(players[i]->entity->y) >> 4;
				if ( secondary_playerx >= 0 && secondary_playerx < map.width && secondary_playery >= 0 && secondary_playery < map.height )
				{
					// if the accessory was inside a shop, no need for LOS checks.
					if ( shoparea[secondary_playery + secondary_playerx * map.height] )
					{
						if ( auto h2 = getPlayerHostility(i) )
						{
							setWantedLevel(*h2, WantedLevel::WANTED_FOR_ACCESSORY, shopkeeper, false);
							++h2->numAccessories;
							Compendium_t::Events_t::eventUpdateCodex(h.player, Compendium_t::CPDM_WANTED_INFLUENCE, "wanted", 1);
							Compendium_t::Events_t::eventUpdateCodex(h2->player, Compendium_t::CPDM_WANTED_CRIMES_RUN, "wanted", 1);
							continue;
						}
					}
				}
			}

			real_t monsterVisionRange = sightranges[SHOPKEEPER];
			int light = players[i]->entity->entityLightAfterReductions(*stats[i], shopkeeper);
			double targetdist = sqrt(pow(shopkeeper->x - players[i]->entity->x, 2) + pow(shopkeeper->y - players[i]->entity->y, 2));

			if ( targetdist > monsterVisionRange )
			{
				continue;
			}
			if ( targetdist < light )
			{
				Entity* ohitentity = hit.entity;
				real_t tangent = atan2(players[i]->entity->y - shopkeeper->y, players[i]->entity->x - shopkeeper->x);
				lineTrace(shopkeeper, shopkeeper->x, shopkeeper->y, tangent, monsterVisionRange, 0, true);

				bool found = hit.entity == players[i]->entity;
				hit.entity = ohitentity;

				if ( found )
				{
					if ( auto h2 = getPlayerHostility(i) )
					{
						setWantedLevel(*h2, WantedLevel::WANTED_FOR_ACCESSORY, shopkeeper, false);
						++h2->numAccessories;
						Compendium_t::Events_t::eventUpdateCodex(h.player, Compendium_t::CPDM_WANTED_INFLUENCE, "wanted", 1);
						Compendium_t::Events_t::eventUpdateCodex(h2->player, Compendium_t::CPDM_WANTED_CRIMES_RUN, "wanted", 1);
					}
				}
			}
		}
	}
}

void ShopkeeperPlayerHostility_t::onShopkeeperDeath(Entity* my, Stat* myStats, Entity* attacker)
{
	if ( shopIsMysteriousShopkeeper(my) ) { return; }
	if ( my && myStats && attacker && myStats->type == SHOPKEEPER )
	{
		if ( attacker->behavior == &actPlayer )
		{
			if ( auto h = getPlayerHostility(attacker->skill[2]) )
			{
				setWantedLevel(*h, WantedLevel::WANTED_FOR_KILL, my, true);
				++h->numKills;
				Compendium_t::Events_t::eventUpdateCodex(h->player, Compendium_t::CPDM_WANTED_CRIMES_RUN, "wanted", 1);
			}
		}
		else if ( attacker->behavior == &actMonster )
		{
			if ( Entity* leader = attacker->monsterAllyGetPlayerLeader() )
			{
				ShopkeeperPlayerHostility.onShopkeeperHit(my, myStats, leader);
			}
		}
	}
}
void ShopkeeperPlayerHostility_t::onShopkeeperHit(Entity* my, Stat* myStats, Entity* attacker)
{
	if ( shopIsMysteriousShopkeeper(my) ) { return; }
	if ( my && myStats && attacker && myStats->type == SHOPKEEPER )
	{
		if ( attacker->behavior == &actPlayer )
		{
			if ( auto h = getPlayerHostility(attacker->skill[2]) )
			{
				setWantedLevel(*h, WantedLevel::WANTED_FOR_AGGRESSION, my, true);
				++h->numAggressions;
				Compendium_t::Events_t::eventUpdateCodex(h->player, Compendium_t::CPDM_WANTED_CRIMES_RUN, "wanted", 1);
			}
		}
	}
}

void ShopkeeperPlayerHostility_t::updateShopkeeperActMonster(Entity& my, Stat& myStats, bool ringconflict)
{
	// unused? if attacking after a ring of conflict then it could set enemy
	return;
	if ( ringconflict ) { return; }
	if ( shopIsMysteriousShopkeeper(&my) ) { return; }
	if ( Entity* entity = uidToEntity(my.monsterTarget) )
	{
		if ( entity->behavior == &actPlayer )
		{
			const int player = entity->skill[2];
			if ( auto h = getPlayerHostility(player) )
			{
				auto oldWantedLevel = h->wantedLevel;
				setWantedLevel(*h, WantedLevel::WANTED_FOR_AGGRESSION_SHOPKEEP_INITIATED, &my, false);
				if ( h->numAggressions == 0 && oldWantedLevel == WantedLevel::NO_WANTED_LEVEL )
				{
					++h->numAggressions;
				}
			}
		}
	}
}

void ShopkeeperPlayerHostility_t::serverSendClientUpdate(const bool force)
{
	if ( multiplayer != SERVER ) { return; }

	for ( int c = 1; c < MAXPLAYERS; ++c )
	{
		if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
		{
			continue;
		}
		
		for ( auto& entry : playerHostility[c] )
		{
			auto& hostility = entry.second;
			if ( !force && !hostility.bRequiresNetUpdate )
			{
				continue;
			}
			hostility.bRequiresNetUpdate = false;

			strcpy((char*)net_packet->data, "SHPH");
			net_packet->data[4] = (int)hostility.wantedLevel;
			const Uint16 max = 0xFFFF;
			SDLNet_Write16(std::min(max, (Uint16)hostility.numKills), &net_packet->data[5]);
			SDLNet_Write16(std::min(max, (Uint16)hostility.numAggressions), &net_packet->data[7]);
			SDLNet_Write16(std::min(max, (Uint16)hostility.numAccessories), &net_packet->data[9]);
			SDLNet_Write32(hostility.type, &net_packet->data[11]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

ShopkeeperPlayerHostility_t ShopkeeperPlayerHostility;

void Entity::updateEntityOnHit(Entity* attacker, bool alertTarget)
{
	if ( !attacker ) return;
	if ( attacker == this ) { return; }

	if ( Stat* myStats = getStats() )
	{
		if ( myStats->type == SHOPKEEPER )
		{
			if ( alertTarget )
			{
				if ( attacker->behavior == &actPlayer )
				{
					ShopkeeperPlayerHostility.onShopkeeperHit(this, myStats, attacker);
				}
				/*else if ( attacker->behavior == &actMonster )
				{
					if ( Entity* leader = attacker->monsterAllyGetPlayerLeader() )
					{
						ShopkeeperPlayerHostility.onShopkeeperHit(this, myStats, leader);
					}
				}*/
			}
		}
		else if ( myStats->type == MIMIC )
		{
			disturbMimic(attacker, true, true);
		}
		else if ( myStats->type == BAT_SMALL )
		{
			disturbBat(attacker, true, true);
		}
	}
}

MonsterAllyFormation_t monsterAllyFormations;
bool MonsterAllyFormation_t::getFollowLocation(Uint32 uid, Uint32 leaderUid, std::pair<int, int>& outPos)
{
	outPos.first = -1;
	outPos.second = -1;
	Entity* leader = uidToEntity(leaderUid);
	if ( !leader ) { return false; }
	if ( leader->behavior != &actPlayer && leader->behavior != &actMonster ) { return false; }

	auto& leaderUnits = units[leaderUid];
	auto findMelee = leaderUnits.meleeUnits.find(uid);
	auto findRanged = leaderUnits.rangedUnits.find(uid);

	bool found = false;
	if ( findMelee != leaderUnits.meleeUnits.end() )
	{
		if ( findMelee->second.init )
		{
			outPos.first = findMelee->second.x;
			outPos.second = findMelee->second.y;
			found = true;
		}
	}
	if ( findRanged != leaderUnits.rangedUnits.end() )
	{
		if ( findRanged->second.init )
		{
			outPos.first = findRanged->second.x;
			outPos.second = findRanged->second.y;
			found = true;
		}
	}

	return found;
}

void MonsterAllyFormation_t::updateOnPathFail(Uint32 uid, Entity* entity)
{
	if ( !entity )
	{
		entity = uidToEntity(uid);
	}

	if ( !entity )
	{
		return;
	}

	if ( Stat* myStats = entity->getStats() )
	{
		if ( myStats->leader_uid != 0 )
		{
			auto find = units.find(myStats->leader_uid);
			if ( find != units.end() )
			{
				auto find2 = find->second.meleeUnits.find(uid);
				if ( find2 != find->second.meleeUnits.end() )
				{
					find2->second.pathingDelay = std::min(10, find2->second.pathingDelay + 1);
					find2->second.tryExtendPath = std::max(0, find2->second.tryExtendPath - 2);
					if ( find2->second.pathingDelay == 5 )
					{
						// see if we can't do an extended search to succeed
						find2->second.tryExtendPath = 10;
					}
					return;
				}
				auto find3 = find->second.rangedUnits.find(uid);
				if ( find3 != find->second.rangedUnits.end() )
				{
					find3->second.pathingDelay = std::min(10, find3->second.pathingDelay + 1);
					find3->second.tryExtendPath = std::max(0, find3->second.tryExtendPath - 2);
					if ( find3->second.pathingDelay == 5 )
					{
						// see if we can't do an extended search to succeed
						find3->second.tryExtendPath = 10; 
					}
					return;
				}
			}
		}
	}
}

void MonsterAllyFormation_t::updateOnPathSucceed(Uint32 uid, Entity* entity)
{
	if ( !entity )
	{
		entity = uidToEntity(uid);
	}

	if ( !entity )
	{
		return;
	}

	if ( Stat* myStats = entity->getStats() )
	{
		if ( myStats->leader_uid != 0 )
		{
			auto find = units.find(myStats->leader_uid);
			if ( find != units.end() )
			{
				auto find2 = find->second.meleeUnits.find(uid);
				if ( find2 != find->second.meleeUnits.end() )
				{
					find2->second.pathingDelay = std::max(0, find2->second.pathingDelay - 2);
					find2->second.tryExtendPath = std::max(0, find2->second.tryExtendPath - 1);
					return;
				}
				auto find3 = find->second.rangedUnits.find(uid);
				if ( find3 != find->second.rangedUnits.end() )
				{
					find3->second.pathingDelay = std::max(0, find3->second.pathingDelay - 2);
					find3->second.tryExtendPath = std::max(0, find3->second.tryExtendPath - 1);
					return;
				}
			}
		}
	}
}

void MonsterAllyFormation_t::updateOnFollowCommand(Uint32 uid, Entity* entity)
{
	if ( !entity )
	{
		entity = uidToEntity(uid);
	}

	if ( !entity )
	{
		return;
	}

	if ( Stat* myStats = entity->getStats() )
	{
		if ( myStats->leader_uid != 0 )
		{
			auto find = units.find(myStats->leader_uid);
			if ( find != units.end() )
			{
				auto find2 = find->second.meleeUnits.find(uid);
				if ( find2 != find->second.meleeUnits.end() )
				{
					find2->second.pathingDelay = 0;
					find2->second.tryExtendPath = 10;
					return;
				}
				auto find3 = find->second.rangedUnits.find(uid);
				if ( find3 != find->second.rangedUnits.end() )
				{
					find3->second.pathingDelay = 0;
					find3->second.tryExtendPath = 10;
					return;
				}
			}
		}
	}
}

int MonsterAllyFormation_t::getFollowerChaseLeaderInterval(Entity& my, Stat& myStats)
{
	if ( myStats.leader_uid != 0 )
	{
		auto find = units.find(myStats.leader_uid);
		if ( find != units.end() )
		{
			auto find2 = find->second.meleeUnits.find(my.getUID());
			if ( find2 != find->second.meleeUnits.end() )
			{
				return find2->second.pathingDelay * TICKS_PER_SECOND + TICKS_PER_SECOND;
			}
			auto find3 = find->second.rangedUnits.find(my.getUID());
			if ( find3 != find->second.rangedUnits.end() )
			{
				return find3->second.pathingDelay * TICKS_PER_SECOND + TICKS_PER_SECOND;
			}
		}
	}
	return TICKS_PER_SECOND;
}

int MonsterAllyFormation_t::getFollowerPathingDelay(Entity& my, Stat& myStats)
{
	if ( myStats.leader_uid != 0 )
	{
		auto find = units.find(myStats.leader_uid);
		if ( find != units.end() )
		{
			auto find2 = find->second.meleeUnits.find(my.getUID());
			if ( find2 != find->second.meleeUnits.end() )
			{
				return find2->second.pathingDelay;
			}
			auto find3 = find->second.rangedUnits.find(my.getUID());
			if ( find3 != find->second.rangedUnits.end() )
			{
				return find3->second.pathingDelay;
			}
		}
	}
	return 0;
}

int MonsterAllyFormation_t::getFollowerTryExtendedPathSearch(Entity& my, Stat& myStats)
{
	if ( myStats.leader_uid != 0 )
	{
		auto find = units.find(myStats.leader_uid);
		if ( find != units.end() )
		{
			auto find2 = find->second.meleeUnits.find(my.getUID());
			if ( find2 != find->second.meleeUnits.end() )
			{
				return find2->second.tryExtendPath;
			}
			auto find3 = find->second.rangedUnits.find(my.getUID());
			if ( find3 != find->second.rangedUnits.end() )
			{
				return find3->second.tryExtendPath;
			}
		}
	}
	return 0;
}

void MonsterAllyFormation_t::updateFormation(Uint32 leaderUid, Uint32 monsterUpdateUid)
{
	Entity* leader = uidToEntity(leaderUid);
	if ( !leader ) { return; }
	if ( !(leader->behavior == &actPlayer || leader->behavior == &actMonster) ) { return; }
	auto& leaderUnits = units[leaderUid];

	if ( monsterUpdateUid == 0 )
	{
		if ( (ticks - leaderUnits.updatedOnTick) < TICKS_PER_SECOND )
		{
			return;
		}
		leaderUnits.updatedOnTick = ticks;
	}
	else if ( monsterUpdateUid != 0 ) // monster updated on behalf of leader
	{
		if ( leaderUnits.meleeUnits.find(monsterUpdateUid) == leaderUnits.meleeUnits.end() )
		{
			leaderUnits.meleeUnits[monsterUpdateUid] = MonsterAllies_t::FormationInfo_t(); // insert myself
		}
		return;
	}

	if ( leader->behavior == &actPlayer )
	{
		if ( Stat* leaderStats = leader->getStats() )
		{
			for ( node_t* allyNode = leaderStats->FOLLOWERS.first; allyNode != nullptr; allyNode = allyNode->next )
			{
				Uint32* c = (Uint32*)allyNode->element;
				if ( !c ) {	continue; }
				Uint32 allyUid = *c;
				Entity* ally = uidToEntity(allyUid);
				if ( !ally ) { continue; }
				Stat* stat = ally->getStats();
				if ( !stat ) { continue; }
				if ( stat->type == GYROBOT || monsterIsImmobileTurret(ally, stat) )
				{
					continue;
				}

				bool isRanged = ally->hasRangedWeapon();
				if ( isRanged )
				{
					if ( leaderUnits.rangedUnits.find(allyUid) == leaderUnits.rangedUnits.end() )
					{
						leaderUnits.rangedUnits[allyUid] = MonsterAllies_t::FormationInfo_t();
					}
					if ( leaderUnits.meleeUnits.find(allyUid) != leaderUnits.meleeUnits.end() )
					{
						leaderUnits.meleeUnits.erase(allyUid);
					}
				}
				else
				{
					if ( leaderUnits.meleeUnits.find(allyUid) == leaderUnits.meleeUnits.end() )
					{
						leaderUnits.meleeUnits[allyUid] = MonsterAllies_t::FormationInfo_t();
					}
					if ( leaderUnits.rangedUnits.find(allyUid) != leaderUnits.rangedUnits.end() )
					{
						leaderUnits.rangedUnits.erase(allyUid);
					}
				}
			}
		}
	}

	size_t formationIndex = 0;
	for ( auto& unit : leaderUnits.meleeUnits )
	{
		if ( unit.second.expired ) { continue; }
		Entity* ally = uidToEntity(unit.first);
		if ( !ally )
		{
			unit.second.expired = true;
			continue;
		}
		bool found = false;
		while ( !found )
		{
			if ( formationIndex >= formationShape.size() )
			{
				break;
			}

			real_t offsetx = formationShape[formationIndex].second;
			real_t offsety = formationShape[formationIndex].first;

			real_t x = leader->x;
			real_t y = leader->y;
			x += (offsety * cos(leader->yaw + PI / 2) * 16.0);
			y += (offsety * sin(leader->yaw + PI / 2) * 16.0);

			x += (-offsetx * cos(leader->yaw + PI) * 16.0);
			y += (-offsetx * sin(leader->yaw + PI) * 16.0);
			/*messagePlayer(0, MESSAGE_DEBUG, "%.2f %.2f", x / 16, y / 16);

			Entity* particle = spawnMagicParticle(leader);
			particle->sprite = 576;
			particle->x = x;
			particle->y = y;
			particle->z = 0;
			particle->scalex = 2.0;
			particle->scaley = 2.0;
			particle->scalez = 2.0;*/

			Entity* ohitentity = hit.entity;
			real_t oldx = ally->x;
			real_t oldy = ally->y;
			ally->x = x;
			ally->y = y;
			double tangent = atan2(leader->y - ally->y, leader->x - ally->x);
			lineTraceTarget(ally, ally->x, ally->y, tangent, 128, 0, false, leader);
			if ( hit.entity == leader )
			{
				found = true;
				unit.second.x = (static_cast<int>(x) >> 4);
				unit.second.y = (static_cast<int>(y) >> 4);
				unit.second.init = true;
			}
			ally->x = oldx;
			ally->y = oldy;
			hit.entity = ohitentity;
			++formationIndex;
		}
	}
	for ( auto& unit : leaderUnits.rangedUnits )
	{
		if ( unit.second.expired ) { continue; }
		Entity* ally = uidToEntity(unit.first);
		if ( !ally )
		{
			unit.second.expired = true;
			continue;
		}
		bool found = false;
		while ( !found )
		{
			if ( formationIndex >= formationShape.size() )
			{
				break;
			}

			real_t offsetx = formationShape[formationIndex].second;
			real_t offsety = formationShape[formationIndex].first;

			real_t x = leader->x;
			real_t y = leader->y;
			x += (offsety * cos(leader->yaw + PI / 2) * 16.0);
			y += (offsety * sin(leader->yaw + PI / 2) * 16.0);

			x += (-offsetx * cos(leader->yaw + PI) * 16.0);
			y += (-offsetx * sin(leader->yaw + PI) * 16.0);
			/*messagePlayer(0, MESSAGE_DEBUG, "%.2f %.2f", x / 16, y / 16);

			Entity* particle = spawnMagicParticle(leader);
			particle->sprite = 576;
			particle->x = x;
			particle->y = y;
			particle->z = 0;
			particle->scalex = 2.0;
			particle->scaley = 2.0;
			particle->scalez = 2.0;*/

			Entity* ohitentity = hit.entity;
			real_t oldx = ally->x;
			real_t oldy = ally->y;
			ally->x = x;
			ally->y = y;
			double tangent = atan2(leader->y - ally->y, leader->x - ally->x);
			lineTraceTarget(ally, ally->x, ally->y, tangent, 128, 0, false, leader);
			if ( hit.entity == leader )
			{
				found = true;
				unit.second.x = (static_cast<int>(x) >> 4);
				unit.second.y = (static_cast<int>(y) >> 4);
				unit.second.init = true;
			}
			ally->x = oldx;
			ally->y = oldy;
			hit.entity = ohitentity;
			++formationIndex;
		}
	}
}

/*-------------------------------------------------------------------------------

	summonMonster

	summons a monster near (but not at) the given location

-------------------------------------------------------------------------------*/

void summonMonsterClient(Monster creature, long x, long y, Uint32 uid)
{
	if ( Entity* entity = summonMonster(creature, x, y) )
	{
		entity->flags[INVISIBLE] = false;
		entity->setUID(uid);
	}
}

Entity* summonMonster(Monster creature, long x, long y, bool forceLocation)
{
    auto entity = summonMonsterNoSmoke(creature, x, y, forceLocation);

    // make a puff
    if (entity) {
        if (creature == MINOTAUR) {
            // extra big poof
            auto poof = spawnPoof(entity->x, entity->y, -8, 2.0);
        }
        else if (creature == GYROBOT) {
            // small poof
            auto poof = spawnPoof(entity->x, entity->y, 4, 0.5);
        }
        else {
            (void)spawnPoof(entity->x, entity->y, 0, 1.0);
        }
    }

    return entity;
}

Entity* summonMonsterNoSmoke(Monster creature, long x, long y, bool forceLocation)
{
	Entity* entity = newEntity(-1, 1, map.entities, map.creatures); //Monster entity.
	//Set the monster's variables.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->x = x;
	entity->y = y;
	entity->z = 6;
	if ( creature == MIMIC )
	{
		entity->yaw = 90 * (local_rng.rand() % 4) * PI / 180.0;
		entity->monsterLookDir = entity->yaw;
	}
	else
	{
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	}
	entity->behavior = &actMonster;
	entity->flags[UPDATENEEDED] = true;
	entity->flags[INVISIBLE] = true;
	entity->ranbehavior = true;
	entity->skill[5] = nummonsters;

	Stat* myStats = nullptr;
	if ( multiplayer != CLIENT )
	{
		// Need to give the entity its list stuff.
		// create an empty first node for traversal purposes
		node_t* node = nullptr;
		node = list_AddNodeFirst(&entity->children);
		node->element = nullptr;
		node->deconstructor = &emptyDeconstructor;

		myStats = new Stat(creature + 1000);
		node = list_AddNodeLast(&entity->children); //ASSUMING THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node->element = myStats;
		node->size = sizeof(myStats);
		node->deconstructor = &statDeconstructor;
		if ( entity->parent )
		{
			myStats->leader_uid = entity->parent;
			entity->parent = 0;
		}
		myStats->type = creature;
	}
	else
	{
		//Give dummy stats.
		entity->clientStats = new Stat(creature + 1000);
	}

	if ( multiplayer != CLIENT )
	{
		if ( myStats->type == SLIME )
		{
			myStats->setAttribute("slime_type", "terrain_spawn_override");
		}
	}

	// Find a free tile next to the source and then spawn it there.
	if ( multiplayer != CLIENT && !forceLocation )
	{
		if ( entityInsideSomething(entity) )
		{
			entity->x = x;
			entity->y = y - 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // north
			}
			entity->x = x;
			entity->y = y + 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // south
			}
			entity->x = x - 16;
			entity->y = y;
			if (!entityInsideSomething(entity))
			{
				goto end;    // west
			}
			entity->x = x + 16;
			entity->y = y;
			if (!entityInsideSomething(entity))
			{
				goto end;    // east
			}
			entity->x = x + 16;
			entity->y = y - 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // northeast
			}
			entity->x = x + 16;
			entity->y = y + 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // southeast
			}
			entity->x = x - 16;
			entity->y = y - 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // northwest
			}
			entity->x = x - 16;
			entity->y = y + 16;
			if (!entityInsideSomething(entity))
			{
				goto end;    // southwest
			}

			// we can't have monsters in walls...
			list_RemoveNode(entity->mynode);
			return nullptr;
		}
	}

end:

	if ( multiplayer != CLIENT )
	{
		if ( myStats->type == SLIME )
		{
			myStats->attributes.erase("slime_type");
		}
	}

	nummonsters++;

	if ( multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "SUMM");
		SDLNet_Write32((Uint32)creature, &net_packet->data[4]);
		SDLNet_Write32((Uint32)entity->x, &net_packet->data[8]);
		SDLNet_Write32((Uint32)entity->y, &net_packet->data[12]);
		SDLNet_Write32(entity->getUID(), &net_packet->data[16]);
		net_packet->len = 20;

		for ( int c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return entity;
}

void summonManyMonster(Monster creature)
{
	for ( long x = 1; x < map.width - 1; ++x )
	{
		for ( long y = 1; y < map.height - 1; ++y )
		{
			summonMonster(creature, (x * 16) + 8, (y * 16) + 8);
		}
	}
}

/*-------------------------------------------------------------------------------

	monsterMoveAside

	Causes the monster given in *entity to move aside from the entity given
	in *my

-------------------------------------------------------------------------------*/

bool monsterMoveAside(Entity* my, Entity* entity, bool ignoreMonsterState)
{
	if ( !my || !entity )
	{
		return false;
	}

	if ( !ignoreMonsterState && my->monsterState != MONSTER_STATE_WAIT )
	{
		return false;
	}

	int x = 0, y = 0;
	if ( cos(entity->yaw) > .4 )
	{
		y += 16;
		if ( checkObstacle(my->x, my->y + y, my, NULL) )
		{
			y -= 32;
			if ( checkObstacle(my->x, my->y + y, my, NULL) )
			{
				y = 0;
				x += 16;
			}
		}
	}
	else if ( cos(entity->yaw) < -.4 )
	{
		y -= 16;
		if ( checkObstacle(my->x, my->y + y, my, NULL) )
		{
			y += 32;
			if ( checkObstacle(my->x, my->y + y, my, NULL) )
			{
				y = 0;
				x -= 16;
			}
		}
	}
	if ( sin(entity->yaw) > .4 )
	{
		x -= 16;
		if ( checkObstacle(my->x + x, my->y, my, NULL) )
		{
			x += 32;
			if ( checkObstacle(my->x + x, my->y, my, NULL) )
			{
				x = 0;
				y += 16;
			}
		}
	}
	else if ( sin(entity->yaw) < -.4 )
	{
		x += 16;
		if ( checkObstacle(my->x + x, my->y, my, NULL) )
		{
			x -= 32;
			if ( checkObstacle(my->x + x, my->y, my, NULL) )
			{
				x = 0;
				y -= 16;
			}
		}
	}

	// move away
	if ( x != 0 || y != 0 )
	{
		my->monsterState = MONSTER_STATE_PATH;
		my->monsterReleaseAttackTarget();
		my->monsterTargetX = my->x + x;
		my->monsterTargetY = my->y + y;
		serverUpdateEntitySkill(my, 0);
		return true;
	}
	return false;
}

int getMonsterInteractGreeting(Stat& myStats)
{
	if ( myStats.type == BUGBEAR )
	{
		return 6259;
	}
	else if ( myStats.type < BUGBEAR )
	{
		return 4262 + myStats.type;
	}
	else
	{
		return 4288; // ...
	}
}

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

int devilstate = 0;
int devilacted = 0;
int devilroar = 0;
int devilsummonedtimes = 0;

bool makeFollower(int monsterclicked, bool ringconflict, char namesays[64], 
	Entity* my, Stat* myStats, bool checkReturnValueOnly)
{
	if ( !myStats || monsterclicked < 0 || monsterclicked >= MAXPLAYERS )
	{
		return false;
	}
	if ( ringconflict )
	{
		//Instant fail if ring of conflict is in effect. You have no allies!
		return false;
	}

	if ( !players[monsterclicked] || !players[monsterclicked]->entity || !stats[monsterclicked] )
	{
		return false;
	}

	Monster race = my->getRace();

	if ( myStats->leader_uid != 0 )
	{
		//Handle the "I have a leader!" situation.
		if ( myStats->leader_uid == players[monsterclicked]->entity->getUID() )
		{
			//Follows this player already!
			if ( my->getINT() > -2 && race == HUMAN )
			{
				//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(535), namesays, stats[monsterclicked]->name);
				players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
					Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(535), stats[monsterclicked]->name);
			}
			else
			{
				//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(534), namesays);
				players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
					Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(getMonsterInteractGreeting(*myStats)), stats[monsterclicked]->name);
			}
		}
		else
		{
			//Follows somebody else.
			if ( my->getINT() > -2 && race == HUMAN )
			{
				//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(536), namesays, stats[monsterclicked]->name);
				players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
					Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(536), stats[monsterclicked]->name);
			}
			else
			{
				//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(534), namesays);
				players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
					Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(getMonsterInteractGreeting(*myStats)), stats[monsterclicked]->name);
			}

			if ( my->checkFriend(players[monsterclicked]->entity) )
			{
				//If friendly, move aside.
				monsterMoveAside(my, players[monsterclicked]->entity);
			}
		}

		return false;
	}

	bool canAlly = false;
	bool roseEvent = false;
	if ( skillCapstoneUnlocked(monsterclicked, PRO_LEADERSHIP) )
	{
		int allowedFollowers = 8;
		int numFollowers = 0;
		for ( node_t* node = stats[monsterclicked]->FOLLOWERS.first; node; node = node->next )
		{
			Entity* follower = nullptr;
			if ( (Uint32*)node->element )
			{
				follower = uidToEntity(*((Uint32*)node->element));
			}
			if ( follower )
			{
				Stat* followerStats = follower->getStats();
				if ( followerStats )
				{
					if ( !(followerStats->type == SENTRYBOT || followerStats->type == GYROBOT
						|| followerStats->type == SPELLBOT || followerStats->type == DUMMYBOT) )
					{
						++numFollowers;
					}
				}
			}
		}
		if ( allowedFollowers > numFollowers )
		{
			//Can control humans & goblins & goatmen & insectoids.
			//TODO: Control humanoids in general? Or otherwise something from each tileset.
			if ( (stats[monsterclicked]->type == HUMAN && race == HUMAN)
				|| race == GOBLIN
				|| race == AUTOMATON
				|| race == GOATMAN
				|| race == INSECTOID
				|| race == GYROBOT )
			{
				canAlly = true;
			}

			//TODO: If enemies (e.g. goblin or an angry human), require the player to be unseen by this creature to gain control of it.

			if ( stats[monsterclicked]->type == SKELETON )
			{
				if ( race == GHOUL )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == VAMPIRE )
			{
				if ( race == VAMPIRE && !MonsterData_t::nameMatchesSpecialNPCName(*myStats, "bram kindly") )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == INCUBUS || stats[monsterclicked]->type == SUCCUBUS )
			{
				if ( race == INCUBUS || race == SUCCUBUS )
				{
					canAlly = true;
				}
				else if ( race == HUMAN && (myStats->EFFECTS[EFF_DRUNK] || myStats->EFFECTS[EFF_CONFUSED])
					&& stats[monsterclicked]->type != INCUBUS )
				{
					canAlly = true;
					if ( stats[monsterclicked]->type == SUCCUBUS )
					{
						steamAchievementClient(monsterclicked, "BARONY_ACH_TEMPTRESS");
					}
					if ( myStats->EFFECTS[EFF_CONFUSED] )
					{
						my->setEffect(EFF_CONFUSED, false, 0, false);
					}
				}
			}
			else if ( stats[monsterclicked]->type == GOATMAN )
			{
				if ( race == GOATMAN )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == GOBLIN )
			{
				if ( race == GOBLIN )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == AUTOMATON )
			{
				if ( race == AUTOMATON || race == HUMAN )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == RAT )
			{
				if ( race == RAT )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == SPIDER )
			{
				if ( race == SPIDER	|| race == SCARAB || race == SCORPION )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == INSECTOID )
			{
				if ( race == INSECTOID || race == SCARAB || race == SCORPION )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == TROLL )
			{
				if ( race == TROLL )
				{
					canAlly = true;
				}
			}
			else if ( stats[monsterclicked]->type == CREATURE_IMP )
			{
				if ( race == CREATURE_IMP && !(!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
				{
					canAlly = true; // non-boss imps
				}
			}
			if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
			{
				canAlly = true;
			}
			if ( stats[monsterclicked]->mask && stats[monsterclicked]->mask->type == MASK_MOUTH_ROSE
				&& players[monsterclicked]->entity->effectShapeshift == NOTHING )
			{
				if ( race == INCUBUS || race == SUCCUBUS )
				{
					canAlly = true;
					roseEvent = true;
				}
			}
		}
		else
		{
			messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(3482));
		}
	}
	else
	{
		bool tryAlly = my->checkFriend(players[monsterclicked]->entity);
		if ( stats[monsterclicked]->type == SUCCUBUS )
		{
			if ( race == HUMAN && (myStats->EFFECTS[EFF_DRUNK] || myStats->EFFECTS[EFF_CONFUSED]) )
			{
				tryAlly = true;
			}
		}
		if ( stats[monsterclicked]->mask && stats[monsterclicked]->mask->type == MASK_MOUTH_ROSE
			&& players[monsterclicked]->entity->effectShapeshift == NOTHING )
		{
			if ( race == INCUBUS || race == SUCCUBUS )
			{
				tryAlly = true;
			}
		}
		else if ( strcmp(myStats->name, "") && !monsterNameIsGeneric(*myStats)
			&& ((stats[monsterclicked]->getModifiedProficiency(PRO_LEADERSHIP) + stats[monsterclicked]->CHR) < 60) )
		{
			if ( race != HUMAN )
			{
				tryAlly = false;
				messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(3481), myStats->name);
			}
		}
		if ( tryAlly )
		{
			if ( myStats->leader_uid == 0 )
			{
				int allowedFollowers = std::min(8, std::max(4, 2 * (stats[monsterclicked]->getModifiedProficiency(PRO_LEADERSHIP) / 20)));
				int numFollowers = 0;
				for ( node_t* node = stats[monsterclicked]->FOLLOWERS.first; node; node = node->next )
				{
					Entity* follower = nullptr;
					if ( (Uint32*)node->element )
					{
						follower = uidToEntity(*((Uint32*)node->element));
					}
					if ( follower )
					{
						Stat* followerStats = follower->getStats();
						if ( followerStats )
						{
							if ( !(followerStats->type == SENTRYBOT || followerStats->type == GYROBOT
								|| followerStats->type == SPELLBOT || followerStats->type == DUMMYBOT) )
							{
								++numFollowers;
							}
						}
					}
				}
				if ( allowedFollowers > numFollowers )
				{
					if ( race == AUTOMATON || race == GYROBOT )
					{
						canAlly = true;
					}
					if ( stats[monsterclicked]->type == HUMAN )
					{
						canAlly = true;
					}
					else if ( stats[monsterclicked]->type == SKELETON )
					{
						if ( race == GHOUL )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == VAMPIRE )
					{
						if ( race == VAMPIRE && !MonsterData_t::nameMatchesSpecialNPCName(*myStats, "bram kindly") )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == SUCCUBUS || stats[monsterclicked]->type == INCUBUS )
					{
						if ( race == INCUBUS || race == SUCCUBUS )
						{
							canAlly = true;
						}
						else if ( race == HUMAN && (myStats->EFFECTS[EFF_DRUNK] || myStats->EFFECTS[EFF_CONFUSED])
							&& stats[monsterclicked]->type != INCUBUS )
						{
							canAlly = true;
							if ( stats[monsterclicked]->type == SUCCUBUS )
							{
								steamAchievementClient(monsterclicked, "BARONY_ACH_TEMPTRESS");
							}
							if ( myStats->EFFECTS[EFF_CONFUSED] )
							{
								my->setEffect(EFF_CONFUSED, false, 0, false);
							}
						}
					}
					else if ( stats[monsterclicked]->type == GOATMAN )
					{
						if ( race == GOATMAN )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == GOBLIN )
					{
						if ( race == GOBLIN )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == RAT )
					{
						if ( race == RAT )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == AUTOMATON )
					{
						if ( race == AUTOMATON || race == HUMAN )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == SPIDER )
					{
						if ( race == SPIDER	|| race == SCARAB || race == SCORPION )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == INSECTOID )
					{
						if ( race == INSECTOID || race == SCARAB || race == SCORPION )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == TROLL )
					{
						if ( race == TROLL )
						{
							canAlly = true;
						}
					}
					else if ( stats[monsterclicked]->type == CREATURE_IMP )
					{
						if ( race == CREATURE_IMP && !(!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
						{
							canAlly = true; // non-boss imps
						}
					}
					if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
					{
						canAlly = true;
					}
					if ( stats[monsterclicked]->mask && stats[monsterclicked]->mask->type == MASK_MOUTH_ROSE
						&& players[monsterclicked]->entity->effectShapeshift == NOTHING )
					{
						if ( race == INCUBUS || race == SUCCUBUS )
						{
							canAlly = true;
							roseEvent = true;
						}
					}
				}
				else
				{
					if ( numFollowers >= 8 )
					{
						//messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(3482));
						players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
							Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(3482));
					}
					else
					{
						//messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(3480));
						players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
							Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(3480));
					}
				}
			}
		}
	}

	if ( !canAlly )
	{
		//This one does not want to join your ever-enlarging cult.
		if ( my->getINT() > -2 && race == HUMAN )
		{
			//Human tells off the player.
			//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(530 + local_rng.rand() % 4), namesays);
			players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
				Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(530 + local_rng.rand() % 4));

			// move aside
			monsterMoveAside(my, players[monsterclicked]->entity);
		}
		else
		{
			//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(534), namesays);
			players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
				Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(getMonsterInteractGreeting(*myStats)), stats[monsterclicked]->name);
		}

		return false;
	}

	if ( checkReturnValueOnly )
	{
		return true;
	}

	node_t* newNode = list_AddNodeLast(&stats[monsterclicked]->FOLLOWERS);
	newNode->deconstructor = &defaultDeconstructor;
	Uint32* myuid = (Uint32*) (malloc(sizeof(Uint32)));
	newNode->element = myuid;
	*myuid = my->getUID();
    
    if (monsterclicked >= 0 && monsterclicked < MAXPLAYERS && ((myStats->type == HUMAN || myStats->type == SLIME) || myStats->name[0]))
    {
        // give us a random name (if necessary)
		if ( myStats->type == HUMAN 
			&& !myStats->name[0] 
			&& !monsterNameIsGeneric(*myStats)) {
			auto& names = myStats->sex == FEMALE ?
				randomNPCNamesFemale : randomNPCNamesMale;
			const int choice = local_rng.uniform(0, (int)names.size() - 1);
			auto name = names[choice].c_str();
			size_t len = names[choice].size();
			stringCopy(myStats->name, name, sizeof(Stat::name), len);
		}
		else if ( myStats->type == SLIME
			&& !myStats->name[0] )
		{
			std::string name = getMonsterLocalizedName(SLIME);
			stringCopy(myStats->name, name.c_str(), sizeof(Stat::name), name.size());
		}
        
        // ... and a nametag
		if (!monsterNameIsGeneric(*myStats) || myStats->type == SLIME) {
			if (monsterclicked == clientnum || splitscreen) {
				Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
				nametag->x = my->x;
				nametag->y = my->y;
				nametag->z = my->z - 6;
				nametag->sizex = 1;
				nametag->sizey = 1;
				nametag->flags[NOUPDATE] = true;
				nametag->flags[PASSABLE] = true;
				nametag->flags[SPRITE] = true;
				nametag->flags[UNCLICKABLE] = true;
				nametag->flags[BRIGHT] = true;
				nametag->behavior = &actSpriteNametag;
				nametag->parent = my->getUID();
				nametag->scalex = 0.2;
				nametag->scaley = 0.2;
				nametag->scalez = 0.2;
				nametag->skill[0] = monsterclicked;
				nametag->skill[1] = playerColor(monsterclicked, colorblind_lobby, true);
			}
		}
    }
    
	if ( my->getINT() > -2 && race == HUMAN )
	{
		//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(525 + local_rng.rand() % 4), namesays, stats[monsterclicked]->name);
		players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
			Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(525 + local_rng.rand() % 4), stats[monsterclicked]->name);
	}
	else
	{
		//This one can't speak, so generic "The %s decides to follow you!" message.
		messagePlayerMonsterEvent(monsterclicked, 0xFFFFFFFF, *myStats, Language::get(529), Language::get(3128), MSG_COMBAT);
	}
	spawnMagicEffectParticles(my->x, my->y, my->z, 685);
	monsterMoveAside(my, players[monsterclicked]->entity);
	players[monsterclicked]->entity->increaseSkill(PRO_LEADERSHIP);
	my->monsterState = MONSTER_STATE_WAIT; // be ready to follow
	myStats->leader_uid = players[monsterclicked]->entity->getUID();
	my->monsterAllyIndex = monsterclicked;
	if ( multiplayer == SERVER )
	{
		serverUpdateEntitySkill(my, 42); // update monsterAllyIndex for clients.
	}
	if ( monsterclicked > 0 && multiplayer == SERVER )
	{
		//Tell the client he suckered somebody into his cult.
		strcpy((char*) (net_packet->data), "LEAD");
		SDLNet_Write32((Uint32 )my->getUID(), &net_packet->data[4]);
		std::string name = myStats->name;
		if ( name != "" && name == MonsterData_t::getSpecialNPCName(*myStats) )
		{
			name = myStats->getAttribute("special_npc");
			name.insert(0, "$");
		}
        SDLNet_Write32(myStats->type, &net_packet->data[8]);
		strcpy((char*)(&net_packet->data[12]), name.c_str());
		net_packet->data[12 + strlen(name.c_str())] = 0;
		net_packet->address.host = net_clients[monsterclicked - 1].host;
		net_packet->address.port = net_clients[monsterclicked - 1].port;
		net_packet->len = 12 + (int)strlen(name.c_str()) + 1;
		sendPacketSafe(net_sock, -1, net_packet, monsterclicked - 1);

		serverUpdateAllyStat(monsterclicked, my->getUID(), myStats->LVL, myStats->HP, myStats->MAXHP, myStats->type);
	}

	// update flags for colors.
	my->flags[USERFLAG2] = true;
	serverUpdateEntityFlag(my, USERFLAG2);
	if ( monsterChangesColorWhenAlly(myStats) )
	{
		int bodypart = 0;
		for ( node_t* node = my->children.first; node != nullptr; node = node->next )
		{
			if ( bodypart >= LIMB_HUMANOID_TORSO )
			{
				Entity* tmp = (Entity*)node->element;
				if ( tmp )
				{
					tmp->flags[USERFLAG2] = true;
					//serverUpdateEntityFlag(tmp, USERFLAG2);
				}
			}
			++bodypart;
		}
	}

	for ( node_t* node = stats[monsterclicked]->FOLLOWERS.first; node != nullptr; node = node->next )
	{
		Uint32* c = (Uint32*)node->element;
		Entity* entity = nullptr;
		if ( c )
		{
			entity = uidToEntity(*c);
		}
		if ( entity && entity->monsterTarget == *myuid )
		{
			entity->monsterReleaseAttackTarget(); // followers stop punching the new target.
		}
	}

	if ( !FollowerMenu[monsterclicked].recentEntity && players[monsterclicked]->isLocalPlayer() )
	{
		FollowerMenu[monsterclicked].recentEntity = my;
	}

	Compendium_t::Events_t::eventUpdateMonster(monsterclicked, Compendium_t::CPDM_RECRUITED, my, 1);
	if ( myStats->type == HUMAN && myStats->getAttribute("special_npc") == "merlin" )
	{
		Compendium_t::Events_t::eventUpdateWorld(monsterclicked, Compendium_t::CPDM_MERLINS, "magicians guild", 1);
	}
	Compendium_t::Events_t::eventUpdateCodex(monsterclicked, Compendium_t::CPDM_RACE_RECRUITS, "races", 1);
	if ( (stats[monsterclicked]->type != HUMAN && stats[monsterclicked]->type != AUTOMATON) && myStats->type == HUMAN )
	{
		steamAchievementClient(monsterclicked, "BARONY_ACH_PITY_FRIEND");
	}
	if ( stats[monsterclicked]->type == VAMPIRE && myStats->type == VAMPIRE )
	{
		if ( !strncmp(myStats->name, "young vampire", strlen("young vampire")) )
		{
			steamAchievementClient(monsterclicked, "BARONY_ACH_YOUNG_BLOOD");
		}
	}
	if ( stats[monsterclicked]->sex != myStats->sex )
	{
		if ( stats[monsterclicked]->mask
			&& (stats[monsterclicked]->mask->type == MASK_SPOOKY
				|| stats[monsterclicked]->mask->type == MASK_GOLDEN
				|| stats[monsterclicked]->mask->type == MASK_ARTIFACT_VISOR
				|| stats[monsterclicked]->mask->type == MASK_STEEL_VISOR
				|| stats[monsterclicked]->mask->type == MASK_CRYSTAL_VISOR
				|| stats[monsterclicked]->mask->type == MASK_PLAGUE
				))
		{
			steamAchievementClient(monsterclicked, "BARONY_ACH_SSSMOKIN");
		}
	}
	if ( myStats->type == HUMAN && stats[monsterclicked]->type == HUMAN && stats[monsterclicked]->stat_appearance == 0
		&& stats[monsterclicked]->playerRace == RACE_AUTOMATON )
	{
		achievementObserver.updatePlayerAchievement(monsterclicked, AchievementObserver::Achievement::BARONY_ACH_REAL_BOY,
			AchievementObserver::AchievementEvent::REAL_BOY_HUMAN_RECRUIT);
	}
	if ( stats[monsterclicked]->type == TROLL && myStats->type == TROLL )
	{
		serverUpdatePlayerGameplayStats(monsterclicked, STATISTICS_FORUM_TROLL, AchievementObserver::FORUM_TROLL_RECRUIT_TROLL);
	}
	if ( stats[monsterclicked]->stat_appearance == 0
		&& (stats[monsterclicked]->playerRace == RACE_INCUBUS || stats[monsterclicked]->playerRace == RACE_SUCCUBUS) )
	{
		if ( myStats->type == HUMAN )
		{
			if ( stats[monsterclicked]->type == INCUBUS || stats[monsterclicked]->type == SUCCUBUS )
			{
				serverUpdatePlayerGameplayStats(monsterclicked, STATISTICS_PIMPING_AINT_EASY, 1);
			}
		}
		else if ( myStats->type == INCUBUS || myStats->type == SUCCUBUS )
		{
			serverUpdatePlayerGameplayStats(monsterclicked, STATISTICS_PIMPING_AINT_EASY, 1);
		}
	}
	if ( stats[monsterclicked]->stat_appearance == 0
		&& (stats[monsterclicked]->playerRace == RACE_GOBLIN) )
	{
		if ( myStats->type == GOBLIN )
		{
			serverUpdatePlayerGameplayStats(monsterclicked, STATISTICS_TRIBE_SUBSCRIBE, 1);
		}
	}
	if ( client_classes[monsterclicked] == CLASS_SHAMAN )
	{
		if ( players[monsterclicked]->entity->effectPolymorph != 0 || players[monsterclicked]->entity->effectShapeshift != 0 )
		{
			achievementObserver.playerAchievements[monsterclicked].socialButterfly++;
		}
	}

	if ( roseEvent && stats[monsterclicked]->mask )
	{
		Item* armor = stats[monsterclicked]->mask;
		if ( !myStats->mask )
		{
			if ( myStats->mask = newItem(armor->type, armor->status,
				armor->beatitude, armor->count, armor->appearance,
				armor->identified, nullptr) )
			{
				myStats->mask->ownerUid = players[monsterclicked]->entity->getUID();
			}
		}
		else
		{
			if ( Item* stolenArmor = newItem(armor->type, armor->status,
				armor->beatitude, armor->count, armor->appearance,
				armor->identified, &myStats->inventory) )
			{
				stolenArmor->ownerUid = players[monsterclicked]->entity->getUID();
			}
		}

		if ( monsterclicked > 0 && multiplayer == SERVER && !players[monsterclicked]->isLocalPlayer() )
		{
			strcpy((char*)net_packet->data, "STLA");
			net_packet->data[4] = 9;
			SDLNet_Write32(static_cast<Uint32>(armor->type), &net_packet->data[5]);
			SDLNet_Write32(static_cast<Uint32>(armor->status), &net_packet->data[9]);
			SDLNet_Write32(static_cast<Uint32>(armor->beatitude), &net_packet->data[13]);
			SDLNet_Write32(static_cast<Uint32>(armor->count), &net_packet->data[17]);
			SDLNet_Write32(static_cast<Uint32>(armor->appearance), &net_packet->data[21]);
			net_packet->data[25] = armor->identified;
			net_packet->address.host = net_clients[monsterclicked - 1].host;
			net_packet->address.port = net_clients[monsterclicked - 1].port;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, monsterclicked - 1);
		}

		messagePlayer(monsterclicked, MESSAGE_COMBAT, Language::get(6087), 
			getMonsterLocalizedName(myStats->type).c_str(), armor->getName());

		stats[monsterclicked]->mask = nullptr;
		if ( armor->node )
		{
			list_RemoveNode(armor->node);
		}
		else
		{
			free(armor);
		}

		if ( players[monsterclicked] )
		{
			steamAchievementEntity(players[monsterclicked]->entity, "BARONY_ACH_MON_PETIT");
		}
	}

	return true;
}

void printFollowerTableForSkillsheet(int monsterclicked, Entity* my, Stat* myStats)
{
	if ( !players[monsterclicked]->entity ) { return; }
	if ( !my || !myStats ) { return; }

	std::string outputList = "{";
	int originalLeadershipSkill = stats[monsterclicked]->getModifiedProficiency(PRO_LEADERSHIP);
	Sint32 originalCHR = stats[monsterclicked]->CHR;
	stats[monsterclicked]->CHR = 10;

	outputList += "\"version\": 1";

	std::unordered_map<Monster, std::vector<int>> allyListBase;
	for ( int iterations = 0; iterations < 2; ++iterations )
	{
		outputList += ",";
		if ( iterations == 0 )
		{
			stats[monsterclicked]->setProficiency(PRO_LEADERSHIP, 50);
			outputList += "\"leadership_allies_base\":";
		}
		else if ( iterations == 1 )
		{
			stats[monsterclicked]->setProficiency(PRO_LEADERSHIP, 100);
			outputList += "\"leadership_allies_legendary\":";
		}

		outputList += "{";

		Monster originalPlayerType = stats[monsterclicked]->type;
		std::unordered_map<Monster, std::vector<int>> allyList;
		std::unordered_set<Monster> skipMonsterTypes = {
			SENTRYBOT,
			GYROBOT,
			SPELLBOT,
			DUMMYBOT,
			SHOPKEEPER,
			DEVIL,
			LICH,
			LICH_FIRE,
			LICH_ICE
		};
		std::unordered_set<Monster> playerRaces;
		for ( int i = 0; i < NUMRACES; ++i )
		{
			playerRaces.insert(players[monsterclicked]->entity->getMonsterFromPlayerRace(i));
		}
		for ( auto& playerRace : playerRaces )
		{
			stats[monsterclicked]->type = playerRace;
			Monster originalRace = my->getRace();
			for ( int j = 0; j < NUMMONSTERS; ++j )
			{
				myStats->type = (Monster)j;
				if ( skipMonsterTypes.find(myStats->type) != skipMonsterTypes.end() ) { continue; }

				char namesays[64] = "";
				if ( makeFollower(monsterclicked, false, namesays, my, myStats, true) )
				{
					if ( iterations == 1 )
					{
						if ( allyListBase.find(stats[monsterclicked]->type) != allyListBase.end() )
						{
							auto& vec = allyListBase[stats[monsterclicked]->type];
							if ( std::find(vec.begin(), vec.end(), myStats->type) != vec.end() )
							{
								// exists in base list, skip adding this entry
								continue;
							}
						}
					}
					allyList[stats[monsterclicked]->type].push_back(myStats->type);
				}
			}
			myStats->type = originalRace;
		}
		stats[monsterclicked]->type = originalPlayerType;

		bool firstRace = true;
		for ( auto& playerRace : playerRaces )
		{
			if ( !firstRace ) { outputList += ",\n"; }
			firstRace = false;

			outputList += "\"";
			outputList += monstertypename[(int)playerRace];
			outputList += "\": [";
			std::string arrayList = "";
			if ( allyList.find(playerRace) != allyList.end() )
			{
				bool firstAlly = true;
				for ( auto& ally : allyList[playerRace] )
				{
					if ( !firstAlly ) { arrayList += ",\n"; }
					firstAlly = false;
					arrayList += "\"";
					arrayList += monstertypename[(int)ally];
					arrayList += "\"";
				}
			}
			outputList += arrayList;
			outputList += "]";
		}
		outputList += "}";

		if ( iterations == 0 )
		{
			allyListBase = allyList; // store this to compare with legendary leadership values.
		}
	}

	stats[monsterclicked]->setProficiency(PRO_LEADERSHIP, originalLeadershipSkill);
	stats[monsterclicked]->CHR = originalCHR;

	// special exeptions here.
	outputList += ",\"leadership_allies_unique_recruits\":{";
	{
		outputList += "\"";
		outputList += monstertypename[SUCCUBUS];
		outputList += "\":";
		outputList += "[{";
			outputList += "\"";
			outputList += monstertypename[HUMAN];
			outputList += "\":\"";
			outputList += "   (Drunk)";
			outputList += "\"";
			outputList += "}";

			outputList += ",{";
			outputList += "\"";
			outputList += monstertypename[HUMAN];
			outputList += "\":\"";
			outputList += "   (Confused)";
			outputList += "\"";
			outputList += "}";
		outputList += "]";

		outputList += ",";
		outputList += "\"";
		outputList += monstertypename[GOATMAN];
		outputList += "\":";
		outputList += "[{";
			outputList += "\"";
			outputList += monstertypename[HUMAN];
			outputList += "\":\"";
			outputList += "   (Throw Booze\\n   while drunk)";
			outputList += "\"";
			outputList += "}";
		outputList += "]";
	}
	outputList += "}";

	outputList += "}";

	rapidjson::Document d;
	d.Parse(outputList.c_str());

	std::string outputPath = outputdir;
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/skillsheet_leadership_entries.json";
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	d.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());

	FileIO::close(fp);
	messagePlayer(0, MESSAGE_MISC, "Exported file: %s", outputPath.c_str());
}

void sentrybotPickSpotNoise(Entity* my, Stat* myStats)
{
	if ( !my || !myStats )
	{
		return;
	}
	bool doSpecialNoise = false;
	switch ( my->getUID() % 3 )
	{
		case 0:
			if ( my->ticks % 60 < 20 )
			{
				doSpecialNoise = true;
			}
		case 1:
			if ( my->ticks % 60 >= 20 && my->ticks % 60 < 40 )
			{
				doSpecialNoise = true;
			}
		case 2:
			if ( my->ticks % 60 >= 40 )
			{
				doSpecialNoise = true;
			}
			break;
		default:
			break;
	}
	if ( doSpecialNoise && local_rng.rand() % 3 == 0 )
	{
		MONSTER_SOUND = playSoundEntity(my, 466 + local_rng.rand() % 3, 64);
	}
	else
	{
		MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128);
	}
}

void mimicResetIdle(Entity* my);
void batResetIdle(Entity* my);

void monsterAnimate(Entity* my, Stat* myStats, double dist)
{
	switch ( my->getMonsterTypeFromSprite() ) {
	case HUMAN: humanMoveBodyparts(my, myStats, dist); break;
	case RAT: ratAnimate(my, dist); break;
	case GOBLIN: goblinMoveBodyparts(my, myStats, dist); break;
	case SLIME: slimeAnimate(my, myStats, dist); break;
	case TROLL: trollMoveBodyparts(my, myStats, dist); break;
	case SPIDER: spiderMoveBodyparts(my, myStats, dist); break;
	case GHOUL: ghoulMoveBodyparts(my, myStats, dist); break;
	case SKELETON: skeletonMoveBodyparts(my, myStats, dist); break;
	case SCORPION: scorpionAnimate(my, dist); break;
	case CREATURE_IMP: impMoveBodyparts(my, myStats, dist); break;
	case GNOME: gnomeMoveBodyparts(my, myStats, dist); break;
	case DEMON: demonMoveBodyparts(my, myStats, dist); actDemonCeilingBuster(my); break;
	case SUCCUBUS: succubusMoveBodyparts(my, myStats, dist); break;
	case LICH: lichAnimate(my, dist); break;
	case MINOTAUR: minotaurMoveBodyparts(my, myStats, dist); actMinotaurCeilingBuster(my); break;
	case DEVIL: devilMoveBodyparts(my, myStats, dist); break;
	case SHOPKEEPER: shopkeeperMoveBodyparts(my, myStats, dist); break;
	case KOBOLD: koboldMoveBodyparts(my, myStats, dist); break;
	case SCARAB: scarabAnimate(my, myStats, dist); break;
	case CRYSTALGOLEM: crystalgolemMoveBodyparts(my, myStats, dist); break;
	case INCUBUS: incubusMoveBodyparts(my, myStats, dist); break;
	case VAMPIRE: vampireMoveBodyparts(my, myStats, dist); break;
	case SHADOW: shadowMoveBodyparts(my, myStats, dist); break;
	case COCKATRICE: cockatriceMoveBodyparts(my, myStats, dist); break;
	case INSECTOID: insectoidMoveBodyparts(my, myStats, dist); break;
	case GOATMAN: goatmanMoveBodyparts(my, myStats, dist); break;
	case AUTOMATON: automatonMoveBodyparts(my, myStats, dist); break;
	case LICH_ICE: lichIceAnimate(my, myStats, dist); break;
	case LICH_FIRE: lichFireAnimate(my, myStats, dist); break;
	case SENTRYBOT: sentryBotAnimate(my, myStats, dist); break;
	case SPELLBOT: sentryBotAnimate(my, myStats, dist); break;
	case GYROBOT: gyroBotAnimate(my, myStats, dist); break;
	case DUMMYBOT: dummyBotAnimate(my, myStats, dist); break;
	case MIMIC: mimicAnimate(my, myStats, dist); break;
	case BAT_SMALL: batAnimate(my, myStats, dist); break;
	case BUGBEAR: bugbearMoveBodyparts(my, myStats, dist); break;
	default:
		break;
	}
}

void actMonster(Entity* my)
{
	if (!my)
	{
		return;
	}

	if (movie || MainMenu::isCutsceneActive())
	{
	    return;
	}

	int x, y, c, i;
	double dist, dist2;
	list_t* path;
	node_t* node, *node2;
	double dir;
	double tangent;
	Stat* myStats;
	Entity* entity;
	Stat* hitstats = NULL;
	bool hasrangedweapon = false;
	bool myReflex;
	Sint32 previousMonsterState = my->monsterState;

	my->removeLightField();
	if ( my->flags[BURNING] )
	{
		my->light = addLight(my->x / 16, my->y / 16, "npc_burning");
	}

	// this is mostly a SERVER function.
	// however, there is a small part for clients:
	if ( multiplayer == CLIENT )
	{
		if ( !MONSTER_INIT && my->getMonsterTypeFromSprite() != NOTHING 
			&& (checkSpriteType(my->sprite) != 1 || my->skill[2] == -4) ) 
			// if checkSpriteType == 1, skill[2] -4 means server has sent model details so no longer editor sprite (blue slime 189 conflicts editor bugbear 189 fix)
		{
			MONSTER_INIT = 1;

			my->createWorldUITooltip();

			// make two empty nodes
			node = list_AddNodeLast(&my->children);
			node->element = nullptr;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
			node = list_AddNodeLast(&my->children);
			node->element = nullptr;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;

			switch (my->getMonsterTypeFromSprite()) {
			case HUMAN: initHuman(my, nullptr); break;
			case RAT: initRat(my, nullptr); break;
			case GOBLIN: initGoblin(my, nullptr); break;
			case SLIME: initSlime(my, nullptr); break;
			case SCORPION: initScorpion(my, nullptr); break;
			case SUCCUBUS: initSuccubus(my, nullptr); break;
			case TROLL: initTroll(my, nullptr); break;
			case SHOPKEEPER: initShopkeeper(my, nullptr); break;
			case SKELETON: initSkeleton(my, nullptr); break;
			case MINOTAUR: initMinotaur(my, nullptr); break;
			case GHOUL: initGhoul(my, nullptr); break;
			case DEMON: initDemon(my, nullptr); break;
			case SPIDER: initSpider(my, nullptr); break;
			case LICH: initLich(my, nullptr); break;
			case CREATURE_IMP: initImp(my, nullptr); break;
			case GNOME: initGnome(my, nullptr); break;
			case DEVIL: initDevil(my, nullptr); break;
			case CRYSTALGOLEM: initCrystalgolem(my, nullptr); break;
			case COCKATRICE: initCockatrice(my, nullptr); break;
			case AUTOMATON: initAutomaton(my, NULL); break;
			case SCARAB: initScarab(my, nullptr); break;
			case KOBOLD: initKobold(my, nullptr); break;
			case SHADOW: initShadow(my, nullptr); break;
			case VAMPIRE: initVampire(my, nullptr); break;
			case INCUBUS: initIncubus(my, nullptr); break;
			case INSECTOID: initInsectoid(my, nullptr); break;
			case GOATMAN: initGoatman(my, nullptr); break;
			case LICH_FIRE: initLichFire(my, nullptr); break;
			case LICH_ICE: initLichIce(my, nullptr); break;
			case SENTRYBOT: initSentryBot(my, nullptr); break;
			case SPELLBOT: initSentryBot(my, nullptr); break;
			case GYROBOT: initGyroBot(my, nullptr); break;
			case DUMMYBOT: initDummyBot(my, nullptr); break;
			case MIMIC: initMimic(my, nullptr); break;
			case BAT_SMALL: initBat(my, nullptr); break;
			case BUGBEAR: initBugbear(my, nullptr); break;
			default: printlog("Unknown monster, can't init!"); break;
			}
		}
		else if (MONSTER_INIT)
		{
		    const auto dist = sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY);
			monsterAnimate(my, nullptr, dist);

			if ( !intro )
			{
				my->handleEffectsClient();
			}
		}

		// request entity update (check if I've been deleted)
		if ( my->ticks % TICKS_PER_SECOND == my->getUID() % TICKS_PER_SECOND )
		{
			strcpy((char*)net_packet->data, "ENTE");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(my->getUID(), &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		return;
	}

	if ( (my->monsterExtraReflexTick > 0 && ticks % (TICKS_PER_SECOND) == (Uint32)my->monsterExtraReflexTick) )
	{
		my->monsterExtraReflexTick = 0;
		myReflex = true;
	}
	else if ( ticks % (TICKS_PER_SECOND) == my->getUID() % (TICKS_PER_SECOND / 2) )
	{
		my->monsterExtraReflexTick = 0;
		myReflex = true;
	}
	else
	{
		myReflex = false;
	}

	// init
	if ( MONSTER_INIT < 2 )   // 0 means no initialization, 1 means stats are initialized
	{
		my->skill[2] = -4; // tells clients to set this entity behavior to actMonster
		myStats = my->getStats();
		if (myStats)
		{
			myStats->monster_sound = NULL;
			switch ( myStats->type )
			{
				case HUMAN: initHuman(my, myStats); break;
				case RAT: initRat(my, myStats); break;
				case GOBLIN: initGoblin(my, myStats); break;
				case SLIME: initSlime(my, myStats); break;
				case SCORPION: initScorpion(my, myStats); break;
				case SUCCUBUS: initSuccubus(my, myStats); break;
				case TROLL: initTroll(my, myStats); break;
				case SHOPKEEPER: initShopkeeper(my, myStats); break;
				case SKELETON: initSkeleton(my, myStats); break;
				case MINOTAUR: initMinotaur(my, myStats); break;
				case GHOUL: initGhoul(my, myStats); break;
				case DEMON: initDemon(my, myStats); break;
				case SPIDER: initSpider(my, myStats); break;
				case LICH: initLich(my, myStats); break;
				case CREATURE_IMP: initImp(my, myStats); break;
				case GNOME: initGnome(my, myStats); break;
				case DEVIL:
					devilstate = 0;
					devilacted = 0;
					devilroar = 0;
					devilsummonedtimes = 0;
					initDevil(my, myStats);
					break;
				case KOBOLD: initKobold (my, myStats); break;
				case SCARAB: initScarab (my, myStats); break;
				case CRYSTALGOLEM: initCrystalgolem (my, myStats); break;
				case INCUBUS: initIncubus (my, myStats); break;
				case VAMPIRE: initVampire (my, myStats); break;
				case SHADOW: initShadow (my, myStats); break;
				case COCKATRICE: initCockatrice (my, myStats); break;
				case INSECTOID: initInsectoid (my, myStats); break;
				case GOATMAN: initGoatman (my, myStats); break;
				case AUTOMATON: initAutomaton (my, myStats); break;
				case LICH_ICE: initLichIce (my, myStats); break;
				case LICH_FIRE: initLichFire (my, myStats); break;
				case SENTRYBOT: my->sprite = 872; initSentryBot(my, myStats); break;
				case SPELLBOT: my->sprite = 885; initSentryBot(my, myStats); break;
				case GYROBOT: initGyroBot(my, myStats); break;
				case DUMMYBOT: initDummyBot(my, myStats); break;
				case MIMIC: initMimic(my, myStats); break;
				case BAT_SMALL: initBat(my, myStats); break;
				case BUGBEAR: initBugbear(my, myStats); break;
				default: break; //This should never be reached.
			}
		}

		MONSTER_INIT = 2;
		if ( myStats->type == MIMIC )
		{
			// no turn, handled earlier
		}
		else if ( myStats->type != LICH && myStats->type != DEVIL )
		{
			my->monsterLookDir = (local_rng.rand() % 360) * PI / 180;
		}
		else
		{
			my->monsterLookDir = PI;
		}
		my->monsterLookTime = local_rng.rand() % 120;
		my->monsterMoveTime = local_rng.rand() % 10;
		MONSTER_SOUND = NULL;
		if ( MONSTER_NUMBER == -1 )
		{
			MONSTER_NUMBER = nummonsters;
			nummonsters++;
		}
		/*if( local_rng.rand()%20==0 ) { // 20% chance
			MONSTER_STATE = 2; // start hunting the player immediately
			MONSTER_TARGET = local_rng.rand()%numplayers;
			MONSTER_TARGETX = players[MONSTER_TARGET]->x;
			MONSTER_TARGETY = players[MONSTER_TARGET]->y;
		} else {
			MONSTER_TARGET = -1;
		}*/

		if ( uidToEntity(my->monsterTarget) == nullptr )
		{
			my->monsterTarget = 0;
		}

		my->createWorldUITooltip();

		/*// create an empty first node for traversal purposes //GOING TO ASSUME THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node = list_AddNodeFirst(my->children);
		node->element = NULL;
		node->deconstructor = &emptyDeconstructor;*/

		// assign stats to the monster
		//myStats = (Stat *) malloc(sizeof(Stat)); //GOING TO ASSUME THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		//myStats->type = RAT; //GOING TO ASSUME THIS IS ALREADY PROPERLY SET WHEN THE FUNCTION IS CALLED.
		//TODO: Move the rest of this into the monster specific init functions.
		/*node = list_AddNodeLast(my->children); //ASSUMING THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node->element = myStats;
		node->deconstructor = &defaultDeconstructor;*/

		return;
	}

	myStats = my->getStats();
	if ( myStats == NULL )
	{
		printlog("ERROR: monster entity at %p has no stats struct!", my);
		return;
	}
	myStats->defending = false;
	myStats->sneaking = 0;

	// levitation
	bool levitating = isLevitating(myStats);

	if ( myStats->type == MINOTAUR )
	{
		int c;
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			assailant[c] = true; // as long as this is active, combat music doesn't turn off
			assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
		}
	}

	if ( myStats->type == SHADOW && my->monsterTarget != 0 )
	{
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( players[c] && players[c]->entity && players[c]->entity->getUID() == my->monsterTarget )
			{
				assailant[c] = true; //Keeps combat music on as long as a shadow is hunting you down down down!
				assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
				break;
			}
		}
	}

	if ( my->ticks == 120 + MONSTER_NUMBER )
	{
		serverUpdateBodypartIDs(my);
	}

	// some special herx behavior
	if ( myStats->type == LICH )
	{
		// destroying room lights
		if ( myStats->HP <= myStats->MAXHP / 2 )
		{
			node_t* node, *nextnode;
			bool foundlights = false;
			for ( node = map.entities->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				Entity* tempEntity = (Entity*)node->element;

				if ( tempEntity->behavior == &actTorch || tempEntity->behavior == &actCampfire )
				{
					foundlights = true;
					if ( tempEntity->light )
					{
						list_RemoveNode(tempEntity->light->node);
						tempEntity->light = nullptr;
					}
					list_RemoveNode(tempEntity->mynode);
				}
			}
			if ( foundlights )
			{
#ifdef USE_FMOD
				if ( MONSTER_SOUND )
				{
					MONSTER_SOUND->stop();
				}
#elif defined USE_OPENAL
				if ( MONSTER_SOUND )
				{
					OPENAL_Channel_Stop(MONSTER_SOUND);
				}
#endif
				int c;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					MONSTER_SOUND = playSoundPlayer(c, 179, 128);
					playSoundPlayer(c, 166, 128);
					Uint32 color = makeColorRGB(255, 0, 255);
					messagePlayerColor(c, MESSAGE_WORLD, color, Language::get(512));
				}
			}
		}
		// dodging away
		if ( ( ( local_rng.rand() % 4 == 0 && my->monsterState != 6 ) || ( local_rng.rand() % 10 == 0 && my->monsterState == MONSTER_STATE_LICH_SUMMON) ) && myStats->OLDHP != myStats->HP )
		{
			playSoundEntity(my, 180, 128);
			my->monsterState = MONSTER_STATE_LICH_DODGE; // dodge state
			double dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
			MONSTER_VELX = cos(dir) * 5;
			MONSTER_VELY = sin(dir) * 5;
			my->monsterSpecialTimer = 0;
		}

		// check walls
		if ( my->monsterLichAllyStatus == 0 )
		{
			if ( !strcmp(map.name, "Boss") )
			{
				int x1 = 34;
				int y1 = 14;
				int x2 = 38;
				int y2 = 18;

				for ( int x = x1; x <= x2 && my->monsterLichAllyStatus == 0; ++x )
				{
					for ( int y = y1; y <= y2 && my->monsterLichAllyStatus == 0; ++y )
					{
						if ( x > x1 && x < x2 && y > y1 && y < y2 )
						{
							// inner empty section
							continue;
						}

						int mapIndex = (y) * MAPLAYERS + (x) * MAPLAYERS * map.height;
						if ( !map.tiles[OBSTACLELAYER + mapIndex] )
						{
							// wall has been broken, fights on
							my->monsterLichAllyStatus = 1;

							//int arena_x1 = 25;
							//int arena_y1 = 5;
							//int arena_x2 = 47;
							//int arena_y2 = 27;
							//for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
							//{
							//	if ( Entity* entity = (Entity*)node->element )
							//	{
							//		int x = entity->x / 16;
							//		int y = entity->y / 16;
							//		if ( !(x >= arena_x1 && x <= arena_x2 && y >= arena_y1 && y <= arena_y2) )
							//		{
							//			// outside arena, teleport in
							//			if ( !entity->monsterIsTinkeringCreation() )
							//			{
							//				if ( Entity* leader = entity->monsterAllyGetPlayerLeader() )
							//				{
							//					int tele_x = 26 + local_rng.rand() % 3;
							//					int tele_y = 15 + local_rng.rand() % 3;
							//					entity->x = tele_x * 16.0 + 8.0;
							//					entity->y = tele_y * 16.0 + 8.0;
							//				}
							//			}
							//		}
							//	}
							//}
							break;
						}
					}
				}
			}
		}
	}

	if ( ((myStats->type == LICH_FIRE && my->monsterState != MONSTER_STATE_LICHFIRE_DIE)
		|| (myStats->type == LICH_ICE && my->monsterState != MONSTER_STATE_LICHICE_DIE))
		&& myStats->HP > 0 )
	{
		//messagePlayer(0, "state: %d", my->monsterState);
		if ( my->monsterLichBattleState >= LICH_BATTLE_READY )
		{
			for ( int c = 0; c < MAXPLAYERS; c++ )
			{
				assailant[c] = true; // as long as this is active, combat music doesn't turn off
				assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
			}
		}
		if ( my->monsterSpecialTimer > 0 )
		{
			--my->monsterSpecialTimer;
		}
		else
		{
			my->monsterSpecialTimer = 0;
			if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS )
			{
				my->monsterState = MONSTER_STATE_LICH_TELEPORT_ROAMING;
				my->monsterSpecialTimer = 60;
				if ( myStats->type == LICH_FIRE )
				{
					my->lichFireTeleport();
				}
				else
				{
					my->lichIceTeleport();
				}
			}
		}

		if ( my->monsterState != MONSTER_STATE_ATTACK && my->monsterState <= MONSTER_STATE_HUNT )
		{
			my->monsterHitTime = HITRATE * 2;
		}
		//messagePlayer(0, "Ally state: %d", my->monsterLichAllyStatus);
		Entity* lichAlly = nullptr;
		if ( my->ticks > (TICKS_PER_SECOND) 
			&& my->monsterLichAllyStatus == LICH_ALLY_ALIVE 
			&& ticks % (TICKS_PER_SECOND * 2) == 0 )
		{
			if ( myStats->type == LICH_ICE )
			{
				numMonsterTypeAliveOnMap(LICH_FIRE, lichAlly);
				if ( lichAlly == nullptr )
				{
					//messagePlayer(0, "DEAD");
					my->monsterLichAllyStatus = LICH_ALLY_DEAD;
					my->monsterLichAllyUID = 0;
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						if ( multiplayer == SINGLE )
						{
							if ( c == clientnum )
							{
								playSoundPlayer(c, 392, 128);
							}
						}
						else
						{
							playSoundPlayer(c, 392, 128);
						}
						messagePlayerColor(c, MESSAGE_WORLD, uint32ColorBaronyBlue, Language::get(2647));
					}
				}
				else if ( lichAlly && my->monsterLichAllyUID == 0 )
				{
					my->monsterLichAllyUID = lichAlly->getUID();
				}
			}
			else
			{
				numMonsterTypeAliveOnMap(LICH_ICE, lichAlly);
				if ( lichAlly == nullptr )
				{
					//messagePlayer(0, "DEAD");
					my->monsterLichAllyStatus = LICH_ALLY_DEAD;
					my->monsterLichAllyUID = 0;
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						if ( multiplayer == SINGLE )
						{
							if ( c == clientnum )
							{
								playSoundPlayer(c, 391, 128);
							}
						}
						else
						{
							playSoundPlayer(c, 391, 128);
						}
						messagePlayerColor(c, MESSAGE_WORLD, uint32ColorOrange, Language::get(2649));
					}
				}
				else if ( lichAlly && my->monsterLichAllyUID == 0 )
				{
					my->monsterLichAllyUID = lichAlly->getUID();
				}
			}
		}
		real_t lichDist = 0.f;
		Entity* target = uidToEntity(my->monsterTarget);

		if ( myStats->OLDHP != myStats->HP && myStats->HP > 0 )
		{
			if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS
				&& my->monsterSpecialTimer < 250 )
			{
				if ( local_rng.rand() % 8 == 0 )
				{
					my->monsterState = MONSTER_STATE_LICH_TELEPORT_ROAMING;
					my->lichFireTeleport();
					my->monsterSpecialTimer = 60;
				}
			}
			if ( my->monsterState <= MONSTER_STATE_HUNT )
			{
				switch ( my->monsterLichBattleState )
				{
					// track when a teleport can happen, battleState needs to be odd numbered to allow stationary teleport
					case 0:
						if ( myStats->HP <= myStats->MAXHP * 0.9 )
						{
							my->monsterLichBattleState = 1;
						}
						break;
					case 2:
						if ( myStats->HP <= myStats->MAXHP * 0.7 )
						{
							my->monsterLichBattleState = 3;
						}
						break;
					case 4:
						if ( myStats->HP <= myStats->MAXHP * 0.5 )
						{
							my->monsterLichBattleState = 5;
						}
						break;
					case 6:
						if ( myStats->HP <= myStats->MAXHP * 0.3 )
						{
							my->monsterLichBattleState = 7;
						}
						break;
					case 8:
						if ( myStats->HP <= myStats->MAXHP * 0.1 )
						{
							my->monsterLichBattleState = 9;
						}
						break;
					default:
						break;
				}
				if ( my->monsterLichBattleState % 2 == 1
					&& (local_rng.rand() % 5 == 0
						|| (local_rng.rand() % 4 == 0 && my->monsterLichTeleportTimer > 0)
						|| (local_rng.rand() % 2 == 0 && my->monsterLichAllyStatus == LICH_ALLY_DEAD))
					)
				{
					// chance to change state to teleport after being hit.
					if ( my->monsterLichAllyUID != 0 )
					{
						lichAlly = uidToEntity(my->monsterLichAllyUID);
					}
					if ( myStats->type == LICH_FIRE )
					{
						if ( !myStats->EFFECTS[EFF_VAMPIRICAURA] )
						{
							if ( (lichAlly && lichAlly->monsterState != MONSTER_STATE_LICH_CASTSPELLS)
								|| my->monsterLichAllyStatus == LICH_ALLY_DEAD
								|| multiplayer != SINGLE )
							{
								// don't teleport if ally is casting spells. unless multiplayer, then go nuts!
								my->monsterState = MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY;
								my->lichFireTeleport();
								my->monsterSpecialTimer = 80;
								++my->monsterLichBattleState;
							}
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						if ( (lichAlly && lichAlly->monsterState != MONSTER_STATE_LICH_CASTSPELLS)
							|| my->monsterLichAllyStatus == LICH_ALLY_DEAD
							|| multiplayer != SINGLE )
						{
							// don't teleport if ally is casting spells. unless multiplayer, then go nuts!
							my->monsterState = MONSTER_STATE_LICHICE_TELEPORT_STATIONARY;
							my->lichIceTeleport();
							my->monsterSpecialTimer = 80;
							++my->monsterLichBattleState;
						}
					}
				}
			}
		}
		if ( my->monsterSpecialTimer == 0 && my->monsterAttack == 0 )
		{
			if ( my->monsterState <= MONSTER_STATE_HUNT && my->monsterTarget )
			{
				if ( target )
				{
					lichDist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
				}
				if ( ticks % 30 == 0 )
				{
					// check tiles around the monster.
					int sides = 0;
					int my_x = static_cast<int>(my->x) >> 4;
					int my_y = static_cast<int>(my->y) >> 4;
					int mapIndex = (my_y) * MAPLAYERS + (my_x + 1) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y) * MAPLAYERS + (my_x - 1) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y + 1) * MAPLAYERS + (my_x) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y - 1) * MAPLAYERS + (my_x) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					//messagePlayer(0, "sides: %d, timer %d", sides, my->monsterLichTeleportTimer);
					if ( sides == 0 )
					{
						my->monsterLichTeleportTimer = 0;
					}
					else
					{
						if ( sides >= 2 )
						{
							my->monsterLichTeleportTimer++;
						}
						else
						{
							if ( local_rng.rand() % 3 == 0 )
							{
								my->monsterLichTeleportTimer++;
							}
						}
						if ( my->monsterLichTeleportTimer >= 3 )
						{
							// let's teleport, reset the counter inside the teleport functions.
							if ( myStats->type == LICH_FIRE )
							{
								my->lichFireTeleport();
							}
							else
							{
								my->lichIceTeleport();
							}
							my->monsterSpecialTimer = 40;
						}
					}
				}
				if ( myStats->type == LICH_FIRE )
				{
					if ( (	my->monsterLichFireMeleePrev == LICH_ATK_RISING_SINGLE
							|| my->monsterLichFireMeleePrev == LICH_ATK_HORIZONTAL_RETURN)
							&& local_rng.rand() % 4 == 0
							&& ticks % 10 == 0
						)
					{
						// chance to dodge immediately after the above 2 attacks
						playSoundEntity(my, 180, 128);
						dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
						MONSTER_VELX = cos(dir) * 3;
						MONSTER_VELY = sin(dir) * 3;
						my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
						my->monsterSpecialTimer = 20;
						my->monsterLichFireMeleePrev = 0;
						my->monsterLichFireMeleeSeq = LICH_ATK_BASICSPELL_SINGLE;
					}
					else if ( myStats->OLDHP != myStats->HP )
					{
						if ( local_rng.rand() % 4 == 0 )
						{
							// chance to dodge on hp loss
							playSoundEntity(my, 180, 128);
							dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
							my->monsterSpecialTimer = 20;
						}
					}
					else if ( lichDist > 64 )
					{
						if ( target && local_rng.rand() % 100 == 0 )
						{
							// chance to dodge towards the target if distance is great enough.
							playSoundEntity(my, 180, 128);
							tangent = atan2(target->y - my->y, target->x - my->x);
							dir = tangent;
							while ( dir < 0 )
							{
								dir += 2 * PI;
							}
							while ( dir > 2 * PI )
							{
								dir -= 2 * PI;
							}
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
							my->monsterSpecialTimer = 50;
						}
					}
				}
				else if ( myStats->type == LICH_ICE )
				{
					int enemiesInMelee = 0;
					if ( ticks % 50 == 0 )
					{
						// grab enemies around the lich once per second to determine threat level.
						enemiesInMelee = numTargetsAroundEntity(my, 32.0, PI, MONSTER_TARGET_ENEMY);
					}
					if ( myStats->OLDHP != myStats->HP )
					{
						if ( local_rng.rand() % 3 == 0 )
						{
							// chance to dodge on hp loss
							playSoundEntity(my, 180, 128);
							dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHICE_DODGE;
							my->monsterSpecialTimer = 30;
							if ( local_rng.rand() % 2 == 0 )
							{
								// prepare off-hand spell after dodging
								my->monsterLichIceCastPrev = 0;
								my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
							}
						}
					}
					else if ( (lichDist < 32) || (enemiesInMelee > 1) )
					{
						if ( (target && ticks % 10 == 0 && local_rng.rand() % 100 == 0) || (enemiesInMelee > 1 && local_rng.rand() % 8 == 0) )
						{
							// chance to dodge away from target if distance is low enough.
							playSoundEntity(my, 180, 128);
							if ( target )
							{
								tangent = atan2(target->y - my->y, target->x - my->x);
								dir = tangent + PI;
							}
							else
							{
								dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
							}
							while ( dir < 0 )
							{
								dir += 2 * PI;
							}
							while ( dir > 2 * PI )
							{
								dir -= 2 * PI;
							}
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHICE_DODGE;
							my->monsterSpecialTimer = 20;
							if ( local_rng.rand() % 2 == 0 )
							{
								// prepare off-hand spell after dodging
								my->monsterLichIceCastPrev = 0;
								my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
							}
						}
						else if ( (ticks % 50 == 0 && local_rng.rand() % 10 == 0) || (enemiesInMelee > 1 && local_rng.rand() % 4 == 0) )
						{
							my->monsterSpecialTimer = 100;
							my->monsterLichIceCastPrev = 0;
							my->monsterLichIceCastSeq = LICH_ATK_CHARGE_AOE;
						}
					}
					else if ( lichDist > 64 )
					{
						// chance to dodge towards the target if distance is great enough.
						if ( local_rng.rand() % 100 == 0 )
						{
							if ( my->monsterLichAllyUID != 0 )
							{
								lichAlly = uidToEntity(my->monsterLichAllyUID);
							}
							if ( lichAlly && target )
							{
								// if ally is close to your target, then don't dodge towards.
								if ( entityDist(lichAlly, target) > 48.0 )
								{
									playSoundEntity(my, 180, 128);
									tangent = atan2(target->y - my->y, target->x - my->x);
									dir = tangent;
									while ( dir < 0 )
									{
										dir += 2 * PI;
									}
									while ( dir > 2 * PI )
									{
										dir -= 2 * PI;
									}
									MONSTER_VELX = cos(dir) * 3;
									MONSTER_VELY = sin(dir) * 3;
									my->monsterState = MONSTER_STATE_LICHICE_DODGE;
									my->monsterSpecialTimer = 50;
								}
							}
							if ( my->monsterState != MONSTER_STATE_LICHICE_DODGE )
							{
								// chance to dodge sideways if not set above
								playSoundEntity(my, 180, 128);
								dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
								MONSTER_VELX = cos(dir) * 3;
								MONSTER_VELY = sin(dir) * 3;
								my->monsterState = MONSTER_STATE_LICHICE_DODGE;
								my->monsterSpecialTimer = 30;
								if ( local_rng.rand() % 10 == 0 )
								{
									// prepare off-hand spell after dodging
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
								}
							}
						}
					}
					else if ( my->monsterLichMeleeSwingCount > 3 )
					{
						// reached x successive normal attacks, either move/teleport/dodge around the map
						my->monsterLichMeleeSwingCount = 0;
						if ( local_rng.rand() % 10 > 0 )
						{
							if ( local_rng.rand() % 2 == 0 )
							{
								my->monsterTarget = 0;
								my->monsterTargetX = my->x - 50 + local_rng.rand() % 100;
								my->monsterTargetY = my->y - 50 + local_rng.rand() % 100;
								my->monsterState = MONSTER_STATE_PATH; // path state
							}
							else
							{
								// chance to dodge
								playSoundEntity(my, 180, 128);
								dir = my->yaw - (PI / 2) + PI * (local_rng.rand() % 2);
								MONSTER_VELX = cos(dir) * 3;
								MONSTER_VELY = sin(dir) * 3;
								my->monsterState = MONSTER_STATE_LICHICE_DODGE;
								my->monsterSpecialTimer = 30;
								if ( local_rng.rand() % 2 == 0 )
								{
									// prepare off-hand spell after dodging
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
								}
								else
								{
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_FALLING_DIAGONAL;
								}
							}
						}
					}
				}
			}
			else if ( my->monsterState == MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY
				|| my->monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY )
			{
				my->monsterState = MONSTER_STATE_LICH_CASTSPELLS;
				my->monsterSpecialTimer = 500; // cast spells for 10 seconds.
				my->monsterHitTime = 0;
				my->monsterLichMagicCastCount = 0;
				my->monsterLichFireMeleeSeq = 0;
				// acquire a new target.
				lichDist = 1024;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures need to be targeted.
				{
					Entity* tempEntity = (Entity*)node->element;
					if ( tempEntity->behavior == &actPlayer 
						&& (sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2)) < lichDist)
						)
					{
						lichDist = sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2));
						target = tempEntity;
					}
				}
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_LICH_CASTSPELLS);
				}
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 0.0, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 2 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 4 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 6 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 8 * PI / 5, 500);
			}
			else if ( my->monsterState == MONSTER_STATE_LICH_TELEPORT_ROAMING )
			{
				my->monsterHitTime = 0;
				my->monsterLichMagicCastCount = 0;
				my->monsterLichFireMeleeSeq = 0;
				// acquire a new target.
				lichDist = 1024;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures need to be targeted.
				{
					Entity* tempEntity = (Entity*)node->element;
					if ( tempEntity && tempEntity->behavior == &actPlayer
						&& (sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2)) < lichDist)
						)
					{
						lichDist = sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2));
						target = tempEntity;
					}
				}
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					my->monsterState = MONSTER_STATE_PATH;
				}
				else
				{
					my->monsterState = MONSTER_STATE_WAIT;
				}
			}
		}
	}

	// hunger, regaining hp/mp, poison, etc.
	if ( !intro )
	{
		my->handleEffects(myStats);
	}
	if ( myStats->HP <= 0
		&& my->monsterState != MONSTER_STATE_LICH_DEATH
		&& my->monsterState != MONSTER_STATE_DEVIL_DEATH
		&& my->monsterState != MONSTER_STATE_LICHFIRE_DIE 
		&& my->monsterState != MONSTER_STATE_LICHICE_DIE )
	{
		//TODO: Refactor die function.
		// drop all equipment
		entity = dropItemMonster(myStats->helmet, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->helmet = NULL;
		entity = dropItemMonster(myStats->breastplate, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->breastplate = NULL;
		entity = dropItemMonster(myStats->gloves, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->gloves = NULL;
		entity = dropItemMonster(myStats->shoes, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->shoes = NULL;
		entity = dropItemMonster(myStats->shield, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->shield = NULL;
		if ( myStats->weapon )
		{
			if ( itemCategory(myStats->weapon) != SPELLBOOK )
			{
				entity = dropItemMonster(myStats->weapon, my, myStats);
				if ( entity )
				{
					entity->flags[USERFLAG1] = true;
				}
			}
			else
			{
				// spellbooks are not dropped
				if ( myStats->weapon->node )
				{
					list_RemoveNode(myStats->weapon->node);
				}
				else
				{
					free(myStats->weapon);
				}
			}
			myStats->weapon = NULL;
		}
		entity = dropItemMonster(myStats->cloak, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->cloak = NULL;
		entity = dropItemMonster(myStats->amulet, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->amulet = NULL;
		entity = dropItemMonster(myStats->ring, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->ring = NULL;
		entity = dropItemMonster(myStats->mask, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->mask = NULL;
		node_t* nextnode = NULL;

		int mapIndex = 0;
		if ( my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4 )
		{
			mapIndex = (int)(my->y / 16)* MAPLAYERS + (int)(my->x / 16) * MAPLAYERS * map.height;
		}

		for ( node = myStats->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			for ( c = item->count; c > 0; c-- )
			{
				bool wasDroppable = item->isDroppable;
				if ( myStats->type == SHOPKEEPER )
				{
					auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
					if ( rng.rand() % 2 )
					{
						item->isDroppable = false; // sometimes don't drop inventory
					}
				}
				if ( myStats->type == GNOME )
				{
					if ( item->type == TOOL_BEARTRAP || (item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB) )
					{
						if ( myStats->getAttribute("gnome_type").find("gnome2") != std::string::npos )
						{
							if ( !(swimmingtiles[map.tiles[mapIndex]] || lavatiles[map.tiles[mapIndex]]) )
							{
								item->isDroppable = false;
								if ( item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB )
								{
									item->applyBomb(my, item->type, Item::ItemBombPlacement::BOMB_FLOOR, Item::ItemBombFacingDirection::BOMB_UP, my, nullptr);
								}
								else if ( item->type == TOOL_BEARTRAP )
								{
									Entity* trap = item_ToolBeartrap(item, my);
									if ( trap )
									{
										trap->x = my->x;
										trap->y = my->y;
									}
									if ( !item )
									{
										// consumed
										continue;
									}
								}
							}
						}
					}
				}
				bool wasQuiver = itemTypeIsQuiver(item->type);
				entity = dropItemMonster(item, my, myStats); // returns nullptr on "undroppables"
				if ( entity )
				{
					entity->flags[USERFLAG1] = true;    // makes items passable, improves performance
				}
				if ( wasQuiver )
				{
					break; // always drop the whole stack.
				}
				if ( c > 1 )
				{
					item->isDroppable = wasDroppable;
				}
			}
		}

		// broadcast my player allies about my death
		int playerFollower = MAXPLAYERS;
		for (c = 0; c < MAXPLAYERS; c++)
		{
			if (players[c] && players[c]->entity)
			{
				if (myStats->leader_uid == players[c]->entity->getUID())
				{
					playerFollower = c;
					if ( stats[c] )
					{
						for ( node_t* allyNode = stats[c]->FOLLOWERS.first; allyNode != nullptr; allyNode = allyNode->next )
						{
							if ( *((Uint32*)allyNode->element) == my->getUID() )
							{
								list_RemoveNode(allyNode);
								if ( myStats->monsterIsCharmed == 1 && client_classes[c] == CLASS_MESMER )
								{
									steamStatisticUpdateClient(c, STEAM_STAT_SURROGATES, STEAM_STAT_INT, 1);
								}
								if ( !players[c]->isLocalPlayer() )
								{
									serverRemoveClientFollower(c, my->getUID());
								}
								else
								{
									if ( FollowerMenu[c].recentEntity && (FollowerMenu[c].recentEntity->getUID() == 0
										|| FollowerMenu[c].recentEntity->getUID() == my->getUID()) )
									{
										FollowerMenu[c].recentEntity = nullptr;
									}
									if ( FollowerMenu[c].followerToCommand == my )
									{
										FollowerMenu[c].closeFollowerMenuGUI();
									}
								}
								break;
							}
						}
					}
					break;
				}
			}
		}

		bool skipObituary = false;
		if ( my->monsterAllySummonRank != 0 && myStats->MP > 0 )
		{
			skipObituary = true;
		}

		if ( playerFollower < MAXPLAYERS && !skipObituary )
		{
			messagePlayerMonsterEvent(playerFollower, 0xFFFFFFFF, *myStats, Language::get(1499), Language::get(2589), MSG_OBITUARY);
		}

		// drop gold
		if ( gameplayCustomManager.inUse() )
		{
			int numGold = myStats->GOLD * (gameplayCustomManager.globalGoldPercent / 100.f);
			myStats->GOLD = numGold;
		}
		if ( myStats->GOLD > 0 && myStats->monsterNoDropItems == 0 )
		{
			entity = newEntity(myStats->GOLD < 5 ? 1379 : 130, 0, map.entities, nullptr); // 130 = goldbag model
			entity->sizex = 4;
			entity->sizey = 4;
			entity->x = my->x;
			entity->y = my->y;
			entity->goldAmount = myStats->GOLD; // amount
			entity->z = 0;
			entity->vel_z = (-40 - local_rng.rand() % 5) * .01;
			entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
			entity->goldBouncing = 0;
			entity->flags[PASSABLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->behavior = &actGoldBag;
		}

		// die
#ifdef USE_FMOD
		if ( MONSTER_SOUND )
		{
			MONSTER_SOUND->stop();
		}
#elif defined USE_OPENAL
		if ( MONSTER_SOUND )
		{
			OPENAL_Channel_Stop(MONSTER_SOUND);
		}
#endif
		myStats = my->getStats();
		switch ( myStats->type )
		{
			case HUMAN:
				humanDie(my);
				break;
			case RAT:
				ratDie(my);
				break;
			case GOBLIN:
				goblinDie(my);
				break;
			case SLIME:
				slimeDie(my);
				break;
			case SCORPION:
				scorpionDie(my);
				break;
			case SUCCUBUS:
				succubusDie(my);
				break;
			case TROLL:
				trollDie(my);
				break;
			case SHOPKEEPER:
				shopkeeperDie(my);
				break;
			case SKELETON:
				skeletonDie(my);
				break;
			case MINOTAUR:
				minotaurDie(my);
				break;
			case GHOUL:
				ghoulDie(my);
				break;
			case DEMON:
				demonDie(my);
				break;
			case SPIDER:
				spiderDie(my);
				break;
			case LICH:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICH_DEATH; // lich death state
				my->monsterSpecialTimer = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case CREATURE_IMP:
				impDie(my);
				break;
			case GNOME:
				gnomeDie(my);
				break;
			case DEVIL:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_DEVIL_DEATH; // devil death state
				my->monsterSpecialTimer = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				MONSTER_ARMBENDED = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case AUTOMATON:
				automatonDie(my);
				break;
			case COCKATRICE:
				cockatriceDie(my);
				break;
			case CRYSTALGOLEM:
				crystalgolemDie(my);
				break;
			case SCARAB:
				scarabDie(my);
				break;
			case KOBOLD:
				koboldDie(my);
				break;
			case SHADOW:
				shadowDie(my);
				break;
			case VAMPIRE:
				vampireDie(my);
				break;
			case INCUBUS:
				incubusDie(my);
				break;
			case INSECTOID:
				insectoidDie(my);
				break;
			case GOATMAN:
				goatmanDie(my);
				break;
			case LICH_FIRE:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICHFIRE_DIE; // lich death state
				my->monsterSpecialTimer = 180;
				my->monsterAttack = 0;
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 0);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case LICH_ICE:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICHICE_DIE; // lich death state
				my->monsterSpecialTimer = 180;
				my->monsterAttack = 0;
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 0);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case SENTRYBOT:
			case SPELLBOT:
				sentryBotDie(my);
				break;
			case GYROBOT:
				gyroBotDie(my);
				break;
			case DUMMYBOT:
				dummyBotDie(my);
				break;
			case MIMIC:
				mimicDie(my);
				break;
			case BAT_SMALL:
				batDie(my);
				break;
			case BUGBEAR:
				bugbearDie(my);
				break;
			default:
				break; //This should never be reached.
		}
		return;
	}

	if ( multiplayer != CLIENT )
	{
		my->effectTimes();
	}

	if ( ticks % TICKS_PER_SECOND == 0 )
	{
		my->checkGroundForItems();
	}

	// check to see if monster can scream again
	if ( MONSTER_SOUND != NULL )
	{
#ifdef DEBUG_EVENT_TIMERS
		auto time1 = std::chrono::high_resolution_clock::now();
		auto time2 = std::chrono::high_resolution_clock::now();
		auto accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
#endif
#ifdef USE_FMOD
		bool playing;
		MONSTER_SOUND->isPlaying(&playing);
		if (!playing)
		{
			MONSTER_SOUND = nullptr;
		}
#elif defined USE_OPENAL
		ALboolean playing;
		OPENAL_Channel_IsPlaying(MONSTER_SOUND, &playing);
		if (!playing)
		{
			MONSTER_SOUND = NULL;
		}
#endif

#ifdef DEBUG_EVENT_TIMERS
		time2 = std::chrono::high_resolution_clock::now();
		accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
		if ( accum > 2 )
		{
			printlog("Large tick time: [actMonster 1] %f", accum);
		}
#endif
	}

	// remove broken equipment
	if ( myStats->helmet != NULL )
	{
		if ( myStats->helmet->status == BROKEN )
		{
			free(myStats->helmet);
			myStats->helmet = NULL;
		}
	}
	if ( myStats->breastplate != NULL )
	{
		if ( myStats->breastplate->status == BROKEN )
		{
			free(myStats->breastplate);
			myStats->breastplate = NULL;
		}
	}
	if ( myStats->gloves != NULL )
	{
		if ( myStats->gloves->status == BROKEN )
		{
			free(myStats->gloves);
			myStats->gloves = NULL;
		}
	}
	if ( myStats->shoes != NULL )
	{
		if ( myStats->shoes->status == BROKEN )
		{
			free(myStats->shoes);
			myStats->shoes = NULL;
		}
	}
	if ( myStats->shield != NULL )
	{
		if ( myStats->shield->status == BROKEN )
		{
			free(myStats->shield);
			myStats->shield = NULL;
		}
	}
	if ( myStats->weapon != NULL )
	{
		if ( myStats->weapon->status == BROKEN )
		{
			free(myStats->weapon);
			myStats->weapon = NULL;
		}
	}
	if ( myStats->cloak != NULL )
	{
		if ( myStats->cloak->status == BROKEN )
		{
			free(myStats->cloak);
			myStats->cloak = NULL;
		}
	}
	if ( myStats->amulet != NULL )
	{
		if ( myStats->amulet->status == BROKEN )
		{
			free(myStats->amulet);
			myStats->amulet = NULL;
		}
	}
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->status == BROKEN )
		{
			free(myStats->ring);
			myStats->ring = NULL;
		}
	}
	if ( myStats->mask != NULL )
	{
		if ( myStats->mask->status == BROKEN )
		{
			free(myStats->mask);
			myStats->mask = NULL;
		}
	}

	// calculate weight
	Sint32 weight = 0;
	if ( myStats->helmet != NULL )
	{
		weight += myStats->helmet->getWeight();
	}
	if ( myStats->breastplate != NULL )
	{
		weight += myStats->breastplate->getWeight();
	}
	if ( myStats->gloves != NULL )
	{
		weight += myStats->gloves->getWeight();
	}
	if ( myStats->shoes != NULL )
	{
		weight += myStats->shoes->getWeight();
	}
	if ( myStats->shield != NULL )
	{
		weight += myStats->shield->getWeight();
	}
	if ( myStats->weapon != NULL )
	{
		weight += myStats->weapon->getWeight();
	}
	if ( myStats->cloak != NULL )
	{
		weight += myStats->cloak->getWeight();
	}
	if ( myStats->amulet != NULL )
	{
		weight += myStats->amulet->getWeight();
	}
	if ( myStats->ring != NULL )
	{
		weight += myStats->ring->getWeight();
	}
	if ( myStats->mask != NULL )
	{
		weight += myStats->mask->getWeight();
	}
	weight += myStats->getGoldWeight();
	weight /= 2; // on monsters weight shouldn't matter so much
	double weightratio = (1000 + my->getSTR() * 100 - weight) / (double)(1000 + my->getSTR() * 100);
	weightratio = fmin(fmax(0, weightratio), 1);
	// determine if I have a ranged weapon or not
	hasrangedweapon = my->hasRangedWeapon();

	// effect of a ring of conflict
	bool ringconflict = false;
	Entity* ringConflictHolder = nullptr;
	if ( myStats->type != LICH_ICE && myStats->type != LICH_FIRE )
	{
		for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures can wear rings, so don't search map.entities.
		{
			Entity* tempentity = (Entity*)node->element;
			if ( tempentity != nullptr && tempentity != my )
			{
				Stat* tempstats = tempentity->getStats();
				if ( tempstats && tempstats->ring && tempstats->ring->type == RING_CONFLICT )
				{
					int conflictRange = 5 * TOUCHRANGE;
					if ( sqrt(pow(my->x - tempentity->x, 2) + pow(my->y - tempentity->y, 2)) < conflictRange )
					{
						tangent = atan2(tempentity->y - my->y, tempentity->x - my->x);
						lineTrace(my, my->x, my->y, tangent, conflictRange, 0, false);
						ringconflict = true;
						if ( hit.entity == tempentity )
						{
							ringConflictHolder = tempentity;
						}
						break;
					}
				}
			}
		}
	}

	// invisibility
	bool handleinvisible = true;
	switch ( myStats->type )
	{
		case HUMAN:
		case GOBLIN:
		case SKELETON:
		case GNOME:
		case KOBOLD:
		case AUTOMATON:
		case INSECTOID:
		case GOATMAN:
		case INCUBUS:
		case SHADOW:
		case VAMPIRE:
		case SUCCUBUS:
		case SHOPKEEPER:
		case LICH_FIRE:
		case LICH_ICE:
		case SENTRYBOT:
		case SPELLBOT:
		case GYROBOT:
		case DUMMYBOT:
		case MIMIC:
		case BUGBEAR:
			handleinvisible = false;
			break;
		default:
			break;
	}
	if ( handleinvisible )
	{
		//TODO: Should this use isInvisible()?
		if ( myStats->EFFECTS[EFF_INVISIBLE] )
		{
			my->flags[INVISIBLE] = true;
			for ( node = list_Node(&my->children, 2); node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				entity->flags[INVISIBLE] = true;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			for ( node = list_Node(&my->children, 2); node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				entity->flags[INVISIBLE] = false;
			}
		}
	}

	// chatting
	char namesays[64];
	if ( !strcmp(myStats->name, "") || monsterNameIsGeneric(*myStats) )
	{
		if ( monsterNameIsGeneric(*myStats) )
		{
			snprintf(namesays, 63, Language::get(1302), myStats->name);
		}
		else
		{
			snprintf(namesays, 63, Language::get(513), getMonsterLocalizedName(myStats->type).c_str());
		}
	}
	else
	{
		snprintf(namesays, 63, Language::get(1302), myStats->name);
	}
	int monsterclicked = -1;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if (inrange[i])
			{
				monsterclicked = i;
			}
		}
	}
	if ( MONSTER_CLICKED )
	{
		monsterclicked = MONSTER_CLICKED - 1;
		MONSTER_CLICKED = 0;
	}
	if ( monsterclicked >= 0 && monsterclicked < MAXPLAYERS
		&& players[monsterclicked] && players[monsterclicked]->entity )
	{
		if ( !my->isMobile() )
		{
			// message the player, "the %s doesn't respond"
			if ( my->isInertMimic() )
			{
				// wake up
				if ( myStats->EFFECTS[EFF_MIMIC_LOCKED] )
				{
					messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(462));
					playSoundEntity(my, 152, 64);
				}
				else if ( my->disturbMimic(players[monsterclicked]->entity, false, false) )
				{
					messagePlayer(monsterclicked, MESSAGE_INTERACTION, Language::get(6081));
				}
			}
			else if ( myStats->type == BAT_SMALL && my->monsterSpecialState == BAT_REST )
			{
				my->disturbBat(players[monsterclicked]->entity, false, false);
			}
			else
			{
				messagePlayerMonsterEvent(monsterclicked, 0xFFFFFFFF, *myStats, Language::get(514), Language::get(515), MSG_COMBAT);
			}
		}
		else
		{
			if (my->monsterTarget == players[monsterclicked]->entity->getUID() && my->monsterState != 4)
			{
				// angry at the player, "En Guarde!"
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					switch (myStats->type)
					{
						case HUMAN:
							if (local_rng.getU8() % 2) {
								players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
									Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(516 + local_rng.uniform(0, 1)),
									Language::get(4234 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
							} else {
								players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
									Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(518 + local_rng.uniform(0, 1)),
									Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
							}
							break;
						case SHOPKEEPER:
							if ( stats[monsterclicked] )
							{
								if ( stats[monsterclicked]->type != HUMAN )
								{
									players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(3243),
										Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
								}
								else
								{
									if (local_rng.getU8() % 2) {
										players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
											Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(516 + local_rng.uniform(0, 1)),
											Language::get(4234 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
									} else {
										players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
											Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(518 + local_rng.uniform(0, 1)),
											Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
									}
								}
							}
							else
							{
								if (local_rng.getU8() % 2) {
									players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(516 + local_rng.uniform(0, 1)),
										Language::get(4234 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
								} else {
									players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(518 + local_rng.uniform(0, 1)),
										Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(stats[monsterclicked]->type).c_str());
								}
							}
							break;
						default:
							break;
					}
				}
			}
			else if (my->monsterState == MONSTER_STATE_TALK)
			{
				// for shopkeepers trading with a player, "I am somewhat busy now."
				if (my->monsterTarget != players[monsterclicked]->entity->getUID())
				{
					switch (myStats->type)
					{
						case SHOPKEEPER:
						case HUMAN:
							//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(520 + local_rng.rand() % 4), namesays);
							players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
								Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(520 + local_rng.rand() % 4));
							break;
						default:
							//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(524), namesays);
							players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
								Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(524));
							break;
					}
				}
			}
			else
			{
				// handle followers/trading
				if ( myStats->type != SHOPKEEPER )
				{
					if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 )
					{
						if ( Player::SkillSheet_t::generateFollowerTableForSkillsheet )
						{
							Player::SkillSheet_t::generateFollowerTableForSkillsheet = false;
							printFollowerTableForSkillsheet(monsterclicked, my, myStats);
						}
						else
						{
							makeFollower(monsterclicked, ringconflict, namesays, my, myStats, false);
						}
					}
					else
					{
						char dialogueName[64];
						if ( !strcmp(myStats->name, "") || monsterNameIsGeneric(*myStats) )
						{
							if ( monsterNameIsGeneric(*myStats) )
							{
								snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), myStats->name);
							}
							else
							{
								snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), getMonsterLocalizedName(myStats->type).c_str());
							}
						}
						else
						{
							snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), myStats->name);
						}
						if ( dialogueName[0] >= 'a' && dialogueName[0] <= 'z' )
						{
							dialogueName[0] = toupper(dialogueName[0]);
						}
						handleMonsterChatter(monsterclicked, ringconflict, dialogueName, my, myStats);
					}
					my->lookAtEntity(*players[monsterclicked]->entity);
				}
				else
				{
					bool canTrade = true;

					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( players[i] && players[i]->entity ) // check hostiles
						{
							if ( uidToEntity(my->monsterTarget) == players[i]->entity )
							{
								canTrade = false;
							}
						}
					}
					if ( my->monsterState != MONSTER_STATE_WAIT )
					{
						canTrade = false;
					}


					if ( !canTrade )
					{
						if ( !my->checkEnemy(players[monsterclicked]->entity) )
						{
							switch ( myStats->type )
							{
								case SHOPKEEPER:
								case HUMAN:
									//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(520 + local_rng.rand() % 3), namesays);
									players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(520 + local_rng.rand() % 3));
									break;
								default:
									//messagePlayer(monsterclicked, MESSAGE_INTERACTION | MESSAGE_WORLD, Language::get(524), namesays);
									players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC, Language::get(524));
									break;
							}
						}
					}
					else if ( myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 ) // mysterious merchant
					{
						// interacting triggers compendium reveal
						auto find = Compendium_t::Events_t::monsterUniqueIDLookup.find("mysterious shop");
						if ( find != Compendium_t::Events_t::monsterUniqueIDLookup.end() )
						{
							auto find2 = Compendium_t::Events_t::monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + find->second);
							if ( find2 != Compendium_t::Events_t::monsterIDToString.end() )
							{
								if ( players[monsterclicked]->isLocalPlayer() )
								{
									auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find2->second];
									if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
									{
										unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
									}
								}
								else if ( monsterclicked > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "CMPU");
									SDLNet_Write32(Compendium_t::Events_t::kEventMonsterOffset + find->second, &net_packet->data[4]);
									net_packet->address.host = net_clients[monsterclicked - 1].host;
									net_packet->address.port = net_clients[monsterclicked - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, monsterclicked - 1);
								}
							}
						}

						bool hasOrb = false;
						for ( node_t* node = myStats->inventory.first; node; node = node->next )
						{
							Item* item = (Item*)node->element;
							if ( item && (item->type == ARTIFACT_ORB_BLUE
									|| item->type == ARTIFACT_ORB_GREEN
									|| item->type == ARTIFACT_ORB_RED)
								)
							{
								hasOrb = true;
								break;
							}
						}
						if ( !hasOrb )
						{
							char dialogueName[64];
							if ( !strcmp(myStats->name, "") || monsterNameIsGeneric(*myStats) )
							{
								if ( monsterNameIsGeneric(*myStats) )
								{
									snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), myStats->name);
								}
								else
								{
									snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), getMonsterLocalizedName(myStats->type).c_str());
								}
							}
							else
							{
								snprintf(dialogueName, sizeof(dialogueName), Language::get(4216), myStats->name);
							}
							if ( dialogueName[0] >= 'a' && dialogueName[0] <= 'z' )
							{
								dialogueName[0] = toupper(dialogueName[0]);
							}
							handleMonsterChatter(monsterclicked, ringconflict, dialogueName, my, myStats);
							my->lookAtEntity(*players[monsterclicked]->entity);
						}
						else
						{
							// shopkeepers start trading
							startTradingServer(my, monsterclicked);
						}
					}
					else if ( players[monsterclicked] && players[monsterclicked]->entity )
					{
						if ( !my->checkEnemy(players[monsterclicked]->entity) )
						{
							// shopkeepers start trading
							startTradingServer(my, monsterclicked);
							if ( stats[monsterclicked] && stats[monsterclicked]->type == HUMAN && stats[monsterclicked]->stat_appearance == 0
								&& stats[monsterclicked]->playerRace == RACE_AUTOMATON )
							{
								achievementObserver.updatePlayerAchievement(monsterclicked, AchievementObserver::Achievement::BARONY_ACH_REAL_BOY,
									AchievementObserver::AchievementEvent::REAL_BOY_SHOP);
							}
						}
					}
				}
			}
		}
	}

	bool isIllusionTaunt = false;
	if ( myStats->type == INCUBUS && !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		isIllusionTaunt = true;
		hasrangedweapon = false;
		Entity* myTarget = uidToEntity(static_cast<Uint32>(my->monsterIllusionTauntingThisUid));
		if ( myTarget )
		{
			if ( my->ticks % 50 == 0 )
			{
				if ( myTarget->monsterTarget != my->getUID() )
				{
					switch ( myTarget->getRace() )
					{
						case LICH:
						case DEVIL:
						case LICH_FIRE:
						case LICH_ICE:
						case MINOTAUR:
							break;
						default:
							myTarget->monsterAcquireAttackTarget(*my, MONSTER_STATE_PATH);
							break;
					}
				}
			}
			if ( my->isMobile() && my->ticks > 10 )
			{
				if ( (my->monsterState != MONSTER_STATE_WAIT && my->monsterHitTime >= 30 && my->monsterHitTime <= 40)
					|| (my->ticks >= 100 && my->monsterAttack == 0) )
				{
					my->monsterReleaseAttackTarget();
					my->attack(MONSTER_POSE_INCUBUS_TAUNT, 0, nullptr);
				}
				else if ( my->monsterState == MONSTER_STATE_WAIT )
				{
					my->monsterHitTime = HITRATE - 3;
					if ( entityDist(my, myTarget) > STRIKERANGE * 1.5 )
					{
						my->monsterState = MONSTER_STATE_PATH;
						my->monsterTarget = myTarget->getUID();
						my->monsterTargetX = myTarget->x;
						my->monsterTargetY = myTarget->y;
					}
					else
					{
						my->monsterState = MONSTER_STATE_ATTACK;
						my->monsterTarget = myTarget->getUID();
						my->monsterTargetX = myTarget->x;
						my->monsterTargetY = myTarget->y;
					}
				}
			}
		}
		else
		{
			my->modHP(-9999);
		}
	}

	if ( my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
	{
		if ( myStats && myStats->leader_uid != 0 )
		{
			Entity* leader = uidToEntity(myStats->leader_uid);
			if ( !leader && my->flags[USERFLAG2] )
			{
				my->flags[USERFLAG2] = false;
				serverUpdateEntityFlag(my, USERFLAG2);
				if ( monsterChangesColorWhenAlly(myStats) )
				{
					int bodypart = 0;
					for ( node_t* node = my->children.first; node != nullptr; node = node->next )
					{
						if ( bodypart >= LIMB_HUMANOID_TORSO )
						{
							Entity* tmp = (Entity*)node->element;
							if ( tmp )
							{
								tmp->flags[USERFLAG2] = false;
								serverUpdateEntityFlag(tmp, USERFLAG2);
							}
						}
						++bodypart;
					}
				}
			}
			else if ( leader && leader->behavior == &actMonster )
			{
				monsterAllyFormations.updateFormation(myStats->leader_uid, my->getUID());
				monsterAllyFormations.updateFormation(myStats->leader_uid);
			}
		}
	}

	bool wasInsideEntity = false;
	if ( my->isMobile() )
	{
		// ghouls rise out of the dirt :O
		if ( myStats->type == GHOUL )
		{
			if ( my->z > -.25 )
			{
				my->z -= .25;
				if ( my->z < -.25 )
				{
					my->z = -.25;
				}
				if (my->ticks % 2 == 0) {
				    const int x = my->x + local_rng.uniform(-3, 3);
				    const int y = my->y + local_rng.uniform(-3, 3);
				    auto poof = spawnPoof(x, y, 6, 0.33, true);
				}
				ghoulMoveBodyparts(my, myStats, 0);
				return;
			}
		}

		// being bumped by someone friendly
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			for ( node2 = currentList->first; node2 != nullptr; node2 = node2->next ) //Can't convert to map.creatures because of doorframes.
			{
				entity = (Entity*)node2->element;
				if ( entity == my )
				{
					continue;
				}
				if ( entity->behavior != &actMonster && entity->behavior != &actPlayer && entity->behavior != &actDoorFrame )
				{
					continue;
				}

				int sizex = my->sizex;
				int sizey = my->sizey;
				if ( entity->getRace() == BUGBEAR && myStats->type == BUGBEAR )
				{
					my->sizex = 8;
					my->sizey = 8;
				}
				bool entityInside = entityInsideEntity(my, entity);
				my->sizex = sizex;
				my->sizey = sizey;
				if ( entityInside && entity->getRace() != GYROBOT )
				{
					if ( entity->behavior != &actDoorFrame )
					{
						double tangent = atan2(my->y - entity->y, my->x - entity->x);
						MONSTER_VELX = cos(tangent) * .1;
						MONSTER_VELY = sin(tangent) * .1;
					}
					else if ( entity->behavior == &actDoorFrame && 
						entity->flags[INVISIBLE] )
					{
						int mapx = (int)floor(entity->x / 16);
						int mapy = (int)floor(entity->y / 16);
						if ( entity->yaw >= -0.1 && entity->yaw <= 0.1 )
						{
							// east/west doorway
							if ( my->y < floor(entity->y / 16) * 16 + 8 )
							{
								bool slide = true;
								if ( mapy - 1 > 0 )
								{
									int index = (mapy - 1) * MAPLAYERS + (mapx) * MAPLAYERS * map.height;
									if ( !map.tiles[OBSTACLELAYER + index] )
									{
										// no effect, wall is missing
										slide = false;
									}
								}

								if ( slide )
								{
									// slide south
									MONSTER_VELX = 0;
									MONSTER_VELY = .25;
								}
							}
							else
							{
								bool slide = true;
								if ( mapy + 1 < map.height )
								{
									int index = (mapy + 1) * MAPLAYERS + (mapx) * MAPLAYERS * map.height;
									if ( !map.tiles[OBSTACLELAYER + index] )
									{
										// no effect, wall is missing
										slide = false;
									}
								}

								if ( slide )
								{
									// slide north
									MONSTER_VELX = 0;
									MONSTER_VELY = -.25;
								}
							}
						}
						else
						{
							// north/south doorway
							if ( my->x < floor(entity->x / 16) * 16 + 8 )
							{
								bool slide = true;
								if ( mapx - 1 > 0 )
								{
									int index = (mapy) * MAPLAYERS + (mapx - 1) * MAPLAYERS * map.height;
									if ( !map.tiles[OBSTACLELAYER + index] )
									{
										// no effect, wall is missing
										slide = false;
									}
								}

								if ( slide )
								{
									// slide east
									MONSTER_VELX = .25;
									MONSTER_VELY = 0;
								}
							}
							else
							{
								bool slide = true;
								if ( mapx + 1 < map.width )
								{
									int index = (mapy) * MAPLAYERS + (mapx + 1) * MAPLAYERS * map.height;
									if ( !map.tiles[OBSTACLELAYER + index] )
									{
										// no effect, wall is missing
										slide = false;
									}
								}

								if ( slide )
								{
									// slide west
									MONSTER_VELX = -.25;
									MONSTER_VELY = 0;
								}
							}
						}
						wasInsideEntity = true;
						//messagePlayer(0, MESSAGE_DEBUG, "path: %d", my->monsterPathCount);
						++my->monsterPathCount;
						if ( my->monsterPathCount > 50 )
						{
							my->monsterPathCount = 0;
							monsterMoveAside(my, my);
						}
						if ( my->monsterState == MONSTER_STATE_ATTACK )
						{
							MONSTER_VELX = 0.f;
							MONSTER_VELY = 0.f;
							continue;
						}
					}
					else
					{
						continue;
					}


					if ( (entity->sprite == 274 || entity->sprite == 646 
						|| entity->sprite == 650 || entity->sprite == 304) 
						&& entity->flags[PASSABLE] == true )
					{
						// LICH/LICH_FIRE/LICH_ICE/DEVIL
						// If these guys are PASSABLE then they're either dying or some other animation
						// Move the monster inside the boss, but don't set PASSABLE to false again.
						clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
					}
					else
					{
						bool oldFlag = entity->flags[PASSABLE];
						entity->flags[PASSABLE] = true;
						clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
						entity->flags[PASSABLE] = oldFlag;
					}
				}
			}
		}

		if ( myStats->type != LICH 
			&& myStats->type != DEVIL 
			&& myStats->type != LICH_ICE
			&& myStats->type != LICH_FIRE )
		{
			if ( my->monsterSpecialTimer > 0 )
			{
				--my->monsterSpecialTimer;
			}
			if ( (int)my->monsterSpecialAttackUnequipSafeguard > 0 )
			{
				my->monsterSpecialAttackUnequipSafeguard -= 1.0;
				if ( (int)my->monsterSpecialAttackUnequipSafeguard <= 0 )
				{
					//messagePlayer(0, MESSAGE_DEBUG, "Cleared monster special");
					my->handleMonsterSpecialAttack(myStats, nullptr, 0.0, true);
				}
			}
		}

		if ( my->monsterAllySpecialCooldown > 0 )
		{
			--my->monsterAllySpecialCooldown;
		}

		if ( myStats->type == AUTOMATON )
		{
			my->automatonRecycleItem();
		}
		else if ( myStats->type == MIMIC )
		{
			if ( my->monsterSpecialState != MIMIC_ACTIVE )
			{
				my->monsterSpecialState = MIMIC_ACTIVE;
				serverUpdateEntitySkill(my, 33);
			}
		}

		if ( myStats->EFFECTS[EFF_PACIFY] || myStats->EFFECTS[EFF_FEAR] )
		{
			my->monsterHitTime = HITRATE / 2; // stop this incrementing to HITRATE but leave monster ready to strike shortly after.
		}

		if ( my->monsterDefend != MONSTER_DEFEND_NONE )
		{
			if ( my->monsterState != MONSTER_STATE_ATTACK
				|| myStats->shield == nullptr )
			{
				myStats->defending = false;
				my->monsterDefend = 0;
				serverUpdateEntitySkill(my, 47);
			}
			else if ( my->monsterAttack == 0 )
			{
				myStats->defending = true;
			}
		}
		else
		{
			myStats->defending = false;
		}

		/*if ( myStats->defending )
		{
			messagePlayer(0, "defending!");
		}*/

		//if ( myStats->type == GNOME )
		//{
		//	std::string state_string;
		//	
		//	switch(my->monsterState)
		//	{
		//	case MONSTER_STATE_WAIT:
		//		state_string = "WAIT";
		//		break;
		//	case MONSTER_STATE_ATTACK:
		//		state_string = "CHARGE";
		//		break;
		//	case MONSTER_STATE_PATH:
		//		state_string = "PATH";
		//		break;
		//	case MONSTER_STATE_HUNT:
		//		state_string = "HUNT";
		//		break;
		//	case MONSTER_STATE_TALK:
		//		state_string = "TALK";
		//		break;
		//	default:
		//		state_string = std::to_string(my->monsterState);
		//		//state_string = "Unknown state";
		//		break;
		//	}
		//	
		//	messagePlayer(0, MESSAGE_DEBUG, "uid: %d | ticks: %d | %s, ATK: %d hittime:%d, atktime:%d, (%d|%d), timer:%d", 
		//		my->getUID(), ticks, state_string.c_str(), my->monsterAttack, my->monsterHitTime, MONSTER_ATTACKTIME, devilstate, devilacted, my->monsterSpecialTimer); //Debug message.
		//}

		//Begin state machine
		if ( my->monsterState == MONSTER_STATE_WAIT ) //Begin wait state
		{
			//my->monsterTarget = -1; //TODO: Setting it to -1 = Bug? -1 may not work properly for cases such as: if ( !my->monsterTarget )
			my->monsterReleaseAttackTarget();
			if ( !myStats->EFFECTS[EFF_KNOCKBACK] )
			{
				MONSTER_VELX = 0;
				MONSTER_VELY = 0;
			}
			else
			{
				// do knockback movement
				my->monsterHandleKnockbackVelocity(my->monsterKnockbackTangentDir, weightratio);
				if ( abs(MONSTER_VELX) > 0.01 || abs(MONSTER_VELY) > 0.01 )
				{
					dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
					my->handleKnockbackDamage(*myStats, hit.entity);
				}
			}
			if ( myReflex && !myStats->EFFECTS[EFF_DISORIENTED] && !isIllusionTaunt )
			{
				if ( myStats->EFFECTS[EFF_FEAR] && my->monsterFearfulOfUid != 0 )
				{
					Entity* scaryEntity = uidToEntity(my->monsterFearfulOfUid);
					if ( scaryEntity )
					{
						my->monsterAcquireAttackTarget(*scaryEntity, MONSTER_STATE_PATH);
						my->lookAtEntity(*scaryEntity);
						if ( previousMonsterState != my->monsterState )
						{
							serverUpdateEntitySkill(my, 0);
						}
						return;
					}
				}

				for ( node2 = map.creatures->first; node2 != nullptr; node2 = node2->next ) //So my concern is that this never explicitly checks for actMonster or actPlayer, instead it relies on there being stats. Now, only monsters and players have stats, so that's not a problem, except...actPlayerLimb can still return a stat from getStat()! D: Meh, if you can find the player's hand, you can find the actual player too, so it shouldn't be an issue.
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] || entity->isInertMimic() )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							// skip if light level is too low and distance is too high
							int light = entity->entityLightAfterReductions(*hitstats, my);
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
							{
								//See invisible.
								light = 1000;
							}
							else if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
							{
								light += 150;
							}
							double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );

							real_t monsterVisionRange = sightranges[myStats->type];
							if ( hitstats->type == DUMMYBOT )
							{
								monsterVisionRange = std::max(monsterVisionRange, 96.0);
							}

							if ( targetdist > monsterVisionRange )
							{
								continue;
							}
							if ( targetdist > TOUCHRANGE && targetdist > light )
							{
								if ( !(myStats->leader_uid == entity->getUID()) 
									&& !(hitstats->leader_uid == my->getUID())
									&& !(my->monsterAllyGetPlayerLeader() && entity->behavior == &actPlayer) )
								{
									if ( !levitating )
									{
										lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, true);
									}
									else
									{
										lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
									}
									if ( hit.entity == entity )
										if ( local_rng.rand() % 100 == 0 )
										{
											entity->increaseSkill(PRO_STEALTH);
										}
								}
								continue;
							}
							bool visiontest = false;
							if ( hitstats->type == DUMMYBOT || myStats->type == SENTRYBOT || myStats->type == SPELLBOT
								|| (ringConflictHolder && ringConflictHolder == entity) )
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}
							else if ( myStats->type != SPIDER )
							{
								if ( dir >= -7 * PI / 16 && dir <= 7 * PI / 16 )
								{
									visiontest = true;
								}
							}
							else
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}

							if ( visiontest )   // vision cone
							{
								if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
								{
									//See invisible
									lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
								}
								else
								{
									lineTrace(my, my->x, my->y, tangent, monsterVisionRange, LINETRACE_IGNORE_ENTITIES, false);
								}
								if ( !hit.entity )
								{
									lineTrace(my, my->x, my->y, tangent, TOUCHRANGE, 0, false);
								}
								if ( hit.entity == entity )
								{
									// charge state
									Entity* attackTarget = hit.entity;
									my->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_ATTACK);

									if ( MONSTER_SOUND == nullptr )
									{
										if ( myStats->type != MINOTAUR )
										{
											if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
											{
												sentrybotPickSpotNoise(my, myStats);
											}
											else
											{
												MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128);
											}
										}
										else
										{
											int c;
											for ( c = 0; c < MAXPLAYERS; ++c )
											{
												if ( c == 0 )
												{
													MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
												}
												else
												{
													playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
												}
											}
										}
									}

									if ( entity != nullptr )
									{
										if ( entity->behavior == &actPlayer && myStats->type != DUMMYBOT )
										{
											assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
											assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
										}
									}

									// alert other monsters of this enemy's presence //TODO: Refactor into its own function.
									for ( node = map.creatures->first; node != nullptr; node = node->next )
									{
										entity = (Entity*)node->element;
										if ( entity->behavior == &actMonster && entity != my )
										{
											Stat* buddystats = entity->getStats();
											if ( buddystats != nullptr )
											{
												if ( entity->checkFriend(my) )
												{
													if ( entity->monsterState == MONSTER_STATE_WAIT )   // monster is waiting
													{
														if ( !entity->checkFriend(attackTarget) )
														{
															tangent = atan2( entity->y - my->y, entity->x - my->x );
															lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
															if ( hit.entity == entity )
															{
																entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
															}
														}
													}
												}
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}

			// minotaurs and liches chase players relentlessly.
			if (myReflex)
			{
				if (myStats->type == MINOTAUR 
					|| myStats->type == LICH 
					|| myStats->type == LICH_FIRE 
					|| myStats->type == LICH_ICE 
					|| (myStats->type == CREATURE_IMP && strstr(map.name, "Boss") && !my->monsterAllyGetPlayerLeader())
					|| (myStats->type == AUTOMATON && strstr(myStats->name, "corrupted automaton"))
					|| (myStats->type == SHADOW && !strncmp(map.name, "Hell Boss", 9) && uidToEntity(my->parent) && uidToEntity(my->parent)->getRace() == DEVIL) )
				{
					double distToPlayer = 0;
					int c, playerToChase = -1;
					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity && my->checkEnemy(players[c]->entity))
						{
							list_t* playerPath = generatePath((int)floor(my->x / 16), (int)floor(my->y / 16), 
								(int)floor(players[c]->entity->x / 16), (int)floor(players[c]->entity->y / 16), my, players[c]->entity,
								GeneratePathTypes::GENERATE_PATH_BOSS_TRACKING_IDLE);
							if ( playerPath == NULL )
							{
								continue;
							}
							else
							{
								list_FreeAll(playerPath);
								free(playerPath);
							}
							if (!distToPlayer)
							{
								distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								playerToChase = c;
							}
							else
							{
								double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								if (newDistToPlayer < distToPlayer)
								{
									distToPlayer = newDistToPlayer;
									playerToChase = c;
								}
							}
						}
					}
					if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
					{
						if ( myStats->type == SHADOW )
						{
							my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_ATTACK);
						}
						else
						{
							my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
						}
						if ( previousMonsterState != my->monsterState )
						{
							serverUpdateEntitySkill(my, 0);
						}
						return;
					}
				}
				else if ( myStats->type == SHADOW && my->monsterTarget && my->monsterState != MONSTER_STATE_ATTACK )
				{
					//Fix shadow state.
					my->monsterState = MONSTER_STATE_PATH;
					//my->monsterTargetX = my->monsterTarget.x;
					//my->monsterTargetY = my->monsterTarget.y;
					serverUpdateEntitySkill(my, 0); //Update monster state because it changed.
					return;
				}
			}

			// follow the leader :)
			if ( myStats->leader_uid != 0
				&& my->monsterAllyState == ALLY_STATE_DEFAULT 
				&& !myStats->EFFECTS[EFF_FEAR]
				&& !myStats->EFFECTS[EFF_DISORIENTED]
				&& !myStats->EFFECTS[EFF_ROOTED]
				&& !isIllusionTaunt
				&& !monsterIsImmobileTurret(my, myStats)
				&& my->getUID() % TICKS_PER_SECOND == ticks % monsterAllyFormations.getFollowerChaseLeaderInterval(*my, *myStats)
				 )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					real_t followx = leader->x;
					real_t followy = leader->y;
					if ( myStats->type == GYROBOT )
					{
						// follow ahead of the leader.
						real_t startx = leader->x;
						real_t starty = leader->y;
						// draw line from the leaders direction until we hit a wall or 48 dist
						real_t previousx = startx;
						real_t previousy = starty;
						std::unordered_set<int> checkedTiles;
						for ( int iterations = 0; iterations < 20; ++iterations)
						{
							startx += 4 * cos(leader->yaw);
							starty += 4 * sin(leader->yaw);

							int mapx = (static_cast<int>(startx) >> 4);
							int mapy = (static_cast<int>(starty) >> 4);
							int index = (mapy) * MAPLAYERS + (mapx) * MAPLAYERS * map.height;
							if ( !map.tiles[OBSTACLELAYER + index] )
							{
								bool foundObstacle = false;
								if ( checkedTiles.find(mapx + mapy * 10000) == checkedTiles.end() )
								{
									if ( checkObstacle((mapx << 4) + 8, (mapy << 4) + 8, my, leader) )
									{
										foundObstacle = true;
									}
									checkedTiles.insert(mapx + mapy * 10000);
								}
								if ( foundObstacle )
								{
									break;
								}

								// store the last known good coordinate
								previousx = startx;
								previousy = starty;
							}
							else if ( map.tiles[OBSTACLELAYER + index] )
							{
								// hit a wall.
								break;
							}
							if ( sqrt(pow(leader->x - previousx, 2) + pow(leader->y - previousy, 2)) > WAIT_FOLLOWDIST )
							{
								break;
							}
						}
						followx = previousx;
						followy = previousy;
						// createParticleFollowerCommand(previousx, previousy, 0, 174); debug particle
					}
					double dist = sqrt(pow(my->x - followx, 2) + pow(my->y - followy, 2));

					if ( dist > WAIT_FOLLOWDIST )
					{
						bool doFollow = true;
						if ( my->monsterTarget != 0 )
						{
							doFollow = my->isFollowerFreeToPathToPlayer(myStats);
						}

						if ( doFollow )
						{
							my->monsterReleaseAttackTarget();
							std::pair<int, int> followPos;
							if ( myStats->type == GYROBOT )
							{
								if ( my->monsterSetPathToLocation(static_cast<int>(followx) / 16, static_cast<int>(followy) / 16, 0,
									GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW) )
								{
									// try closest first
									my->monsterState = MONSTER_STATE_HUNT; // hunt state
								}
							}
							else if ( monsterAllyFormations.getFollowLocation(my->getUID(), leader->getUID(), followPos) )
							{
								if ( my->monsterSetPathToLocation(followPos.first, followPos.second, 1,
									GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW) )
								{
									my->monsterState = MONSTER_STATE_HUNT; // hunt state
								}
							}
							if ( previousMonsterState != my->monsterState )
							{
								serverUpdateEntitySkill(my, 0);
								if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
								{
									serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
								}
							}
							return;
						}
					}
					else if ( myStats->type != GYROBOT )
					{
						tangent = atan2( leader->y - my->y, leader->x - my->x );
						lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						if ( hit.entity != leader )
						{
							bool doFollow = true;
							if ( my->monsterTarget != 0 )
							{
								doFollow = my->isFollowerFreeToPathToPlayer(myStats);
							}
							if ( doFollow )
							{
								my->monsterReleaseAttackTarget();
								std::pair<int, int> followPos;
								if ( monsterAllyFormations.getFollowLocation(my->getUID(), leader->getUID(), followPos) )
								{
									if ( my->monsterSetPathToLocation(followPos.first, followPos.second, 1,
										GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW) )
									{
										my->monsterState = MONSTER_STATE_HUNT; // hunt state
									}
									if ( previousMonsterState != my->monsterState )
									{
										serverUpdateEntitySkill(my, 0);
										if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
										{
											serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
										}
									}
								}
								return;
							}
						}
					}
				}
			}

			// look
			my->monsterLookTime++;
			if ( my->monsterLookTime >= 120 
				&& myStats->type != LICH 
				&& myStats->type != DEVIL
				&& myStats->type != LICH_FIRE
				&& myStats->type != LICH_ICE )
			{
				my->monsterLookTime = 0;
				my->monsterMoveTime--;
				if ( myStats->type != GHOUL && (myStats->type != SPIDER || (myStats->type == SPIDER && my->monsterAllyGetPlayerLeader()))
					&& !myStats->EFFECTS[EFF_FEAR] && !isIllusionTaunt )
				{
					if ( monsterIsImmobileTurret(my, myStats) )
					{
						if ( abs(my->monsterSentrybotLookDir) > 0.001 )
						{
							my->monsterLookDir = my->monsterSentrybotLookDir + (-30 + local_rng.rand() % 61) * PI / 180;
						}
						else
						{
							my->monsterLookDir = (local_rng.rand() % 360) * PI / 180;
						}
					}
					else if ( myStats->type == MIMIC )
					{
						my->monsterLookDir = 90 * (local_rng.rand() % 4) * PI / 180;
					}
					else
					{
						my->monsterLookDir = (local_rng.rand() % 360) * PI / 180;
					}
				}
				if ( !myStats->EFFECTS[EFF_FEAR] && my->monsterTarget == 0 && my->monsterState == MONSTER_STATE_WAIT && my->monsterAllyGetPlayerLeader() )
				{
					// allies should try intelligently scan for enemies in radius.
					if ( monsterIsImmobileTurret(my, myStats) && myStats->LVL < 5 )
					{
						// don't scan cause dumb robot.
					}
					else
					{
						real_t dist = sightranges[myStats->type];
						for ( node = map.creatures->first; node != nullptr; node = node->next )
						{
							Entity* target = (Entity*)node->element;
							if ( target->behavior == &actMonster && my->checkEnemy(target) )
							{
								real_t oldDist = dist;
								dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
								if ( dist < sightranges[myStats->type] && dist <= oldDist )
								{
									double tangent = atan2(target->y - my->y, target->x - my->x);
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
									if ( hit.entity == target )
									{
										//my->monsterLookTime = 1;
										//my->monsterMoveTime = local_rng.rand() % 10 + 1;
										my->monsterLookDir = tangent;
										if ( monsterIsImmobileTurret(my, myStats) )
										{
											if ( myStats->LVL >= 10 )
											{
												my->monsterHitTime = HITRATE * 2 - 20;
											}
										}
										break;
									}
								}
							}
						}
					}
				}
				if ( local_rng.rand() % 3 == 0 && !isIllusionTaunt )
				{
					if ( !MONSTER_SOUND )
					{
						if ( myStats->type != MINOTAUR )
						{
							if ( (local_rng.rand() % 3 == 0 && !my->monsterAllyGetPlayerLeader()) || (my->monsterAllyGetPlayerLeader() && local_rng.rand() % 8 == 0) || myStats->type == DUMMYBOT )
							{
								// idle sounds. if player follower, reduce noise frequency by 66%.
								bool doIdleSound = true;
#ifdef USE_FMOD
								if ( myStats->type == KOBOLD )
								{
									doIdleSound = local_rng.rand() % 2 == 0;
									for ( node_t* node = map.creatures->first; node && doIdleSound; node = node->next )
									{
										Entity* entity = (Entity*)node->element;
										if ( entity->behavior == &actMonster && entity->skill[19] == MONSTER_IDLESND ) // skill 19 is monster idle snd
										{
											if ( Stat* stats = entity->getStats() )
											{
												if ( stats->monster_sound )
												{
													bool playing;
													stats->monster_sound->isPlaying(&playing);
													if ( playing )
													{
														doIdleSound = false;
														break;
													}
												}
											}
										}
									}
								}
#endif
								if ( doIdleSound )
								{
									MONSTER_SOUND = playSoundEntity(my, MONSTER_IDLESND + (local_rng.rand() % MONSTER_IDLEVAR), 128);
								}
							}
						}
						else
						{
							int c;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( c == 0 )
								{
									MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
								}
								else
								{
									playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
								}
							}
						}
					}
				}
			}
			if ( my->monsterMoveTime == 0 
				&& (uidToEntity(myStats->leader_uid) == NULL || my->monsterAllyState == ALLY_STATE_DEFEND)
				&& !myStats->EFFECTS[EFF_FEAR] 
				&& !myStats->EFFECTS[EFF_DISORIENTED]
				&& !myStats->EFFECTS[EFF_ROOTED]
				&& !isIllusionTaunt
				&& !(monsterIsImmobileTurret(my, myStats))
				&& myStats->type != DEVIL )
			{
				std::vector<std::pair<int, int>> possibleCoordinates;
				my->monsterMoveTime = local_rng.rand() % 30;
				if ( myStats->type == MIMIC || myStats->type == BAT_SMALL )
				{
					my->monsterMoveTime = 2 + local_rng.rand() % 4;
				}
				int goodspots = 0;
				int centerX = static_cast<int>(my->x / 16); // grab the coordinates in small form.
				int centerY = static_cast<int>(my->y / 16); // grab the coordinates in small form.

				int searchLimitX = (map.width / 2);
				int searchLimitY = (map.height / 2);
				if ( myStats->type == MIMIC )
				{
					searchLimitX = 5;
					searchLimitY = 5;
				}
				else if ( myStats->type == BAT_SMALL )
				{
					searchLimitX = 7;
					searchLimitY = 7;
				}
				int lowerX = std::max<int>(0, centerX - searchLimitX); // assigned upper/lower x coords from entity start position.
				int upperX = std::min<int>(centerX + searchLimitX, map.width);

				int lowerY = std::max<int>(0, centerY - searchLimitY); // assigned upper/lower y coords from entity start position.
				int upperY = std::min<int>(centerY + searchLimitY, map.height);
				//messagePlayer(0, "my x: %d, my y: %d, rangex: (%d-%d), rangey: (%d-%d)", centerX, centerY, lowerX, upperX, lowerY, upperY);

				if ( myStats->type != SHOPKEEPER && (myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 && my->monsterAllyState == ALLY_STATE_DEFAULT) )
				{
					for ( x = lowerX; x < upperX; x++ )
					{
						for ( y = lowerY; y < upperY; y++ )
						{
							if ( !checkObstacle(x << 4, y << 4, my, NULL) )
							{
								goodspots++;
								possibleCoordinates.push_back(std::make_pair(x, y));
							}
						}
					}
				}
				else
				{
					for ( x = 0; x < map.width; x++ )
					{
						for ( y = 0; y < map.height; y++ )
						{
							if ( x << 4 >= my->monsterPathBoundaryXStart && x << 4 <= my->monsterPathBoundaryXEnd
								&& y << 4 >= my->monsterPathBoundaryYStart && y << 4 <= my->monsterPathBoundaryYEnd )
								if ( !checkObstacle(x << 4, y << 4, my, NULL) )
								{
									goodspots++;
									possibleCoordinates.push_back(std::make_pair(x, y));
								}
						}
					}
				}
				if ( goodspots )
				{
					int chosenspot = local_rng.rand() % goodspots;
					x = possibleCoordinates.at(chosenspot).first;
					y = possibleCoordinates.at(chosenspot).second;
					//messagePlayer(0, "Chose distance: %.1fpercent", 100 * (sqrt(pow(my->x / 16 - (x), 2) + pow(my->y / 16 - (y), 2))) / sqrt(pow(map.height, 2) + pow(map.width, 2)));
					/*for ( x = 0; x < map.width; x++ )
					{
						for ( y = 0; y < map.height; y++ )
						{
							if ( !checkObstacle(x << 4, y << 4, my, NULL) )
							{
								if ( currentspot == chosenspot )
								{
									foundit = true;
									break;
								}
								else
								{
									currentspot++;
								}
							}
						}
						if ( foundit )
						{
							break;
						}
					}*/
					path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, NULL,
						GeneratePathTypes::GENERATE_PATH_IDLE_WALK);
					if ( my->children.first != NULL )
					{
						list_RemoveNode(my->children.first);
					}
					node = list_AddNodeFirst(&my->children);
					node->element = path;
					node->deconstructor = &listDeconstructor;
					my->monsterState = MONSTER_STATE_HUNT; // hunt state
				}
			}

			// rotate monster
			dir = my->monsterRotate();

			if ( myStats->type == SHADOW && !uidToEntity(my->monsterTarget) && my->monsterSpecialTimer == 0 && my->monsterSpecialState == 0 && ticks%500 == 0 && local_rng.rand()%5 == 0 )
			{
				//Random chance for a shadow to teleport around the map if it has nothing better to do.
				//messagePlayer(0, "Shadow idle telepotty.");
				my->monsterSpecialState = SHADOW_TELEPORT_ONLY;
				my->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_PASIVE_TELEPORT;
				my->shadowTeleportToTarget(nullptr, 3); // teleport in closer range
				my->monsterState = MONSTER_STATE_WAIT;
			}
		} //End wait state
		else if ( my->monsterState == MONSTER_STATE_ATTACK ) //Begin charge state
		{
			entity = uidToEntity(my->monsterTarget);
			if ( entity && entity->isInertMimic() )
			{
				entity = nullptr;
				my->monsterReleaseAttackTarget();
			}
			if ( entity == nullptr )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
					if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
					{
						serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
					}
				}
				if ( myStats->type == SHADOW )
				{
					//messagePlayer(0, "DEBUG: Shadow lost entity.");
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
				}
				return;
			}
			if ( entity != nullptr )
			{
				if ( entity->behavior == &actPlayer && myStats->type != DUMMYBOT )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
			}
			my->monsterTargetX = entity->x;
			my->monsterTargetY = entity->y;
			hitstats = entity->getStats();

			if ( myStats->type == SHOPKEEPER && strncmp(map.name, "Mages Guild", 11) )
			{
				// shopkeepers hold a grudge against players
				for ( c = 0; c < MAXPLAYERS; ++c )
				{
					if ( players[c] && players[c]->entity )
					{
						if ( my->monsterTarget == players[c]->entity->getUID() )
						{
							ShopkeeperPlayerHostility.updateShopkeeperActMonster(*my, *myStats, ringconflict);
							break;
						}
					}
				}
			}

			if ( myStats->type != DEVIL )
			{
				// skip if light level is too low and distance is too high
				int light = entity->entityLightAfterReductions(*hitstats, my);
				if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
				{
					//See invisible.
					light = 1000;
				}
				else if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
				{
					light += 150;
				}
				double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );

				real_t monsterVisionRange = sightranges[myStats->type];
				if ( hitstats && hitstats->type == DUMMYBOT )
				{
					monsterVisionRange = std::max(monsterVisionRange, 96.0);
				}
				if ( myStats->EFFECTS[EFF_FEAR] )
				{
					targetdist = 0.0; // so we can always see our scary target.
				}

				if ( targetdist > monsterVisionRange )
				{
					// if target has left my sight, decide whether or not to path or retreat (stay put).
					if ( my->shouldRetreat(*myStats) && !myStats->EFFECTS[EFF_FEAR] )
					{
						my->monsterMoveTime = 0;
						my->monsterState = MONSTER_STATE_WAIT; // wait state
					}
					else
					{
						my->monsterState = MONSTER_STATE_PATH; // path state
					}
				}
				else
				{
					if ( targetdist > TOUCHRANGE && targetdist > light && myReflex )
					{
						if ( !(myStats->leader_uid == entity->getUID())
							&& !(hitstats->leader_uid == my->getUID())
							&& !(my->monsterAllyGetPlayerLeader() && entity->behavior == &actPlayer) )
						{
							tangent = atan2( my->monsterTargetY - my->y, my->monsterTargetX - my->x );
							if ( !levitating )
							{
								lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, true);
							}
							else
							{
								lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
							}
							if ( hit.entity == entity )
							{	
								if ( local_rng.rand() % 100 == 0 )
								{
									entity->increaseSkill(PRO_STEALTH);
								}
							}
						}
						// if target is within sight range but light level is too low and out of melee range.
						// decide whether or not to path or retreat (stay put).
						if ( my->shouldRetreat(*myStats) && !myStats->EFFECTS[EFF_FEAR] )
						{
							my->monsterMoveTime = 0;
							my->monsterState = MONSTER_STATE_WAIT; // wait state
						}
						else
						{
							my->monsterState = MONSTER_STATE_PATH; // path state
						}
					}
					else
					{
						if ( myStats->EFFECTS[EFF_FEAR] )
						{
							myReflex = false; // don't determine if you lost sight of the scary monster.
						}

						if ( myReflex )
						{
							tangent = atan2( my->monsterTargetY - my->y, my->monsterTargetX - my->x );

							if ( myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] > 0 && !hasrangedweapon )
							{
								if ( local_rng.rand() % 10 == 0 )
								{
									node_t* node = itemNodeInInventory(myStats, -1, SPELLBOOK);
									if ( node != nullptr )
									{
										bool swapped = swapMonsterWeaponWithInventoryItem(my, myStats, node, true, true);
										if ( swapped )
										{
											my->monsterSpecialState = MONSTER_SPELLCAST_GENERIC;
											int timer = (myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] >> 4) & 0xFFFF;
											my->monsterSpecialTimer = timer > 0 ? timer : 250;
											hasrangedweapon = true;
										}
									}
								}
							}

							if ( !levitating )
							{
								if ( hasrangedweapon )
								{
									dist = lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
									if ( hit.entity == entity )
									{
										Entity* ohitentity = hit.entity;
										real_t odist = dist;

										Entity* hitentity1 = nullptr;
										{
											real_t my1 = my->y + 2.5 * sin(tangent + PI / 2);
											real_t mx1 = my->x + 2.5 * cos(tangent + PI / 2);
											real_t tangent1 = atan2(my->monsterTargetY - my1, my->monsterTargetX - mx1);
											dist = lineTraceTarget(my, mx1, my1, tangent1, monsterVisionRange, 0, false, entity);
											hitentity1 = hit.entity;
										}
										Entity* hitentity2 = nullptr;
										{
											real_t my2 = my->y - 2.5 * sin(tangent + PI / 2);
											real_t mx2 = my->x - 2.5 * cos(tangent + PI / 2);
											real_t tangent2 = atan2(my->monsterTargetY - my2, my->monsterTargetX - mx2);
											dist = lineTraceTarget(my, mx2, my2, tangent2, monsterVisionRange, 0, false, entity);
											hitentity2 = hit.entity;
										}
										if ( hitentity1 && hitentity2 )
										{
											hit.entity = ohitentity;
											dist = odist;
										}
										else
										{
											hit.entity = nullptr;
										}
									}
								}
								else
								{
									dist = lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, true);
								}
							}
							else
							{
								dist = lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
							}
						}
						else
						{
							dist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
						}

						if ( hit.entity != entity && myReflex )
						{
							// if I currently lost sight of my target in a straight line in front of me
							// decide whether or not to path or retreat (stay put).
							if ( my->shouldRetreat(*myStats) && !myStats->EFFECTS[EFF_FEAR] )
							{
								my->monsterMoveTime = 0;
								my->monsterState = MONSTER_STATE_WAIT; // wait state
							}
							else
							{
								my->monsterState = MONSTER_STATE_PATH; // path state
								my->monsterExtraReflexTick = my->getUID() % (TICKS_PER_SECOND / 2) + 10;
							}
						}
						else
						{
							// chaaaarge
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							double tangent2 = tangent;

							// get movement dir
							int goAgain = 0;
timeToGoAgain:
							if ( targetdist > TOUCHRANGE * 1.5 && !hasrangedweapon && !my->shouldRetreat(*myStats) && my->getINT() > -2 )
							{
								if ( MONSTER_FLIPPEDANGLE < 5 )
								{
									if ( (my->ticks + my->getUID()) % (TICKS_PER_SECOND * 4) > TICKS_PER_SECOND * 2 )
									{
										tangent2 += PI / 6;
									}
									else
									{
										tangent2 -= PI / 6;
									}
								}
								else
								{
									if ( (my->ticks + my->getUID()) % (TICKS_PER_SECOND * 4) > TICKS_PER_SECOND * 2 )
									{
										tangent2 += PI / 6;
									}
									else
									{
										tangent2 -= PI / 6;
									}
								}

								Entity* tempHitEntity = hit.entity;
								if ( lineTrace(my, my->x, my->y, tangent2, TOUCHRANGE, 1, false) < TOUCHRANGE )
								{
									MONSTER_FLIPPEDANGLE = (MONSTER_FLIPPEDANGLE < 5) * 10;
									goAgain++;
									if ( goAgain < 2 )
									{
										hit.entity = tempHitEntity;
										goto timeToGoAgain;
									}
									else
									{
										tangent2 = tangent;
									}
								}
								hit.entity = tempHitEntity;
							}
							else
							{
								tangent2 = tangent;
							}

							int myDex = my->monsterGetDexterityForMovement();
							real_t maxVelX = cos(tangent2) * .045 * (myDex + 10) * weightratio;
							real_t maxVelY = sin(tangent2) * .045 * (myDex + 10) * weightratio;
							if ( !myStats->EFFECTS[EFF_KNOCKBACK] )
							{
								MONSTER_VELX = maxVelX;
								MONSTER_VELY = maxVelY;
							}

							int rangedWeaponDistance = 160;
							if ( hasrangedweapon )
							{
								int effectiveDistance = my->getMonsterEffectiveDistanceOfRangedWeapon(myStats->weapon);
								if ( effectiveDistance < rangedWeaponDistance )
								{
									// shorter range xbows etc should advance at a little less than the extremity.
									rangedWeaponDistance = effectiveDistance - 10; 
								}
								if ( myStats->weapon && myStats->weapon->type == SPELLBOOK_DASH )
								{
									rangedWeaponDistance = TOUCHRANGE;
								}
							}

							int chaseRange = 16;
							if ( myStats->type == BUGBEAR )
							{
								chaseRange = 20;
							}

							if ( monsterIsImmobileTurret(my, myStats) || myStats->EFFECTS[EFF_ROOTED] )
							{
								// this is just so that the monster rotates. it doesn't actually move
								MONSTER_VELX = maxVelX * 0.01;
								MONSTER_VELY = maxVelY * 0.01;
							}
							else if ( !myStats->EFFECTS[EFF_KNOCKBACK] && 
								((dist > chaseRange && !hasrangedweapon && !my->shouldRetreat(*myStats))
									|| (hasrangedweapon && dist > rangedWeaponDistance)) )
							{
								if ( my->shouldRetreat(*myStats) )
								{
									MONSTER_VELX *= -.5;
									MONSTER_VELY *= -.5;
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								else
								{
									if ( myStats->type == BUGBEAR && my->monsterStrafeDirection != 0 )
									{
										double strafeTangent = tangent2;
										if ( dist < 24 )
										{
											// move diagonally
											strafeTangent -= ((PI / 4) * my->monsterStrafeDirection);
										}
										else
										{
											// move sideways (dist between 64 and 100 from backupWithRangedWeapon)
											strafeTangent -= ((PI / 2) * my->monsterStrafeDirection);
										}
										if ( ticks % TICKS_PER_SECOND == 0 && my->monsterStrafeDirection != 0 && local_rng.rand() % 10 == 0 )
										{
											my->setBugbearStrafeDir(true);
											//my->monsterStrafeDirection *= -1;
										}
										real_t maxVelX = cos(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
										real_t maxVelY = sin(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
										MONSTER_VELX = maxVelX;
										MONSTER_VELY = maxVelY;
									}

									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								if ( hit.entity != NULL )
								{
									if ( hit.entity->behavior == &actDoor )
									{
										// opens the door if unlocked and monster can do it
										if ( !hit.entity->doorLocked && my->getINT() > -2 )
										{
											if ( !hit.entity->doorDir && !hit.entity->doorStatus )
											{
												hit.entity->doorStatus = 1 + (my->x > hit.entity->x);
												playSoundEntity(hit.entity, 21, 96);
											}
											else if ( hit.entity->doorDir && !hit.entity->doorStatus )
											{
												hit.entity->doorStatus = 1 + (my->y < hit.entity->y);
												playSoundEntity(hit.entity, 21, 96);
											}
										}
										else
										{
											// can't open door, so break it down
											my->monsterHitTime++;
											if ( my->monsterHitTime >= HITRATE )
											{
												my->monsterAttack = my->getAttackPose(); // random attack motion
												my->monsterHitTime = 0;
												my->monsterAttackTime = 0;
												hit.entity->doorHealth--; // decrease door health
												if ( myStats->STR > 20 )
												{
													hit.entity->doorHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
													hit.entity->doorHealth = std::max(hit.entity->doorHealth, 0);
												}
												if ( myStats->type == MINOTAUR )
												{
													hit.entity->doorHealth = 0;    // minotaurs smash doors instantly
												}
												updateEnemyBar(my, hit.entity, Language::get(674), hit.entity->skill[4], hit.entity->skill[9],
													false, DamageGib::DMG_DEFAULT);
												playSoundEntity(hit.entity, 28, 64);
												if ( hit.entity->doorHealth <= 0 )
												{
													// set direction of splinters
													if ( !hit.entity->doorDir )
													{
														hit.entity->doorSmacked = (my->x > hit.entity->x);
													}
													else
													{
														hit.entity->doorSmacked = (my->y < hit.entity->y);
													}
												}
											}
										}
									}
									else if ( hit.entity->behavior == &actFurniture && myStats->type == MINOTAUR )
									{
										hit.entity->furnitureHealth = 0;
										playSoundEntity(hit.entity, 28, 64);
									}
									else if ( hit.entity->behavior == &actChest && myStats->type == MINOTAUR )
									{
										hit.entity->skill[3] = 0; // chestHealth
										playSoundEntity(hit.entity, 28, 64);
									}
									else if ( hit.entity->isDamageableCollider() && myStats->type == MINOTAUR )
									{
										hit.entity->colliderCurrentHP = 0;
										hit.entity->colliderKillerUid = 0;
										playSoundEntity(hit.entity, 28, 64);
									}
									else if ( hit.entity->behavior == &actFurniture )
									{
										// break it down!
										my->monsterHitTime++;
										if ( my->monsterHitTime >= HITRATE )
										{
											my->monsterAttack = my->getAttackPose(); // random attack motion
											my->monsterHitTime = HITRATE / 4;
											my->monsterAttackTime = 0;
											hit.entity->furnitureHealth--; // decrease door health
											if ( myStats->STR > 20 )
											{
												hit.entity->furnitureHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
												hit.entity->furnitureHealth = std::max(hit.entity->furnitureHealth, 0);
											}
											playSoundEntity(hit.entity, 28, 64);
										}
									}
									else if ( hit.entity->isDamageableCollider() && myStats->type != GYROBOT && myStats->type != BAT_SMALL )
									{
										// break it down!
										my->monsterHitTime++;
										if ( my->monsterHitTime >= HITRATE )
										{
											if ( !hasrangedweapon )
											{
												my->monsterAttack = my->getAttackPose(); // random attack motion
											}
											my->monsterHitTime = HITRATE / 4;
											my->monsterAttackTime = 0;
											int damage = 2 + local_rng.rand() % 3;
											damage += std::max(0, myStats->STR / 8);

											hit.entity->colliderCurrentHP -= damage;
											hit.entity->colliderKillerUid = 0;

											int sound = 28;
											if ( hit.entity->getColliderSfxOnHit() > 0 )
											{
												sound = hit.entity->getColliderSfxOnHit();
											}
											playSoundEntity(hit.entity, sound, 64);
										}
									}
									else if ( (hit.entity->behavior == &actBoulder || hit.entity->behavior == &::actDaedalusShrine) && !hit.entity->flags[PASSABLE] && myStats->type == MINOTAUR )
									{
										// asplode the rock
										magicDig(nullptr, nullptr, 0, 1);
										hit.entity = nullptr;
									}
									else
									{
										if ( my->shouldRetreat(*myStats) && !myStats->EFFECTS[EFF_FEAR] )
										{
											my->monsterMoveTime = 0;
											my->monsterState = MONSTER_STATE_WAIT; // wait state
										}
										else
										{
											my->monsterState = MONSTER_STATE_PATH; // path state
										}
									}
								}
								else
								{
									if ( my->shouldRetreat(*myStats) && !myStats->EFFECTS[EFF_FEAR] )
									{
										my->monsterMoveTime = 0;
										my->monsterState = MONSTER_STATE_WAIT; // wait state
									}
									else if ( dist2 <= 0.1 && myStats->HP > myStats->MAXHP / 3 )
									{
										my->monsterState = MONSTER_STATE_PATH; // path state
									}
								}
							}
							else
							{
								if ( my->backupWithRangedWeapon(*myStats, dist, hasrangedweapon) || my->shouldRetreat(*myStats) )
								{
									// injured monsters or monsters with ranged weapons back up
									if ( myStats->type == LICH_ICE )
									{
										double strafeTangent = tangent2;
										//messagePlayer(0, "strafe: %d", my->monsterStrafeDirection);
										if ( ticks % 10 == 0 && my->monsterStrafeDirection != 0 && local_rng.rand() % 10 == 0 )
										{
											Entity* lichAlly = nullptr;
											if ( my->monsterLichAllyUID != 0 )
											{
												lichAlly = uidToEntity(my->monsterLichAllyUID);
											}
											Entity* tmpEntity = hit.entity;
											lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
											if ( hit.entity != tmpEntity )
											{
												// sight is blocked, keep strafing.
											}
											else
											{
												my->monsterStrafeDirection = 0;
											}
											hit.entity = tmpEntity;
										}
										if ( dist < 64 )
										{
											// move diagonally
											strafeTangent -= ((PI / 4) * my->monsterStrafeDirection);
										}
										else
										{
											// move sideways (dist between 64 and 100 from backupWithRangedWeapon)
											strafeTangent -= ((PI / 2) * my->monsterStrafeDirection);
										}
										MONSTER_VELX = cos(strafeTangent) * .045 * (my->getDEX() + 10) * weightratio * -.5;
										MONSTER_VELY = sin(strafeTangent) * .045 * (my->getDEX() + 10) * weightratio * -.5;
									}
									else
									{
										int myDex = my->monsterGetDexterityForMovement();
										real_t maxVelX = cos(tangent2) * .045 * (myDex + 10) * weightratio * -.5;
										real_t maxVelY = sin(tangent2) * .045 * (myDex + 10) * weightratio * -.5;
										if ( myStats->EFFECTS[EFF_KNOCKBACK] )
										{
											my->monsterHandleKnockbackVelocity(tangent2, weightratio);
										}
										else
										{
											MONSTER_VELX = maxVelX;
											MONSTER_VELY = maxVelY;

											if ( myStats->type == BUGBEAR && my->monsterStrafeDirection != 0 )
											{
												double strafeTangent = tangent2;
												if ( dist < 24 )
												{
													// move diagonally
													strafeTangent -= ((PI / 4) * my->monsterStrafeDirection);
												}
												else
												{
													// move sideways (dist between 64 and 100 from backupWithRangedWeapon)
													strafeTangent -= ((PI / 2) * my->monsterStrafeDirection);
												}
												if ( ticks % TICKS_PER_SECOND == 0 && my->monsterStrafeDirection != 0 && local_rng.rand() % 10 == 0 )
												{
													my->setBugbearStrafeDir(true);
													//my->monsterStrafeDirection *= -1;
												}
												real_t maxVelX = cos(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
												real_t maxVelY = sin(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
												MONSTER_VELX = maxVelX;
												MONSTER_VELY = maxVelY;
											}

											if ( my->backupWithRangedWeapon(*myStats, dist, hasrangedweapon)
												&& wasInsideEntity )
											{
												MONSTER_VELX *= .01;
												MONSTER_VELY *= .01;
											}
										}
									}
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
									my->handleKnockbackDamage(*myStats, hit.entity);
								}
								else
								{
									if ( myStats->type == BUGBEAR && my->monsterStrafeDirection != 0 )
									{
										double strafeTangent = tangent2;
										if ( dist < 24 )
										{
											// move diagonally
											strafeTangent -= ((PI / 4) * my->monsterStrafeDirection);
										}
										else
										{
											// move sideways (dist between 64 and 100 from backupWithRangedWeapon)
											strafeTangent -= ((PI / 2) * my->monsterStrafeDirection);
										}
										if ( ticks % TICKS_PER_SECOND == 0 && my->monsterStrafeDirection != 0 && local_rng.rand() % 10 == 0 )
										{
											my->setBugbearStrafeDir(true);
											//my->monsterStrafeDirection *= -1;
										}
										real_t maxVelX = cos(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
										real_t maxVelY = sin(strafeTangent) * .045 * (myDex + 10) * weightratio * -.5;
										MONSTER_VELX = maxVelX;
										MONSTER_VELY = maxVelY;

										dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
									}
									else
									{
										// this is just so that the monster rotates. it doesn't actually move
										int myDex = my->monsterGetDexterityForMovement();
										MONSTER_VELX = cos(tangent) * .02 * .045 * (myDex + 10) * weightratio;
										MONSTER_VELY = sin(tangent) * .02 * .045 * (myDex + 10) * weightratio;
									}
								}
							}

							int previousStrafeDirection = my->monsterStrafeDirection;
							bool previousBackupWithRangedWeapon = my->backupWithRangedWeapon(*myStats, dist, hasrangedweapon);
							bool previousShouldRetreat = my->shouldRetreat(*myStats);

							my->handleMonsterAttack(myStats, entity, dist);

							// bust ceilings
							/*if( myStats->type == MINOTAUR ) {
								if( my->x>=0 && my->y>=0 && my->x<map.width<<4 && my->y<map.height<<4 ) {
									if( map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] )
										map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] = 0;
								}
							}*/

							hasrangedweapon = my->hasRangedWeapon(); // re-update this status check if weapon was consumed/stowed away

							// rotate monster
							if ( myStats->type == BUGBEAR && previousStrafeDirection != 0 )
							{
								real_t tempVelX = cos(tangent2) * .045 * (myDex + 10) * weightratio;
								real_t tempVelY = sin(tangent2) * .045 * (myDex + 10) * weightratio;
								// override if we're strafing, keep facing the target
								dir = my->yaw - atan2(tempVelY, tempVelX);
							}
							else if ( previousBackupWithRangedWeapon || previousShouldRetreat )
							{
								int myDex = my->getDEX();
								if ( my->monsterAllyGetPlayerLeader() )
								{
									myDex = std::min(myDex, MONSTER_ALLY_DEXTERITY_SPEED_CAP);
								}
								real_t tempVelX = cos(tangent2) * .045 * (myDex + 10) * weightratio * -.5;
								real_t tempVelY = sin(tangent2) * .045 * (myDex + 10) * weightratio * -.5;
								if ( myStats->type == LICH_ICE )
								{
									// override if we're strafing, keep facing the target
									dir = my->yaw - atan2(-tempVelY, -tempVelX);
								}
								else if ( myStats->EFFECTS[EFF_KNOCKBACK] )
								{
									// in knockback, the velocitys change sign from negative/positive or positive/negative.
									// this makes monsters moonwalk if the direction to rotate is assumed the same.
									// so we compare goal velocity direction (sin or cos(tangent)) and see if we've reached that sign.

									int multX = 1;
									int multY = 1;
									if ( cos(tangent2) >= 0 == MONSTER_VELX > 0 )
									{
										// same sign.
										multX = 1;
									}
									else
									{
										// opposite signed.
										multX = -1;
									}
									if ( sin(tangent2) >= 0 == MONSTER_VELY > 0 )
									{
										// same sign.
										multY = 1;
									}
									else
									{
										// opposite signed.
										multY = -1;
									}
									dir = my->yaw - atan2(multY * MONSTER_VELY, multX * MONSTER_VELX);
								}
								else
								{
									dir = my->yaw - atan2( -MONSTER_VELY, -MONSTER_VELX );
								}
							}
							else
							{
								dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );
							}
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}
							if ( myStats->type == SLIME )
							{
								if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2 )
								{
									my->yaw -= dir / 8;
								}
								else
								{
									my->yaw -= dir / 2;
								}
							}
							else
							{
								my->yaw -= dir / 2;
								//messagePlayer(0, MESSAGE_DEBUG, "yaw: %.2f dir: %.2f, velx: %.2f vely: %.2f", my->yaw, dir / 2, my->vel_x, my->vel_y);
							}
							while ( my->yaw < 0 )
							{
								my->yaw += 2 * PI;
							}
							while ( my->yaw >= 2 * PI )
							{
								my->yaw -= 2 * PI;
							}
						}
					}
				}
			}
			else
			{
				// devil specific code
				if ( !MONSTER_ATTACK || MONSTER_ATTACK == 4 )
				{
					my->monsterSpecialTimer++;
					int difficulty = 40;
					int numPlayers = 0;

					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity )
						{
							++numPlayers;
						}
					}
					if ( numPlayers > 0 )
					{
						difficulty /= numPlayers; // 40/20/13/10 - basically how long you get to wail on Baphy. Shorter is harder.
						difficulty = std::max(10, difficulty);
					}

					if ( my->monsterSpecialTimer > 60 || (devilstate == 72 && my->monsterSpecialTimer > difficulty))
					{
						if ( !devilstate ) // devilstate is 0 at the start of the fight and doesn't return to 0.
						{
							if ( !MONSTER_ATTACK )
							{
								int c;
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									playSoundPlayer(c, 204, 64);
								}
								playSoundEntity(my, 204, 128);
								MONSTER_ATTACK = 4;
								MONSTER_ATTACKTIME = 0;
								MONSTER_ARMBENDED = 1;
								serverUpdateEntitySkill(my, 8);
								serverUpdateEntitySkill(my, 9);
								serverUpdateEntitySkill(my, 10);
								for ( int c = 0; c < MAXPLAYERS; ++c )
								{
									if ( players[c] && players[c]->entity )
									{
										my->devilSummonMonster(nullptr, SHADOW, 5, c);
									}
								}
								my->devilSummonMonster(nullptr, DEMON, 5);
							}
							else if ( MONSTER_ATTACKTIME > 90 )
							{
								my->monsterState = MONSTER_STATE_DEVIL_TELEPORT; // devil teleport state
							}
						}
						else
						{
							if ( !devilacted )
							{
								switch ( devilstate )
								{
									case 72:
										my->monsterState = MONSTER_STATE_DEVIL_SUMMON; // devil summoning state
										break;
									case 73:
										MONSTER_ATTACK = 5 + local_rng.rand() % 2; // fireballs
										break;
									case 74:
										my->monsterState = MONSTER_STATE_DEVIL_BOULDER; // devil boulder drop
										break;
								}
								devilacted = 1;
							}
							else
							{
								if ( local_rng.rand() % 2 && devilstate == 73 )
								{
									MONSTER_ATTACK = 5 + local_rng.rand() % 2; // more fireballs
								}
								else
								{
									my->monsterState = MONSTER_STATE_DEVIL_TELEPORT; // devil teleport state
								}
							}
						}
						my->monsterSpecialTimer = 0;
					}
				}
				else if ( MONSTER_ATTACK == 5 || MONSTER_ATTACK == 6 )
				{
					// throw fireballs
					my->yaw = my->yaw + MONSTER_WEAPONYAW;
					castSpell(my->getUID(), &spell_fireball, true, false);
					my->yaw = my->yaw - MONSTER_WEAPONYAW;

					// let's throw one specifically aimed at our players to be mean.
					if ( MONSTER_ATTACKTIME == 10 )
					{
						real_t oldYaw = my->yaw;
						tangent = atan2(entity->y - my->y, entity->x - my->x);
						my->yaw = tangent;
						(void)castSpell(my->getUID(), &spell_fireball, true, false);
						for ( c = 0; c < MAXPLAYERS; ++c )
						{
							if ( players[c] && players[c]->entity && entity != players[c]->entity )
							{
								tangent = atan2(entity->y - my->y, entity->x - my->x);
								real_t dir = oldYaw - tangent;
								while ( dir >= PI )
								{
									dir -= PI * 2;
								}
								while ( dir < -PI )
								{
									dir += PI * 2;
								}
								if ( dir >= -7 * PI / 16 && dir <= 7 * PI / 16 )
								{
									my->yaw = tangent;
                                    (void)castSpell(my->getUID(), &spell_fireball, true, false);
								}
							}
						}
						my->yaw = oldYaw;
					}
				}

				// rotate monster
				tangent = atan2( entity->y - my->y, entity->x - my->x );
				MONSTER_VELX = cos(tangent);
				MONSTER_VELY = sin(tangent);
				dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );
				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}
			}
		} //End charge state
		else if ( my->monsterState == MONSTER_STATE_PATH )     //Begin path state
		{
			if ( myStats->type == DEVIL )
			{
				my->monsterState = MONSTER_STATE_ATTACK;
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				return;
			}
			else if ( myStats->type == DUMMYBOT )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				my->monsterMoveTime = 0;
				return;
			}
			else if ( monsterIsImmobileTurret(my, myStats) )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				return;
			}

			entity = uidToEntity(my->monsterTarget);

			//Don't path if your target dieded!
			if ( entity == nullptr && my->monsterTarget != 0 )
			{
				my->monsterReleaseAttackTarget(true);
				my->monsterState = MONSTER_STATE_WAIT; // wait state
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				return;
			}

			if ( entity != nullptr )
			{
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
				my->monsterTargetX = entity->x;
				my->monsterTargetY = entity->y;
			}
			x = ((int)floor(my->monsterTargetX)) >> 4;
			y = ((int)floor(my->monsterTargetY)) >> 4;
			path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, uidToEntity(my->monsterTarget),
				GeneratePathTypes::GENERATE_PATH_TO_HUNT_MONSTER_TARGET);
			if ( my->children.first != nullptr )
			{
				list_RemoveNode(my->children.first);
			}
			node = list_AddNodeFirst(&my->children);
			node->element = path;
			node->deconstructor = &listDeconstructor;
			my->monsterState = MONSTER_STATE_HUNT; // hunt state
			/*if ( myStats->type == SHADOW && entity )
			{
				if ( path == nullptr )
				{
					messagePlayer(0, "Warning: Shadow failed to generate a path to its target.");
				}
			}*/
		} //End path state.
		else if ( my->monsterState == MONSTER_STATE_HUNT ) //Begin hunt state
		{
			if ( myStats->type == SHADOW && my->monsterSpecialState == SHADOW_TELEPORT_ONLY )
			{
				//messagePlayer(0, "Shadow in special state teleport only! Aborting hunt state.");
				my->monsterState = MONSTER_STATE_WAIT;
				return; //Don't do anything, yer casting a spell!
			}
			//Do the shadow's passive teleport to catch up to their target..
			if ( myStats->type == SHADOW && my->monsterSpecialTimer == 0 && my->monsterTarget )
			{
				Entity* target = uidToEntity(my->monsterTarget);
				if ( !target )
				{
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
					return;
				}

				//If shadow has no path to target, then should do the passive teleport.
				bool passiveTeleport = false;
				if ( my->children.first ) //First child is the path.
				{
					if ( !my->children.first->element )
					{
						//messagePlayer(0, "No path for shadow!");
						passiveTeleport = true;
					}
				}
				else
				{
					//messagePlayer(0, "No path for shadow!");
					passiveTeleport = true; //Path is saved as first child. If no first child, no path!
				}

				//Shadow has path to target, but still passive teleport if far enough away.
				if ( !passiveTeleport )
				{
					int specialRoll = local_rng.rand() % 50;
					//messagePlayer(0, "roll %d", specialRoll);
					double targetdist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
					if ( specialRoll <= (2 + (targetdist > 80 ? 4 : 0)) )
					{
						passiveTeleport = true;
					}
				}

				if ( passiveTeleport )
				{
					//messagePlayer(0, "Shadow is doing a passive tele.");
					my->monsterSpecialState = SHADOW_TELEPORT_ONLY;
					my->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_PASIVE_TELEPORT;
					my->shadowTeleportToTarget(target, 3); // teleport in closer range
					my->monsterState = MONSTER_STATE_WAIT;
					if ( target && target->behavior == actPlayer )
					{
						messagePlayer(target->skill[2], MESSAGE_HINT, Language::get(2518));
					}
					return;
				}
			}

			if ( myReflex && (myStats->type != LICH || my->monsterSpecialTimer <= 0) )
			{
				for ( node2 = map.creatures->first; node2 != nullptr; node2 = node2->next ) //Stats only exist on a creature, so don't iterate all map.entities.
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] || entity->isInertMimic() )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							// skip if light level is too low and distance is too high
							int light = entity->entityLightAfterReductions(*hitstats, my);
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
							{
								//See invisible.
								light = 1000;
							}
							double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );

							real_t monsterVisionRange = sightranges[myStats->type];
							if ( hitstats->type == DUMMYBOT )
							{
								monsterVisionRange = std::max(monsterVisionRange, 96.0);
							}

							if ( targetdist > monsterVisionRange )
							{
								continue;
							}
							if ( targetdist > TOUCHRANGE && targetdist > light )
							{
								if ( !(myStats->leader_uid == entity->getUID())
									&& !(hitstats->leader_uid == my->getUID())
									&& !(my->monsterAllyGetPlayerLeader() && entity->behavior == &actPlayer) )
								{
									if ( !levitating )
									{
										lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, true);
									}
									else
									{
										lineTrace(my, my->x, my->y, tangent, monsterVisionRange, 0, false);
									}
									if ( hit.entity == entity )
									{
										if ( local_rng.rand() % 100 == 0 )
										{
											entity->increaseSkill(PRO_STEALTH);
										}
									}
								}
								continue;
							}
							bool visiontest = false;
							if ( hitstats->type == DUMMYBOT || myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}
							else if ( myStats->type != SPIDER )
							{
								if ( my->monsterAllyGetPlayerLeader() )
								{
									if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
									{
										visiontest = true; // increase ally vision when hunting.
									}
								}
								else
								{
									if ( dir >= -7 * PI / 16 && dir <= 7 * PI / 16 )
									{
										visiontest = true;
									}
								}
							}
							else
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}
							if ( visiontest )   // vision cone
							{
								lineTrace(my, my->x + 1, my->y, tangent, monsterVisionRange, 0, (levitating == false));
								if ( hit.entity == entity )
								{
									lineTrace(my, my->x - 1, my->y, tangent, monsterVisionRange, 0, (levitating == false));
									if ( hit.entity == entity )
									{
										lineTrace(my, my->x, my->y + 1, tangent, monsterVisionRange, 0, (levitating == false));
										if ( hit.entity == entity )
										{
											lineTrace(my, my->x, my->y - 1, tangent, monsterVisionRange, 0, (levitating == false));
											if ( hit.entity == entity )
											{
												Entity& attackTarget = *hit.entity;
												// charge state
												if ( my->monsterTarget == entity->getUID() )
												{
													// this is when a monster is chasing it's known target.
													// let's to be ready to strike.
													// otherwise, we bumped into a new unexpected target, don't modify hitTime
													if ( hasrangedweapon )
													{
														// 120 ms reaction time
														if ( my->monsterHitTime < HITRATE )
														{
															if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
															{
																my->monsterHitTime = std::max(HITRATE, my->monsterHitTime);
															}
															else
															{
																my->monsterHitTime = std::max(HITRATE - 6, my->monsterHitTime);
															}
														}
														else
														{
															// bows have 2x hitrate time compared to standard weapons.
															my->monsterHitTime = std::max(2 * HITRATE - 6, my->monsterHitTime);
														}
													}
													else
													{
														// melee 240ms
														my->monsterHitTime = std::max(HITRATE - 12, my->monsterHitTime);
													}
												}
												//messagePlayer(0, "hunt -> attack, %d", my->monsterHitTime);
												my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);

												if ( MONSTER_SOUND == NULL )
												{
													if ( myStats->type != MINOTAUR )
													{
														if ( myStats->type != LICH || local_rng.rand() % 3 == 0 )
														{
															if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
															{
																sentrybotPickSpotNoise(my, myStats);
															}
															else
															{
																MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128);
															}
														}
													}
													else
													{
														int c;
														for ( c = 0; c < MAXPLAYERS; c++ )
														{
															if ( c == 0 )
															{
																MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
															}
															else
															{
																playSoundPlayer( c, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128 );
															}
														}
													}
												}

												if ( entity != nullptr )
												{
													if ( entity->behavior == &actPlayer && myStats->type != DUMMYBOT )
													{
														assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
														assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
													}
												}
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			// minotaurs and liches chase players relentlessly.
			if ( myStats->type == MINOTAUR 
				|| (myStats->type == LICH && my->monsterSpecialTimer <= 0)
				|| ((myStats->type == LICH_FIRE || myStats->type == LICH_ICE) && my->monsterSpecialTimer <= 0 )
				|| (myStats->type == CREATURE_IMP && strstr(map.name, "Boss") && !my->monsterAllyGetPlayerLeader())
				|| (myStats->type == AUTOMATON && strstr(myStats->name, "corrupted automaton")) )
			{
				bool shouldHuntPlayer = false;
				Entity* playerOrNot = uidToEntity(my->monsterTarget);
				if (playerOrNot)
				{
					if (ticks % 180 == 0 && playerOrNot->behavior == &actPlayer)
					{
						shouldHuntPlayer = true;
					}
				}
				else if (ticks % 180 == 0)
				{
					shouldHuntPlayer = true;
				}
				if (shouldHuntPlayer)
				{
					double distToPlayer = 0;
					int c, playerToChase = -1;
					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity)
						{
							list_t* playerPath = generatePath((int)floor(my->x / 16), (int)floor(my->y / 16),
								(int)floor(players[c]->entity->x / 16), (int)floor(players[c]->entity->y / 16), my, players[c]->entity,
								GeneratePathTypes::GENERATE_PATH_BOSS_TRACKING_HUNT);
							if ( playerPath == NULL )
							{
								continue;
							}
							else
							{
								list_FreeAll(playerPath);
								free(playerPath);
							}
							if (!distToPlayer)
							{
								distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								playerToChase = c;
							}
							else
							{
								double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								if (newDistToPlayer < distToPlayer)
								{
									distToPlayer = newDistToPlayer;
									playerToChase = c;
								}
							}
						}
					}
					if (playerToChase >= 0)
					{
						// path state
						if ( players[playerToChase] && players[playerToChase]->entity )
						{
							my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
						}
						if ( previousMonsterState != my->monsterState )
						{
							serverUpdateEntitySkill(my, 0);
						}
						return;
					}
				}
			}
			else if ( myStats->type == SHADOW && my->monsterTarget && (ticks % 180 == 0) )
			{
				if ( !uidToEntity(my->monsterTarget) )
				{
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
					if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
					{
						serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
					}
					return;
				}
				my->monsterState = MONSTER_STATE_PATH;
				serverUpdateEntitySkill(my, 0); //Update state.
				if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
				{
					serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
				}
				return;
			}

			// lich cooldown
			if ( myStats->type == LICH )
			{
				if ( my->monsterSpecialTimer > 0 )
				{
					my->monsterSpecialTimer--;
				}
			}

			// follow the leader :)
			if ( uidToEntity(my->monsterTarget) == nullptr
				&& myStats->leader_uid != 0 
				&& my->monsterAllyState == ALLY_STATE_DEFAULT 
				&& !monsterIsImmobileTurret(my, myStats)
				&& !myStats->EFFECTS[EFF_ROOTED]
				&& my->getUID() % TICKS_PER_SECOND == ticks % monsterAllyFormations.getFollowerChaseLeaderInterval(*my, *myStats)
				 )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					real_t followx = leader->x;
					real_t followy = leader->y;
					if ( myStats->type == GYROBOT )
					{
						// follow ahead of the leader.
						real_t startx = leader->x;
						real_t starty = leader->y;
						// draw line from the leaders direction until we hit a wall or 48 dist
						real_t previousx = startx;
						real_t previousy = starty;
						std::unordered_set<int> checkedTiles;
						for ( int iterations = 0; iterations < 20; ++iterations )
						{
							startx += 4 * cos(leader->yaw);
							starty += 4 * sin(leader->yaw);

							int mapx = (static_cast<int>(startx) >> 4);
							int mapy = (static_cast<int>(starty) >> 4);
							int index = (mapy) * MAPLAYERS + (mapx) * MAPLAYERS * map.height;
							if ( !map.tiles[OBSTACLELAYER + index] )
							{
								bool foundObstacle = false;
								if ( checkedTiles.find(mapx + mapy * 10000) == checkedTiles.end() )
								{
									if ( checkObstacle((mapx << 4) + 8, (mapy << 4) + 8, my, leader) )
									{
										foundObstacle = true;
									}
									checkedTiles.insert(mapx + mapy * 10000);
								}
								if ( foundObstacle )
								{
									break;
								}

								// store the last known good coordinate
								previousx = startx;
								previousy = starty;
							}
							else if ( map.tiles[OBSTACLELAYER + index] )
							{
								break;
							}
							if ( sqrt(pow(leader->x - previousx, 2) + pow(leader->y - previousy, 2)) > HUNT_FOLLOWDIST )
							{
								break;
							}
						}
						followx = previousx;
						followy = previousy;
						//createParticleFollowerCommand(previousx, previousy, 0, 175); debug particle
					}
					double dist = sqrt(pow(my->x - followx, 2) + pow(my->y - followy, 2));
					if ( dist > HUNT_FOLLOWDIST  )
					{
						bool doFollow = true;
						if ( my->monsterTarget != 0 )
						{
							doFollow = my->isFollowerFreeToPathToPlayer(myStats);
						}

						if ( doFollow )
						{
							if ( myStats->type == GYROBOT )
							{
								my->monsterSetPathToLocation(static_cast<int>(followx) / 16, static_cast<int>(followy) / 16, 0,
									GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW2);
							}
							else
							{
								std::pair<int, int> followPos;
								if ( monsterAllyFormations.getFollowLocation(my->getUID(), leader->getUID(), followPos) )
								{
									my->monsterSetPathToLocation(followPos.first, followPos.second, 2,
										GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW2);
								}
							}
							my->monsterState = MONSTER_STATE_HUNT; // hunt state
							if ( previousMonsterState != my->monsterState )
							{
								serverUpdateEntitySkill(my, 0);
							}
							return;
						}
					}
					else if ( myStats->type != GYROBOT )
					{
						bool doFollow = true;
						if ( my->monsterTarget != 0 )
						{
							doFollow = my->isFollowerFreeToPathToPlayer(myStats);
						}

						if ( doFollow )
						{
							double tangent = atan2( leader->y - my->y, leader->x - my->x );
							Entity* ohitentity = hit.entity;
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
							if ( hit.entity != leader )
							{
								my->monsterReleaseAttackTarget();
								std::pair<int, int> followPos;
								if ( monsterAllyFormations.getFollowLocation(my->getUID(), leader->getUID(), followPos) )
								{
									if ( my->monsterSetPathToLocation(followPos.first, followPos.second, 1,
										GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW) )
									{
										my->monsterState = MONSTER_STATE_HUNT; // hunt state
									}
									if ( previousMonsterState != my->monsterState )
									{
										serverUpdateEntitySkill(my, 0);
										if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
										{
											serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
										}
									}
								}
								return;
							}
							hit.entity = ohitentity;
						}
					}
				}
			}

			entity = uidToEntity(my->monsterTarget);
			if ( entity != NULL )
			{
				if ( entity->behavior == &actPlayer && myStats->type != DUMMYBOT )
				{
					assailant[entity->skill[2]] = true; // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
			}
			if ( my->children.first != NULL )
			{
				if ( my->children.first->element != NULL )
				{
					path = (list_t*)my->children.first->element;
					if ( path->first != NULL && !myStats->EFFECTS[EFF_ROOTED] )
					{
						auto pathnode = (pathnode_t*)path->first->element;
						dist = sqrt( pow(pathnode->y * 16 + 8 - my->y, 2) + pow(pathnode->x * 16 + 8 - my->x, 2) );
						if ( dist <= 2 )
						{
							list_RemoveNode(path->first);
							if ( local_rng.rand() % 8 == 0 )
							{
								if ( !MONSTER_SOUND )
								{
									if ( myStats->type != MINOTAUR )
									{
										if ( !my->monsterAllyGetPlayerLeader() || (my->monsterAllyGetPlayerLeader() && local_rng.rand() % 3 == 0) )
										{
											MONSTER_SOUND = playSoundEntity(my, MONSTER_IDLESND + (local_rng.rand() % MONSTER_IDLEVAR), 128);
										}
									}
									else
									{
										int c;
										for ( c = 0; c < MAXPLAYERS; c++ )
										{
											if ( c == 0 )
											{
												MONSTER_SOUND = playSoundPlayer( c, MONSTER_IDLESND + (local_rng.rand() % MONSTER_IDLEVAR), 128 );
											}
											else
											{
												playSoundPlayer( c, MONSTER_IDLESND + (local_rng.rand() % MONSTER_IDLEVAR), 128 );
											}
										}
									}
								}
							}
						}
						else
						{
							// move monster
							tangent = atan2( pathnode->y * 16 + 8 - my->y, pathnode->x * 16 + 8 - my->x );
							int myDex = my->getDEX();
							if ( my->monsterAllyGetPlayerLeader() )
							{
								myDex = std::min(myDex, MONSTER_ALLY_DEXTERITY_SPEED_CAP);
							}
							real_t maxVelX = cos(tangent) * .045 * (myDex + 10) * weightratio;
							real_t maxVelY = sin(tangent) * .045 * (myDex + 10) * weightratio;
							if ( myStats->EFFECTS[EFF_KNOCKBACK] )
							{
								my->monsterHandleKnockbackVelocity(tangent, weightratio);
							}
							else
							{
								MONSTER_VELX = maxVelX;
								MONSTER_VELY = maxVelY;
							}
							dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
							my->handleKnockbackDamage(*myStats, hit.entity);
							if ( hit.entity != NULL )
							{
								if ( hit.entity->behavior == &actDoor )
								{
									// opens the door if unlocked and monster can do it
									if ( !hit.entity->doorLocked && my->getINT() > -2 )
									{
										if ( !hit.entity->doorDir && !hit.entity->doorStatus )
										{
											hit.entity->doorStatus = 1 + (my->x > hit.entity->x);
											playSoundEntity(hit.entity, 21, 96);
										}
										else if ( hit.entity->doorDir && !hit.entity->doorStatus )
										{
											hit.entity->doorStatus = 1 + (my->y < hit.entity->y);
											playSoundEntity(hit.entity, 21, 96);
										}
									}
									else
									{
										// can't open door, so break it down
										my->monsterHitTime++;
										if ( my->monsterHitTime >= HITRATE )
										{
											my->monsterAttack = my->getAttackPose(); // random attack motion
											my->monsterAttackTime = 0;
											my->monsterHitTime = 0;
											hit.entity->doorHealth--; // decrease door health
											if ( myStats->STR > 20 )
											{
												hit.entity->doorHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
												hit.entity->doorHealth = std::max(hit.entity->doorHealth, 0);
											}
											if ( myStats->type == MINOTAUR )
											{
												hit.entity->doorHealth = 0;    // minotaurs smash doors instantly
											}
											updateEnemyBar(my, hit.entity, Language::get(674), hit.entity->skill[4], hit.entity->skill[9],
												false, DamageGib::DMG_DEFAULT);
											playSoundEntity(hit.entity, 28, 64);
											if ( hit.entity->doorHealth <= 0 )
											{
												// set direction of splinters
												if ( !hit.entity->doorDir )
												{
													hit.entity->doorSmacked = (my->x > hit.entity->x);
												}
												else
												{
													hit.entity->doorSmacked = (my->y < hit.entity->y);
												}
											}
										}
									}
								}
								else if ( hit.entity->behavior == &actFurniture )
								{
									// break it down!
									my->monsterHitTime++;
									if ( my->monsterHitTime >= HITRATE )
									{
										my->monsterAttack = my->getAttackPose(); // random attack motion
										my->monsterAttackTime = 0;
										my->monsterHitTime = HITRATE / 4;
										hit.entity->furnitureHealth--; // decrease door health
										if ( myStats->STR > 20 )
										{
											hit.entity->furnitureHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
											hit.entity->furnitureHealth = std::max(hit.entity->furnitureHealth, 0);
										}
										if ( myStats->type == MINOTAUR )
										{
											hit.entity->furnitureHealth = 0;    // minotaurs smash furniture instantly
										}
										playSoundEntity(hit.entity, 28, 64);
									}
								}
								else if ( hit.entity->isDamageableCollider() && myStats->type != GYROBOT && myStats->type != BAT_SMALL )
								{
									// break it down!
									my->monsterHitTime++;
									if ( my->monsterHitTime >= HITRATE )
									{
										if ( !hasrangedweapon )
										{
											my->monsterAttack = my->getAttackPose(); // random attack motion
										}
										my->monsterHitTime = HITRATE / 4;
										my->monsterAttackTime = 0;
										int damage = 2 + local_rng.rand() % 3;
										damage += std::max(0, myStats->STR / 8);

										hit.entity->colliderCurrentHP -= damage;
										hit.entity->colliderKillerUid = 0;

										int sound = 28;
										if ( hit.entity->getColliderSfxOnHit() > 0 )
										{
											sound = hit.entity->getColliderSfxOnHit();
										}
										playSoundEntity(hit.entity, sound, 64);
									}
								}
								else if ( hit.entity->behavior == &actChest && myStats->type == MINOTAUR )
								{
									hit.entity->skill[3] = 0; // chestHealth
								}
								else if ( hit.entity->behavior == &actChest )
								{
									Entity* ohitentity = hit.entity;
									real_t slipFactor = sqrt(pow(my->vel_x, 2) + pow(my->vel_y, 2));
									if ( hit.side == VERTICAL )
									{
										if ( my->vel_x > 0.0 )
										{
											clipMove(&my->x, &my->y, slipFactor, 0.0, my);
										}
										else
										{
											clipMove(&my->x, &my->y, -slipFactor, 0.0, my);
										}
									}
									else if ( hit.side == HORIZONTAL )
									{
										if ( my->vel_y > 0.0 )
										{
											clipMove(&my->x, &my->y, 0.0, slipFactor, my);
										}
										else
										{
											clipMove(&my->x, &my->y, 0.0, -slipFactor, my);
										}
									}

									++my->monsterPathCount;
									if ( my->monsterPathCount > 50 )
									{
										my->monsterPathCount = 0;
										//messagePlayer(0, MESSAGE_DEBUG, "remaking path!");
										my->monsterMoveBackwardsAndPath(true);
									}
								}
								else if ( hit.entity->isDamageableCollider() && myStats->type == MINOTAUR )
								{
									hit.entity->colliderCurrentHP = 0;
									hit.entity->colliderKillerUid = 0;
								}
								else if ( (hit.entity->behavior == &actBoulder || hit.entity->behavior == &::actDaedalusShrine) && !hit.entity->flags[PASSABLE] && myStats->type == MINOTAUR )
								{
									// asplode the rock
									magicDig(nullptr, nullptr, 0, 1);
									hit.entity = nullptr;
								}
								else if ( hit.entity->behavior == &actMonster )
								{
									Stat* yourStats = hit.entity->getStats();
									if ( hit.entity->getUID() == my->monsterTarget )
									{
										//TODO: Refactor with setMonsterStateAttack().
										my->monsterState = MONSTER_STATE_ATTACK; // charge state

										// this is when a monster is bumps into it's known target.
										// let's to be ready to strike.
										// otherwise, we bumped into a new unexpected target, don't modify hitTime
										if ( hasrangedweapon )
										{
											// 120 ms reaction time
											if ( my->monsterHitTime < HITRATE )
											{
												if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
												{
													my->monsterHitTime = std::max(HITRATE, my->monsterHitTime);
												}
												else
												{
													my->monsterHitTime = std::max(HITRATE - 6, my->monsterHitTime);
												}
											}
											else
											{
												// bows have 2x hitrate time compared to standard weapons.
												my->monsterHitTime = std::max(2 * HITRATE - 6, my->monsterHitTime);
											}
										}
										else
										{
											// melee 240ms
											my->monsterHitTime = std::max(HITRATE - 12, my->monsterHitTime);
										}
										//messagePlayer(0, "bump1 -> attack, %d", my->monsterHitTime);
									}
									else if ( yourStats )
									{
										if ( !my->checkEnemy(hit.entity) )
										{
											// would you kindly move out of the way, sir?
											if ( !monsterMoveAside(hit.entity, my) )
											{
												my->monsterState = MONSTER_STATE_PATH;    // try something else and remake path
											}
											++my->monsterPathCount;
											if ( my->monsterPathCount > 100 )
											{
												my->monsterPathCount = 0;
												//messagePlayer(0, "running into monster like a fool!");
												my->monsterMoveBackwardsAndPath();
											}
										}
										else if ( my->checkEnemy(hit.entity) )
										{
											// charge state
											Entity& attackTarget = *hit.entity;
											my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);
										}
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									if ( my->checkEnemy(hit.entity) )
									{
										// charge state
										Entity& attackTarget = *hit.entity;
										if ( my->monsterTarget == hit.entity->getUID() )
										{
											// this is when a monster is bumps into it's known target.
											// let's to be ready to strike.
											// otherwise, we bumped into a new unexpected target, don't modify hitTime
											if ( hasrangedweapon )
											{
												// 120 ms reaction time
												if ( my->monsterHitTime < HITRATE )
												{
													if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
													{
														my->monsterHitTime = std::max(HITRATE, my->monsterHitTime);
													}
													else
													{
														my->monsterHitTime = std::max(HITRATE - 6, my->monsterHitTime);
													}
												}
												else
												{
													// bows have 2x hitrate time compared to standard weapons.
													my->monsterHitTime = std::max(2 * HITRATE - 6, my->monsterHitTime);
												}
											}
											else
											{
												// melee 240ms
												my->monsterHitTime = std::max(HITRATE - 12, my->monsterHitTime);
											}
										}
										//messagePlayer(0, "bump2 -> attack, %d", my->monsterHitTime);
										my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);
									}
									else
									{
										my->monsterState = MONSTER_STATE_PATH; // try something else and remake path
									}
								}
								else
								{
									my->monsterState = MONSTER_STATE_PATH; // remake path
									if ( myStats->type != LICH_FIRE && myStats->type != LICH_ICE )
									{
										if ( hit.entity->behavior == &actGate || hit.entity->behavior == &actBoulder
											 )
										{
											++my->monsterPathCount;
											if ( hit.entity->behavior == &actBoulder )
											{
												my->monsterPathCount += 5;
											}
											if ( my->monsterPathCount > 100 )
											{
												my->monsterPathCount = 0;
												//messagePlayer(0, MESSAGE_DEBUG, "remaking path!");
												my->monsterMoveBackwardsAndPath();
											}
										}
										else
										{
											my->monsterPathCount = 0;
											//messagePlayer(0, MESSAGE_DEBUG, "remaking path 2!");
										}
									}
								}
							}
							else
							{
								if ( dist2 <= 0.1 )
								{
									my->monsterState = MONSTER_STATE_PATH;    // remake path
								}
							}

							// rotate monster
							if ( myStats->EFFECTS[EFF_KNOCKBACK] )
							{
								// in knockback, the velocitys change sign from negative/positive or positive/negative.
								// this makes monsters moonwalk if the direction to rotate is assumed the same.
								// so we compare goal velocity direction (sin or cos(tangent)) and see if we've reached that sign.

								int multX = 1;
								int multY = 1;
								if ( cos(tangent) >= 0 == MONSTER_VELX > 0 )
								{
									// same sign.
									multX = 1;
								}
								else
								{
									// opposite signed.
									multX = -1;
								}
								if ( sin(tangent) >= 0 == MONSTER_VELY > 0 )
								{
									// same sign.
									multY = 1;
								}
								else
								{
									// opposite signed.
									multY = -1;
								}
								dir = my->yaw - atan2(multY * MONSTER_VELY, multX * MONSTER_VELX);
							}
							else
							{
								dir = my->yaw - atan2(MONSTER_VELY, MONSTER_VELX);
							}
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}
							my->yaw -= dir / 2;
							while ( my->yaw < 0 )
							{
								my->yaw += 2 * PI;
							}
							while ( my->yaw >= 2 * PI )
							{
								my->yaw -= 2 * PI;
							}
						}
					}
					else
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							my->lookAtEntity(*target);
							/*if ( myStats->type == SHADOW )
							{
								messagePlayer(0, "[SHADOW] No path #1: Resetting to wait state.");
							}*/
						}
						my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
						if ( !target && myStats->type == MIMIC )
						{
							mimicResetIdle(my);
						}
						else if ( !target && myStats->type == BAT_SMALL )
						{
							batResetIdle(my);
						}
						else if ( my->monsterAllyState == ALLY_STATE_MOVETO )
						{
							if ( my->monsterAllyInteractTarget != 0 )
							{
								//messagePlayer(0, "Interacting with a target!");
								if ( my->monsterAllySetInteract() )
								{
									if ( myStats->type == GYROBOT )
									{
										my->monsterSpecialState = GYRO_INTERACT_LANDING;
										my->monsterState = MONSTER_STATE_WAIT;
										serverUpdateEntitySkill(my, 33); // for clients to keep track of animation
									}
									else
									{

										if ( my->monsterAllyIndex >= 0 && FollowerMenu[my->monsterAllyIndex].entityToInteractWith
											&& FollowerMenu[my->monsterAllyIndex].entityToInteractWith->behavior == &actItem )
										{
											//my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM);
										}
										else
										{
											my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_OTHER);
										}
										my->monsterAllyInteractTarget = 0;
										my->monsterAllyState = ALLY_STATE_DEFAULT;
										if ( target )
										{
											if ( target->behavior == &actTeleporter || target->behavior == &actTeleportShrine )
											{
												my->monsterAllyState = ALLY_STATE_DEFEND;
												my->createPathBoundariesNPC(5);
											}
										}
									}
								}
							}
							else
							{
								//messagePlayer(0, "Sent a move to command, defending here!");
								// scan for enemies after reaching move point.
								real_t dist = sightranges[myStats->type];
								for ( node = map.creatures->first; node != nullptr; node = node->next )
								{
									Entity* target = (Entity*)node->element;
									if ( target && target->behavior == &actMonster && !target->isInertMimic() && my->checkEnemy(target) )
									{
										real_t oldDist = dist;
										dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
										if ( dist < sightranges[myStats->type] && dist <= oldDist )
										{
											double tangent = atan2(target->y - my->y, target->x - my->x);
											lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
											if ( hit.entity == target )
											{
												my->monsterLookTime = 1;
												my->monsterMoveTime = local_rng.rand() % 10 + 1;
												my->monsterLookDir = tangent;
												break;
											}
										}
									}
								}
								my->monsterAllyState = ALLY_STATE_DEFEND;
								my->createPathBoundariesNPC(5);
								if ( myStats->type == GYROBOT && my->monsterSpecialState == GYRO_RETURN_PATHING )
								{
									my->monsterSpecialState = GYRO_RETURN_LANDING;
									my->monsterState = MONSTER_STATE_WAIT;
									serverUpdateEntitySkill(my, 33); // for clients to keep track of animation
									playSoundEntity(my, 449, 128);
								}
							}

							if ( my->monsterAllyState != ALLY_STATE_MOVETO )
							{
								serverUpdateEntitySkill(my, 43); // update monsterAllyState
							}
						}
						else if ( !target && my->monsterAllyGetPlayerLeader() )
						{
							// scan for enemies after reaching move point.
							real_t dist = sightranges[myStats->type];
							for ( node = map.creatures->first; node != nullptr; node = node->next )
							{
								Entity* target = (Entity*)node->element;
								if ( target && target->behavior == &actMonster && !target->isInertMimic() && my->checkEnemy(target) )
								{
									real_t oldDist = dist;
									dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
									if ( dist < sightranges[myStats->type] && dist <= oldDist )
									{
										double tangent = atan2(target->y - my->y, target->x - my->x);
										lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
										if ( hit.entity == target )
										{
											my->monsterLookTime = 1;
											my->monsterMoveTime = local_rng.rand() % 10 + 1;
											my->monsterLookDir = tangent;
											break;
										}
									}
								}
							}
						}
					}
				}
				else
				{
					Entity* target = uidToEntity(my->monsterTarget);
					if ( target )
					{
						double tangent = atan2( target->y - my->y, target->x - my->x );
						my->monsterLookTime = 1;
						my->monsterMoveTime = local_rng.rand() % 10 + 1;
						my->monsterLookDir = tangent;
						/*if ( myStats->type == SHADOW )
						{
							messagePlayer(0, "[SHADOW] No path #2: Resetting to wait state.");
						}*/
					}
					my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
					if ( !target && myStats->type == MIMIC )
					{
						mimicResetIdle(my);
					}
					else if ( my->monsterAllyState == ALLY_STATE_MOVETO )
					{
						//messagePlayer(0, "Couldn't reach, retrying.");
						if ( target )
						{
							if ( my->monsterAllySetInteract() )
							{
								if ( myStats->type == GYROBOT )
								{
									my->monsterSpecialState = GYRO_INTERACT_LANDING;
									my->monsterState = MONSTER_STATE_WAIT;
									serverUpdateEntitySkill(my, 33); // for clients to keep track of animation
								}
								else
								{
									// we found our interactable within distance.
									//messagePlayer(0, "Found my interactable.");
									if ( my->monsterAllyIndex >= 0 && FollowerMenu[my->monsterAllyIndex].entityToInteractWith
										&& FollowerMenu[my->monsterAllyIndex].entityToInteractWith->behavior == &actItem )
									{
										//my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM);
									}
									else
									{
										my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_OTHER);
									}
									my->monsterAllyInteractTarget = 0;

									if ( target && (target->behavior == &actTeleporter || target->behavior == &actTeleportShrine) )
									{
										my->monsterAllyState = ALLY_STATE_DEFEND;
										my->createPathBoundariesNPC(5);
									}
									else
									{
										my->monsterAllyState = ALLY_STATE_DEFAULT;
									}
								}
							}
							else if ( my->monsterSetPathToLocation(static_cast<int>(target->x / 16), static_cast<int>(target->y / 16), 1,
								GeneratePathTypes::GENERATE_PATH_INTERACT_MOVE) ) // try closest tiles
							{
								my->monsterState = MONSTER_STATE_HUNT;
								my->monsterAllyState = ALLY_STATE_MOVETO;
								//messagePlayer(0, "Moving to my interactable!.");
								my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_REPATH);
							}
							else if ( my->monsterSetPathToLocation(static_cast<int>(target->x / 16), static_cast<int>(target->y / 16), 2,
								GeneratePathTypes::GENERATE_PATH_INTERACT_MOVE) ) // expand search
							{
								my->monsterState = MONSTER_STATE_HUNT;
								my->monsterAllyState = ALLY_STATE_MOVETO;
								//messagePlayer(0, "Moving to my interactable!.");
								my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_REPATH);
							}
							else
							{
								// no path possible, give up.
								//messagePlayer(0, "I can't get to my target.");
								my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_FAIL);
								my->monsterAllyInteractTarget = 0;
								my->monsterAllyState = ALLY_STATE_DEFAULT;
							}
						}
						else
						{
							//messagePlayer(0, "Issued move command, defending here.");
							// scan for enemies after reaching move point.
							real_t dist = sightranges[myStats->type];
							for ( node = map.creatures->first; node != nullptr; node = node->next )
							{
								Entity* target = (Entity*)node->element;
								if ( target && target->behavior == &actMonster && !target->isInertMimic() && my->checkEnemy(target) )
								{
									real_t oldDist = dist;
									dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
									if ( dist < sightranges[myStats->type] && dist <= oldDist )
									{
										double tangent = atan2(target->y - my->y, target->x - my->x);
										lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
										if ( hit.entity == target )
										{
											my->monsterLookTime = 1;
											my->monsterMoveTime = local_rng.rand() % 10 + 1;
											my->monsterLookDir = tangent;
											break;
										}
									}
								}
							}
							my->monsterAllyState = ALLY_STATE_DEFEND;
							my->createPathBoundariesNPC(5);
						}

						if ( my->monsterAllyState != ALLY_STATE_MOVETO )
						{
							serverUpdateEntitySkill(my, 43); // update monsterAllyState
						}
					}
				}
			}
			else
			{
				Entity* target = uidToEntity(my->monsterTarget);
				if ( target )
				{
					double tangent = atan2( target->y - my->y, target->x - my->x );
					my->monsterLookTime = 1;
					my->monsterMoveTime = local_rng.rand() % 10 + 1;
					my->monsterLookDir = tangent;
					/*if ( myStats->type == SHADOW )
					{
						messagePlayer(0, "[SHADOW] No path #3: Resetting to wait state.");
					}*/
				}
				my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
				//TODO: Replace with lookAtEntity();
			}
		}
		else if ( my->monsterState == MONSTER_STATE_TALK )     //Begin talk state
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;

			// turn towards target
			Entity* target = uidToEntity(my->monsterTarget);
			if ( target != NULL )
			{
				dir = my->yaw - atan2( target->y - my->y, target->x - my->x );
				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}

				// abandon conversation if distance is too great
				if ( sqrt( pow(my->x - target->x, 2) + pow(my->y - target->y, 2) ) > TOUCHRANGE )
				{
					my->monsterState = MONSTER_STATE_WAIT;
					my->monsterTarget = 0;
					int player = -1;
					if ( target && target->behavior == &actPlayer )
					{
						player = target->skill[2];
					}
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						players[player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
					}
					else if ( player > 0 && !players[player]->isLocalPlayer() )
					{
						// inform client of abandonment
						strcpy((char*)net_packet->data, "SHPC");
						SDLNet_Write32(my->getUID(), &net_packet->data[4]);
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
					monsterMoveAside(my, target);
				}
			}
			else
			{
				// abandon conversation
				my->monsterState = MONSTER_STATE_WAIT;
				my->monsterTarget = 0;
			}
		} //End talk state
		else if ( my->monsterState == MONSTER_STATE_LICH_DODGE )     // dodge state (herx)
		{
			double dist = 0;
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )   // hit obstacle
			{
				my->monsterSpecialTimer = 60;
				if ( local_rng.rand() % 2 )
				{
					my->monsterState = MONSTER_STATE_WAIT; // wait state
				}
				else
				{
					my->monsterState = MONSTER_STATE_LICH_SUMMON; // summoning state
				}
			}
			else
			{
				my->monsterSpecialTimer++;
				if ( my->monsterSpecialTimer > 20 )
				{
					my->monsterSpecialTimer = 60;
					if ( local_rng.rand() % 2 )
					{
						my->monsterState = MONSTER_STATE_WAIT; // wait state
					}
					else
					{
						my->monsterState = MONSTER_STATE_LICH_SUMMON; // summoning state
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_SUMMON )     // summoning state (herx)
		{
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer )
			{
				my->monsterSpecialTimer--;
			}
			else
			{
				my->monsterSpecialTimer = 60;
				my->monsterState = MONSTER_STATE_WAIT; // wait state
				playSoundEntity(my, 166, 128);

				Monster creature = NOTHING;
				switch ( local_rng.rand() % 5 )
				{
					case 0:
					case 1:
						creature = CREATURE_IMP;
						break;
					case 2:
					case 3:
					case 4:
						creature = DEMON;
						break;
				}
				if ( creature != DEMON )
				{
					if ( Entity* summon = summonMonster(creature, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8) )
					{
						if ( Stat* summonStats = summon->getStats() )
						{
							summonStats->monsterNoDropItems = 1;
							summonStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
							summonStats->LVL = 5;
							std::string lich_num_summons = myStats->getAttribute("lich_num_summons");
							if ( lich_num_summons == "" )
							{
								myStats->setAttribute("lich_num_summons", "1");
							}
							else
							{
								int numSummons = std::stoi(myStats->getAttribute("lich_num_summons"));
								if ( numSummons >= 25 )
								{
									summonStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] = 1;
								}
								++numSummons;
								myStats->setAttribute("lich_num_summons", std::to_string(numSummons));
							}
						}
					}
				}
				if ( Entity* summon = summonMonster(creature, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8) )
				{
					if ( Stat* summonStats = summon->getStats() )
					{
						summonStats->monsterNoDropItems = 1;
						summonStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
						summonStats->LVL = 5;
						std::string lich_num_summons = myStats->getAttribute("lich_num_summons");
						if ( lich_num_summons == "" )
						{
							myStats->setAttribute("lich_num_summons", "1");
						}
						else
						{
							int numSummons = std::stoi(myStats->getAttribute("lich_num_summons"));
							if ( numSummons >= 25 )
							{
								summonStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] = 1;
							}
							++numSummons;
							myStats->setAttribute("lich_num_summons", std::to_string(numSummons));
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_DEATH )     // lich death state
		{
			my->yaw += .5; // rotate
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				int c;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					playSoundPlayer(c, 186, 128);
				}
			}
			if ( my->monsterSpecialTimer % 10 == 0 )
			{
				spawnExplosion(my->x - 8 + local_rng.rand() % 16, my->y - 8 + local_rng.rand() % 16, -4 + local_rng.rand() % 8);
			}
			my->monsterSpecialTimer++;
			if ( my->monsterSpecialTimer > 180 )
			{
				lichDie(my);
				return;
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_DIE 
			|| my->monsterState == MONSTER_STATE_LICHICE_DIE )     // lich death state
		{
			if ( my->monsterSpecialTimer < 100 )
			{
				my->yaw += .5; // rotate
				if ( my->yaw >= PI * 2 )
				{
					my->yaw -= PI * 2;
				}
			}
			//messagePlayer(0, "timer: %d", my->monsterSpecialTimer);
			if ( my->monsterSpecialTimer == 180 )
			{
				if ( myStats->type == LICH_FIRE )
				{
					my->monsterAttack = MONSTER_POSE_SPECIAL_WINDUP1;
				}
				else if ( myStats->type == LICH_ICE )
				{
					my->monsterAttack = MONSTER_POSE_SPECIAL_WINDUP2;
				}
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				for ( int c = 0; c < MAXPLAYERS; c++ )
				{
					if ( myStats->type == LICH_FIRE )
					{
						if ( multiplayer == SINGLE )
						{
							if ( c == clientnum )
							{
								playSoundPlayer(c, 376, 128); // only play sfx once in splitscreen
							}
						}
						else
						{
							playSoundPlayer(c, 376, 128);
						}
						messagePlayerColor(c, MESSAGE_WORLD, uint32ColorOrange, Language::get(2646));
					}
					else if ( myStats->type == LICH_ICE )
					{
						if ( multiplayer == SINGLE )
						{
							if ( c == clientnum )
							{
								playSoundPlayer(c, 381, 128); // only play sfx once in splitscreen
							}
						}
						else
						{
							playSoundPlayer(c, 381, 128);
						}
						messagePlayerColor(c, MESSAGE_WORLD, uint32ColorBaronyBlue, Language::get(2648));
					}
				}
			}
			if ( my->monsterSpecialTimer % 15 == 0 )
			{
				spawnExplosion(my->x - 8 + local_rng.rand() % 16, my->y - 8 + local_rng.rand() % 16, my->z -4 + local_rng.rand() % 8);
			}
			--my->monsterSpecialTimer;
			if ( my->monsterSpecialTimer <= 0 )
			{
				if ( myStats->type == LICH_FIRE )
				{
					lichFireDie(my);
					return;
				}
				else if ( myStats->type == LICH_ICE )
				{
					lichIceDie(my);
					return;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_DEATH )     // devil death state
		{
			my->z += .5; // descend slowly
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			/*if( MONSTER_SPECIAL==0 ) {
				int c;
				for( c=0; c<MAXPLAYERS; c++ )
					playSoundPlayer(c,186,128);
			}*/
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->x += cos(my->yaw + PI / 2) * 2;
				my->y += sin(my->yaw + PI / 2) * 2;
			}
			else if ( my->monsterSpecialTimer % 2 == 0 )
			{
				my->x += cos(my->yaw + PI / 2) * 4;
				my->y += sin(my->yaw + PI / 2) * 4;
			}
			else
			{
				my->x -= cos(my->yaw + PI / 2) * 4;
				my->y -= sin(my->yaw + PI / 2) * 4;
			}
			if ( my->monsterSpecialTimer % 10 == 0 )
			{
				spawnExplosion(my->x - 24 + local_rng.rand() % 48, my->y - 24 + local_rng.rand() % 48, -16 + local_rng.rand() % 32);
			}
			my->monsterSpecialTimer++;
			if ( my->z > 96 )
			{
				devilDie(my);
				return;
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_TELEPORT )     // devil teleport state
		{
			my->flags[PASSABLE] = true;
			my->yaw += .1; // rotate
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			my->z = std::min<int>(my->z + 1, 64); // descend
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			MONSTER_ARMBENDED = 1;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
			}
			++my->monsterSpecialTimer;
			if ( my->z >= 64 )
			{
				node_t* node;
				int c = 0;
				for ( node = map.entities->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actDevilTeleport )
					{
						if ( entity->x == my->x && entity->y == my->y )
						{
							continue;
						}
						switch ( entity->sprite )
						{
							case 72:
								if ( devilstate == 74 )
								{
									c++;
								}
								continue;
							case 73:
								if ( devilstate == 0 || devilstate == 72 )
								{
									c++;
								}
								continue;
							case 74:
								if ( devilstate == 73 )
								{
									c++;
								}
								continue;
							default:
								continue;
						}
					}
				}
				if ( c )
				{
					int i = local_rng.rand() % c;
					c = 0;
					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity->behavior == &actDevilTeleport )
						{
							if ( entity->x == my->x && entity->y == my->y )
							{
								continue;
							}
							switch ( entity->sprite )
							{
								case 72:
									if ( devilstate == 74 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								case 73:
									if ( devilstate == 0 || devilstate == 72 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								case 74:
									if ( devilstate == 73 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								default:
									continue;
							}
							my->x = entity->x;
							my->y = entity->y;
							devilstate = entity->sprite;
							devilacted = 0;
							break;
						}
					}
				}
				my->monsterSpecialTimer = 30;
				my->monsterState = MONSTER_STATE_DEVIL_RISING;
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_RISING )     // devil rising state (post-teleport)
		{
			if ( my->monsterSpecialTimer <= 0 )
			{
				my->z = std::max<int>(my->z - 1, -4); // ascend
			}
			else
			{
				--my->monsterSpecialTimer;
				if ( my->monsterSpecialTimer <= 0 )
				{
					if ( myStats->HP > 0 )
					{
						my->flags[PASSABLE] = false;
					}
					node_t* node;
					for ( node = map.creatures->first; node != nullptr; node = node->next ) //Since it only looks at entities that have stats, only creatures can have stats; don't iterate map.entities.
					{
						Entity* entity = (Entity*)node->element;
						if ( entity == my )
						{
							continue;
						}
						if ( entityInsideEntity(my, entity) )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								if ( stats->HP > 0 )
								{
									stats->HP = 0;
								}
							}
						}
					}
				}
			}
			if ( !devilroar )
			{
				if ( my->z <= -4 )
				{
					int j = local_rng.rand() % 5;
					int c;
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						playSoundPlayer(c, 204 + j, 64);
					}
					playSoundEntity(my, 204 + j, 128);
					devilroar = 1;
					MONSTER_ATTACK = 4;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ARMBENDED = 1;
					serverUpdateEntitySkill(my, 8);
					serverUpdateEntitySkill(my, 9);
					serverUpdateEntitySkill(my, 10);
				}
				else
				{
					my->yaw += .1; // rotate
					if ( my->yaw >= PI * 2 )
					{
						my->yaw -= PI * 2;
					}
				}
			}
			else
			{
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Only inspects players, so don't iterate map.entities.
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
					MONSTER_VELX = my->monsterTargetX - my->x;
					MONSTER_VELY = my->monsterTargetY - my->y;
				}
				else
				{
					MONSTER_VELX = 0;
					MONSTER_VELY = 0;
				}

				// rotate monster
				dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );

				// To prevent the Entity's position from being updated by dead reckoning on the CLient, set the velocity to 0 after usage
				MONSTER_VELX = 0.0;
				MONSTER_VELY = 0.0;

				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}

				if ( MONSTER_ATTACKTIME > 60 )
				{
					my->monsterState = MONSTER_STATE_ATTACK;
					MONSTER_ATTACK = 0;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ARMBENDED = 0;
					serverUpdateEntitySkill(my, 8);
					serverUpdateEntitySkill(my, 9);
					serverUpdateEntitySkill(my, 10);
					devilroar = 0;
					MONSTER_VELX = 0;
					MONSTER_VELY = 0;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_SUMMON )     // devil summoning state
		{
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			++my->monsterSpecialTimer;
			if ( my->monsterSpecialTimer == 20 ) // start the spawn animations
			{
				int numToSpawn = 2;
				int numPlayers = 0;
				std::vector<int> alivePlayers;
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					if ( !client_disconnected[c] )
					{
						++numToSpawn;
						++numPlayers;
						if ( players[c] && players[c]->entity )
						{
							alivePlayers.push_back(c);
						}
					}
				}

				int spawnedShadows = 0;
				while ( numToSpawn > 0 )
				{
					if ( devilsummonedtimes % 2 == 1 && my->devilGetNumMonstersInArena(SHADOW) + spawnedShadows < numPlayers ) 
					{
						// odd numbered spawns.
						// let's make some shadows.
						if ( !alivePlayers.empty() )
						{
							int vectorEntry = local_rng.rand() % alivePlayers.size();
							my->devilSummonMonster(nullptr, SHADOW, 5, alivePlayers[vectorEntry]);
							alivePlayers.erase(alivePlayers.begin() + vectorEntry);
						}
						else
						{
							my->devilSummonMonster(nullptr, SHADOW, 9);
						}
						++spawnedShadows;
					}
					else
					{
						switch ( local_rng.rand() % 5 )
						{
							case 0:
							case 1:
								my->devilSummonMonster(nullptr, CREATURE_IMP, 9);
								break;
							case 2:
							case 3:
							case 4:
								my->devilSummonMonster(nullptr, DEMON, 7);
								break;
						}
					}
					--numToSpawn;
				}
				++devilsummonedtimes;
			}
			else if ( my->monsterSpecialTimer > 100 ) // end this state.
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->monsterSpecialTimer = 0;
				my->monsterState = MONSTER_STATE_ATTACK;
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Only inspects players, so don't iterate map.entities. Technically, only needs to iterate through the players[] array, eh?
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_BOULDER )     // devil boulder spawn state
		{
			int angle = -1;
			if ( (int)(my->x / 16) == 14 && (int)(my->y / 16) == 32 )
			{
				angle = 0;
			}
			else if ( (int)(my->x / 16) == 32 && (int)(my->y / 16) == 14 )
			{
				angle = 1;
			}
			else if ( (int)(my->x / 16) == 50 && (int)(my->y / 16) == 32 )
			{
				angle = 2;
			}
			else if ( (int)(my->x / 16) == 32 && (int)(my->y / 16) == 50 )
			{
				angle = 3;
			}
			std::unordered_set<int> lavalLocationsXY = { 22,23,24,31,32,33,40,41,42 };
			int numLavaBoulders = 0;

			my->yaw = angle * PI / 2;
			my->monsterSpecialTimer++;
			if ( my->monsterSpecialTimer == 10 )
			{
				MONSTER_ATTACK = 1;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);

				my->castOrbitingMagicMissile(SPELL_BLEED, 32.0, 0.0, 300);
				my->castOrbitingMagicMissile(SPELL_BLEED, 32.0, 2 * PI / 5, 300);
				my->castOrbitingMagicMissile(SPELL_BLEED, 32.0, 4 * PI / 5, 300);
				my->castOrbitingMagicMissile(SPELL_BLEED, 32.0, 6 * PI / 5, 300);
				my->castOrbitingMagicMissile(SPELL_BLEED, 32.0, 8 * PI / 5, 300);
			}
			if ( my->monsterSpecialTimer == 40 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; c++ )
				{
					my->yaw = ((double)c + ((local_rng.rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 7; ++c )
				{
					if ( c == 6 && (angle == 1 || angle == 2) )
					{
						continue;
					}
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (32 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (20 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (20 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (32 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}

					if ( lavalLocationsXY.find(static_cast<int>(entity->x / 16)) != lavalLocationsXY.end()
						|| lavalLocationsXY.find(static_cast<int>(entity->y / 16)) != lavalLocationsXY.end()
						|| myStats->HP < myStats->MAXHP * 0.5 )
					{
						// will roll over lava or Baphy < 50% HP
						int chance = 4;
						if ( myStats->HP < myStats->MAXHP * 0.25 )
						{
							chance = 1;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.5 )
						{
							chance = 2;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.75 )
						{
							chance = 3;
						}

						if ( local_rng.rand() % chance == 0 || (numLavaBoulders < 2 && local_rng.rand() % 2) )
						{
							entity->sprite = 989; // lava boulder.
							++numLavaBoulders;
						}
					}

					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 60 )
			{
				MONSTER_ATTACK = 2;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( my->monsterSpecialTimer == 90 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; ++c )
				{
					my->yaw = ((double)c + ((local_rng.rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				for ( c = 0; c < MAXPLAYERS; ++c )
				{
					my->devilBoulderSummonIfPlayerIsHiding(c);
				}
				my->yaw = oyaw;

				for ( c = 0; c < 7; ++c )
				{
					if ( c == 6 && (angle == 0 || angle == 3) )
					{
						continue;
					}
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (20 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (32 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (32 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (20 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}

					if ( lavalLocationsXY.find(static_cast<int>(entity->x / 16)) != lavalLocationsXY.end()
						|| lavalLocationsXY.find(static_cast<int>(entity->y / 16)) != lavalLocationsXY.end() 
						|| myStats->HP < myStats->MAXHP * 0.5 )
					{
						// will roll over lava or Baphy < 50% HP
						int chance = 4;
						if ( myStats->HP < myStats->MAXHP * 0.25 )
						{
							chance = 1;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.5 )
						{
							chance = 2;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.75 )
						{
							chance = 3;
						}

						if ( local_rng.rand() % chance == 0 || (numLavaBoulders < 2 && local_rng.rand() % 2) )
						{
							entity->sprite = 989; // lava boulder.
							++numLavaBoulders;
						}
					}

					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 180 )
			{
				MONSTER_ATTACK = 3;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( my->monsterSpecialTimer == 210 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; ++c )
				{
					my->yaw = ((double)c + ((local_rng.rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				for ( c = 0; c < MAXPLAYERS; ++c )
				{
					my->devilBoulderSummonIfPlayerIsHiding(c);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 12; ++c )
				{
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (21 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (21 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (21 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (21 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}

					if ( lavalLocationsXY.find(static_cast<int>(entity->x / 16)) != lavalLocationsXY.end()
						|| lavalLocationsXY.find(static_cast<int>(entity->y / 16)) != lavalLocationsXY.end()
						|| myStats->HP < myStats->MAXHP * 0.5 )
					{
						// will roll over lava or Baphy < 50% HP
						int chance = 4;
						if ( myStats->HP < myStats->MAXHP * 0.25 )
						{
							chance = 1;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.5 )
						{
							chance = 2;
						}
						else if ( myStats->HP < myStats->MAXHP * 0.75 )
						{
							chance = 3;
						}

						if ( local_rng.rand() % chance == 0 || (numLavaBoulders < 3 && local_rng.rand() % 2) )
						{
							entity->sprite = 989; // lava boulder.
							++numLavaBoulders;
						}
					}

					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 300 )   // 300 blaze it I guess
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->monsterSpecialTimer = 0;
				my->monsterState = MONSTER_STATE_ATTACK;
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Iterate map.creatures, since only inspecting players, not all entities. Technically should just iterate over players[]?
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_GENERIC_DODGE )
		{
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);

			Entity* target = uidToEntity(my->monsterTarget);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )
			{
				my->monsterSpecialTimer = 0; // hit obstacle
			}

			MONSTER_VELX *= 0.95;
			MONSTER_VELY *= 0.95;

			if ( my->monsterSpecialTimer == 0 )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				MONSTER_VELX = 0;
				MONSTER_VELY = 0;
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					my->monsterHitTime = HITRATE * 2;
					if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < STRIKERANGE )
					{
						if ( local_rng.rand() % 2 == 0 )
						{
							my->handleMonsterAttack(myStats, target, 0.f);
						}
						else
						{
							my->monsterHitTime = 25;
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_GENERIC_CHARGE )
		{
			Entity* target = uidToEntity(my->monsterTarget);

			real_t tangent = my->yaw;
			if ( target )
			{
				real_t tangent2 = atan2(target->y - my->y, target->x - my->x);
				real_t dist = lineTraceTarget(my, my->x, my->y, tangent2, sightranges[myStats->type], 0, false, target);
				if ( hit.entity == target )
				{
					tangent = tangent2;
				}
			}

			dir = my->yaw - atan2(sin(tangent), cos(tangent));
			while ( dir >= PI )
			{
				dir -= PI * 2;
			}
			while ( dir < -PI )
			{
				dir += PI * 2;
			}
			my->yaw -= dir / 64;
			while ( my->yaw < 0 )
			{
				my->yaw += 2 * PI;
			}
			while ( my->yaw >= 2 * PI )
			{
				my->yaw -= 2 * PI;
			}

			int myDex = my->monsterGetDexterityForMovement() + 30;
			real_t maxVelX = cos(my->yaw) * .045 * (myDex + 10) * weightratio;
			real_t maxVelY = sin(my->yaw) * .045 * (myDex + 10) * weightratio;
			MONSTER_VELX = maxVelX;
			MONSTER_VELY = maxVelY;

			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);

			bool stopPath = false;
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )
			{
				my->monsterSpecialTimer = 0; // hit obstacle
			}

			if ( my->monsterSpecialTimer == 0 )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				MONSTER_VELX = 0;
				MONSTER_VELY = 0;
				if ( target )
				{
					my->monsterHitTime = HITRATE * 2;
					if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < TOUCHRANGE )
					{
						my->monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
					}
					else
					{
						my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_DODGE
			|| my->monsterState == MONSTER_STATE_LICHICE_DODGE )
		{
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
			Entity* target = uidToEntity(my->monsterTarget);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )
			{
				my->monsterSpecialTimer = 0; // hit obstacle
			}
			if ( target && my->monsterSpecialTimer != 0 && myStats->type == LICH_FIRE )
			{
				dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
				if ( dist < STRIKERANGE && local_rng.rand() % 20 == 0 )
				{
					my->monsterSpecialTimer = 0; // close enough to target, chance to stop early
				}
			}
			if ( my->monsterSpecialTimer == 0 )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				MONSTER_VELX = 0;
				MONSTER_VELY = 0;
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					if ( myStats->type == LICH_FIRE )
					{
						my->monsterHitTime = HITRATE * 2;
						if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < STRIKERANGE )
						{
							if ( local_rng.rand() % 2 == 0 )
							{
								my->monsterLichFireMeleeSeq = LICH_ATK_RISING_RAIN;
								my->handleMonsterAttack(myStats, target, 0.f);
							}
							else
							{
								my->monsterHitTime = 25;
							}
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						my->monsterHitTime = HITRATE * 2;
						if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < STRIKERANGE * 2 )
						{
							my->monsterLichIceCastSeq = LICH_ATK_CHARGE_AOE;
							my->handleMonsterAttack(myStats, target, 0.f);
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS )
		{
			++my->monsterHitTime;
			if ( myStats->type == LICH_FIRE )
			{
				if ( my->monsterLichFireMeleeSeq == 0 )
				{
					if ( my->monsterHitTime >= 60 || (my->monsterLichAllyStatus == LICH_ALLY_DEAD && my->monsterHitTime >= 45) )
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							tangent = atan2(target->y - my->y, target->x - my->x);
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
							/*if ( hit.entity )
							{
								messagePlayer(0, "sprite: %d", hit.entity->sprite);
							}*/
							if ( hit.entity == target && local_rng.rand() % 5 > 0 )
							{
								switch ( local_rng.rand() % 3 )
								{
									case 0:
										my->monsterLichFireMeleeSeq = LICH_ATK_BASICSPELL_SINGLE;
										my->handleMonsterAttack(myStats, target, entityDist(my, target));
										my->monsterLichFireMeleeSeq = 0;
										break;
									case 1:
										my->monsterLichFireMeleeSeq = LICH_ATK_RISING_SINGLE;
										break;
									case 2:
										my->monsterLichFireMeleeSeq = LICH_ATK_HORIZONTAL_SINGLE;
										break;
									default:
										break;
								}
								my->monsterLichMagicCastCount = 0;
								//my->handleMonsterAttack(myStats, target, entityDist(my, target));
							}
							else
							{
								my->monsterLichFireMeleeSeq = LICH_ATK_RISING_RAIN;
								my->handleMonsterAttack(myStats, target, entityDist(my, target));
								my->monsterLichFireMeleeSeq = 0;
							}
							my->monsterHitTime = 0;
						}
						else
						{
							real_t distToPlayer = 0.f;
							int playerToChase = -1;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( players[c] && players[c]->entity )
								{
									if ( !distToPlayer )
									{
										distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										playerToChase = c;
									}
									else
									{
										double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										if ( newDistToPlayer < distToPlayer )
										{
											distToPlayer = newDistToPlayer;
											playerToChase = c;
										}
									}
								}
							}
							if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
							{
								my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
							}
						}
					}
				}
			
				if ( my->monsterLichFireMeleeSeq != 0
					&& my->monsterHitTime >= 0
					&& my->monsterHitTime % 10 == 0 )
				{
					if ( my->monsterLichFireMeleeSeq == LICH_ATK_RISING_SINGLE )
					{
						if ( my->monsterLichMagicCastCount < 3 + local_rng.rand() % 2 )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(MONSTER_POSE_MELEE_WINDUP3, 0, nullptr);
							}
							else
							{
								Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw += (PI / 64) * (-1 + local_rng.rand() % 3);
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							my->monsterLichFireMeleeSeq = 0;
							my->monsterHitTime = 0;
						}
					}
					else if ( my->monsterLichFireMeleeSeq == LICH_ATK_HORIZONTAL_SINGLE )
					{
						if ( my->monsterLichMagicCastCount < 2 )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(MONSTER_POSE_MELEE_WINDUP2, 0, nullptr);
							}
							else
							{
								Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw += PI / 16;
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
								spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw -= PI / 16;
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
								spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							my->monsterLichFireMeleeSeq = 0;
							my->monsterHitTime = 0;
						}
					}
				}
			}
			else if ( myStats->type == LICH_ICE )
			{
				if ( my->monsterLichIceCastSeq == 0 )
				{
					if ( my->monsterHitTime >= 60 || (my->monsterLichAllyStatus == LICH_ALLY_DEAD && my->monsterHitTime >= 45) )
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							tangent = atan2(target->y - my->y, target->x - my->x);
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
							switch ( local_rng.rand() % 4 )
							{
								case 0:
								case 1:
									my->monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
									break;
								case 2:
								case 3:
									if ( my->monsterLichAllyStatus == LICH_ALLY_DEAD )
									{
										Entity* dummyEntity = nullptr;
										if ( numMonsterTypeAliveOnMap(AUTOMATON, dummyEntity) <= 1 )
										{
											my->monsterLichIceCastSeq = LICH_ATK_SUMMON;
										}
										else
										{
											my->monsterLichIceCastSeq = LICH_ATK_RISING_SINGLE;
										}
									}
									else
									{
										my->monsterLichIceCastSeq = LICH_ATK_RISING_SINGLE;
									}
									break;
								default:
									break;
							}
							my->monsterHitTime = 0;
							my->monsterLichMagicCastCount = 0;
						}
						else
						{
							real_t distToPlayer = 0.f;
							int playerToChase = -1;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( players[c] && players[c]->entity )
								{
									if ( !distToPlayer )
									{
										distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										playerToChase = c;
									}
									else
									{
										double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										if ( newDistToPlayer < distToPlayer )
										{
											distToPlayer = newDistToPlayer;
											playerToChase = c;
										}
									}
								}
							}
							if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
							{
								my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
							}
						}
					}
				}

				if ( my->monsterLichIceCastSeq != 0
					&& my->monsterHitTime >= 0
					&& my->monsterHitTime % 10 == 0 )
				{
					if ( my->monsterLichIceCastSeq == LICH_ATK_RISING_SINGLE
						|| my->monsterLichIceCastSeq == LICH_ATK_HORIZONTAL_SINGLE
						|| my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
					{
						int castLimit = 6;
						if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
						{
							castLimit = 2 + local_rng.rand() % 2;
						}
						if ( my->monsterLichMagicCastCount < castLimit )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(my->getAttackPose(), 0, nullptr);
							}
							else
							{
								if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
								{
									my->lichIceSummonMonster(AUTOMATON);
								}
								else
								{
									Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_MAGICMISSILE), true, false);
									real_t horizontalSpeed = 4.0;
									Entity* target = uidToEntity(my->monsterTarget);
									if ( target )
									{
										real_t spellDistance = sqrt(pow(spell->x - target->x, 2) + pow(spell->y - target->y, 2));
										spell->vel_z = 22.0 / (spellDistance / horizontalSpeed);
										if ( local_rng.rand() % 3 >= 1 )
										{
											// spells will track the velocity of the target.
											real_t ticksToHit = (spellDistance / horizontalSpeed);
											real_t predictx = target->x + (target->vel_x * ticksToHit);
											real_t predicty = target->y + (target->vel_y * ticksToHit);
											tangent = atan2(predicty - spell->y, predictx - spell->x); // assume target will be here when spell lands.
											//messagePlayer(0, "x: %f->%f, y: %f->%f, angle offset: %f", target->x, predictx, target->y, predicty, spell->yaw - tangent);
											spell->yaw = tangent;
										}
										else
										{
											// do some minor variations in spell angle
											spell->yaw += ((PI * (-4 + local_rng.rand() % 9)) / 100);
										}
									}
									else
									{
										spell->vel_z = 1.6;
										// do some minor variations in spell angle
										spell->yaw += ((PI * (-4 + local_rng.rand() % 9)) / 100);
									}
									spell->vel_x = horizontalSpeed * cos(spell->yaw);
									spell->vel_y = horizontalSpeed * sin(spell->yaw);
									spell->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
									spell->z = -22.0;
									spell->pitch = atan2(spell->vel_z, horizontalSpeed);
								}
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
							{
								my->monsterHitTime = 45;
							}
							else
							{
								my->monsterHitTime = 0;
							}
							my->monsterLichIceCastSeq = 0;
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY
			|| my->monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY )
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;
		}
		//End state machine.

	}
	else
	{
		if ( myStats && myStats->type == BAT_SMALL && my->monsterSpecialState == BAT_REST )
		{
			my->monsterReleaseAttackTarget();

			if ( myReflex ) // randomly dont check
			{
				for ( node_t* node2 = map.creatures->first; node2 != nullptr; node2 = node2->next )
				{
					Entity* entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					Stat* hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2(entity->y - my->y, entity->x - my->x);
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							bool visiontest = false;
							real_t monsterVisionRange = 40.0; //sightranges[myStats->type];
							int sizex = my->sizex;
							int sizey = my->sizey;
							my->sizex = std::max(my->sizex, 4); // override size temporarily
							my->sizey = std::max(my->sizey, 4);
							if ( entityInsideEntity(my, entity) && !hitstats->sneaking )
							{
								visiontest = true;
							}
							my->sizex = sizex;
							my->sizey = sizey;
							if ( !visiontest )
							{
								if ( local_rng.rand() % 4 != 0 )
								{
									continue; // random chance to ignore
								}

								// skip if light level is too low and distance is too high
								int light = entity->entityLightAfterReductions(*hitstats, my);
								double targetdist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));

								if ( targetdist > monsterVisionRange )
								{
									continue;
								}
								if ( targetdist > TOUCHRANGE && targetdist > light )
								{
									continue;
								}
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}

							if ( visiontest )   // vision cone
							{
								lineTrace(my, my->x, my->y, tangent, monsterVisionRange, LINETRACE_IGNORE_ENTITIES, false);
								if ( !hit.entity )
								{
									lineTrace(my, my->x, my->y, tangent, TOUCHRANGE, 0, false);
								}
								if ( hit.entity == entity )
								{
									// charge state
									Entity* attackTarget = hit.entity;
									if ( my->disturbBat(attackTarget, false, true) )
									{
										my->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_ATTACK);

										if ( MONSTER_SOUND == nullptr )
										{
											MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128);
										}

										my->alertAlliesOnBeingHit(attackTarget);

										if ( entity != nullptr )
										{
											if ( entity->behavior == &actPlayer )
											{
												assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
												assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		else if ( myStats && myStats->type == MIMIC && myStats->EFFECTS[EFF_MIMIC_LOCKED] && !my->isInertMimic() )
		{
			my->monsterHitTime++;
			if ( my->monsterHitTime >= HITRATE )
			{
				my->attack(MONSTER_POSE_MIMIC_LOCKED, 0, nullptr);
				my->monsterHitTime = 0;
				
				if ( !uidToEntity(my->monsterTarget) && myStats->monsterMimicLockedBy != 0 )
				{
					if ( Entity* lockedMe = uidToEntity(myStats->monsterMimicLockedBy) )
					{
						my->monsterAcquireAttackTarget(*lockedMe, MONSTER_STATE_ATTACK);
					}
				}
			}
		}
		else if ( myStats && myStats->type == MIMIC && !my->isInertMimic() )
		{
			if ( myStats->EFFECTS[EFF_ASLEEP] || myStats->EFFECTS[EFF_PARALYZED] )
			{
				if ( my->monsterSpecialState != MIMIC_STATUS_IMMOBILE )
				{
					my->monsterSpecialState = MIMIC_STATUS_IMMOBILE;
					serverUpdateEntitySkill(my, 33);
				}
			}
		}
		else if ( my->isInertMimic() )
		{
			my->monsterReleaseAttackTarget();

			if ( myReflex && my->monsterSpecialState == MIMIC_INERT_SECOND )
			{
				for ( node_t* node2 = map.creatures->first; node2 != nullptr; node2 = node2->next )
				{
					Entity* entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					if ( entity->behavior != &actPlayer )
					{
						continue;
					}
					Stat* hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2(entity->y - my->y, entity->x - my->x);
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							// skip if light level is too low and distance is too high
							int light = entity->entityLightAfterReductions(*hitstats, my);
							double targetdist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));

							real_t monsterVisionRange = 24.0; //sightranges[myStats->type];

							if ( targetdist > monsterVisionRange )
							{
								continue;
							}
							if ( targetdist > TOUCHRANGE && targetdist > light )
							{
								continue;
							}
							bool visiontest = false;
							if ( dir >= -9 * PI / 16 && dir <= 9 * PI / 16 )
							{
								visiontest = true;
							}

							if ( visiontest )   // vision cone
							{
								lineTrace(my, my->x, my->y, tangent, monsterVisionRange, LINETRACE_IGNORE_ENTITIES, false);
								if ( !hit.entity )
								{
									lineTrace(my, my->x, my->y, tangent, TOUCHRANGE, 0, false);
								}
								if ( hit.entity == entity )
								{
									// charge state
									Entity* attackTarget = hit.entity;
									if ( my->disturbMimic(attackTarget, false, true) )
									{
										my->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_ATTACK);

										if ( MONSTER_SOUND == nullptr )
										{
											MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + local_rng.rand() % MONSTER_SPOTVAR, 128);
										}

										if ( entity != nullptr )
										{
											if ( entity->behavior == &actPlayer )
											{
												assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
												assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		if ( myStats->EFFECTS[EFF_KNOCKBACK] )
		{
			my->monsterHandleKnockbackVelocity(my->monsterKnockbackTangentDir, weightratio);
			if ( abs(MONSTER_VELX) > 0.01 || abs(MONSTER_VELY) > 0.01 )
			{
				dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
				my->handleKnockbackDamage(*myStats, hit.entity);
			}
		}
		else
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;
		}
	}

	if ( previousMonsterState != my->monsterState )
	{
		serverUpdateEntitySkill(my, 0);
		if ( my->monsterAllyIndex > 0 && my->monsterAllyIndex < MAXPLAYERS )
		{
			serverUpdateEntitySkill(my, 1); // update monsterTarget for player leaders.
		}

		if ( myStats->type == SHOPKEEPER && previousMonsterState == MONSTER_STATE_TALK )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i]->isLocalPlayer() && shopkeeper[i] == my->getUID() )
				{
					players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
				}
				else if ( i > 0 && !client_disconnected[i] && multiplayer == SERVER && !players[i]->isLocalPlayer() )
				{
					// inform client of abandonment
					strcpy((char*)net_packet->data, "SHPC");
					SDLNet_Write32(my->getUID(), &net_packet->data[4]);
					net_packet->address.host = net_clients[i - 1].host;
					net_packet->address.port = net_clients[i - 1].port;
					net_packet->len = 8;
					sendPacketSafe(net_sock, -1, net_packet, i - 1);
				}
			}
		}
	}

	// move body parts
	myStats = my->getStats();
	if ( myStats != NULL )
	{
	    const auto dist = sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY);
		if ( myStats->getAttribute("monster_portrait") != "" )
		{
			my->yaw = 0.0;
		}
		else
		{
			monsterAnimate(my, myStats, dist);
		}
	}
}

void Entity::handleMonsterAttack(Stat* myStats, Entity* target, double dist)
{
	Stat* hitstats = nullptr;
	int charge = 1;

	if (myStats->type == MINOTAUR) {
	    if (checkFriend(target)) {
	        // I don't know why this happens,
	        // I shouldn't have to know why this happens,
	        // but it does, so this fixes it.
	        return;
	    }
	}

	//TODO: I don't like this function getting called every frame. Find a better place to put it.
	chooseWeapon(target, dist);
	bool hasrangedweapon = this->hasRangedWeapon();
	bool lichRangeCheckOverride = false;
	if ( myStats->type == SLIME )
	{
		if ( monsterSpecialState == SLIME_CAST )
		{
			lichRangeCheckOverride = true;
		}
	}
	if ( myStats->type == LICH_FIRE)
	{
		if ( monsterLichFireMeleeSeq == LICH_ATK_BASICSPELL_SINGLE )
		{
			hasrangedweapon = true;
		}
		if ( monsterState == MONSTER_STATE_LICH_CASTSPELLS )
		{
			lichRangeCheckOverride = true;
		}
	}
	else if ( myStats->type == LICH_ICE )
	{
		// lichice todo
		if ( monsterLichIceCastSeq == LICH_ATK_BASICSPELL_SINGLE )
		{
			hasrangedweapon = true;
		}
		if ( monsterStrafeDirection == 0 && local_rng.rand() % 10 == 0 && ticks % 10 == 0 )
		{
			monsterStrafeDirection = -1 + ((local_rng.rand() % 2 == 0) ? 2 : 0);
		}
	}
	else if ( myStats->type == DUMMYBOT )
	{
		return;
	}
	else
	{
		if ( myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] > 0 )
		{
			if ( monsterSpecialTimer == 0 && monsterSpecialState == 0 && (this->monsterHitTime >= HITRATE / 2) )
			{
				if ( local_rng.rand() % 50 == 0 )
				{
					node_t* node = itemNodeInInventory(myStats, -1, SPELLBOOK);
					if ( node != nullptr )
					{
						bool swapped = swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
						if ( swapped )
						{
							monsterSpecialState = MONSTER_SPELLCAST_GENERIC;
							int timer = (myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] >> 4) & 0xFFFF;
							monsterSpecialTimer = timer > 0 ? timer : 250;
							hasrangedweapon = true;
						}
					}
				}
			}
		}
	}

	int meleeDist = STRIKERANGE;
	if ( myStats->type == BUGBEAR && monsterSpecialState == BUGBEAR_DEFENSE )
	{
		meleeDist = TOUCHRANGE - 1;
	}

	// check the range to the target, depending on ranged weapon or melee.
	if ( (dist < meleeDist && !hasrangedweapon) || (hasrangedweapon && dist < getMonsterEffectiveDistanceOfRangedWeapon(myStats->weapon)) || lichRangeCheckOverride )
	{
		// increment the hit time, don't attack until this reaches the hitrate of the weapon
		this->monsterHitTime++;
		real_t bow = 1;
		if ( hasrangedweapon && myStats->weapon )
		{
			if ( (myStats->weapon->type == SLING 
					|| myStats->weapon->type == SHORTBOW 
					|| myStats->weapon->type == ARTIFACT_BOW
					|| myStats->weapon->type == LONGBOW
					|| myStats->weapon->type == COMPOUND_BOW) )
			{
				bow = 2;
				if ( myStats->weapon->type == COMPOUND_BOW )
				{
					bow = 1.5;
				}
				if ( myStats->shield && itemTypeIsQuiver(myStats->shield->type) )
				{
					if ( myStats->shield->type == QUIVER_LIGHTWEIGHT )
					{
						bow -= 0.5;
					}
				}
			}
			else if ( myStats->weapon->type == CROSSBOW )
			{
				if ( myStats->shield && itemTypeIsQuiver(myStats->shield->type) )
				{
					if ( myStats->shield->type == QUIVER_LIGHTWEIGHT )
					{
						bow = 0.8;
					}
				}
			}
			else if ( myStats->weapon->type == HEAVY_CROSSBOW )
			{
				bow = 1.5;
			}
		}
		if ( myStats->type == BAT_SMALL )
		{
			bow = 1.5;
		}
		else if ( monsterIsImmobileTurret(this, myStats) )
		{
			bow = 2;
			if ( myStats->type == SPELLBOT )
			{
				if ( myStats->LVL >= 15 )
				{
					bow = 1.2;
				}
				else if ( myStats->LVL >= 10 )
				{
					bow = 1.5;
				}
				else if ( myStats->LVL >= 5 )
				{
					bow = 1.8;
				}
				else
				{
					bow = 2;
				}
			}
		}
		// check if ready to attack
		if ( (this->monsterHitTime >= static_cast<int>(HITRATE * monsterGlobalAttackTimeMultiplier * bow) 
				&& (myStats->type != LICH && myStats->type != LICH_ICE))
			|| (this->monsterHitTime >= 5 && myStats->type == LICH)
			|| (this->monsterHitTime >= HITRATE * 2 && myStats->type == LICH_ICE)
			)
		{
			bool shouldAttack = this->handleMonsterSpecialAttack(myStats, nullptr, dist, false);
			if ( !shouldAttack )
			{
				// handleMonsterSpecialAttack processed an action where the monster should not try to attack this frame.
				// e.g unequipping/swapping from special weapon, stops punching the air after casting a spell.
				return;
			}

			if ( myStats->type == LICH )
			{
				this->monsterSpecialTimer++;
				if ( this->monsterSpecialTimer >= 5 )
				{
					this->monsterSpecialTimer = 90;
					this->monsterTarget = 0;
					this->monsterTargetX = this->x - 50 + local_rng.rand() % 100;
					this->monsterTargetY = this->y - 50 + local_rng.rand() % 100;
					this->monsterState = MONSTER_STATE_PATH; // path state
				}
			}

			// reset the hit timer
			this->monsterHitTime = 0;
			int tracedist = 0;
			if ( lichRangeCheckOverride )
			{
				tracedist = 1024;
			}
			else if ( hasrangedweapon )
			{
				tracedist = 160;
				if ( myStats->weapon )
				{
					tracedist = getMonsterEffectiveDistanceOfRangedWeapon(myStats->weapon);
				}
			}
			else
			{
				tracedist = STRIKERANGE;
			}

			// check again for the target in attack range. return the result into hit.entity.
			double newTangent = atan2(target->y - this->y, target->x - this->x);
			if ( lichRangeCheckOverride )
			{
				hit.entity = uidToEntity(monsterTarget);
			}
			else
			{
				lineTrace(this, this->x, this->y, newTangent, tracedist, 0, false);
			}
			if ( hit.entity != nullptr && hit.entity == target )
			{
				// found the target in range
				hitstats = hit.entity->getStats();
				if ( hit.entity->behavior == &actMonster && !hasrangedweapon )
				{
					// alert the monster!
					if ( hit.entity->monsterState != MONSTER_STATE_ATTACK )
					{
						hit.entity->monsterAcquireAttackTarget(*this, MONSTER_STATE_PATH);
					}
				}
				if ( hitstats != nullptr )
				{
					// prepare attack, set the animation of the attack based on the current weapon.
					int pose = this->getAttackPose();

					int oldDefend = monsterDefend;
					monsterDefend = shouldMonsterDefend(*myStats, *hit.entity, *hitstats, dist, hasrangedweapon);
					if ( oldDefend != monsterDefend )
					{
						serverUpdateEntitySkill(this, 47);
					}

					// turn to the target, then reset my yaw.
					double oYaw = this->yaw;
					this->yaw = newTangent;
					if ( myStats->type == LICH_FIRE )
					{
						if ( monsterState != MONSTER_STATE_LICH_CASTSPELLS )
						{
							lichFireSetNextAttack(*myStats);
							//messagePlayer(0, "previous %d, next is %d", monsterLichFireMeleePrev, monsterLichFireMeleeSeq);
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						lichIceSetNextAttack(*myStats);
						//messagePlayer(0, "previous %d, next is %d", monsterLichIceCastPrev, monsterLichIceCastSeq);
						if ( monsterLichIceCastPrev == LICH_ATK_BASICSPELL_SINGLE )
						{
							monsterHitTime = HITRATE;
						}
						if ( monsterSpecialState == LICH_ICE_ATTACK_COMBO )
						{
							monsterHitTime = HITRATE * 2 - 25;
							if ( monsterLichMeleeSwingCount > 1 )
							{
								monsterSpecialState = 0;
								monsterSpecialTimer = 100;
							}
						}
					}
					
					if ( myStats->type == BUGBEAR && monsterSpecialState == BUGBEAR_DEFENSE )
					{
						if ( myStats->shield && myStats->shield->type == STEEL_SHIELD && local_rng.rand() % 3 == 0 && dist <= meleeDist )
						{
							// shield bash
							this->attack(MONSTER_POSE_SPECIAL_WINDUP1, charge, nullptr); // attacku! D:<
						}
						else if ( monsterDefend == MONSTER_DEFEND_HOLD )
						{
							monsterHitTime = HITRATE / 2;
						}
						else
						{
							this->attack(pose, charge, nullptr); // attacku! D:<
						}
					}
					else if ( monsterDefend == MONSTER_DEFEND_HOLD )
					{
						// skip attack, continue defending. offset the hit time to allow for timing variation.
						monsterHitTime = HITRATE / 4;
					}
					else
					{
						this->attack(pose, charge, nullptr); // attacku! D:<
					}
					this->yaw = oYaw;
				}
			}
		}
	}
	else
	{
		if ( myStats->type == BUGBEAR && monsterSpecialState == BUGBEAR_DEFENSE )
		{
			if ( monsterHitTime < HITRATE - 5 )
			{
				++monsterHitTime;
			}
		}
		if ( ticks % (90 + getUID() % 10) == 0 ) 
		{
			if ( myStats->type == BUGBEAR )
			{
				int oldDefend = monsterDefend;
				if ( monsterSpecialState == BUGBEAR_DEFENSE )
				{
					monsterDefend = shouldMonsterDefend(*myStats, *target, *target->getStats(), dist, hasrangedweapon);
				}
				else if ( !hasrangedweapon && dist > TOUCHRANGE && target )
				{
					monsterDefend = shouldMonsterDefend(*myStats, *target, *target->getStats(), dist, hasrangedweapon);
				}
				if ( oldDefend != monsterDefend )
				{
					serverUpdateEntitySkill(this, 47);
				}
			}
			else if ( !hasrangedweapon && dist > TOUCHRANGE && target && target->hasRangedWeapon() )
			{
				int oldDefend = monsterDefend;
				monsterDefend = shouldMonsterDefend(*myStats, *target, *target->getStats(), dist, hasrangedweapon);
				if ( oldDefend != monsterDefend )
				{
					serverUpdateEntitySkill(this, 47);
				}
			}
		}
	}

	return;
}

int limbAnimateWithOvershoot(Entity* limb, int axis, double setpointRate, double setpoint, double endpointRate, double endpoint, int dir)
{
	double speedMultiplier = 1.0;

	if ( monsterGlobalAnimationMultiplier != 10 )
	{
		speedMultiplier = monsterGlobalAnimationMultiplier / 10.0;
		setpointRate = setpointRate * speedMultiplier;
		endpointRate = endpointRate * speedMultiplier;
	}

	if ( axis == 0 || limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE || dir == ANIMATE_DIR_NONE )
	{
		if ( axis == ANIMATE_PITCH )
		{
			limb->pitch = endpoint;
		}
		else if ( axis == ANIMATE_ROLL )
		{
			limb->roll = endpoint;
		}
		else if ( axis == ANIMATE_YAW )
		{
			limb->yaw = endpoint;
		}
		// no animation required.
		return -1;
	}

	if ( axis == ANIMATE_PITCH )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->pitch += setpointRate * dir;
			while ( limb->pitch < 0 )
			{
				limb->pitch += 2 * PI;
			}
			while ( limb->pitch >= 2 * PI )
			{
				limb->pitch -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->pitch, setpointRate, setpoint) )
			{
				limb->pitch = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->pitch -= endpointRate * dir;
			while ( limb->pitch < 0 )
			{
				limb->pitch += 2 * PI;
			}
			while ( limb->pitch >= 2 * PI )
			{
				limb->pitch -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->pitch, endpointRate, endpoint) )
			{
				limb->pitch = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_ROLL )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->roll += setpointRate * dir;
			while ( limb->roll < 0 )
			{
				limb->roll += 2 * PI;
			}
			while ( limb->roll >= 2 * PI )
			{
				limb->roll -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->roll, setpointRate, setpoint) )
			{
				limb->roll = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->roll -= endpointRate * dir;
			while ( limb->roll < 0 )
			{
				limb->roll += 2 * PI;
			}
			while ( limb->roll >= 2 * PI )
			{
				limb->roll -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->roll, endpointRate, endpoint) )
			{
				limb->roll = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_YAW )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->yaw += setpointRate * dir;
			while ( limb->yaw < 0 )
			{
				limb->yaw += 2 * PI;
			}
			while ( limb->yaw >= 2 * PI )
			{
				limb->yaw -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->yaw, setpointRate, setpoint) )
			{
				limb->yaw = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->yaw -= endpointRate * dir;
			while ( limb->yaw < 0 )
			{
				limb->yaw += 2 * PI;
			}
			while ( limb->yaw >= 2 * PI )
			{
				limb->yaw -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->yaw, endpointRate, endpoint) )
			{
				limb->yaw = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_Z )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->z += setpointRate * dir;

			if ( limbAngleWithinRange(limb->z, setpointRate, setpoint) )
			{
				limb->z = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->z -= endpointRate * dir;

			if ( limbAngleWithinRange(limb->z, endpointRate, endpoint) )
			{
				limb->z = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}

	return -1;
}

int limbAnimateToLimit(Entity* limb, int axis, double rate, double setpoint, bool shake, double shakerate)
{
	if ( axis == 0 )
	{
		return 0;
	}

	double speedMultiplier = 1.0;

	if ( monsterGlobalAnimationMultiplier != 10 )
	{
		speedMultiplier = monsterGlobalAnimationMultiplier / 10.0;
		rate = rate * speedMultiplier;
		shakerate = shakerate * speedMultiplier;
	}

	if ( axis == ANIMATE_YAW )
	{
		while ( limb->yaw < 0 )
		{
			limb->yaw += 2 * PI;
		}
		while ( limb->yaw >= 2 * PI )
		{
			limb->yaw -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->yaw, rate, setpoint) )
		{
			limb->yaw = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->yaw += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->yaw -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->yaw += rate;
	} 
	else if ( axis == ANIMATE_PITCH )
	{
		while ( limb->pitch < 0 )
		{
			limb->pitch += 2 * PI;
		}
		while ( limb->pitch >= 2 * PI )
		{
			limb->pitch -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->pitch, rate, setpoint) )
		{
			limb->pitch = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->pitch += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->pitch -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->pitch += rate;
	} 
	else if ( axis == ANIMATE_ROLL )
	{
		while ( limb->roll < 0 )
		{
			limb->roll += 2 * PI;
		}
		while ( limb->roll >= 2 * PI )
		{
			limb->roll -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->roll, rate, setpoint) )
		{
			limb->roll = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->roll += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->roll -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->roll += rate;
	}
	else if ( axis == ANIMATE_Z )
	{
		if ( limbAngleWithinRange(limb->z, rate, setpoint) )
		{
			limb->z = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->z += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->z -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->z += rate;
	}
	else if ( axis == ANIMATE_WEAPON_YAW )
	{
		while ( limb->fskill[5] < 0 )
		{
			limb->fskill[5] += 2 * PI;
		}
		while ( limb->fskill[5] >= 2 * PI )
		{
			limb->fskill[5] -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->fskill[5], rate, setpoint) )
		{
			limb->fskill[5] = setpoint;
			return 1; //reached setpoint
		}
		limb->fskill[5] += rate;
	}

	return 0;
}

int limbAngleWithinRange(real_t angle, double rate, double setpoint)
{
	if ( rate > 0 )
	{
		if ( (angle <= (setpoint + rate)) && (angle >= (setpoint - rate)) )
		{
			return 1;
		}
	}
	else if ( rate < 0 )
	{
		if ( (angle >= (setpoint + rate)) && (angle <= (setpoint - rate)) )
		{
			return 1;
		}
	}

	return 0;
}

real_t normaliseAngle2PI(real_t angle)
{
	while ( angle >= 2 * PI )
	{
		angle -= 2 * PI;
	}
	while ( angle < 0 )
	{
		angle += 2 * PI;
	}

	return angle;
}

bool forceFollower(Entity& leader, Entity& follower)
{
	Stat* leaderStats = leader.getStats();
	Stat* followerStats = follower.getStats();
	if ( !leaderStats || !followerStats )
	{
		printlog("[forceFollower] Error: Either leader or follower did not have stats.");
		return false;
	}

	Uint32* myuid = (Uint32*) (malloc(sizeof(Uint32)));
	*myuid = follower.getUID();

	//Deal with the old leader.
	if ( followerStats->leader_uid != 0 )
	{
		Entity* oldLeader = uidToEntity(followerStats->leader_uid);
		if ( oldLeader )
		{
			Stat* oldLeaderStats = oldLeader->getStats();
			if ( oldLeaderStats )
			{
				if ( leader.behavior == &actPlayer 
					&& oldLeader == &leader )
				{
					steamAchievementClient(leader.skill[2], "BARONY_ACH_CONFESSOR");
				}
				list_RemoveNodeWithElement<Uint32>(oldLeaderStats->FOLLOWERS, *myuid);
				if ( oldLeader->behavior == &actPlayer && !players[oldLeader->skill[2]]->isLocalPlayer() )
				{
					serverRemoveClientFollower(oldLeader->skill[2], *myuid);
				}
			}
		}
	}

	node_t* newNode = list_AddNodeLast(&leaderStats->FOLLOWERS);
	newNode->deconstructor = &defaultDeconstructor;
	newNode->element = myuid;

	follower.monsterState = 0;
	follower.monsterTarget = 0;
	follower.monsterAllyIndex = -1;
	followerStats->leader_uid = leader.getUID();

	for ( node_t* node = leaderStats->FOLLOWERS.first; node != nullptr; node = node->next )
	{
		Uint32* c = (Uint32*)node->element;
		Entity* entity = nullptr;
		if ( c )
		{
			entity = uidToEntity(*c);
		}
		if ( entity && entity->monsterTarget == *myuid )
		{
			entity->monsterReleaseAttackTarget(); // followers stop punching the new target.
		}
	}

	int player = leader.isEntityPlayer();
    
    if (player >= 0 && player < MAXPLAYERS && ((followerStats->type == HUMAN || followerStats->type == SLIME) || followerStats->name[0]))
    {
		// give us a random name (if necessary)
		if ( followerStats->type == HUMAN && 
			!followerStats->name[0] 
			&& !monsterNameIsGeneric(*followerStats)) {
			auto& names = followerStats->sex == FEMALE ?
				randomNPCNamesFemale : randomNPCNamesMale;
			const int choice = local_rng.uniform(0, (int)names.size() - 1);
			auto name = names[choice].c_str();
			size_t len = names[choice].size();
			stringCopy(followerStats->name, name, sizeof(Stat::name), len);
		}
		else if ( followerStats->type == SLIME
			&& !followerStats->name[0] )
		{
			std::string name = getMonsterLocalizedName(SLIME);
			stringCopy(followerStats->name, name.c_str(), sizeof(Stat::name), name.size());
		}
        
        // ... and a nametag
		if (!monsterNameIsGeneric(*followerStats) || followerStats->type == SLIME) {
			if (player == clientnum || splitscreen) {
				Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
				nametag->x = follower.x;
				nametag->y = follower.y;
				nametag->z = follower.z - 6;
				nametag->sizex = 1;
				nametag->sizey = 1;
				nametag->flags[NOUPDATE] = true;
				nametag->flags[PASSABLE] = true;
				nametag->flags[SPRITE] = true;
				nametag->flags[UNCLICKABLE] = true;
				nametag->flags[BRIGHT] = true;
				nametag->behavior = &actSpriteNametag;
				nametag->parent = follower.getUID();
				nametag->scalex = 0.2;
				nametag->scaley = 0.2;
				nametag->scalez = 0.2;
				nametag->skill[0] = player;
				nametag->skill[1] = playerColor(player, colorblind_lobby, true);
			}
		}
    }
    
	if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
	{
		//Tell the client he suckered somebody into his cult.
		strcpy((char*) (net_packet->data), "LEAD");
		SDLNet_Write32((Uint32 )follower.getUID(), &net_packet->data[4]);
		std::string name = followerStats->name;
		if ( name != "" && name == MonsterData_t::getSpecialNPCName(*followerStats) )
		{
			name = followerStats->getAttribute("special_npc");
			name.insert(0, "$");
		}
        SDLNet_Write32(followerStats->type, &net_packet->data[8]);
		strcpy((char*)(&net_packet->data[12]), name.c_str());
		net_packet->data[12 + strlen(name.c_str())] = 0;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 12 + (int)strlen(name.c_str()) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		serverUpdateAllyStat(player, follower.getUID(), followerStats->LVL, followerStats->HP, followerStats->MAXHP, followerStats->type);
	}

	if ( player >= 0 )
	{
		if ( !FollowerMenu[player].recentEntity && players[player]->isLocalPlayer() )
		{
			FollowerMenu[player].recentEntity = &follower;
		}
	}

	if ( player >= 0 )
	{
		if ( leaderStats->type != HUMAN && followerStats->type == HUMAN )
		{
			steamAchievementClient(player, "BARONY_ACH_PITY_FRIEND");
		}
		else if ( leaderStats->type == VAMPIRE && followerStats->type == VAMPIRE )
		{
			if ( !strncmp(followerStats->name, "young vampire", strlen("young vampire")) )
			{
				steamAchievementClient(player, "BARONY_ACH_YOUNG_BLOOD");
			}
		}
	}

	return true;
}

bool Entity::handleMonsterSpecialAttack(Stat* myStats, Entity* target, double dist, bool forceDeinit)
{
	int specialRoll = 0;
	node_t* node = nullptr;
	int enemiesNearby = 0;
	int bonusFromHP = 0;
	bool hasrangedweapon = this->hasRangedWeapon();

	if ( myStats != nullptr )
	{
		if ( myStats->type == LICH 
			|| myStats->type == DEVIL 
			|| myStats->type == SHOPKEEPER
			|| myStats->type == LICH_FIRE
			|| myStats->type == LICH_ICE )
		{
			// monster should attack after this function is called.
			return true;
		}

		if ( this->monsterSpecialTimer == 0 && !forceDeinit )
		{
			if ( myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] > 0 
				&& (monsterSpecialState == MONSTER_SPELLCAST_GENERIC || monsterSpecialState == MONSTER_SPELLCAST_GENERIC2) )
			{
				monsterSpecialState = 0;
				if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
				{
					node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						return true;
					}
					node = itemNodeInInventory(myStats, -1, MAGICSTAFF); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						return true;
					}
					else
					{
						monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
					}
				}
				return true;
			}

			switch ( myStats->type )
			{
				case BUGBEAR:
					break;
				case KOBOLD:
					if ( (hasrangedweapon && !(myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK)) || myStats->weapon == nullptr )
					{
						specialRoll = local_rng.rand() % 20;
						//messagePlayer(0, "Rolled: %d", specialRoll);
						if ( myStats->HP < myStats->MAXHP / 2 )
						{
							if ( (dist < 40 && specialRoll < 10) || (dist < 100 && specialRoll < 5) ) // 50%/25% chance
							{
								node = itemNodeInInventory(myStats, -1, SPELLBOOK);
								if ( node != nullptr )
								{
									swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_KOBOLD;
								}
							}
						}
						else if ( myStats->HP < (0.8 * myStats->MAXHP) )
						{
							if ( (dist < 40 && specialRoll < 5) || (dist < 100 && specialRoll < 2) ) // 25%/10% chance
							{
								node = itemNodeInInventory(myStats, -1, SPELLBOOK);
								if ( node != nullptr )
								{
									swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_KOBOLD;
								}
							}
						}
					}
					break;
				case SUCCUBUS:
					if ( monsterSpecialState == SUCCUBUS_CHARM )
					{
						// special handled in succubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SUCCUBUS_CHARM;
						break;
					}
					break;
				case CRYSTALGOLEM:
					specialRoll = local_rng.rand() % 20;
					enemiesNearby = numTargetsAroundEntity(this, STRIKERANGE, PI, MONSTER_TARGET_ENEMY);
					if ( enemiesNearby > 1 )
					{
						enemiesNearby = std::min(enemiesNearby, 4);
						if ( specialRoll < enemiesNearby * 2 ) // 10% for each enemy > 1, capped at 40%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
							break;
						}
					}		
					
					specialRoll = local_rng.rand() % 20;
					if ( myStats->HP > myStats->MAXHP * 0.8 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.6 )
					{
						if ( specialRoll < 3 ) // 15%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.4 )
					{
						if ( specialRoll < 4 ) // 20%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					break;
				case COCKATRICE:
					specialRoll = local_rng.rand() % 20;
					//specialRoll = 0;
					// check for paralyze first
					enemiesNearby = std::min(numTargetsAroundEntity(this, STRIKERANGE * 2, PI, MONSTER_TARGET_ENEMY), 4);
					
					if ( myStats->HP <= myStats->MAXHP * 0.5 )
					{
						bonusFromHP = 4; // +20% chance if on low health
					}
					if ( specialRoll < (enemiesNearby * 2 + bonusFromHP) ) // +10% for each enemy, capped at 40%
					{
						node = itemNodeInInventory(myStats, -1, SPELLBOOK);
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_STONE;
						}
						break;
					}

					// nothing selected, look for double attack.
					specialRoll = local_rng.rand() % 20;
					if ( myStats->HP > myStats->MAXHP * 0.8 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.6 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.4 )
					{
						if ( specialRoll < 3 ) // 15%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 4 ) // 20%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP <= myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					break;
				case SLIME:
					// spray magic
					if ( monsterSpecialState == SLIME_CAST )
					{
						// special handled in slimeChooseWeapon()
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SLIME_SPRAY;
						break;
					}
					break;
				case SPIDER:
					// spray web
					if ( dist < STRIKERANGE * 2 )
					{
						specialRoll = local_rng.rand() % 20;
						enemiesNearby = std::min(numTargetsAroundEntity(this, STRIKERANGE * 2, PI, MONSTER_TARGET_ENEMY), 4);
						//messagePlayer(0, "spider roll %d", specialRoll);
						if ( myStats->getAttribute("special_npc") != "" )
						{
							if ( myStats->HP <= myStats->MAXHP * 0.4 )
							{
								bonusFromHP = 20; // +100% chance
							}
							else if ( myStats->HP <= myStats->MAXHP * 1.0 )
							{
								bonusFromHP = 10; // +50% chance
							}
						}
						else 
						{
							if ( myStats->HP <= myStats->MAXHP * 0.4 )
							{
								bonusFromHP = 8; // +40% chance
							}
							else if ( myStats->HP <= myStats->MAXHP * 0.8 )
							{
								bonusFromHP = 4; // +20% chance
							}
						}
						if ( specialRoll < (enemiesNearby * 2 + bonusFromHP) ) // +10% for each enemy, capped at 40%
						{
							node = itemNodeInInventory(myStats, -1, SPELLBOOK);
							if ( node != nullptr )
							{
								monsterSpecialState = SPIDER_CAST;
								swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
								this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SPIDER_CAST;
								serverUpdateEntitySkill(this, 33); // for clients to handle animation
							}
							else
							{
								if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
								{
									monsterSpecialState = SPIDER_CAST;
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SPIDER_CAST;
									serverUpdateEntitySkill(this, 33); // for clients to handle animation
								}
							}
							break;
						}
					}
					break;
				case INSECTOID:
					if ( monsterSpecialState == INSECTOID_DOUBLETHROW_FIRST || monsterSpecialState == INSECTOID_DOUBLETHROW_SECOND )
					{
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INSECTOID_THROW;
						break;
					}

					// spray acid
					if ( dist < STRIKERANGE * 2 )
					{
						specialRoll = local_rng.rand() % 20;
						enemiesNearby = std::min(numTargetsAroundEntity(this, STRIKERANGE * 2, PI, MONSTER_TARGET_ENEMY), 4);
						//messagePlayer(0, "insectoid roll %d", specialRoll);
						if ( myStats->HP <= myStats->MAXHP * 0.8 )
						{
							bonusFromHP = 4; // +20% chance if on low health
						}
						if ( specialRoll < (enemiesNearby * 2 + bonusFromHP) ) // +10% for each enemy, capped at 40%
						{
							node = itemNodeInInventory(myStats, -1, SPELLBOOK);
							if ( node != nullptr )
							{
								monsterSpecialState = INSECTOID_ACID;
								swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
								this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INSECTOID_ACID;
								serverUpdateEntitySkill(this, 33); // for clients to handle animation
							}
							else
							{
								if ( myStats->weapon && itemCategory(myStats->weapon) == SPELLBOOK )
								{
									monsterSpecialState = INSECTOID_ACID;
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INSECTOID_ACID;
									serverUpdateEntitySkill(this, 33); // for clients to handle animation
								}
							}
							break;
						}
					}
					// throwing weapon special handled in insectoidChooseWeapon()
					break;
				case INCUBUS:
					if ( monsterSpecialState == INCUBUS_CONFUSION )
					{
						// throwing weapon special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_CONFUSION;
						break;
					}
					else if ( monsterSpecialState == INCUBUS_STEAL )
					{
						// special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_STEAL;
						break;
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT )
					{
						// special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_TELEPORT_TARGET;
						break;
					}
					else if ( monsterSpecialState == INCUBUS_CHARM )
					{
						// special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_CHARM;
						break;
					}
					break;
				case VAMPIRE:
					if ( monsterSpecialState == VAMPIRE_CAST_AURA )
					{
						// special handled in vampireChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_VAMPIRE_AURA;
					}
					else if ( monsterSpecialState == VAMPIRE_CAST_DRAIN )
					{
						// special handled in vampireChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_VAMPIRE_DRAIN;
					}
					break;
				case SHADOW:
					if ( monsterSpecialState == SHADOW_SPELLCAST )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_SPELLCAST;
					}
					else if ( monsterSpecialState == SHADOW_TELEPORT_ONLY )
					{
						// special handled in shadowChooseWeapon(), teleport code in path state.
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_TELEPORT;
						break;
					}
				case GOATMAN:
					if ( monsterSpecialState == GOATMAN_POTION )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOATMAN_DRINK;
					}
					else if ( monsterSpecialState == GOATMAN_THROW )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOATMAN_THROW;
					}
					break;
				default:
					break;
			}

			if ( monsterSpecialTimer > 0 )
			{
				monsterSpecialAttackUnequipSafeguard = (real_t)TICKS_PER_SECOND * 2;
			}
		}
		else if ( this->monsterSpecialTimer > 0 || forceDeinit )
		{
			bool shouldAttack = true;
			bool deinitSuccess = false;

			if ( myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] > 0 )
			{
				if ( monsterSpecialState == MONSTER_SPELLCAST_GENERIC )
				{
					monsterSpecialState = MONSTER_SPELLCAST_GENERIC2;
					return true;
				}
				else if ( monsterSpecialState == MONSTER_SPELLCAST_GENERIC2 || forceDeinit )
				{
					monsterSpecialState = 0;
					node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						monsterSpecialAttackUnequipSafeguard = (real_t)MONSTER_SPECIAL_SAFEGUARD_TIMER_BASE;
						return true;
					}
					node = itemNodeInInventory(myStats, -1, MAGICSTAFF); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						monsterSpecialAttackUnequipSafeguard = (real_t)MONSTER_SPECIAL_SAFEGUARD_TIMER_BASE;
						return true;
					}
					else
					{
						monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
					}
					monsterSpecialAttackUnequipSafeguard = (real_t)MONSTER_SPECIAL_SAFEGUARD_TIMER_BASE;
					return true;
				}
			}

			switch ( myStats->type )
			{
				case KOBOLD:
					node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
					}
					else
					{
						monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);	
					}
					deinitSuccess = true;
					break;
				case GNOME:
					break;
				case SUCCUBUS:
					if ( monsterSpecialState == SUCCUBUS_CHARM || forceDeinit )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						deinitSuccess = true;
					}
					break;
				case SLIME:
					if ( monsterSpecialState == SLIME_CAST || forceDeinit )
					{
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						shouldAttack = false;
						deinitSuccess = true;
					}
					break;
				case SPIDER:
					if ( monsterSpecialState == SPIDER_CAST || forceDeinit )
					{
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						shouldAttack = false;
						deinitSuccess = true;
					}
					break;
				case INSECTOID:
					if ( monsterSpecialState == INSECTOID_ACID || forceDeinit )
					{
						monsterSpecialState = 0;
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						deinitSuccess = true;
					}
					else if ( monsterSpecialState == INSECTOID_DOUBLETHROW_SECOND )
					{
						monsterSpecialState = 0;
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, THROWN);
						}
						shouldAttack = false;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						deinitSuccess = true;
					}
					break;
				case COCKATRICE:
					monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
					deinitSuccess = true;
					break;
				case INCUBUS:
					if ( monsterSpecialState == INCUBUS_CONFUSION )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, POTION);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					else if ( monsterSpecialState == INCUBUS_STEAL )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT_STEAL )
					{
						// this flag will be cleared in incubusChooseWeapon
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT )
					{
						// this flag will be cleared in incubusChooseWeapon
					}
					else if ( monsterSpecialState == INCUBUS_CHARM || forceDeinit )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					break;
				case VAMPIRE:
					if ( monsterSpecialState == VAMPIRE_CAST_AURA || forceDeinit )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					else if ( monsterSpecialState == VAMPIRE_CAST_DRAIN )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					break;
				case SHADOW:
					if ( monsterSpecialState == SHADOW_SPELLCAST || forceDeinit ) //TODO: This code is destroying spells?
					{
						//TODO: Nope, this code isn't destroying spells. Something *before* this code is.
						//messagePlayer(clientnum, "[DEBUG: handleMonsterSpecialAttack()] Resolving shadow's spellcast.");
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						/*Item *spellbook = newItem(static_cast<ItemType>(0), static_cast<Status>(0), 0, 1, local_rng.rand(), 0, &myStats->inventory);
						copyItem(spellbook, myStats->weapon);
						dropItemMonster(myStats->weapon, this, myStats, 1);*/
						shouldAttack = false;
						monsterSpecialState = 0;
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_SPELLCAST;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					break;
				case GOATMAN:
					if ( monsterSpecialState == GOATMAN_POTION || forceDeinit )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, POTION);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					else if ( monsterSpecialState == GOATMAN_THROW )
					{
						node = itemNodeInInventory(myStats, -1, WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, THROWN);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
						deinitSuccess = true;
					}
					break;
				default:
					break;
			}

			if ( deinitSuccess )
			{
				monsterSpecialAttackUnequipSafeguard = (real_t)MONSTER_SPECIAL_SAFEGUARD_TIMER_BASE;
			}

			// Whether monster should attack following the unequip action.
			return shouldAttack;
		}
	}
	// monster should attack after this function is called.
	return true;
}

void getTargetsAroundEntity(Entity* my, Entity* originalTarget, double distToFind, real_t angleToSearch, int searchType, list_t** list)
{
	Entity* entity = nullptr;
	node_t* node = nullptr;
	node_t* node2 = nullptr;

	// aoe
	for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only looks at monsters and players, don't iterate all entities (map.entities).
	{
		entity = (Entity*)node->element;
		if ( (entity->behavior == &actMonster || entity->behavior == &actPlayer) && entity != originalTarget && entity != my )
		{
			if ( searchType == MONSTER_TARGET_ENEMY )
			{
				if ( !my->checkEnemy(entity) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_FRIEND )
			{
				if ( !my->checkFriend(entity) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_PLAYER )
			{
				if ( !(entity->behavior == &actPlayer) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_ALL )
			{
			}

			double aoeTangent = atan2(entity->y - my->y, entity->x - my->x);
			real_t angle = my->yaw - aoeTangent;
			while ( angle >= PI )
			{
				angle -= PI * 2;
			}
			while ( angle < -PI )
			{
				angle += PI * 2;
			}
			if ( abs(angle) <= angleToSearch ) // searches in 2x the given angle, +/- from yaw.
			{
				double dist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
				if ( dist < distToFind )
				{
					//If this is the first entity found, the list needs to be created.
					if ( !(*list) )
					{
						*list = (list_t*)malloc(sizeof(list_t));
						(*list)->first = nullptr;
						(*list)->last = nullptr;
					}
					node2 = list_AddNodeLast(*list);
					node2->element = entity;
					node2->deconstructor = &emptyDeconstructor;
					node2->size = sizeof(Entity*);
				}
			}
		}
	}
	return;
}

int numTargetsAroundEntity(Entity* my, double distToFind, real_t angleToSearch, int searchType)
{
	list_t* aoeTargets = nullptr;
	int count = 0;
	getTargetsAroundEntity(my, nullptr, distToFind, angleToSearch, searchType, &aoeTargets);
	if ( aoeTargets )
	{
		count = list_Size(aoeTargets);
		//messagePlayer(0, "found %d targets", count);
		//Free the list.
		list_FreeAll(aoeTargets);
		free(aoeTargets);
	}
	return count;
}

bool handleMonsterChatter(int monsterclicked, bool ringconflict, char namesays[64], Entity* my, Stat* myStats)
{
	if ( !my )
	{
		return false;
	}
	if ( ringconflict || myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 )
	{
		//Instant fail if ring of conflict is in effect/not NPC
		return false;
	}

	int NPCtype = myStats->MISC_FLAGS[STAT_FLAG_NPC] & 0xFF; // get NPC type, lowest 8 bits.
	int NPClastLines[MAXPLAYERS];
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		NPClastLines[i] = 0xF & ((myStats->MISC_FLAGS[STAT_FLAG_NPC] & 0xFFFF00) >> (8 + i * 4)); // get last line said, next 16 bits.	
	}
	int& NPClastLine = NPClastLines[monsterclicked];

	int numLines = 0;
	int startLine = 2700 + (NPCtype - 1) * MONSTER_NPC_DIALOGUE_LINES; // lang line to start from.
	int currentLine = startLine + 1;

	bool isSequential = false;

	if ( !strcmp(Language::get(startLine), "type:seq") )
	{
		isSequential = true;
	}

	for ( int i = 1; i < MONSTER_NPC_DIALOGUE_LINES; ++i )
	{
		// find the next 9 lines if available.
		if ( !strcmp(Language::get(currentLine), "") )
		{
			break;
		}
		++currentLine;
		++numLines;
	}

	// choose a dialogue line.
	if ( numLines > 0 )
	{
		if ( isSequential )
		{
			// say the next line in series.
			if ( NPClastLine != 0 )
			{
				++NPClastLine;
				if ( (NPClastLine) > numLines )
				{
					// reset to beginning
					NPClastLine = 1;
				}
			}
			else
			{
				// first line being said, choose the first.
				NPClastLine = 1;
			}
		}
		else if ( !isSequential )
		{
			// choose randomly
			NPClastLine = 1 + local_rng.rand() % numLines;
		}
		//messagePlayer(monsterclicked, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(startLine + NPClastLine), namesays, stats[monsterclicked]->name);
		players[monsterclicked]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
			Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_NPC,
			Language::get(startLine + NPClastLine), namesays, stats[monsterclicked]->name);
		myStats->MISC_FLAGS[STAT_FLAG_NPC] = NPCtype;
		Uint32 data = 0;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			data |= ((0xF & NPClastLines[i]) << i * 4);
		}
		data = data << 8;
		myStats->MISC_FLAGS[STAT_FLAG_NPC] |= data;
	}
	return true;
}

int numMonsterTypeAliveOnMap(Monster creature, Entity*& lastMonster)
{
	node_t* node = nullptr;
	Entity* entity = nullptr;
	int monsterCount = 0;
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		entity = (Entity*)node->element;
		if ( entity )
		{
			if ( entity->getRace() == creature )
			{
				lastMonster = entity;
				++monsterCount;
			}
		}
	}
	return monsterCount;
}

void Entity::monsterMoveBackwardsAndPath(bool trySidesFirst)
{
	while ( yaw < 0 )
	{
		yaw += 2 * PI;
	}
	while ( yaw >= 2 * PI )
	{
		yaw -= 2 * PI;
	}
	int x1 = ((int)floor(x)) >> 4;
	int y1 = ((int)floor(y)) >> 4;
	int u, v;
	std::vector<std::pair<int, int>> areaToTry;
	if ( yaw <= PI / 4 || yaw > 7 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 3, y1));
	}
	else if ( yaw > PI / 4 && yaw <= 3 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 3));
	}
	else if ( yaw > 3 * PI / 4 && yaw <= 5 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 3, y1));
	}
	else if ( yaw > 5 * PI / 4 && yaw <= 7 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 3));
	}
	bool foundplace = false;

	if ( trySidesFirst )
	{
		std::vector<std::pair<int, int>> sidesToTry;
		sidesToTry.push_back(areaToTry[1]);
		areaToTry.erase(areaToTry.begin() + 1);
		sidesToTry.push_back(areaToTry[1]);
		areaToTry.erase(areaToTry.begin() + 1);
		sidesToTry.push_back(areaToTry[2]);
		areaToTry.erase(areaToTry.begin() + 2);
		sidesToTry.push_back(areaToTry[2]);
		areaToTry.erase(areaToTry.begin() + 2);
		while ( sidesToTry.size() > 0 && !foundplace )
		{
			size_t index = 0;
			if ( sidesToTry.size() % 2 == 0 )
			{
				index = local_rng.rand() % 2;
			}
			else
			{
				index = 0;
			}

			std::pair<int, int> tmpPair = sidesToTry[index];
			u = tmpPair.first;
			v = tmpPair.second;
			if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
			{
				x1 = u;
				y1 = v;
				foundplace = true;
				break;
			}
			sidesToTry.erase(sidesToTry.begin() + index);
		}
	}
	while ( areaToTry.size() > 0 && !foundplace )
	{
		size_t index = local_rng.rand() % areaToTry.size();
		std::pair<int, int> tmpPair = areaToTry[index];
		u = tmpPair.first;
		v = tmpPair.second;
		if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
		{
			x1 = u;
			y1 = v;
			foundplace = true;
			break;
		}
		areaToTry.erase(areaToTry.begin() + index);
	}
	path = generatePath((int)floor(x / 16), (int)floor(y / 16), x1, y1, this, this,
		GeneratePathTypes::GENERATE_PATH_MONSTER_MOVE_BACKWARDS);
	if ( children.first != NULL )
	{
		list_RemoveNode(children.first);
	}
	node_t* node = list_AddNodeFirst(&this->children);
	node->element = path;
	node->deconstructor = &listDeconstructor;
	monsterState = MONSTER_STATE_HUNT; // hunt state
}

bool Entity::monsterHasLeader()
{
	Stat* myStats = this->getStats();
	if ( myStats )
	{
		if ( myStats->leader_uid != 0 )
		{
			return true;
		}
	}
	return false;
}

void Entity::monsterAllySendCommand(int command, int destX, int destY, Uint32 uid)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( command == -1 || command == ALLY_CMD_CANCEL || monsterAllyIndex == -1 || monsterAllyIndex > MAXPLAYERS )
	{
		return;
	}
	if ( !players[monsterAllyIndex] )
	{
		return;
	}
	if ( !players[monsterAllyIndex]->entity )
	{
		return;
	}
	if ( monsterTarget == players[monsterAllyIndex]->entity->getUID() )
	{
		// angry at owner.
		return;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( !isMobile() )
	{
		// doesn't respond.
		if ( monsterAllySpecial == ALLY_SPECIAL_CMD_REST && myStats->EFFECTS[EFF_ASLEEP]
			&& (command == ALLY_CMD_MOVETO_CONFIRM || command == ALLY_CMD_ATTACK_CONFIRM
				|| command == ALLY_CMD_MOVEASIDE) )
		{
			myStats->EFFECTS[EFF_ASLEEP] = false; // wake up
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
			myStats->EFFECTS[EFF_HP_REGEN] = false; // stop regen
			myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = 0;
			monsterAllySpecial = ALLY_SPECIAL_CMD_NONE;
		}
		else
		{
			messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF, *myStats, Language::get(514), Language::get(515), MSG_COMBAT);
		}
		return;
	}

	bool isTinkeringFollower = FollowerMenu[monsterAllyIndex].isTinkeringFollower(myStats->type);
	int tinkeringLVL = 0;
	int skillLVL = 0;
	if ( stats[monsterAllyIndex] )
	{
		tinkeringLVL = stats[monsterAllyIndex]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[monsterAllyIndex], players[monsterAllyIndex]->entity);
		skillLVL = stats[monsterAllyIndex]->getModifiedProficiency(PRO_LEADERSHIP) + statGetCHR(stats[monsterAllyIndex], players[monsterAllyIndex]->entity);
		if ( isTinkeringFollower )
		{
			skillLVL = tinkeringLVL;
		}
	}

	if ( myStats->type != GYROBOT )
	{
		if ( FollowerMenu[monsterAllyIndex].monsterGyroBotOnlyCommand(command) )
		{
			return;
		}
	}
	else if ( myStats->type == GYROBOT )
	{
		if ( FollowerMenu[monsterAllyIndex].monsterGyroBotDisallowedCommands(command) )
		{
			return;
		}
	}

	// do a final check if player can use this command.
	/*if ( FollowerMenu.optionDisabledForCreature(skillLVL, myStats->type, command) != 0 )
	{
		messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
			*myStats, Language::get(3638), Language::get(3639), MSG_COMBAT);
		return;
	}*/

	switch ( command )
	{
		case ALLY_CMD_RETURN_SOUL:
			if ( monsterAllySummonRank != 0 )
			{
				float manaToRefund = myStats->MAXMP * (myStats->HP / static_cast<float>(myStats->MAXHP));
				setMP(static_cast<int>(manaToRefund));
				setHP(0);
				if ( stats[monsterAllyIndex] && stats[monsterAllyIndex]->MP == 0 )
				{
					steamAchievementClient(monsterAllyIndex, "BARONY_ACH_EXTERNAL_BATTERY");
				}
			}
			break;
		case ALLY_CMD_ATTACK_CONFIRM:
			if ( uid != 0 && uid != getUID() )
			{
				Entity* target = uidToEntity(uid);
				if ( target )
				{
					if ( target->behavior == &actMonster || target->behavior == &actPlayer )
					{
						if ( stats[monsterAllyIndex] ) // check owner's proficiency.
						{
							if ( skillLVL >= SKILL_LEVEL_MASTER || myStats->type != HUMAN )
							{
								// attack anything except if FF is off + friend.
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( !checkFriend(target) )
									{
										monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
										handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK);
									}
									else
									{
										// messagePlayer(0, "Friendly fire is on!");
										handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK_FRIENDLY_FIRE);
									}
								}
								else
								{
									monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
									handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK);
								}
							}
							else if ( skillLVL >= AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM] )
							{
								if ( !checkFriend(target) )
								{
									monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
									handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK);
								}
								else
								{
									handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK_FRIENDLY_FIRE);
								}
							}
						}
						monsterAllyInteractTarget = 0;
					}
					else
					{
						monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
						monsterAllyState = ALLY_STATE_MOVETO;
					}
				}
			}
			break;
		case ALLY_CMD_MOVEASIDE:
			monsterMoveAside(this, players[monsterAllyIndex]->entity);
			handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVEASIDE);
			break;
		case ALLY_CMD_DEFEND:
			monsterAllyState = ALLY_STATE_DEFEND;
			if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
			{
				monsterSentrybotLookDir = monsterLookDir;
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_WAIT);
			}
			else
			{
				createPathBoundariesNPC(5);
				// stop in your tracks!
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_WAIT);
				monsterState = MONSTER_STATE_WAIT; // wait state
				serverUpdateEntitySkill(this, 0);
			}
			break;
		case ALLY_CMD_MOVETO_SELECT:
			break;
		case ALLY_CMD_FOLLOW:
			monsterAllyState = ALLY_STATE_DEFAULT;
			if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
			{
				monsterSentrybotLookDir = 0.0;
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_FOLLOW);
			}
			else
			{
				monsterAllyFormations.updateOnFollowCommand(getUID(), this);
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_FOLLOW);
			}
			break;
		case ALLY_CMD_CLASS_TOGGLE:
			++monsterAllyClass;
			if ( monsterAllyClass > ALLY_CLASS_RANGED )
			{
				monsterAllyClass = ALLY_CLASS_MIXED;
			}
			myStats->allyClass = monsterAllyClass;
			serverUpdateEntitySkill(this, 46);
			break;
		case ALLY_CMD_PICKUP_TOGGLE:
			++monsterAllyPickupItems;
			if ( monsterAllyPickupItems > ALLY_PICKUP_ALL )
			{
				monsterAllyPickupItems = ALLY_PICKUP_NONPLAYER;
			}
			myStats->allyItemPickup = monsterAllyPickupItems;
			serverUpdateEntitySkill(this, 44);
			break;
		case ALLY_CMD_GYRO_LIGHT_TOGGLE:
			++monsterAllyClass;
			if ( monsterAllyClass >= ALLY_GYRO_LIGHT_END )
			{
				monsterAllyClass = ALLY_GYRO_LIGHT_NONE;
			}
			myStats->allyClass = monsterAllyClass;
			serverUpdateEntitySkill(this, 46);
			break;
		case ALLY_CMD_GYRO_DETECT_TOGGLE:
		{
			++monsterAllyPickupItems;
			bool failQuality = false;
			bool failSkillRequirement = false;
			if ( monsterAllyPickupItems >= ALLY_GYRO_DETECT_END )
			{
				monsterAllyPickupItems = ALLY_GYRO_DETECT_NONE;
			}
			else if ( monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_VALUABLE )
			{
				if ( myStats->LVL < 15 )
				{
					failQuality = true;
				}
				if ( skillLVL < SKILL_LEVEL_MASTER )
				{
					failSkillRequirement = true;
				}
			}
			else if ( monsterAllyPickupItems == ALLY_GYRO_DETECT_MONSTERS )
			{
				if ( myStats->LVL < 10 )
				{
					failQuality = true;
				}
				if ( skillLVL < SKILL_LEVEL_EXPERT )
				{
					failSkillRequirement = true;
				}
			}
			else if ( monsterAllyPickupItems == ALLY_GYRO_DETECT_TRAPS
				|| monsterAllyPickupItems == ALLY_GYRO_DETECT_EXITS )
			{
				if ( myStats->LVL < 5 )
				{
					failQuality = true;
				}
				if ( skillLVL < SKILL_LEVEL_SKILLED )
				{
					failSkillRequirement = true;
				}
			}
			else if ( monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_METAL
				|| monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_MAGIC )
			{
				if ( skillLVL < SKILL_LEVEL_BASIC )
				{
					failSkillRequirement = true;
				}
			}

			if ( failSkillRequirement || failQuality )
			{
				if ( skillLVL < SKILL_LEVEL_BASIC )
				{
					messagePlayerColor(monsterAllyIndex, MESSAGE_STATUS, 0xFFFFFFFF, Language::get(3680));
				}
				else
				{
					messagePlayerColor(monsterAllyIndex, MESSAGE_STATUS, 0xFFFFFFFF, Language::get(3679));
				}
				monsterAllyPickupItems = ALLY_GYRO_DETECT_NONE;
			}
			myStats->allyItemPickup = monsterAllyPickupItems;
			serverUpdateEntitySkill(this, 44);
			break;
		}
		case ALLY_CMD_DROP_EQUIP:
			if ( strcmp(myStats->name, "") 
				&& myStats->getAttribute("special_npc") != ""
				&& myStats->type == HUMAN )
			{
				// named humans refuse to drop equipment.
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_HUMAN_REFUSE);
			}
			else if ( myStats->type == GYROBOT )
			{
				bool droppedSomething = false;
				node_t* nextnode = nullptr;
				for ( node_t* node = myStats->inventory.first; node; node = nextnode )
				{
					nextnode = node->next;
					Item* item = (Item*)node->element;
					if ( item )
					{
						if ( itemIsThrowableTinkerTool(item) && item->status > BROKEN )
						{
							int count = item->count;
							this->monsterEquipItem(*item, &myStats->weapon);
							this->attack(0, 0, nullptr);
							if ( count > 1 )
							{
								myStats->weapon = nullptr;
							}
							droppedSomething = true;
							break;
						}
						else
						{
							for ( int c = item->count; c > 0; --c )
							{
								Entity* dropped = dropItemMonster(item, this, myStats, item->count);
								if ( dropped )
								{
									c = 0;
									droppedSomething = true;
								}
							}
						}
					}
				}

				if ( !droppedSomething )
				{
					messagePlayer(monsterAllyIndex, MESSAGE_HINT, Language::get(3868));
				}
			}
			else if ( stats[monsterAllyIndex] )
			{
				Entity* dropped = nullptr;
				bool confirmDropped = false;
				bool dropWeaponOnly = false;
				bool unableToDrop = false;
				Uint32 owner = players[monsterAllyIndex]->entity->getUID();
				if ( skillLVL >= SKILL_LEVEL_MASTER )
				{
					if ( myStats->helmet )
					{
						if ( myStats->helmet->canUnequip(myStats) )
						{
							dropped = dropItemMonster(myStats->helmet, this, myStats);
						}
						else
						{
							unableToDrop = true;
						}
					}
					if ( dropped )
					{
						confirmDropped = true;
						dropped->itemOriginalOwner = owner;
					}
					if ( myStats->breastplate )
					{
						if ( myStats->breastplate->canUnequip(myStats) )
						{
							dropped = dropItemMonster(myStats->breastplate, this, myStats);
						}
						else
						{
							unableToDrop = true;
						}
					}
					if ( dropped )
					{
						confirmDropped = true;
						dropped->itemOriginalOwner = owner;
					}
					if ( myStats->shoes )
					{
						if ( myStats->shoes->canUnequip(myStats) )
						{
							dropped = dropItemMonster(myStats->shoes, this, myStats);
						}
						else
						{
							unableToDrop = true;
						}
					}
					if ( dropped )
					{
						confirmDropped = true;
						dropped->itemOriginalOwner = owner;
					}
					if ( myStats->shield )
					{
						if ( myStats->shield->canUnequip(myStats) )
						{
							dropped = dropItemMonster(myStats->shield, this, myStats, myStats->shield->count);
						}
						else
						{
							unableToDrop = true;
						}
					}
					if ( dropped )
					{
						confirmDropped = true;
						dropped->itemOriginalOwner = owner;
					}
					if ( myStats->cloak )
					{
						if ( myStats->cloak->canUnequip(myStats) )
						{
							dropped = dropItemMonster(myStats->cloak, this, myStats);
						}
						else
						{
							unableToDrop = true;
						}
					}
					if ( dropped )
					{
						confirmDropped = true;
						dropped->itemOriginalOwner = owner;
					}

					if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
					{
						if ( myStats->ring )
						{
							if ( myStats->ring->canUnequip(myStats) )
							{
								dropped = dropItemMonster(myStats->ring, this, myStats);
							}
							else
							{
								unableToDrop = true;
							}
						}
						if ( dropped )
						{
							confirmDropped = true;
							dropped->itemOriginalOwner = owner;
						}
						if ( myStats->amulet )
						{
							if ( myStats->amulet->canUnequip(myStats) )
							{
								dropped = dropItemMonster(myStats->amulet, this, myStats);
							}
							else
							{
								unableToDrop = true;
							}
						}
						if ( dropped )
						{
							confirmDropped = true;
							dropped->itemOriginalOwner = owner;
						}
						if ( confirmDropped )
						{
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_ALL);
						}
					}
					else
					{
						if ( confirmDropped )
						{
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_EQUIP);
						}
					}
				}
				else
				{
					if ( myStats->weapon )
					{
						dropWeaponOnly = true;
					}
				}
				if ( myStats->weapon )
				{
					if ( myStats->weapon->canUnequip(myStats) )
					{
						dropped = dropItemMonster(myStats->weapon, this, myStats, myStats->weapon->count);
					}
					else
					{
						unableToDrop = true;
					}
				}
				if ( dropped )
				{
					dropped->itemOriginalOwner = owner;
					if ( dropWeaponOnly )
					{
						handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_WEAPON);
					}
				}
				/*if ( unableToDrop )
				{
					handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_FAILED);
				}*/
			}
			break;
		case ALLY_CMD_MOVETO_CONFIRM:
		{
			if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
			{
				real_t floatx = destX * 16 + 8;
				real_t floaty = destY * 16 + 8;
				double tangent = atan2(floaty - y, floatx - x);
				monsterLookTime = 1;
				monsterMoveTime = local_rng.rand() % 10 + 1;
				monsterLookDir = tangent;
				monsterSentrybotLookDir = monsterLookDir;
				if ( monsterAllyState != ALLY_STATE_DEFEND )
				{
					handleNPCInteractDialogue(*myStats, ALLY_EVENT_WAIT);
				}
				monsterAllyState = ALLY_STATE_DEFEND;
			}
			else if ( monsterSetPathToLocation(destX, destY, 1,
				GeneratePathTypes::GENERATE_PATH_PLAYER_ALLY_MOVETO) )
			{
				monsterState = MONSTER_STATE_HUNT; // hunt state
				monsterAllyState = ALLY_STATE_MOVETO;
				serverUpdateEntitySkill(this, 0);
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_BEGIN);
			}
			else
			{
				//messagePlayer(0, "no path to destination");
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_FAIL);
			}
			break;
		}
		case ALLY_CMD_GYRO_RETURN:
		{
			if ( players[monsterAllyIndex]->entity )
			{
				destX = static_cast<int>(players[monsterAllyIndex]->entity->x) >> 4;
				destY = static_cast<int>(players[monsterAllyIndex]->entity->y) >> 4;

				bool noground = false;
				const int monsterX = static_cast<int>(x) >> 4;
				const int monsterY = static_cast<int>(y) >> 4;
				if ( monsterX >= 0 && monsterX < map.width )
				{
					if ( monsterY >= 0 && monsterY < map.height )
					{
						int index = (monsterY) * MAPLAYERS + (monsterX) * MAPLAYERS * map.height;
						noground = !map.tiles[index];
					}
				}

				if ( entityDist(this, players[monsterAllyIndex]->entity) < STRIKERANGE
					&& !noground )
				{
					monsterSpecialState = GYRO_RETURN_LANDING;
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					playSoundEntity(this, 449, 128);
				}
				else if ( gyrobotSetPathToReturnLocation(destX, destY, 0) )
				{
					monsterState = MONSTER_STATE_HUNT; // hunt state
					monsterAllyState = ALLY_STATE_MOVETO;
					serverUpdateEntitySkill(this, 0);
					handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_BEGIN);
					monsterSpecialState = GYRO_RETURN_PATHING;
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
				}
				else if ( gyrobotSetPathToReturnLocation(destX, destY, 2) ) // expand search
				{
					monsterState = MONSTER_STATE_HUNT; // hunt state
					monsterAllyState = ALLY_STATE_MOVETO;
					serverUpdateEntitySkill(this, 0);
					handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_BEGIN);
					monsterSpecialState = GYRO_RETURN_PATHING;
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
				}
				else
				{
					//messagePlayer(0, "no path to destination");
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFF, *myStats, Language::get(4317), Language::get(4317), MSG_GENERIC);
				}
			}
			break;
		}
		case ALLY_CMD_SPECIAL:
		{
			if ( monsterAllySpecialCooldown == 0 )
			{
				int duration = TICKS_PER_SECOND * (60);
				if ( myStats->HP < myStats->MAXHP && setEffect(EFF_ASLEEP, true, duration, false) ) // 60 seconds of sleep.
				{
					setEffect(EFF_HP_REGEN, true, duration, false);
					monsterAllySpecial = ALLY_SPECIAL_CMD_REST;
					monsterAllySpecialCooldown = -1; // locked out until next floor.
					serverUpdateEntitySkill(this, 49);
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFF, *myStats, Language::get(398), Language::get(397), MSG_COMBAT);
					if ( players[monsterAllyIndex] && players[monsterAllyIndex]->entity 
						&& myStats->HP < myStats->MAXHP && local_rng.rand() % 3 == 0 )
					{
						players[monsterAllyIndex]->entity->increaseSkill(PRO_LEADERSHIP);
					}
				}
				else
				{
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFF, *myStats, Language::get(3880), Language::get(3880), MSG_GENERIC);
				}
			}
			break;
		}
		case ALLY_CMD_DUMMYBOT_RETURN:
			if ( myStats->type == DUMMYBOT )
			{
				monsterSpecialState = DUMMYBOT_RETURN_FORM;
				serverUpdateEntitySkill(this, 33);
			}
			else if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT )
			{
				monsterSpecialState = DUMMYBOT_RETURN_FORM;
				serverUpdateEntitySkill(this, 33);
				playSoundEntity(this, 469 + local_rng.rand() % 3, 92);
			}
			break;
		default:
			break;
	}
	serverUpdateEntitySkill(this, 43);
	//messagePlayer(0, "received: %d", command);
}

bool Entity::monsterAllySetInteract()
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}
	if ( monsterAllyInteractTarget == 0 )
	{
		return false;
	}
	Entity* target = uidToEntity(monsterAllyInteractTarget);
	if ( !target )
	{
		monsterAllyState = ALLY_STATE_DEFAULT;
		monsterAllyInteractTarget = 0;
		return false;
	}
	// check distance to interactable.
	double range = pow(y - target->y, 2) + pow(x - target->x, 2);
	if ( range < 576 ) // 24 squared
	{
		if ( getMonsterTypeFromSprite() == GYROBOT 
			&& monsterSpecialState != GYRO_INTERACT_LANDING
			&& z < -0.1 )
		{
			// don't set interact yet.
			return true;
		}
		else
		{
			if ( monsterAllyIndex >= 0 )
			{
				FollowerMenu[monsterAllyIndex].entityToInteractWith = target; // set followerInteractedEntity to the mechanism/item/gold etc.
				FollowerMenu[monsterAllyIndex].entityToInteractWith->interactedByMonster = getUID(); // set the remote entity to this monster's uid to lookup later.
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool Entity::isInteractWithMonster()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !players[i]->isLocalPlayer() && client_disconnected[i] )
		{
			continue;
		}
		if ( FollowerMenu[i].entityToInteractWith == nullptr )
		{
			// entity is not set to interact with any monster.
		}
		else
		{
			if ( FollowerMenu[i].entityToInteractWith->interactedByMonster == 0 )
			{
				// recent monster is not set to interact.
			}
			else if ( FollowerMenu[i].entityToInteractWith == this )
			{
				return true; // a monster is set to interact with myself.
			}
		}
	}
	return false;
}

void Entity::clearMonsterInteract()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( FollowerMenu[i].entityToInteractWith == this )
		{
			FollowerMenu[i].entityToInteractWith = nullptr;
		}
	}
	interactedByMonster = 0;
}

bool Entity::monsterSetPathToLocation(int destX, int destY, int adjacentTilesToCheck, int pathingType, bool tryRandomSpot)
{
	int u, v;
	bool foundplace = false;
	int pathToX = destX;
	int pathToY = destY;

	if ( static_cast<int>(x / 16) == destX && static_cast<int>(y / 16) == destY )
	{
		monsterAllyFormations.updateOnPathSucceed(getUID(), this);
		return true; // we're trying to move to the spot we're already at!
	}
	else if ( !checkObstacle((destX << 4) + 8, (destY << 4) + 8, this, nullptr) )
	{
		if ( !tryRandomSpot )
		{
			foundplace = true; // we can path directly to the destination specified.
		}
	}

	std::vector<std::pair<int, std::pair<int, int>>> possibleDestinations; // store distance and the x, y coordinates in each element.

	if ( !foundplace )
	{
		for ( u = destX - adjacentTilesToCheck; u <= destX + adjacentTilesToCheck; u++ )
		{
			for ( v = destY - adjacentTilesToCheck; v <= destY + adjacentTilesToCheck; v++ )
			{
				if ( static_cast<int>(x / 16) == u && static_cast<int>(y / 16) == v )
				{
					// we're trying to move to the spot we're already at!
				}
				else if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
				{
					int distance = pow(x / 16 - u, 2) + pow(y / 16 - v, 2);
					possibleDestinations.push_back(std::make_pair(distance, std::make_pair(u, v)));
				}
			}
		}
	}

	if ( !possibleDestinations.empty() )
	{
		// sort by distance from monster, first result is shortest path.
		std::sort(possibleDestinations.begin(), possibleDestinations.end());
		pathToX = possibleDestinations.at(0).second.first;
		pathToY = possibleDestinations.at(0).second.second;
		foundplace = true;
	}

	path = generatePath(static_cast<int>(floor(x / 16)), static_cast<int>(floor(y / 16)), pathToX, pathToY, 
		this, nullptr, (GeneratePathTypes)pathingType);
	if ( children.first != NULL )
	{
		list_RemoveNode(children.first);
	}
	node_t* node = list_AddNodeFirst(&children);
	node->element = path;
	node->deconstructor = &listDeconstructor;

	if ( path == nullptr || !foundplace )
	{
		return false;
	}
	return true;
}

bool Entity::gyrobotSetPathToReturnLocation(int destX, int destY, int adjacentTilesToCheck, bool tryRandomSpot)
{
	int u, v;
	bool foundplace = false;
	int pathToX = destX;
	int pathToY = destY;

	if ( static_cast<int>(x / 16) == destX && static_cast<int>(y / 16) == destY )
	{
		return true; // we're trying to move to the spot we're already at!
	}
	else if ( !checkObstacle((destX << 4) + 8, (destY << 4) + 8, this, nullptr) )
	{
		int index = (destY)* MAPLAYERS + (destX)* MAPLAYERS * map.height;
		if ( !tryRandomSpot && map.tiles[index] )
		{
			foundplace = true; // we can path directly to the destination specified.
		}
	}

	std::vector<std::pair<int, std::pair<int, int>>> possibleDestinations; // store distance and the x, y coordinates in each element.

	if ( !foundplace )
	{
		for ( u = destX - adjacentTilesToCheck; u <= destX + adjacentTilesToCheck; u++ )
		{
			for ( v = destY - adjacentTilesToCheck; v <= destY + adjacentTilesToCheck; v++ )
			{
				if ( static_cast<int>(x / 16) == u && static_cast<int>(y / 16) == v )
				{
					// we're trying to move to the spot we're already at!
				}
				else if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
				{
					int index = (v) * MAPLAYERS + (u) * MAPLAYERS * map.height;
					if ( !map.tiles[index] )
					{
						continue; // bad spot to land
					}
					int distance = pow(destX - u, 2) + pow(destY - v, 2);
					possibleDestinations.push_back(std::make_pair(distance, std::make_pair(u, v)));
				}
			}
		}
	}

	if ( !possibleDestinations.empty() )
	{
		// sort by distance from monster, first result is shortest path.
		std::sort(possibleDestinations.begin(), possibleDestinations.end());
		pathToX = possibleDestinations.at(0).second.first;
		pathToY = possibleDestinations.at(0).second.second;
		foundplace = true;
	}

	path = generatePath(static_cast<int>(floor(x / 16)), static_cast<int>(floor(y / 16)), pathToX, pathToY, 
		this, nullptr, GeneratePathTypes::GENERATE_PATH_PLAYER_GYRO_RETURN);
	if ( children.first != NULL )
	{
		list_RemoveNode(children.first);
	}
	node_t* node = list_AddNodeFirst(&children);
	node->element = path;
	node->deconstructor = &listDeconstructor;

	if ( path == nullptr || !foundplace )
	{
		return false;
	}
	return true;
}

void Entity::handleNPCInteractDialogue(Stat& myStats, AllyNPCChatter event)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( !isMobile() )
	{
		return;
	}
	if ( monsterAllyIndex < 0 || monsterAllyIndex >= MAXPLAYERS )
	{
		return;
	}
	if ( !stats[monsterAllyIndex] )
	{
		return;
	}

	std::string message;

	if ( myStats.type == HUMAN )
	{
		switch ( event )
		{
			case ALLY_EVENT_MOVEASIDE:
				message = Language::get(535);
				break;
			case ALLY_EVENT_MOVETO_BEGIN:
				if ( local_rng.rand() % 10 == 0 )
				{
					message = Language::get(3079 + local_rng.rand() % 2);
				}
				break;
			case ALLY_EVENT_MOVETO_FAIL:
				message = Language::get(3077 + local_rng.rand() % 2);
				break;
			case ALLY_EVENT_INTERACT_ITEM_CURSED:
				if ( FollowerMenu[monsterAllyIndex].entityToInteractWith && FollowerMenu[monsterAllyIndex].entityToInteractWith->behavior == &actItem )
				{
					Item* item = newItemFromEntity(FollowerMenu[monsterAllyIndex].entityToInteractWith);
					if ( item )
					{
						char fullmsg[256];
						switch ( itemCategory(item) )
						{
							case WEAPON:
							case MAGICSTAFF:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3107));
								break;
							case ARMOR:
							case TOOL:
								switch ( checkEquipType(item) )
								{
									case TYPE_OFFHAND:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3112));
										break;
									case TYPE_HELM:
									case TYPE_HAT:
									case TYPE_BREASTPIECE:
									case TYPE_BOOTS:
									case TYPE_SHIELD:
									case TYPE_GLOVES:
									case TYPE_CLOAK:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3107 + checkEquipType(item)));
										break;
									default:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3117));
										break;
								}
								break;
							case RING:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3107 + TYPE_RING));
								break;
							case AMULET:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3071), Language::get(3107 + TYPE_AMULET));
								break;
							default:
								break;
						}
						message = fullmsg;
					}
					if ( item )
					{
						free(item);
					}
				}
				break;
			case ALLY_EVENT_INTERACT_ITEM_NOUSE:
				message = Language::get(3074);
				break;
			case ALLY_EVENT_INTERACT_ITEM_FOOD_BAD:
				message = Language::get(3088);
				break;
			case ALLY_EVENT_INTERACT_ITEM_FOOD_GOOD:
				if ( myStats.HUNGER > 800 )
				{
					message = Language::get(3076);
				}
				else
				{
					message = Language::get(3075);
				}
				break;
			case ALLY_EVENT_INTERACT_ITEM_FOOD_ROTTEN:
				message = Language::get(3091);
				break;
			case ALLY_EVENT_INTERACT_ITEM_FOOD_FULL:
				message = Language::get(3089 + local_rng.rand() % 2);
				break;
			case ALLY_EVENT_INTERACT_OTHER:
				break;
			case ALLY_EVENT_ATTACK:
			case ALLY_EVENT_SPOT_ENEMY: {
				auto target = uidToEntity(monsterTarget);
				if (!target) {
					break;
				}
				auto targetstats = target->getStats();
				if (!targetstats) {
					break;
				}
				if ( local_rng.getU8() % 2 == 0 )
				{
					return; // random to not chat
				}
				char fullmsg[256] = "";
				if (local_rng.getU8() % 2) {
					snprintf(fullmsg, sizeof(fullmsg), Language::get(516 + local_rng.uniform(0, 1)),
						Language::get(4234 + local_rng.uniform(0, 16)), getMonsterLocalizedName(targetstats->type).c_str());
				} else {
					snprintf(fullmsg, sizeof(fullmsg), Language::get(518 + local_rng.uniform(0, 1)),
						Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(targetstats->type).c_str());
				}
				message = fullmsg;
				break;
			}
			case ALLY_EVENT_ATTACK_FRIENDLY_FIRE:
				message = Language::get(3084 + local_rng.rand() % 2);
				break;
			case ALLY_EVENT_DROP_HUMAN_REFUSE:
				message = Language::get(3135);
				break;
			case ALLY_EVENT_DROP_WEAPON:
			case ALLY_EVENT_DROP_EQUIP:
			case ALLY_EVENT_DROP_ALL:
				if ( local_rng.rand() % 2 )
				{
					if ( local_rng.rand() % 2 && event == ALLY_EVENT_DROP_ALL )
					{
						message = Language::get(3083);
					}
					else
					{
						message = Language::get(3072 + local_rng.rand() % 2);
					}
				}
				else
				{
					message = Language::get(3081 + local_rng.rand() % 2);
				}
				break;
			case ALLY_EVENT_WAIT:
				message = Language::get(3069 + local_rng.rand() % 2);
				break;
			case ALLY_EVENT_FOLLOW:
				message = Language::get(526 + local_rng.rand() % 3);
				break;
			case ALLY_EVENT_MOVETO_REPATH:
				if ( local_rng.rand() % 20 == 0 )
				{
					message = Language::get(3086 + local_rng.rand() % 2);
				}
				break;
			default:
				break;
		}
		if (!message.empty())
		{
			if ( event == ALLY_EVENT_ATTACK || event == ALLY_EVENT_SPOT_ENEMY )
			{
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
						Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, message.c_str(), stats[monsterAllyIndex]->name);
				}
			}
			else
			{
				players[monsterAllyIndex]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
					Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_FOLLOWER_CMD, message.c_str(), stats[monsterAllyIndex]->name);
			}
		}
	}
	else
	{
		char genericStr[128] = "";
		char namedStr[128] = "";

		switch ( event )
		{
			case ALLY_EVENT_MOVEASIDE:
				if ( local_rng.getU8() % 8 == 0 ) 
				{
					players[monsterAllyIndex]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
						Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_FOLLOWER_CMD, Language::get(getMonsterInteractGreeting(myStats)), stats[monsterAllyIndex]->name);
					//messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
					//	myStats, Language::get(3129), Language::get(3130), MSG_COMBAT);
				}
				break;
			case ALLY_EVENT_MOVETO_BEGIN:
				/*if ( local_rng.rand() % 10 == 0 )
				{
					message = Language::get(3079 + local_rng.rand() % 2);
				}*/
				break;
			case ALLY_EVENT_MOVETO_FAIL:
				if ( local_rng.rand() % 2 == 0 )
				{
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, Language::get(3131), Language::get(3132), MSG_COMBAT);
				}
				else
				{
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, Language::get(3133), Language::get(3134), MSG_COMBAT);
				}
				break;
			case ALLY_EVENT_INTERACT_ITEM_CURSED:
				if ( FollowerMenu[monsterAllyIndex].entityToInteractWith && FollowerMenu[monsterAllyIndex].entityToInteractWith->behavior == &actItem )
				{
					Item* item = newItemFromEntity(FollowerMenu[monsterAllyIndex].entityToInteractWith);
					if ( item )
					{
						char fullmsg[256] = "";
						switch ( itemCategory(item) )
						{
							case WEAPON:
							case MAGICSTAFF:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3107));
								break;
							case ARMOR:
							case TOOL:
								switch ( checkEquipType(item) )
								{
									case TYPE_OFFHAND:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3112));
										break;
									case TYPE_HELM:
									case TYPE_HAT:
									case TYPE_BREASTPIECE:
									case TYPE_BOOTS:
									case TYPE_SHIELD:
									case TYPE_GLOVES:
									case TYPE_CLOAK:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3107 + checkEquipType(item)));
										break;
									default:
										snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3117));
										break;
								}
								break;
							case RING:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3107 + TYPE_RING));
								break;
							case AMULET:
								snprintf(fullmsg, sizeof(fullmsg), Language::get(3118), Language::get(3107 + TYPE_AMULET));
								break;
							default:
								break;
						}
						if ( strcmp(fullmsg, "") )
						{
							char genericStr[128];
							strcpy(genericStr, Language::get(3119));
							strcat(genericStr, fullmsg);
							char namedStr[128];
							strcpy(namedStr, Language::get(3120));
							strcat(namedStr, fullmsg);
							messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
								myStats, genericStr, namedStr, MSG_COMBAT);
						}
					}
					if ( item )
					{
						free(item);
					}
				}
				break;
			case ALLY_EVENT_INTERACT_ITEM_NOUSE:
			case ALLY_EVENT_INTERACT_ITEM_FOOD_FULL:
			{
				Item* item = newItemFromEntity(FollowerMenu[monsterAllyIndex].entityToInteractWith);
				if ( item )
				{
					char itemString[64] = "";
					snprintf(itemString, 63, Language::get(3123), items[item->type].getUnidentifiedName());
					strcpy(genericStr, Language::get(3121));
					strcat(genericStr, itemString);
					strcpy(namedStr, Language::get(3122));
					strcat(namedStr, itemString);
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, genericStr, namedStr, MSG_COMBAT);
					free(item);
				}
				break;
			}
			case ALLY_EVENT_INTERACT_ITEM_FOOD_BAD:
			case ALLY_EVENT_INTERACT_ITEM_FOOD_GOOD:
			case ALLY_EVENT_INTERACT_ITEM_FOOD_ROTTEN:
			{
				Item* item = newItemFromEntity(FollowerMenu[monsterAllyIndex].entityToInteractWith);
				if ( item )
				{
					char itemString[64] = "";
					snprintf(itemString, 63, Language::get(3124), items[item->type].getUnidentifiedName());
					strcpy(genericStr, Language::get(3121));
					strcat(genericStr, itemString);
					strcpy(namedStr, Language::get(3122));
					strcat(namedStr, itemString);
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, genericStr, namedStr, MSG_COMBAT);
					free(item);
				}
				break;
			}
			case ALLY_EVENT_INTERACT_OTHER:
				break;
			case ALLY_EVENT_ATTACK:
			case ALLY_EVENT_SPOT_ENEMY:
				break;
			case ALLY_EVENT_ATTACK_FRIENDLY_FIRE:
				break;
			case ALLY_EVENT_DROP_WEAPON:
				strcpy(genericStr, Language::get(3121));
				strcat(genericStr, Language::get(3125));
				strcpy(namedStr, Language::get(3122));
				strcat(namedStr, Language::get(3125));
				messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
					myStats, genericStr, namedStr, MSG_COMBAT);
				break;
			case ALLY_EVENT_DROP_EQUIP:
				strcpy(genericStr, Language::get(3121));
				strcat(genericStr, Language::get(3126));
				strcpy(namedStr, Language::get(3122));
				strcat(namedStr, Language::get(3126));
				messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
					myStats, genericStr, namedStr, MSG_COMBAT);
				break;
			case ALLY_EVENT_DROP_ALL:
				strcpy(genericStr, Language::get(3121));
				strcat(genericStr, Language::get(3127));
				strcpy(namedStr, Language::get(3122));
				strcat(namedStr, Language::get(3127));
				messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
					myStats, genericStr, namedStr, MSG_COMBAT);
				break;
			case ALLY_EVENT_WAIT:
				if ( myStats.type == SENTRYBOT || myStats.type == SPELLBOT )
				{
					messagePlayerColor(monsterAllyIndex, MESSAGE_STATUS, 0xFFFFFFFF,
					    Language::get(3676), monstertypename[myStats.type]);
				}
				else
				{
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, Language::get(3105), Language::get(3106), MSG_COMBAT);
				}
				break;
			case ALLY_EVENT_FOLLOW:
				if ( myStats.type == SENTRYBOT || myStats.type == SPELLBOT )
				{
					messagePlayerColor(monsterAllyIndex, MESSAGE_STATUS, 0xFFFFFFFF,
					    Language::get(3677), monstertypename[myStats.type]);
				}
				else
				{
					messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF,
						myStats, Language::get(529), Language::get(3128), MSG_COMBAT);
				}
				break;
			case ALLY_EVENT_MOVETO_REPATH:
				/*if ( local_rng.rand() % 20 == 0 )
				{
					message = Language::get(3086 + local_rng.rand() % 2);
				}*/
				break;
			default:
				break;
		}
	}
}

int Entity::shouldMonsterDefend(Stat& myStats, const Entity& target, const Stat& targetStats, int targetDist, bool hasrangedweapon)
{
	if ( behavior != &actMonster )
	{
		return MONSTER_DEFEND_NONE;
	}

	if ( !myStats.shield )
	{
		return MONSTER_DEFEND_NONE;
	}

	if ( itemTypeIsQuiver(myStats.shield->type) )
	{
		return MONSTER_DEFEND_NONE;
	}

	if ( monsterSpecialState > 0 && !(myStats.type == BUGBEAR && monsterSpecialState == BUGBEAR_DEFENSE) )
	{
		return MONSTER_DEFEND_NONE;
	}

	if ( myStats.type == LICH
		|| myStats.type == DEVIL
		|| myStats.type == LICH_ICE
		|| myStats.type == LICH_FIRE
		|| myStats.type == SHOPKEEPER
		)
	{
		return MONSTER_DEFEND_NONE;
	}

	bool isPlayerAlly = (monsterAllyIndex >= 0 && monsterAllyIndex < MAXPLAYERS);
	
	if ( !(isPlayerAlly || myStats.type == HUMAN || myStats.type == BUGBEAR) )
	{
		return MONSTER_DEFEND_NONE;
	}
	
	int blockChance = 2; // 10%
	bool targetHasRangedWeapon = target.hasRangedWeapon();

	if ( isPlayerAlly )
	{
		if ( stats[monsterAllyIndex] && players[monsterAllyIndex] && players[monsterAllyIndex]->entity )
		{
			int leaderSkill = std::max(players[monsterAllyIndex]->entity->getCHR(), 0) + stats[monsterAllyIndex]->getModifiedProficiency(PRO_LEADERSHIP);
			blockChance += std::max(0, (leaderSkill / 20) * 2); // 0-25% bonus to blockchance.
		}
	}

	if ( myStats.getModifiedProficiency(PRO_SHIELD) > 0 )
	{
		blockChance += std::min(myStats.getModifiedProficiency(PRO_SHIELD) / 10, 5); // 0-25% bonus to blockchance.
	}

	bool retreatBonus = false;
	if ( backupWithRangedWeapon(myStats, targetDist, hasrangedweapon) )
	{
		if ( targetHasRangedWeapon )
		{
			retreatBonus = true;
		}
		else if ( !targetHasRangedWeapon && targetDist < TOUCHRANGE )
		{
			retreatBonus = true;
		}
	}
	if ( shouldRetreat(myStats) )
	{
		retreatBonus = true;
	}

	if ( retreatBonus )
	{
		blockChance += 2; // 10% bonus to blockchance.
	}

	blockChance = std::min(blockChance, 12); // 60% block hard cap.

	if ( targetHasRangedWeapon && targetStats.weapon )
	{
		if ( itemCategory(targetStats.weapon) == MAGICSTAFF )
		{
			if ( myStats.shield->type != MIRROR_SHIELD )
			{
				// no point defending!
				blockChance = 0;
			}
		}
	}

	if ( myStats.type == BUGBEAR )
	{
		if ( monsterSpecialState == BUGBEAR_DEFENSE )
		{
			if ( targetDist > TOUCHRANGE && !hasrangedweapon )
			{
				blockChance = 20;
			}
			else
			{
				blockChance = 12;
				if ( monsterDefend == MONSTER_DEFEND_NONE )
				{
					blockChance = 18;
				}
			}
		}
		else if ( targetDist > TOUCHRANGE && !hasrangedweapon )
		{
			blockChance = 16;
		}
	}

	if ( local_rng.rand() % 20 < blockChance )
	{
		if ( isPlayerAlly || myStats.type == HUMAN )
		{
			if ( local_rng.rand() % 4 == 0 )
			{
				return MONSTER_DEFEND_HOLD;
			}
			else
			{
				return MONSTER_DEFEND_ALLY;
			}
		}
		else
		{
			return MONSTER_DEFEND_HOLD;
		}
	}

	return false;
}

bool Entity::monsterConsumeFoodEntity(Entity* food, Stat* myStats)
{
	if ( !myStats )
	{
		return false;
	}
	
	if ( !food )
	{
		return false;
	}

	Item* item = newItemFromEntity(food);
	if ( !item || item->type == FOOD_TIN )
	{
		return false;
	}

	if ( !FollowerRadialMenu::allowedInteractFood(myStats->type) )
	{
		handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_NOUSE);
		return false;
	}

	if ( myStats->HUNGER >= 800 )
	{
		handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_FOOD_FULL);
		return false;
	}

	int buffDuration = item->status * TICKS_PER_SECOND * 2; // (2 - 8 seconds)
	bool puking = false;
	int pukeChance = 100;
	// chance of rottenness
	if ( myStats->type == HUMAN )
	{
		switch ( item->status )
		{
			case EXCELLENT:
				pukeChance = 100;
				break;
			case SERVICABLE:
				pukeChance = 25;
				break;
			case WORN:
				pukeChance = 10;
				break;
			case DECREPIT:
				pukeChance = 4;
				break;
			default:
				pukeChance = 100;
				break;
		}
	}

	if ( local_rng.rand() % pukeChance == 0 && pukeChance < 100 )
	{
		buffDuration = 0;
		if ( this->entityCanVomit() )
		{
			this->char_gonnavomit = 40 + local_rng.rand() % 10;
			puking = true;
		}
	}
	else
	{
		if ( item->beatitude >= 0 )
		{
			if ( item->status > WORN )
			{
				buffDuration -= local_rng.rand() % ((buffDuration / 2) + 1); // 50-100% duration
			}
			else
			{
				buffDuration -= local_rng.rand() % ((buffDuration / 4) + 1); // 75-100% duration
			}
		}
		else
		{
			buffDuration = 0;
		}
	}

	if ( puking )
	{
		handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_FOOD_ROTTEN);
	}
	else if ( item->status <= WORN )
	{
		handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_FOOD_BAD);
	}
	else
	{
		handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_FOOD_GOOD);
	}

	int heal = 0;
	switch ( item->type )
	{
		case FOOD_APPLE:
			heal = 5 + item->beatitude;
			buffDuration = std::min(buffDuration, 2 * TICKS_PER_SECOND);
			myStats->HUNGER += 200;
			break;
		case FOOD_BREAD:
			heal = 7 + item->beatitude;
			buffDuration = std::min(buffDuration, 6 * TICKS_PER_SECOND);
			myStats->HUNGER += 400;
			break;
		case FOOD_CHEESE:
			heal = 3 + item->beatitude;
			buffDuration = std::min(buffDuration, 2 * TICKS_PER_SECOND);
			myStats->HUNGER += 100;
			break;
		case FOOD_CREAMPIE:
			heal = 7 + item->beatitude;
			buffDuration = std::min(buffDuration, 6 * TICKS_PER_SECOND);
			myStats->HUNGER += 200;
			break;
		case FOOD_FISH:
			heal = 7 + item->beatitude;
			myStats->HUNGER += 500;
			break;
		case FOOD_MEAT:
			heal = 9 + item->beatitude;
			myStats->HUNGER += 600;
			break;
		case FOOD_TOMALLEY:
			heal = 7 + item->beatitude;
			buffDuration = std::min(buffDuration, 4 * TICKS_PER_SECOND);
			myStats->HUNGER += 400;
			break;
		default:
			free(item);
			handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_NOUSE);
			return false;
			break;
	}

	myStats->HUNGER = std::min(myStats->HUNGER, 1500); // range checking max of 1500 hunger points.

	Entity* leader = monsterAllyGetPlayerLeader();
	if ( puking )
	{
		heal -= 5;
		if ( local_rng.rand() % 4 == 0 )
		{
			// angry at owner.
			if ( leader )
			{
				//monsterAcquireAttackTarget(*leader, MONSTER_STATE_ATTACK);
			}
		}
	}
	this->modHP(heal);

	if ( !puking && leader && local_rng.rand() % 2 == 0 )
	{
		leader->increaseSkill(PRO_LEADERSHIP);
	}

	if ( buffDuration > 0 )
	{
		myStats->EFFECTS[EFF_HP_REGEN] = true;
		myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = buffDuration;
	}

	bool foodEntityConsumed = false;

	if ( item->count > 1 )
	{
		--food->skill[13]; // update the entity on ground item count.
	}
	else
	{
		food->removeLightField();
		list_RemoveNode(food->mynode);
		foodEntityConsumed = true;
	}
	free(item);

	// eating sound
	playSoundEntity(this, 50 + local_rng.rand() % 2, 64);

	if ( leader )
	{
		Compendium_t::Events_t::eventUpdateWorld(leader->skill[2], Compendium_t::CPDM_ALLIES_FED, "the church", 1);
	}

	return foodEntityConsumed;
}

Entity* Entity::monsterAllyGetPlayerLeader()
{
	if ( behavior != &actMonster )
	{
		return nullptr;
	}
	if ( monsterAllyIndex >= 0 && monsterAllyIndex < MAXPLAYERS )
	{
		if ( players[monsterAllyIndex] )
		{
			return players[monsterAllyIndex]->entity;
		}
	}
	return nullptr;
}

bool Entity::monsterAllyEquipmentInClass(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( monsterAllyIndex >= 0 && monsterAllyIndex < MAXPLAYERS )
	{
		// player ally.
		bool hats = true;
		bool helm = true;
		bool gloves = false;
		bool breastplate = true;
		if ( myStats->type == HUMAN || myStats->type == VAMPIRE )
		{
			gloves = true;
		}
		if ( myStats->type == GOATMAN || myStats->type == GNOME || myStats->type == INCUBUS 
			|| myStats->type == SUCCUBUS || myStats->type == INSECTOID || myStats->type == VAMPIRE
			|| myStats->type == KOBOLD )
		{
			hats = false;
			helm = false;
		}
		if ( myStats->type == VAMPIRE )
		{
			breastplate = false;
		}
		if ( myStats->type == AUTOMATON )
		{
			hats = false;
		}

		if ( item.interactNPCUid == getUID() )
		{
			// monster was set to interact with this item, force want it.
			switch ( itemCategory(&item) )
			{
				case ARMOR:
					if ( checkEquipType(&item) == TYPE_HAT )
					{
						if ( myStats->type == KOBOLD && 
							(item.type == HAT_HOOD
								|| item.type == HAT_HOOD_SILVER
								|| item.type == HAT_HOOD_RED
								|| item.type == HAT_HOOD_APPRENTICE
								|| item.type == HAT_HOOD_WHISPERS
								|| item.type == HAT_HOOD_ASSASSIN) )
						{
							return true;
						}
						return hats;
					}
					else if ( checkEquipType(&item) == TYPE_HELM )
					{
						return helm;
					}
					else if ( checkEquipType(&item) == TYPE_BREASTPIECE )
					{
						return breastplate;
					}
					else if ( checkEquipType(&item) == TYPE_GLOVES )
					{
						return gloves;
					}
					return true;
					break;
				case WEAPON:
				case RING:
				case AMULET:
				case MAGICSTAFF:
					return true;
					break;
				case TOOL:
					if ( item.type == TOOL_TORCH || item.type == TOOL_LANTERN || item.type == TOOL_CRYSTALSHARD )
					{
						return true;
					}
					else if ( itemTypeIsQuiver(item.type) )
					{
						return true;
					}
					else
					{
						return false;
					}
					break;
				default:
					return false;
					break;
			}
		}
		else if ( monsterAllyClass == ALLY_CLASS_MIXED )
		{
			// pick up all default items.
		}
		else if ( monsterAllyClass == ALLY_CLASS_RANGED )
		{
			if ( itemTypeIsQuiver(item.type) )
			{
				return true;
			}
			switch ( itemCategory(&item) )
			{
				case WEAPON:
					return isRangedWeapon(item);
				case ARMOR:
					switch ( item.type )
					{
						case CRYSTAL_BREASTPIECE:
						case CRYSTAL_HELM:
						case CRYSTAL_SHIELD:
						case STEEL_BREASTPIECE:
						case STEEL_HELM:
						case STEEL_SHIELD:
						case IRON_BREASTPIECE:
						case IRON_HELM:
						case IRON_SHIELD:
							return false;
							break;
						default:
							break;
					}
					if ( checkEquipType(&item) == TYPE_HAT )
					{
						return hats;
					}
					else if ( checkEquipType(&item) == TYPE_HELM )
					{
						return helm;
					}
					break;
				case MAGICSTAFF:
					return false;
					break;
				case THROWN:
					if ( myStats->type == GOATMAN )
					{
						return true;
					}
					return false;
					break;
				case POTION:
					if ( myStats->type == GOATMAN )
					{
						switch ( item.type )
						{
							case POTION_BOOZE:
								return true;
							case POTION_HEALING:
								return true;
							default:
								return false;
						}
					}
					return false;
					break;
				case TOOL:
					if ( itemTypeIsQuiver(item.type) )
					{
						return true;
					}
					break;
				default:
					return false;
					break;
			}
		}
		else if ( monsterAllyClass == ALLY_CLASS_MELEE )
		{
			switch ( itemCategory(&item) )
			{
				case WEAPON:
					return !isRangedWeapon(item);
				case ARMOR:
					if ( checkEquipType(&item) == TYPE_HAT )
					{
						return hats;
					}
					else if ( checkEquipType(&item) == TYPE_HELM )
					{
						return helm;
					}
					return true;
				case MAGICSTAFF:
					return false;
				case THROWN:
					return false;
				case TOOL:
					if ( itemTypeIsQuiver(item.type) )
					{
						return false;
					}
					break;
				default:
					return false;
			}
		}
	}
	return false;
}

bool Entity::monsterIsTinkeringCreation()
{
	int race = this->getMonsterTypeFromSprite();
	if ( behavior != &actMonster )
	{
		return false;
	}
	if ( race == GYROBOT || race == DUMMYBOT || race == SENTRYBOT || race == SPELLBOT )
	{
		return true;
	}
	return false;
}

void Entity::monsterHandleKnockbackVelocity(real_t monsterFacingTangent, real_t weightratio)
{
	// this function makes the monster accelerate to running forwards or 0 movement speed after being knocked back.
	// vel_x, vel_y are set on knockback impact and this slowly accumulates speed from the knocked back movement by a factor of monsterKnockbackVelocity.
	real_t maxVelX = cos(monsterFacingTangent) * .045 * (monsterGetDexterityForMovement() + 10) * weightratio;
	real_t maxVelY = sin(monsterFacingTangent) * .045 * (monsterGetDexterityForMovement() + 10) * weightratio;
	bool mobile = ((monsterState == MONSTER_STATE_WAIT) || isMobile()); // if immobile, the intended max speed is 0 (stopped).
	
	if ( maxVelX > 0 )
	{
		this->vel_x = std::min(this->vel_x + (this->monsterKnockbackVelocity * maxVelX), mobile ? maxVelX : 0.0);
	}
	else
	{
		this->vel_x = std::max(this->vel_x + (this->monsterKnockbackVelocity * maxVelX), mobile ? maxVelX : 0.0);
	}
	if ( maxVelY > 0 )
	{
		this->vel_y = std::min(this->vel_y + (this->monsterKnockbackVelocity * maxVelY), mobile ? maxVelY : 0.0);
	}
	else
	{
		this->vel_y = std::max(this->vel_y + (this->monsterKnockbackVelocity * maxVelY), mobile ? maxVelY : 0.0);
	}
	this->monsterKnockbackVelocity *= 1.1;
}

int Entity::monsterGetDexterityForMovement()
{
	int myDex = this->getDEX();
	if ( this->monsterAllyGetPlayerLeader() )
	{
		myDex = std::min(myDex, MONSTER_ALLY_DEXTERITY_SPEED_CAP);
	}
	Stat* myStats = getStats();
	if ( myStats )
	{
		if ( myStats->EFFECTS[EFF_DASH] )
		{
			myDex += 30;
		}
		if ( myStats->type == MIMIC && monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
		{
			myDex += 3;
		}
	}
	return myDex;
}

void Entity::monsterGenerateQuiverItem(Stat* myStats, bool lesserMonster)
{
	if ( !myStats )
	{
		return;
	}

	auto& rng = entity_rng ? *entity_rng : local_rng;

	int ammo = 5;
	if ( currentlevel >= 15 )
	{
		ammo += 5 + rng.rand() % 16;
	}
	else
	{
		ammo += rng.rand() % 11;
	}
	switch ( myStats->type )
	{
		case GNOME:
			switch ( rng.rand() % 5 )
			{
			case 0:
			case 1:
			case 2:
				myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
				break;
			case 3:
			case 4:
				if ( myStats->weapon && myStats->weapon->type == SHORTBOW )
				{
					myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
				}
				else
				{
					// nothing
				}
				break;
			default:
				break;
			}
			break;
		case HUMAN:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_SILVER, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 18 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_CRYSTAL, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case GOBLIN:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 18 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case INSECTOID:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_HUNTING, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 18 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_CRYSTAL, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case KOBOLD:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 28 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_CRYSTAL, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case INCUBUS:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 18 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_CRYSTAL, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case SKELETON:
			switch ( rng.rand() % 5 )
			{
				case 0:
				case 1:
					myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 2:
				case 3:
					myStats->shield = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					break;
				case 4:
					if ( currentlevel >= 18 )
					{
						if ( rng.rand() % 2 )
						{
							myStats->shield = newItem(QUIVER_FIRE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
						}
					}
					else
					{
						myStats->shield = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, ammo, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr);
					}
					break;
				default:
					break;
			}
			break;
		case AUTOMATON:
			myStats->shield = newItem(QUIVER_PIERCE, SERVICABLE, 0, ammo, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			break;
		default:
			break;
	}
}

int Entity::getMonsterEffectiveDistanceOfRangedWeapon(Item* weapon)
{
	if ( !weapon )
	{
		return 160;
	}

	if ( getMonsterTypeFromSprite() == SENTRYBOT )
	{
		return sightranges[SENTRYBOT];
	}
	else if ( getMonsterTypeFromSprite() == SPELLBOT )
	{
		return sightranges[SPELLBOT];
	}

	int distance = 160;
	switch ( weapon->type )
	{
		case SLING:
		case CROSSBOW:
		case HEAVY_CROSSBOW:
			distance = 100;
			break;
		case LONGBOW:
			distance = 200;
			break;
		default:
			break;
	}
	return distance;
}

bool Entity::isFollowerFreeToPathToPlayer(Stat* myStats)
{
	Entity* currentTarget = uidToEntity(monsterTarget);
	if ( currentTarget )
	{
		if ( monsterState == MONSTER_STATE_ATTACK )
		{
			// fighting something.
			return false;
		}
		else if ( entityDist(this, currentTarget) < TOUCHRANGE * 2 )
		{
			// in the vicinity of our target
			return false;
		}
	}
	return true;
}

bool Entity::isInertMimic() const
{
	if ( behavior == &actMonster && getMonsterTypeFromSprite() == MIMIC )
	{
		if ( monsterSpecialState == MIMIC_INERT || monsterSpecialState == MIMIC_INERT_SECOND )
		{
			return true;
		}
	}
	return false;
}

bool Entity::isUntargetableBat(real_t* outDist) const
{
	if ( behavior == &actMonster && getMonsterTypeFromSprite() == BAT_SMALL )
	{
		if ( bodyparts.size() >= 1 )
		{
			auto& body = bodyparts[0];
			if ( body->z < -7.5 )
			{
				if ( outDist )
				{
					*outDist = body->z;
				}
				return true;
			}
		}
	}
	return false;
}

void batResetIdle(Entity* my)
{
	if ( !my ) { return; }
	// reset to inert after wandering with no target

	bool canRest = true;
	int x = my->x / 16;
	int y = my->y / 16;
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && canRest; ++it )
	{
		list_t* currentList = *it;
		for ( node_t* node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == my )
			{
				continue;
			}
			if ( entity->behavior == &actCeilingTile 
				|| entity->behavior == &actColliderDecoration 
				|| entity->behavior == &actFurniture
				|| entity->behavior == &actBell
				|| entity->behavior == &actStalagCeiling )
			{
				int x2 = entity->x / 16;
				int y2 = entity->y / 16;
				if ( x == x2 && y == y2 )
				{
					canRest = false;
					break;
				}
			}
		}
	}

	if ( canRest )
	{
		my->monsterSpecialState = BAT_REST;
		serverUpdateEntitySkill(my, 33);

		my->monsterLookDir = (PI / 2) * (local_rng.rand() % 4);
	}
}

void mimicResetIdle(Entity* my)
{
	if ( !my ) { return; }
	// reset to inert after wandering with no target
	my->monsterSpecialState = MIMIC_INERT_SECOND;
	serverUpdateEntitySkill(my, 33);

	playSoundEntity(my, 22, 64);

	int x = (static_cast<int>(my->x) >> 4);
	int y = (static_cast<int>(my->y) >> 4);
	// look for a wall to turn against

	std::vector<std::pair<int, int>> tilesToCheck;
	tilesToCheck.push_back(std::make_pair(x + 1, y));
	tilesToCheck.push_back(std::make_pair(x - 1, y));
	tilesToCheck.push_back(std::make_pair(x, y + 1));
	tilesToCheck.push_back(std::make_pair(x, y - 1));

	bool foundWall = false;
	for ( auto& pair : tilesToCheck )
	{
		int tx = pair.first;
		int ty = pair.second;
		if ( map.tiles[OBSTACLELAYER + ((ty)*MAPLAYERS + (tx)*MAPLAYERS * map.height)] )
		{
			if ( tx == x + 1 )
			{
				my->monsterLookDir = PI;
			}
			else if ( tx == x - 1 )
			{
				my->monsterLookDir = 0.0;
			}
			else if ( ty == y - 1 )
			{
				my->monsterLookDir = PI / 2;
			}
			else if ( ty == y + 1 )
			{
				my->monsterLookDir = 3 * PI / 2;
			}
			foundWall = true;
		}
	}

	if ( !foundWall )
	{
		for ( auto& pair : tilesToCheck )
		{
			int tx = pair.first;
			int ty = pair.second;

			if ( checkObstacle((tx << 4) + 8, (ty << 4) + 8, my, nullptr) )
			{
				if ( tx == x + 1 || tx == x - 1 )
				{
					my->monsterLookDir = PI / 2 + (local_rng.rand() % 2) * PI;
				}
				else if ( ty == y + 1 || ty == y - 1 )
				{
					my->monsterLookDir = 0.0 + (local_rng.rand() % 2) * PI;
				}
				foundWall = true;
				break;
			}
		}
	}
	if ( !foundWall )
	{
		my->monsterLookDir = (PI / 2) * (local_rng.rand() % 4);
	}
}
