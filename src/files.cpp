/*-------------------------------------------------------------------------------

	BARONY
	File: files.cpp
	Desc: contains code for file i/o

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <fstream>
#include <list>
#include <string>

#include "main.hpp"
#include "files.hpp"
#include "engine/audio/sound.hpp"
#include "entity.hpp"
#include "book.hpp"
#include "menu.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "mod_tools.hpp"

std::vector<int> gamemods_modelsListModifiedIndexes;
std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImagesToReload;
std::unordered_map<std::string, int> mapHashes = 
{
	{ "start.lmp", 251890 },
	{ "mine.lmp", 80741 },
	{ "mine00.lmp", 12890 },
	{ "mine01.lmp", 52889 },
	{ "mine02.lmp", 16102 },
	{ "mine03.lmp", 17657 },
	{ "mine04.lmp", 21711 },
	{ "mine05.lmp", 33835 },
	{ "mine06.lmp", 79441 },
	{ "mine07.lmp", 107931 },
	{ "mine08.lmp", 50089 },
	{ "mine09.lmp", 57257 },
	{ "mine10.lmp", 117885 },
	{ "mine11.lmp", 74234 },
	{ "mine12.lmp", 30195 },
	{ "mine13.lmp", 43438 },
	{ "mine14.lmp", 54278 },
	{ "mine15.lmp", 44160 },
	{ "mine16.lmp", 47683 },
	{ "mine17.lmp", 18741 },
	{ "mine18.lmp", 48006 },
	{ "mine19.lmp", 21099 },
	{ "mine20.lmp", 16836 },
	{ "mine21.lmp", 26317 },
	{ "mine22.lmp", 20018 },
	{ "mine23.lmp", 20477 },
	{ "mine24.lmp", 17657 },
	{ "mine25.lmp", 11992 },
	{ "mine26.lmp", 6638 },
	{ "mine27.lmp", 6777 },
	{ "mine28.lmp", 12641 },
	{ "mine29.lmp", 7172 },
	{ "mine30.lmp", 56499 },
	{ "mine31.lmp", 38812 },
	{ "mine32.lmp", 7789 },
	{ "mine33.lmp", 11931 },
	{ "mine01a.lmp", 4094 },
	{ "mine01b.lmp", 5156 },
	{ "mine01c.lmp", 5524 },
	{ "mine01d.lmp", 2848 },
	{ "mine01e.lmp", 33508 },
	{ "mine01f.lmp", 2424 },
	{ "mine01g.lmp", 14146 },
	{ "mine02a.lmp", 1964 },
	{ "mine02b.lmp", 2050 },
	{ "mine02c.lmp", 4229 },
	{ "mine02d.lmp", 19759 },
	{ "mine02e.lmp", 2052 },
	{ "mine02f.lmp", 375 },
	{ "mine02g.lmp", 820 },
	{ "mine03a.lmp", 3440 },
	{ "mine03b.lmp", 49719 },
	{ "mine03c.lmp", 3274 },
	{ "mine03d.lmp", 1451 },
	{ "mine03e.lmp", 1335 },
	{ "mine03f.lmp", 6794 },
	{ "mine04a.lmp", 2250 },
	{ "mine04b.lmp", 5791 },
	{ "mine04c.lmp", 7082 },
	{ "mine04d.lmp", 2925 },
	{ "mine04e.lmp", 914 },
	{ "mine04f.lmp", 1042 },
	{ "mine04g.lmp", 2410 },
	{ "mine05a.lmp", 4057 },
	{ "mine05b.lmp", 8869 },
	{ "mine05c.lmp", 3943 },
	{ "mine05d.lmp", 6466 },
	{ "mine05e.lmp", 3486 },
	{ "mine06a.lmp", 23088 },
	{ "mine06b.lmp", 58816 },
	{ "mine06c.lmp", 27429 },
	{ "mine06d.lmp", 121194 },
	{ "mine06e.lmp", 25051 },
	{ "mine07a.lmp", 13341 },
	{ "mine07b.lmp", 32531 },
	{ "mine07c.lmp", 49306 },
	{ "mine07d.lmp", 17195 },
	{ "mine07e.lmp", 13818 },
	{ "mine08a.lmp", 2824 },
	{ "mine08b.lmp", 6387 },
	{ "mine08c.lmp", 3666 },
	{ "mine08d.lmp", 27312 },
	{ "mine08e.lmp", 1614 },
	{ "mine09a.lmp", 10581 },
	{ "mine09b.lmp", 10445 },
	{ "mine09c.lmp", 46588 },
	{ "mine09d.lmp", 30484 },
	{ "mine09e.lmp", 10147 },
	{ "mine10a.lmp", 178759 },
	{ "mine10b.lmp", 70918 },
	{ "mine10c.lmp", 21147 },
	{ "mine10d.lmp", 52593 },
	{ "mine10e.lmp", 60889 },
	{ "mine11a.lmp", 7752 },
	{ "mine11b.lmp", 7909 },
	{ "mine11c.lmp", 4787 },
	{ "mine11d.lmp", 20681 },
	{ "mine11e.lmp", 84691 },
	{ "mine12a.lmp", 2408 },
	{ "mine12b.lmp", 1064 },
	{ "mine12c.lmp", 1064 },
	{ "mine12d.lmp", 1064 },
	{ "mine12e.lmp", 1066 },
	{ "mine12f.lmp", 1123 },
	{ "mine12g.lmp", 3173 },
	{ "mine12h.lmp", 51714 },
	{ "mine12i.lmp", 29186 },
	{ "mine13a.lmp", 6070 },
	{ "mine13b.lmp", 6864 },
	{ "mine13c.lmp", 94569 },
	{ "mine13d.lmp", 9751 },
	{ "mine13e.lmp", 10821 },
	{ "mine14a.lmp", 8371 },
	{ "mine14b.lmp", 11253 },
	{ "mine14c.lmp", 43615 },
	{ "mine14d.lmp", 10230 },
	{ "mine14e.lmp", 6706 },
	{ "mine15a.lmp", 14464 },
	{ "mine15b.lmp", 19060 },
	{ "mine15c.lmp", 12155 },
	{ "mine15d.lmp", 11889 },
	{ "mine15e.lmp", 6134 },
	{ "mine16a.lmp", 5567 },
	{ "mine16b.lmp", 4501 },
	{ "mine16c.lmp", 11200 },
	{ "mine16d.lmp", 15776 },
	{ "mine16e.lmp", 22921 },
	{ "mine16f.lmp", 34277 },
	{ "mine17a.lmp", 4756 },
	{ "mine17b.lmp", 6235 },
	{ "mine17c.lmp", 7385 },
	{ "mine17d.lmp", 5919 },
	{ "mine17e.lmp", 7393 },
	{ "mine17f.lmp", 10133 },
	{ "mine18a.lmp", 10791 },
	{ "mine18b.lmp", 16439 },
	{ "mine18c.lmp", 18972 },
	{ "mine18d.lmp", 32406 },
	{ "mine18e.lmp", 11209 },
	{ "mine19a.lmp", 2012 },
	{ "mine19b.lmp", 2634 },
	{ "mine19c.lmp", 2334 },
	{ "mine19d.lmp", 2496 },
	{ "mine19e.lmp", 4139 },
	{ "mine20a.lmp", 968 },
	{ "mine20b.lmp", 1281 },
	{ "mine20c.lmp", 2087 },
	{ "mine20d.lmp", 1077 },
	{ "mine20e.lmp", 10418 },
	{ "mine21a.lmp", 4635 },
	{ "mine21b.lmp", 4623 },
	{ "mine21c.lmp", 4872 },
	{ "mine21d.lmp", 5830 },
	{ "mine21e.lmp", 5158 },
	{ "mine22a.lmp", 937 },
	{ "mine22b.lmp", 1229 },
	{ "mine22c.lmp", 1229 },
	{ "mine22d.lmp", 1431 },
	{ "mine22e.lmp", 3687 },
	{ "mine22f.lmp", 28494 },
	{ "mine23a.lmp", 2292 },
	{ "mine23b.lmp", 2524 },
	{ "mine23c.lmp", 2557 },
	{ "mine23d.lmp", 1984 },
	{ "mine23e.lmp", 14561 },
	{ "mine23f.lmp", 3794 },
	{ "mine24a.lmp", 2300 },
	{ "mine24b.lmp", 2455 },
	{ "mine24c.lmp", 1544 },
	{ "mine24d.lmp", 868 },
	{ "mine24e.lmp", 1612 },
	{ "mine25a.lmp", 1508 },
	{ "mine25b.lmp", 1095 },
	{ "mine25c.lmp", 7206 },
	{ "mine25d.lmp", 475 },
	{ "mine25e.lmp", 945 },
	{ "mine26a.lmp", 646 },
	{ "mine26b.lmp", 568 },
	{ "mine26c.lmp", 650 },
	{ "mine26d.lmp", 729 },
	{ "mine26e.lmp", 1912 },
	{ "mine27a.lmp", 647 },
	{ "mine27b.lmp", 646 },
	{ "mine27c.lmp", 719 },
	{ "mine27d.lmp", 727 },
	{ "mine27e.lmp", 2659 },
	{ "mine28a.lmp", 1867 },
	{ "mine28b.lmp", 1981 },
	{ "mine28c.lmp", 2255 },
	{ "mine28d.lmp", 2242 },
	{ "mine28e.lmp", 2099 },
	{ "mine29a.lmp", 631 },
	{ "mine29b.lmp", 663 },
	{ "mine29c.lmp", 851 },
	{ "mine29d.lmp", 787 },
	{ "mine29e.lmp", 586 },
	{ "mine30a.lmp", 24107 },
	{ "mine30b.lmp", 41755 },
	{ "mine30c.lmp", 19330 },
	{ "mine30d.lmp", 9532 },
	{ "mine30e.lmp", 9175 },
	{ "mine31a.lmp", 14939 },
	{ "mine31b.lmp", 14098 },
	{ "mine31c.lmp", 13828 },
	{ "mine31d.lmp", 6625 },
	{ "mine31e.lmp", 8149 },
	{ "mine32a.lmp", 2775 },
	{ "mine32b.lmp", 2944 },
	{ "mine32c.lmp", 1755 },
	{ "mine32d.lmp", 266 },
	{ "mine32e.lmp", 2044 },
	{ "mine33a.lmp", 63 },
	{ "mine33b.lmp", 276 },
	{ "mine33c.lmp", 480 },
	{ "mine33d.lmp", 848 },
	{ "mine33e.lmp", 298 },
	{ "mine33f.lmp", 258 },
	{ "mine33g.lmp", 2972 },

	{ "minetoswamp.lmp", 215809 },
	{ "minesecret.lmp", 25068 },
	{ "gnomishmines.lmp", 603055 },
	{ "minetown.lmp", 551422 },

	{ "swamp.lmp", 121669 },
	{ "swamp00.lmp", 13629 },
	{ "swamp01.lmp", 29459 },
	{ "swamp02.lmp", 39297 },
	{ "swamp03.lmp", 35148 },
	{ "swamp04.lmp", 51827 },
	{ "swamp05.lmp", 20558 },
	{ "swamp06.lmp", 60048 },
	{ "swamp07.lmp", 91130 },
	{ "swamp08.lmp", 31288 },
	{ "swamp09.lmp", 29565 },
	{ "swamp10.lmp", 27653 },
	{ "swamp11.lmp", 60958 },
	{ "swamp12.lmp", 52448 },
	{ "swamp13.lmp", 26009 },
	{ "swamp14.lmp", 41242 },
	{ "swamp15.lmp", 1092 },
	{ "swamp16.lmp", 10251 },
	{ "swamp17.lmp", 31463 },
	{ "swamp18.lmp", 18634 },
	{ "swamp19.lmp", 20258 },
	{ "swamp20.lmp", 33017 },
	{ "swamp21.lmp", 16295 },
	{ "swamp22.lmp", 55275 },
	{ "swamp23.lmp", 63299 },
	{ "swamp24.lmp", 28993 },
	{ "swamp25.lmp", 34758 },
	{ "swamp26.lmp", 24126 },
	{ "swamp27.lmp", 26062 },
	{ "swamp28.lmp", 6067 },
	{ "swamp29.lmp", 6111 },
	{ "swamp30.lmp", 54169 },
	{ "swamp31.lmp", 24310 },
	{ "swamp32.lmp", 27134 },
	{ "swamp33.lmp", 31993 },
	{ "swamp34.lmp", 36795 },

	{ "swamp01a.lmp", 5303 },
	{ "swamp01b.lmp", 5811 },
	{ "swamp01c.lmp", 10334 },
	{ "swamp01d.lmp", 5395 },
	{ "swamp01e.lmp", 4060 },
	{ "swamp02a.lmp", 9131 },
	{ "swamp02b.lmp", 11457 },
	{ "swamp02c.lmp", 8877 },
	{ "swamp02d.lmp", 7327 },
	{ "swamp02e.lmp", 4203 },
	{ "swamp03a.lmp", 6073 },
	{ "swamp03b.lmp", 25871 },
	{ "swamp03c.lmp", 3520 },
	{ "swamp03d.lmp", 7686 },
	{ "swamp03e.lmp", 15932 },
	{ "swamp04a.lmp", 6753 },
	{ "swamp04b.lmp", 5875 },
	{ "swamp04c.lmp", 4610 },
	{ "swamp04d.lmp", 7818 },
	{ "swamp04e.lmp", 18431 },
	{ "swamp05a.lmp", 2611 },
	{ "swamp05b.lmp", 2966 },
	{ "swamp05c.lmp", 5344 },
	{ "swamp05d.lmp", 1683 },
	{ "swamp05e.lmp", 2746 },
	{ "swamp06a.lmp", 12354 },
	{ "swamp06b.lmp", 19675 },
	{ "swamp06c.lmp", 39770 },
	{ "swamp06d.lmp", 67634 },
	{ "swamp06e.lmp", 6409 },
	{ "swamp07a.lmp", 12315 },
	{ "swamp07b.lmp", 7290 },
	{ "swamp07c.lmp", 20314 },
	{ "swamp07d.lmp", 45805 },
	{ "swamp07e.lmp", 38538 },
	{ "swamp08a.lmp", 3734 },
	{ "swamp08b.lmp", 5429 },
	{ "swamp08c.lmp", 9406 },
	{ "swamp08d.lmp", 31893 },
	{ "swamp08e.lmp", 2913 },
	{ "swamp09a.lmp", 1516 },
	{ "swamp09b.lmp", 6762 },
	{ "swamp09c.lmp", 4454 },
	{ "swamp09d.lmp", 4955 },
	{ "swamp09e.lmp", 3428 },
	{ "swamp10a.lmp", 2322 },
	{ "swamp10b.lmp", 5243 },
	{ "swamp10c.lmp", 5260 },
	{ "swamp10d.lmp", 6311 },
	{ "swamp10e.lmp", 2344 },
	{ "swamp10f.lmp", 3680 },
	{ "swamp11a.lmp", 4862 },
	{ "swamp11b.lmp", 9784 },
	{ "swamp11c.lmp", 22356 },
	{ "swamp11d.lmp", 35152 },
	{ "swamp11e.lmp", 37481 },
	{ "swamp12a.lmp", 6740 },
	{ "swamp12b.lmp", 14581 },
	{ "swamp12c.lmp", 6641 },
	{ "swamp12d.lmp", 9487 },
	{ "swamp12e.lmp", 13987 },
	{ "swamp13a.lmp", 6292 },
	{ "swamp13b.lmp", 4572 },
	{ "swamp13c.lmp", 9103 },
	{ "swamp13d.lmp", 4093 },
	{ "swamp13e.lmp", 5738 },
	{ "swamp14a.lmp", 9092 },
	{ "swamp14b.lmp", 4317 },
	{ "swamp14c.lmp", 14974 },
	{ "swamp14d.lmp", 19525 },
	{ "swamp14e.lmp", 6608 },
	{ "swamp16a.lmp", 1249 },
	{ "swamp16b.lmp", 1494 },
	{ "swamp16c.lmp", 876 },
	{ "swamp16d.lmp", 1863 },
	{ "swamp16e.lmp", 1233 },
	{ "swamp17a.lmp", 5602 },
	{ "swamp17b.lmp", 10976 },
	{ "swamp17c.lmp", 8919 },
	{ "swamp17d.lmp", 6550 },
	{ "swamp17e.lmp", 4447 },
	{ "swamp18a.lmp", 934 },
	{ "swamp18b.lmp", 3264 },
	{ "swamp18c.lmp", 5153 },
	{ "swamp18d.lmp", 2444 },
	{ "swamp18e.lmp", 939 },
	{ "swamp18f.lmp", 3207 },
	{ "swamp18g.lmp", 3646 },
	{ "swamp19a.lmp", 3302 },
	{ "swamp19b.lmp", 1808 },
	{ "swamp19c.lmp", 3136 },
	{ "swamp19d.lmp", 3705 },
	{ "swamp19e.lmp", 5054 },
	{ "swamp20a.lmp", 4654 },
	{ "swamp20b.lmp", 10215 },
	{ "swamp20c.lmp", 4315 },
	{ "swamp20d.lmp", 26165 },
	{ "swamp20e.lmp", 6173 },
	{ "swamp21a.lmp", 1887 },
	{ "swamp21b.lmp", 3323 },
	{ "swamp21c.lmp", 6881 },
	{ "swamp21d.lmp", 25271 },
	{ "swamp21e.lmp", 3401 },
	{ "swamp22a.lmp", 13094 },
	{ "swamp22b.lmp", 5215 },
	{ "swamp22c.lmp", 11756 },
	{ "swamp22d.lmp", 8710 },
	{ "swamp22e.lmp", 26057 },
	{ "swamp23a.lmp", 9089 },
	{ "swamp23b.lmp", 17419 },
	{ "swamp23c.lmp", 10863 },
	{ "swamp23d.lmp", 9704 },
	{ "swamp23e.lmp", 11640 },
	{ "swamp24a.lmp", 4663 },
	{ "swamp24b.lmp", 4348 },
	{ "swamp24c.lmp", 1980 },
	{ "swamp24d.lmp", 5928 },
	{ "swamp24e.lmp", 5507 },
	{ "swamp25a.lmp", 5838 },
	{ "swamp25b.lmp", 2703 },
	{ "swamp25c.lmp", 12252 },
	{ "swamp25d.lmp", 5226 },
	{ "swamp25e.lmp", 5717 },
	{ "swamp25f.lmp", 10354 },
	{ "swamp26a.lmp", 2440 },
	{ "swamp26b.lmp", 2398 },
	{ "swamp26c.lmp", 1564 },
	{ "swamp26d.lmp", 3045 },
	{ "swamp26e.lmp", 1262 },
	{ "swamp27a.lmp", 3167 },
	{ "swamp27b.lmp", 3128 },
	{ "swamp27c.lmp", 2059 },
	{ "swamp27d.lmp", 5202 },
	{ "swamp27e.lmp", 4065 },
	{ "swamp28a.lmp", 676 },
	{ "swamp28b.lmp", 524 },
	{ "swamp28c.lmp", 2016 },
	{ "swamp28d.lmp", 1353 },
	{ "swamp28e.lmp", 1457 },
	{ "swamp28f.lmp", 338 },
	{ "swamp29a.lmp", 591 },
	{ "swamp29b.lmp", 596 },
	{ "swamp29c.lmp", 1336 },
	{ "swamp29d.lmp", 2619 },
	{ "swamp29e.lmp", 1391 },
	{ "swamp29f.lmp", 1249 },
	{ "swamp29g.lmp", 1249 },
	{ "swamp30a.lmp", 17250 },
	{ "swamp30b.lmp", 69206 },
	{ "swamp30c.lmp", 20925 },
	{ "swamp30d.lmp", 18887 },
	{ "swamp30e.lmp", 37453 },
	{ "swamp31a.lmp", 7582 },
	{ "swamp31b.lmp", 9169 },
	{ "swamp31c.lmp", 2026 },
	{ "swamp31d.lmp", 5365 },
	{ "swamp31e.lmp", 35837 },
	{ "swamp32a.lmp", 5335 },
	{ "swamp32b.lmp", 6716 },
	{ "swamp32c.lmp", 4638 },
	{ "swamp32d.lmp", 6124 },
	{ "swamp32e.lmp", 6889 },
	{ "swamp33a.lmp", 9437 },
	{ "swamp33b.lmp", 11693 },
	{ "swamp33c.lmp", 5121 },
	{ "swamp33d.lmp", 7910 },
	{ "swamp33e.lmp", 10948 },
	{ "swamp34a.lmp", 6929 },
	{ "swamp34b.lmp", 68279 },
	{ "swamp34c.lmp", 23952 },
	{ "swamp34d.lmp", 5546 },
	{ "swamp34e.lmp", 14412 },

	{ "swamptolabyrinth.lmp", 213824 },
	{ "swampsecret.lmp", 13392 },
	{ "temple.lmp", 4872600 },
	{ "greatcastle.lmp", 8642476 },

	{ "labyrinth.lmp", 219311 },
	{ "labyrinth00.lmp", 12194 },
	{ "labyrinth01.lmp", 4079 },
	{ "labyrinth02.lmp", 472 },
	{ "labyrinth03.lmp", 24615 },
	{ "labyrinth04.lmp", 8421 },
	{ "labyrinth05.lmp", 25767 },
	{ "labyrinth06.lmp", 4296 },
	{ "labyrinth07.lmp", 4297 },
	{ "labyrinth08.lmp", 7962 },
	{ "labyrinth09.lmp", 9494 },
	{ "labyrinth10.lmp", 10304 },
	{ "labyrinth11.lmp", 89828 },
	{ "labyrinth12.lmp", 39775 },
	{ "labyrinth13.lmp", 9162 },
	{ "labyrinth14.lmp", 40743 },
	{ "labyrinth15.lmp", 35557 },
	{ "labyrinth16.lmp", 12319 },
	{ "labyrinth17.lmp", 11146 },
	{ "labyrinth18.lmp", 6437 },
	{ "labyrinth19.lmp", 7121 },
	{ "labyrinth20.lmp", 10027 },
	{ "labyrinth21.lmp", 6939 },
	{ "labyrinth22.lmp", 5829 },
	{ "labyrinth23.lmp", 13042 },
	{ "labyrinth24.lmp", 14902 },
	{ "labyrinth25.lmp", 6399 },
	{ "labyrinth26.lmp", 1913 },
	{ "labyrinth27.lmp", 1916 },
	{ "labyrinth28.lmp", 1755 },
	{ "labyrinth29.lmp", 5155 },
	{ "labyrinth30.lmp", 21119 },
	{ "labyrinth31.lmp", 28195 },
	{ "labyrinth32.lmp", 28071 },

	{ "labyrinthtoruins.lmp", 137530 },
	{ "labyrinthsecret.lmp", 26508 },
	{ "sokoban.lmp", 140500 },
	{ "minotaur.lmp", 2644636 },

	{ "ruins.lmp", 11472 },
	{ "ruins00.lmp", 6373 },
	{ "ruins01.lmp", 95175 },
	{ "ruins02.lmp", 95577 },
	{ "ruins03.lmp", 18465 },
	{ "ruins04.lmp", 11113 },
	{ "ruins05.lmp", 16194 },
	{ "ruins06.lmp", 1890 },
	{ "ruins07.lmp", 2486 },
	{ "ruins08.lmp", 4682 },
	{ "ruins09.lmp", 4704 },
	{ "ruins10.lmp", 10987 },
	{ "ruins11.lmp", 13490 },
	{ "ruins12.lmp", 6662 },
	{ "ruins13.lmp", 183250 },
	{ "ruins14.lmp", 766 },
	{ "ruins15.lmp", 772 },
	{ "ruins16.lmp", 4681 },
	{ "ruins17.lmp", 1848 },
	{ "ruins18.lmp", 9066 },
	{ "ruins19.lmp", 6897 },
	{ "ruins20.lmp", 40374 },
	{ "ruins21.lmp", 10664 },
	{ "ruins22.lmp", 11523 },
	{ "ruins23.lmp", 88 },
	{ "ruins24.lmp", 36565 },
	{ "ruins25.lmp", 1218 },
	{ "ruins26.lmp", 17108 },
	{ "ruins27.lmp", 1468 },
	{ "ruins28.lmp", 276 },
	{ "ruins29.lmp", 13217 },
	{ "ruins30.lmp", 39050 },
	{ "ruins31.lmp", 46626 },
	{ "ruins32.lmp", 32243 },
	{ "ruins33.lmp", 1597 },
	{ "ruins34.lmp", 381 },

	{ "ruinssecret.lmp", 66694 },
	{ "mysticlibrary.lmp", 377973 },

	{ "boss.lmp", 1491809 },

	{ "underworld.lmp", 253866 },
	{ "underworld00.lmp", 26338 },
	{ "underworld01.lmp", 63364 },
	{ "underworld01a.lmp", 15722 },
	{ "underworld01b.lmp", 23578 },
	{ "underworld01c.lmp", 16338 },
	{ "underworld01d.lmp", 13107 },
	{ "underworld01e.lmp", 21000 },
	{ "underworld02.lmp", 35793 },
	{ "underworld02a.lmp", 7965 },
	{ "underworld02b.lmp", 11142 },
	{ "underworld02c.lmp", 12186 },
	{ "underworld02d.lmp", 9758 },
	{ "underworld02e.lmp", 11286 },
	{ "underworld03.lmp", 7801 },
	{ "underworld03a.lmp", 739 },
	{ "underworld03b.lmp", 1458 },
	{ "underworld03c.lmp", 2290 },
	{ "underworld03d.lmp", 1345 },
	{ "underworld03e.lmp", 1423 },
	{ "underworld04.lmp", 13996 },
	{ "underworld04a.lmp", 2482 },
	{ "underworld04b.lmp", 2530 },
	{ "underworld04c.lmp", 3130 },
	{ "underworld04d.lmp", 3326 },
	{ "underworld04e.lmp", 3949 },
	{ "underworld05.lmp", 40061 },
	{ "underworld05a.lmp", 6687 },
	{ "underworld05b.lmp", 8086 },
	{ "underworld05c.lmp", 15740 },
	{ "underworld05d.lmp", 12373 },
	{ "underworld05e.lmp", 11278 },
	{ "underworld06.lmp", 40061 },
	{ "underworld06a.lmp", 9012 },
	{ "underworld06b.lmp", 9570 },
	{ "underworld06c.lmp", 11087 },
	{ "underworld06d.lmp", 21558 },
	{ "underworld06e.lmp", 12493 },
	{ "underworld07.lmp", 27085 },
	{ "underworld07a.lmp", 6214 },
	{ "underworld07b.lmp", 8553 },
	{ "underworld07c.lmp", 6616 },
	{ "underworld07d.lmp", 46782 },
	{ "underworld07e.lmp", 5087 },
	{ "underworld08.lmp", 89325 },
	{ "underworld08a.lmp", 14990 },
	{ "underworld08b.lmp", 23417 },
	{ "underworld08c.lmp", 27050 },
	{ "underworld08d.lmp", 38494 },
	{ "underworld08e.lmp", 66467 },
	{ "underworld09.lmp", 76992 },
	{ "underworld09a.lmp", 32467 },
	{ "underworld09b.lmp", 44375 },
	{ "underworld09c.lmp", 35307 },
	{ "underworld09d.lmp", 23598 },
	{ "underworld09e.lmp", 31008 },
	{ "underworld10.lmp", 118384 },
	{ "underworld10a.lmp", 99959 },
	{ "underworld10b.lmp", 47378 },
	{ "underworld10c.lmp", 37212 },
	{ "underworld10d.lmp", 41839 },
	{ "underworld10e.lmp", 48179 },
	{ "underworld10f.lmp", 50632 },
	{ "underworld11.lmp", 71754 },
	{ "underworld11a.lmp", 12213 },
	{ "underworld11b.lmp", 25786 },
	{ "underworld11c.lmp", 13884 },
	{ "underworld11d.lmp", 14126 },
	{ "underworld11e.lmp", 8680 },
	{ "underworld11f.lmp", 17197 },
	{ "underworld12.lmp", 55688 },
	{ "underworld12a.lmp", 12590 },
	{ "underworld12b.lmp", 21302 },
	{ "underworld12c.lmp", 14416 },
	{ "underworld12d.lmp", 21990 },
	{ "underworld12e.lmp", 11802 },
	{ "underworld13.lmp", 55688 },
	{ "underworld13a.lmp", 12987 },
	{ "underworld13b.lmp", 14430 },
	{ "underworld13c.lmp", 15534 },
	{ "underworld13d.lmp", 31839 },
	{ "underworld13e.lmp", 23587 },
	{ "underworld14.lmp", 59545 },
	{ "underworld14a.lmp", 10537 },
	{ "underworld14b.lmp", 61997 },
	{ "underworld14c.lmp", 15043 },
	{ "underworld14d.lmp", 7921 },
	{ "underworld14e.lmp", 22800 },
	{ "underworld15.lmp", 119344 },
	{ "underworld15a.lmp", 48251 },
	{ "underworld15b.lmp", 48500 },
	{ "underworld15c.lmp", 39405 },
	{ "underworld15d.lmp", 22539 },
	{ "underworld15e.lmp", 88967 },
	{ "underworld16.lmp", 7801 },
	{ "underworld16a.lmp", 1213 },
	{ "underworld16b.lmp", 1486 },
	{ "underworld16c.lmp", 2136 },
	{ "underworld16d.lmp", 1496 },
	{ "underworld16e.lmp", 2136 },
	{ "underworld17.lmp", 77818 },
	{ "underworld17a.lmp", 21652 },
	{ "underworld17b.lmp", 40598 },
	{ "underworld17c.lmp", 35498 },
	{ "underworld17d.lmp", 16492 },
	{ "underworld17e.lmp", 18237 },
	{ "underworld18.lmp", 130922 },
	{ "underworld18a.lmp", 23389 },
	{ "underworld18b.lmp", 37706 },
	{ "underworld18c.lmp", 26004 },
	{ "underworld18d.lmp", 24737 },
	{ "underworld18e.lmp", 19698 },
	{ "underworld19.lmp", 147747 },
	{ "underworld19a.lmp", 57816 },
	{ "underworld19b.lmp", 47551 },
	{ "underworld19c.lmp", 73591 },
	{ "underworld19d.lmp", 48973 },
	{ "underworld19e.lmp", 111087 },
	{ "underworld20.lmp", 147553 },
	{ "underworld20a.lmp", 44406 },
	{ "underworld20b.lmp", 55526 },
	{ "underworld20c.lmp", 203037 },
	{ "underworld20d.lmp", 43029 },
	{ "underworld20e.lmp", 55805 },
	{ "underworld21.lmp", 144910 },
	{ "underworld21a.lmp", 47510 },
	{ "underworld21b.lmp", 76444 },
	{ "underworld21c.lmp", 48584 },
	{ "underworld21d.lmp", 48359 },
	{ "underworld21e.lmp", 30448 },
	{ "underworld22.lmp", 143298 },
	{ "underworld22a.lmp", 64856 },
	{ "underworld22b.lmp", 51769 },
	{ "underworld22c.lmp", 59299 },
	{ "underworld22d.lmp", 42229 },
	{ "underworld22e.lmp", 36337 },
	{ "underworld23.lmp", 27113 },
	{ "underworld23a.lmp", 4307 },
	{ "underworld23b.lmp", 6289 },
	{ "underworld23c.lmp", 9948 },
	{ "underworld23d.lmp", 10823 },
	{ "underworld23e.lmp", 9851 },
	{ "underworld24.lmp", 27113 },
	{ "underworld24a.lmp", 4337 },
	{ "underworld24b.lmp", 6629 },
	{ "underworld24c.lmp", 7018 },
	{ "underworld24d.lmp", 5098 },
	{ "underworld24e.lmp", 7379 },
	{ "underworld25.lmp", 60508 },
	{ "underworld25a.lmp", 39942 },
	{ "underworld25b.lmp", 16973 },
	{ "underworld25c.lmp", 13818 },
	{ "underworld25d.lmp", 20478 },
	{ "underworld25e.lmp", 13543 },
	{ "underworld26.lmp", 11893 },
	{ "underworld26a.lmp", 2706 },
	{ "underworld26b.lmp", 3206 },
	{ "underworld26c.lmp", 3087 },
	{ "underworld26d.lmp", 3343 },
	{ "underworld26e.lmp", 3667 },
	{ "underworld27.lmp", 122626 },
	{ "underworld27a.lmp", 58537 },
	{ "underworld27b.lmp", 44335 },
	{ "underworld27c.lmp", 22451 },
	{ "underworld27d.lmp", 42906 },
	{ "underworld27e.lmp", 44300 },
	{ "underworld28.lmp", 32847 },
	{ "underworld28a.lmp", 12273 },
	{ "underworld28b.lmp", 7472 },
	{ "underworld28c.lmp", 7413 },
	{ "underworld28d.lmp", 5599 },
	{ "underworld28e.lmp", 8830 },
	{ "underworld29.lmp", 40133 },
	{ "underworld29a.lmp", 15495 },
	{ "underworld29b.lmp", 14568 },
	{ "underworld29c.lmp", 12048 },
	{ "underworld29d.lmp", 16846 },
	{ "underworld29e.lmp", 17605 },
	{ "underworld30.lmp", 112313 },
	{ "underworld30a.lmp", 97910 },
	{ "underworld30b.lmp", 190872 },
	{ "underworld30c.lmp", 47525 },
	{ "underworld30d.lmp", 49543 },
	{ "underworld30e.lmp", 46600 },

	{ "hell.lmp", 145792 },
	{ "hell00.lmp", 43314 },
	{ "hell01.lmp", 15404 },
	{ "hell02.lmp", 17388 },
	{ "hell03.lmp", 23065 },
	{ "hell04.lmp", 2222 },
	{ "hell05.lmp", 3790 },
	{ "hell06.lmp", 2270 },
	{ "hell07.lmp", 3790 },
	{ "hell08.lmp", 10844 },
	{ "hell09.lmp", 11219 },
	{ "hell10.lmp", 19144 },
	{ "hell11.lmp", 4066 },
	{ "hell12.lmp", 20327 },
	{ "hell13.lmp", 3600 },
	{ "hell14.lmp", 28891 },
	{ "hell15.lmp", 99289 },
	{ "hell16.lmp", 357141 },
	{ "hell17.lmp", 14157 },
	{ "hell18.lmp", 14717 },
	{ "hell19.lmp", 5471 },
	{ "hell20.lmp", 15568 },
	{ "hell21.lmp", 8218 },
	{ "hell22.lmp", 14855 },
	{ "hell23.lmp", 8685 },
	{ "hell24.lmp", 61443 },
	{ "hell25.lmp", 53518 },
	{ "hell26.lmp", 4116 },
	{ "hell27.lmp", 2333 },
	{ "hell28.lmp", 1329 },
	{ "hell29.lmp", 36832 },
	{ "hellboss.lmp", 835271 },
	{ "hamlet.lmp", 8619230 },
	{ "caves.lmp", 1065461 },
	{ "caves00.lmp", 70935 },
	{ "caves01.lmp", 13350 },
	{ "caves02.lmp", 6995 },
	{ "caves03.lmp", 11883 },
	{ "caves04.lmp", 15294 },
	{ "caves05.lmp", 10359 },
	{ "caves06.lmp", 8376 },
	{ "caves07.lmp", 9198 },
	{ "caves08.lmp", 6873 },
	{ "caves09.lmp", 102879 },
	{ "caves10.lmp", 28899 },
	{ "caves11.lmp", 35066 },
	{ "caves12.lmp", 39802 },
	{ "caves13.lmp", 45478 },
	{ "caves14.lmp", 37757 },
	{ "caves15.lmp", 26887 },
	{ "caves16.lmp", 233992 },
	{ "caves17.lmp", 31734 },
	{ "caves18.lmp", 36806 },
	{ "caves19.lmp", 25878 },
	{ "caves20.lmp", 33315 },
	{ "caves21.lmp", 16169 },
	{ "caves22.lmp", 26212 },
	{ "caves23.lmp", 35367 },
	{ "caves24.lmp", 81965 },
	{ "caves25.lmp", 19691 },
	{ "caves26.lmp", 41694 },
	{ "caves27.lmp", 10622 },
	{ "caves28.lmp", 8712 },
	{ "caves01a.lmp", 5 },
	{ "caves01b.lmp", 898 },
	{ "caves01c.lmp", 1238 },
	{ "caves01d.lmp", 709 },
	{ "caves01e.lmp", 5 },
	{ "caves01f.lmp", 955 },
	{ "caves09a.lmp", 2836 },
	{ "caves09b.lmp", 6610 },
	{ "caves09c.lmp", 4989 },
	{ "caves09d.lmp", 6168 },
	{ "caves09e.lmp", 3999 },
	{ "caves13a.lmp", 1855 },
	{ "caves13b.lmp", 1678 },
	{ "caves13c.lmp", 6637 },
	{ "caves13d.lmp", 3017 },
	{ "caves13e.lmp", 2892 },
	{ "caves24a.lmp", 175812 },
	{ "caves24b.lmp", 293908 },
	{ "caves24c.lmp", 266809 },
	{ "caves24d.lmp", 72369 },
	{ "cavestocitadel.lmp", 276221 },
	{ "cavessecret.lmp", 198959 },
	{ "caveslair.lmp", 4602584 },
	{ "citadel.lmp", 729069 },
	{ "citadel00.lmp", 23997 },
	{ "citadel01.lmp", 30094 },
	{ "citadel02.lmp", 20605 },
	{ "citadel03.lmp", 23292 },
	{ "citadel04.lmp", 23377 },
	{ "citadel05.lmp", 24208 },
	{ "citadel06.lmp", 22342 },
	{ "citadel07.lmp", 24158 },
	{ "citadel08.lmp", 23917 },
	{ "citadel09.lmp", 31320 },
	{ "citadel10.lmp", 31323 },
	{ "citadel11.lmp", 31330 },
	{ "citadel12.lmp", 31330 },
	{ "citadel13.lmp", 62357 },
	{ "citadel14.lmp", 121487 },
	{ "citadel15.lmp", 62368 },
	{ "citadel16.lmp", 55383 },
	{ "citadel17.lmp", 129662 },
	{ "citadel01a.lmp", 2025 },
	{ "citadel01b.lmp", 2224 },
	{ "citadel01c.lmp", 2025 },
	{ "citadel01d.lmp", 2377 },
	{ "citadel01e.lmp", 582 },
	{ "citadel01f.lmp", 204 },
	{ "citadel01g.lmp", 6 },
	{ "citadel02a.lmp", 4 },
	{ "citadel02b.lmp", 4 },
	{ "citadel02c.lmp", 4 },
	{ "citadel02d.lmp", 4 },
	{ "citadel02e.lmp", 4 },
	{ "citadel02f.lmp", 4 },
	{ "citadel02g.lmp", 4 },
	{ "citadel09a.lmp", 4930 },
	{ "citadel09b.lmp", 6181 },
	{ "citadel09c.lmp", 5270 },
	{ "citadel10a.lmp", 4930 },
	{ "citadel10b.lmp", 8900 },
	{ "citadel10c.lmp", 6072 },
	{ "citadel11a.lmp", 4930 },
	{ "citadel11b.lmp", 7722 },
	{ "citadel11c.lmp", 4860 },
	{ "citadel11d.lmp", 4179 },
	{ "citadel12a.lmp", 4930 },
	{ "citadel12b.lmp", 5874 },
	{ "citadel12c.lmp", 6072 },
	{ "citadel13a.lmp", 21969 },
	{ "citadel13b.lmp", 12526 },
	{ "citadel14a.lmp", 171089 },
	{ "citadel14b.lmp", 42457 },
	{ "citadel14c.lmp", 41425 },
	{ "citadel14d.lmp", 43379 },
	{ "citadel15a.lmp", 9038 },
	{ "citadel15b.lmp", 24927 },
	{ "citadel16a.lmp", 11376 },
	{ "citadel16b.lmp", 6314 },
	{ "citadel17a.lmp", 7279 },
	{ "citadel17b.lmp", 1729 },
	{ "citadel17c.lmp", 2155 },
	{ "citadel17d.lmp", 8339 },
	{ "citadel18.lmp", 31650 },
	{ "citadel19.lmp", 11786 },
	{ "citadel20.lmp", 11905 },
	{ "citadel21.lmp", 59257 },
	{ "citadelsecret.lmp", 104229 },
	{ "bramscastle.lmp", 2954489 },
	{ "sanctum.lmp", 5630316 },

	{ "shop00.lmp", 8172 },
	{ "shop01.lmp", 14016 },
	{ "shop02.lmp", 39289 },
	{ "shop02a.lmp", 4905 },
	{ "shop02b.lmp", 5250 },
	{ "shop02c.lmp", 5481 },
	{ "shop02d.lmp", 4467 },
	{ "shop02e.lmp", 4962 },
	{ "shop02f.lmp", 4025 },
	{ "shop03.lmp", 9675 },
	{ "shop04.lmp", 15493 },
	{ "shop05.lmp", 12766 },
	{ "shop06.lmp", 16915 },
	{ "shop07.lmp", 9336 },
	{ "shop08.lmp", 22622 },
	{ "shop09.lmp", 10094 },
	{ "shop10.lmp", 9078 },

	{ "shop-roomgen00.lmp", 12620 },
	{ "shop-roomgen01.lmp", 9620 },
	{ "shop-roomgen02.lmp", 40191 },
	{ "shop-roomgen02a.lmp", 4754 },
	{ "shop-roomgen02b.lmp", 5250 },
	{ "shop-roomgen02c.lmp", 5481 },
	{ "shop-roomgen02d.lmp", 4467 },
	{ "shop-roomgen02e.lmp", 4335 },
	{ "shop-roomgen02f.lmp", 4025 },
	{ "shop-roomgen03.lmp", 5588 },
	{ "shop-roomgen04.lmp", 5883 },
	{ "shop-roomgen05.lmp", 11557 },
	{ "shop-roomgen06.lmp", 11991 },
	{ "shop-roomgen07.lmp", 9350 },
	{ "shop-roomgen08.lmp", 12734 },
	{ "shop-roomgen09.lmp", 9405 },
	{ "shop-roomgen10.lmp", 8641 },

	{ "shopcitadel.lmp", 25021 },
	{ "warpzone.lmp", 3133088 },
	{ "tutorial_hub.lmp", -1 },
	{ "tutorial1.lmp", -1 },
	{ "tutorial2.lmp", -1 },
	{ "tutorial3.lmp", -1 },
	{ "tutorial4.lmp", -1 },
	{ "tutorial5.lmp", -1 },
	{ "tutorial6.lmp", -1 },
	{ "tutorial7.lmp", -1 },
	{ "tutorial8.lmp", -1 },
	{ "tutorial9.lmp", -1 },
	{ "tutorial10.lmp", -1 }
};

const std::vector<std::string> officialLevelsTxtOrder =
{
	"start",
	"mine",
	"mine",
	"mine",
	"mine",
	"minetoswamp",
	"swamp",
	"swamp",
	"swamp",
	"swamp",
	"swamptolabyrinth",
	"labyrinth",
	"labyrinth",
	"labyrinth",
	"labyrinth",
	"labyrinthtoruins",
	"ruins",
	"ruins",
	"ruins",
	"ruins",
	"boss",
	"hell",
	"hell",
	"hell",
	"hellboss",
	"hamlet",
	"caves",
	"caves",
	"caves",
	"caves",
	"cavestocitadel",
	"citadel",
	"citadel",
	"citadel",
	"citadel",
	"sanctum"
};

const std::vector<std::string> officialSecretlevelsTxtOrder = 
{
	"warpzone",
	"warpzone",
	"warpzone",
	"gnomishmines",
	"minetown",
	"warpzone",
	"underworld",
	"underworld",
	"temple",
	"greatcastle",
	"warpzone",
	"warpzone",
	"sokoban",
	"warpzone",
	"minotaur",
	"warpzone",
	"warpzone",
	"mysticlibrary",
	"warpzone",
	"underworld",
	"underworld",
	"warpzone",
	"warpzone",
	"warpzone",
	"boss",
	"warpzone",
	"warpzone",
	"warpzone",
	"warpzone",
	"caveslair",
	"warpzone",
	"warpzone",
	"warpzone",
	"warpzone",
	"bramscastle",
	"warpzone",
	"warpzone",
	"warpzone"
};

/*-------------------------------------------------------------------------------

	glLoadTexture

	Binds the given image to an opengl texture name

-------------------------------------------------------------------------------*/

void glLoadTexture(SDL_Surface* image, int texnum)
{
	SDL_LockSurface(image);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texid[texnum]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, (image->pitch / 4));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//#ifdef APPLE
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->w, image->h, 0, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, image->pixels);
	//#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	//#endif
	SDL_UnlockSurface(image);
}


bool completePath(char *dest, const char * const filename, const char *base) {
	if (!(filename && filename[0])) {
		return false;
	}

	// Already absolute
	if (filename[0] == '/') {
		strncpy(dest, filename, PATH_MAX);
		return true;
	}
#ifdef NINTENDO
	if (strncmp(filename, base, strlen(base)) == 0) {
		strncpy(dest, filename, PATH_MAX);
		return true;
	}
#endif

#ifdef WINDOWS
	// Already absolute (drive letter in path)
	if ( filename[1] == ':' ) {
		strncpy(dest, filename, PATH_MAX);
		return true;
	}
#endif

	snprintf(dest, PATH_MAX, "%s/%s", base, filename);
	return true;
}

File* openDataFile(const char * const filename, const char * const mode) {
	char path[PATH_MAX];
	completePath(path, filename);
	File* result = FileIO::open(path, mode);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}

DIR* openDataDir(const char * const name) {
	char path[PATH_MAX];
	completePath(path, name);
	DIR * result = opendir(path);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}


bool dataPathExists(const char * const path, bool complete) {
#ifdef NINTENDO
	return true;
#endif
    if (complete) {
	    char full_path[PATH_MAX];
	    completePath(full_path, path);
	    return access(full_path, F_OK) != -1;
    } else {
        return access(path, F_OK) != -1;
    }
}

void openLogFile() {
#ifdef NINTENDO
	return;
#endif
	char path[PATH_MAX];
	completePath(path, "log.txt", outputdir);

	logfile = freopen(path, "wb" /*or "wt"*/, stderr);
}


/*-------------------------------------------------------------------------------

	loadImage

	Loads the image specified in filename, binds it to an opengl texture name,
	and returns the image as an SDL_Surface

-------------------------------------------------------------------------------*/

SDL_Surface* loadImage(char const * const filename)
{
	char full_path[PATH_MAX];
	completePath(full_path, filename);
	SDL_Surface* originalSurface;

	if ( imgref >= MAXTEXTURES )
	{
		printlog("critical error! No more room in allsurfaces[], MAXTEXTURES reached.\n");
		printlog("aborting...\n");
		exit(1);
	}
	if ( (originalSurface = IMG_Load(full_path)) == NULL )
	{
		printlog("error: failed to load image '%s'\n", full_path);
		exit(1); // critical error
		return NULL;
	}

	// translate the original surface to an RGBA surface
	//int w = pow(2, ceil( log(std::max(originalSurface->w,originalSurface->h))/log(2) ) ); // round up to the nearest power of two
	SDL_Surface* newSurface = SDL_CreateRGBSurface(0, originalSurface->w, originalSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(originalSurface, NULL, newSurface, NULL); // blit onto a purely RGBA Surface

	// load the new surface as a GL texture
	allsurfaces[imgref] = newSurface;
	allsurfaces[imgref]->userdata = (void *)((long int)imgref);
	glLoadTexture(allsurfaces[imgref], imgref);

	// free the translated surface
	SDL_FreeSurface(originalSurface);

	imgref++;
	return allsurfaces[imgref - 1];
}

/*-------------------------------------------------------------------------------

	loadVoxel

	Loads a voxel model from the given filename

-------------------------------------------------------------------------------*/

voxel_t* loadVoxel(char* filename)
{
	//char filename2[1024];
	File* file;
	voxel_t* model;

	if ( filename != nullptr )
	{
		//bool has_ext = strstr(filename, ".vox") == NULL;
		//snprintf(filename2, 1024, "%s%s", filename, has_ext ? "" : ".vox");
		const char* path = PHYSFS_getRealDir(filename);
		if ( !path ) {
			printlog("error: loadVoxel could not find file: %s", filename);
			return nullptr;
		}
		std::string filenamePath = path;
		filenamePath.append(PHYSFS_getDirSeparator()).append(filename);

		if ( (file = openDataFile(filenamePath.c_str(), "rb")) == nullptr )
		{
			return nullptr;
		}
		model = (voxel_t*)malloc(sizeof(voxel_t));
		model->sizex = 0;
		file->read(&model->sizex, sizeof(Sint32), 1);
		model->sizey = 0;
		file->read(&model->sizey, sizeof(Sint32), 1);
		model->sizez = 0;
		file->read(&model->sizez, sizeof(Sint32), 1);
		model->data = (Uint8*)malloc(sizeof(Uint8) * model->sizex * model->sizey * model->sizez);
		memset(model->data, 0, sizeof(Uint8)*model->sizex * model->sizey * model->sizez);
		file->read(model->data, sizeof(Uint8), model->sizex * model->sizey * model->sizez);
		file->read(&model->palette, sizeof(Uint8), 256 * 3);
		int c;
		for ( c = 0; c < 256; c++ )
		{
			model->palette[c][0] = model->palette[c][0] << 2;
			model->palette[c][1] = model->palette[c][1] << 2;
			model->palette[c][2] = model->palette[c][2] << 2;
		}
		FileIO::close(file);

		return model;
	}
	else
	{
		return nullptr;
	}
}

#ifndef EDITOR
static ConsoleVariable<int> cvar_hell_ambience("/hell_ambience", 32);
#endif

/*-------------------------------------------------------------------------------

	loadMap

	Loads a map from the given filename

-------------------------------------------------------------------------------*/

int loadMap(const char* filename2, map_t* destmap, list_t* entlist, list_t* creatureList, int *checkMapHash)
{
	File* fp;
	char valid_data[16];
	Uint32 numentities;
	Uint32 c;
	Sint32 x, y;
	Entity* entity;
	Sint32 sprite;
	Stat* myStats;
	Stat* dummyStats;
	sex_t s;
	int editorVersion = 0;
	char filename[1024];
	int mapHashData = 0;
	if ( checkMapHash )
	{
		*checkMapHash = 0;
	}

	char oldmapname[64];
	strcpy(oldmapname, map.name);

	printlog("LoadMap %s", filename2);

	if (! (filename2 && filename2[0]))
	{
		printlog("map filename empty or null");
		return -1;
	}

	if ( !PHYSFS_isInit() )
	{
		strcpy(filename, "maps/");
		strcat(filename, filename2);
	}
	else
	{
		strcpy(filename, filename2);
	}


	// add extension if missing
	if ( strstr(filename, ".lmp") == nullptr )
	{
		strcat(filename, ".lmp");
	}

	// load the file!
	if ((fp = openDataFile(filename, "rb")) == nullptr)
	{
		printlog("warning: failed to open file '%s' for map loading!\n", filename);
		if ( destmap == &map && game )
		{
			printlog("error: main map failed to load, aborting.\n");
			mainloop = 0;
		}
		return -1;
	}

	// read map version number
	fp->read(valid_data, sizeof(char), strlen("BARONY LMPV2.0"));
	if ( strncmp(valid_data, "BARONY LMPV2.7", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.7 version of editor - teleport shrine dest x update
		editorVersion = 27;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.6", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.6 version of editor - player starts/doors/gates
		editorVersion = 26;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.5", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.5 version of editor - scripting
		editorVersion = 25;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.4", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.4 version of editor - boulder trap properties
		editorVersion = 24;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.3", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.3 version of editor - map flags
		editorVersion = 23;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.2", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.2 version of editor - submaps
		editorVersion = 22;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.1", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.1 version of editor - skybox
		editorVersion = 21;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.0", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.0 version of editor
		editorVersion = 2;
	}
	else
	{
		fp->seek(0, File::SeekMode::SET);
		fp->read(valid_data, sizeof(char), strlen("BARONY"));
		if ( strncmp(valid_data, "BARONY", strlen("BARONY")) == 0 )
		{
			// V1.0 version of editor
			editorVersion = 1;
		}
		else
		{
			printlog("warning: file '%s' is an invalid map file.\n", filename);
			FileIO::close(fp);
			if ( destmap == &map && game )
			{
				printlog("error: main map failed to load, aborting.\n");
				mainloop = 0;
			}
			return -1;
		}
	}

	list_FreeAll(entlist);

	if ( destmap == &map )
	{
		// remove old lights
		list_FreeAll(&light_l);
		// remove old world UI
		if ( destmap->worldUI )
		{
			list_FreeAll(map.worldUI);
		}
	}
	if ( destmap->tiles != nullptr )
	{
		free(destmap->tiles);
		destmap->tiles = nullptr;
	}
	if ( destmap->vismap != nullptr )
	{
		free(destmap->vismap);
		destmap->vismap = nullptr;
	}
	fp->read(destmap->name, sizeof(char), 32); // map name
	fp->read(destmap->author, sizeof(char), 32); // map author
	fp->read(&destmap->width, sizeof(Uint32), 1); // map width
	fp->read(&destmap->height, sizeof(Uint32), 1); // map height

	mapHashData += destmap->width + destmap->height;

	// map skybox
	if ( editorVersion == 1 || editorVersion == 2 )
	{
		if ( strncmp(destmap->name, "Hell", 4) == 0 )
		{
			destmap->skybox = 77;
		}
		else
		{
			destmap->skybox = 0;
		}
	}
	else
	{
		fp->read(&destmap->skybox, sizeof(Uint32), 1); // map skybox
	}

	// misc map flags
	if ( editorVersion == 1 || editorVersion == 2 || editorVersion == 21 || editorVersion == 22 )
	{
		for ( c = 0; c < MAPFLAGS; c++ )
		{
			destmap->flags[c] = 0;
		}
	}
	else
	{
		fp->read(destmap->flags, sizeof(Sint32), MAPFLAGS); // map flags
	}
	destmap->tiles = (Sint32*) malloc(sizeof(Sint32) * destmap->width * destmap->height * MAPLAYERS);
	destmap->vismap = (bool*) malloc(sizeof(bool) * destmap->width * destmap->height);
	fp->read(destmap->tiles, sizeof(Sint32), destmap->width * destmap->height * MAPLAYERS);
	fp->read(&numentities, sizeof(Uint32), 1); // number of entities on the map

	for ( c = 0; c < destmap->width * destmap->height * MAPLAYERS; ++c )
	{
		mapHashData += destmap->tiles[c];
	}

	for (c = 0; c < numentities; c++)
	{
		fp->read(&sprite, sizeof(Sint32), 1);
		entity = newEntity(sprite, 0, entlist, nullptr); //TODO: Figure out when we need to assign an entity to the global monster list. And do it!
		switch( editorVersion )
		{	case 1:
				// V1.0 of editor version
			switch ( checkSpriteType(sprite) )
			{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					setSpriteAttributes(entity, nullptr, nullptr);
					break;
				default:
					break;
			}
				break;
			case 2:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
				// V2.0+ of editor version
				switch ( checkSpriteType(sprite) )
				{
					case 1:
						if ( multiplayer != CLIENT )
						{
							// need to give the entity its list stuff.
							// create an empty first node for traversal purposes
							node_t* node2 = list_AddNodeFirst(&entity->children);
							node2->element = NULL;
							node2->deconstructor = &emptyDeconstructor;

							myStats = new Stat(entity->sprite);
							node2 = list_AddNodeLast(&entity->children);
							node2->element = myStats;
							node2->size = sizeof(myStats);
							node2->deconstructor = &statDeconstructor;

							sex_t dummyVar = MALE; 
							// we don't actually embed the sex from the editor
							// advance the fp since we read in 0 always.
							// otherwise it would overwrite the value of a handplaced succubus or a certain icey lich.
							// certainly were a lot of male adventurers locked in cells...
							fp->read(&dummyVar, sizeof(sex_t), 1);
							fp->read(&myStats->name, sizeof(char[128]), 1);
							fp->read(&myStats->HP, sizeof(Sint32), 1);
							fp->read(&myStats->MAXHP, sizeof(Sint32), 1);
							fp->read(&myStats->OLDHP, sizeof(Sint32), 1);
							fp->read(&myStats->MP, sizeof(Sint32), 1);
							fp->read(&myStats->MAXMP, sizeof(Sint32), 1);
							fp->read(&myStats->STR, sizeof(Sint32), 1);
							fp->read(&myStats->DEX, sizeof(Sint32), 1);
							fp->read(&myStats->CON, sizeof(Sint32), 1);
							fp->read(&myStats->INT, sizeof(Sint32), 1);
							fp->read(&myStats->PER, sizeof(Sint32), 1);
							fp->read(&myStats->CHR, sizeof(Sint32), 1);
							fp->read(&myStats->LVL, sizeof(Sint32), 1);
							fp->read(&myStats->GOLD, sizeof(Sint32), 1);

							fp->read(&myStats->RANDOM_MAXHP, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_HP, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_MAXMP, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_MP, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_STR, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_CON, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_DEX, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_INT, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_PER, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_CHR, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_LVL, sizeof(Sint32), 1);
							fp->read(&myStats->RANDOM_GOLD, sizeof(Sint32), 1);

							if ( editorVersion >= 22 )
							{
								fp->read(&myStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM);
							}
							else
							{
								// read old map formats
								fp->read(&myStats->EDITOR_ITEMS, sizeof(Sint32), 96);
							}
							fp->read(&myStats->MISC_FLAGS, sizeof(Sint32), 32);
						}
						//Read dummy values to move fp for the client
						else
						{
							dummyStats = new Stat(entity->sprite);
							fp->read(&dummyStats->sex, sizeof(sex_t), 1);
							fp->read(&dummyStats->name, sizeof(char[128]), 1);
							fp->read(&dummyStats->HP, sizeof(Sint32), 1);
							fp->read(&dummyStats->MAXHP, sizeof(Sint32), 1);
							fp->read(&dummyStats->OLDHP, sizeof(Sint32), 1);
							fp->read(&dummyStats->MP, sizeof(Sint32), 1);
							fp->read(&dummyStats->MAXMP, sizeof(Sint32), 1);
							fp->read(&dummyStats->STR, sizeof(Sint32), 1);
							fp->read(&dummyStats->DEX, sizeof(Sint32), 1);
							fp->read(&dummyStats->CON, sizeof(Sint32), 1);
							fp->read(&dummyStats->INT, sizeof(Sint32), 1);
							fp->read(&dummyStats->PER, sizeof(Sint32), 1);
							fp->read(&dummyStats->CHR, sizeof(Sint32), 1);
							fp->read(&dummyStats->LVL, sizeof(Sint32), 1);
							fp->read(&dummyStats->GOLD, sizeof(Sint32), 1);

							fp->read(&dummyStats->RANDOM_MAXHP, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_HP, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_MAXMP, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_MP, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_STR, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_CON, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_DEX, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_INT, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_PER, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_CHR, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_LVL, sizeof(Sint32), 1);
							fp->read(&dummyStats->RANDOM_GOLD, sizeof(Sint32), 1);

							if ( editorVersion >= 22 )
							{
								fp->read(&dummyStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM);
							}
							else
							{
								fp->read(&dummyStats->EDITOR_ITEMS, sizeof(Sint32), 96);
							}
							fp->read(&dummyStats->MISC_FLAGS, sizeof(Sint32), 32);
							delete dummyStats;
						}
						break;
					case 2:
						fp->read(&entity->yaw, sizeof(real_t), 1);
						fp->read(&entity->skill[9], sizeof(Sint32), 1);
						fp->read(&entity->chestLocked, sizeof(Sint32), 1);
						break;
					case 3:
						fp->read(&entity->skill[10], sizeof(Sint32), 1);
						fp->read(&entity->skill[11], sizeof(Sint32), 1);
						fp->read(&entity->skill[12], sizeof(Sint32), 1);
						fp->read(&entity->skill[13], sizeof(Sint32), 1);
						fp->read(&entity->skill[15], sizeof(Sint32), 1);
						if ( editorVersion >= 22 )
						{
							fp->read(&entity->skill[16], sizeof(Sint32), 1);
						}
						break;
					case 4:
						fp->read(&entity->skill[0], sizeof(Sint32), 1);
						fp->read(&entity->skill[1], sizeof(Sint32), 1);
						fp->read(&entity->skill[2], sizeof(Sint32), 1);
						fp->read(&entity->skill[3], sizeof(Sint32), 1);
						fp->read(&entity->skill[4], sizeof(Sint32), 1);
						fp->read(&entity->skill[5], sizeof(Sint32), 1);
						break;
					case 5:
						fp->read(&entity->yaw, sizeof(real_t), 1);
						fp->read(&entity->crystalNumElectricityNodes, sizeof(Sint32), 1);
						fp->read(&entity->crystalTurnReverse, sizeof(Sint32), 1);
						fp->read(&entity->crystalSpellToActivate, sizeof(Sint32), 1);
						break;
					case 6:
						fp->read(&entity->leverTimerTicks, sizeof(Sint32), 1);
						break;
					case 7:
						if ( editorVersion >= 24 )
						{
							fp->read(&entity->boulderTrapRefireAmount, sizeof(Sint32), 1);
							fp->read(&entity->boulderTrapRefireDelay, sizeof(Sint32), 1);
							fp->read(&entity->boulderTrapPreDelay, sizeof(Sint32), 1);
						}
						else
						{
							setSpriteAttributes(entity, nullptr, nullptr);
						}
						break;
					case 8:
						fp->read(&entity->pedestalOrbType, sizeof(Sint32), 1);
						fp->read(&entity->pedestalHasOrb, sizeof(Sint32), 1);
						fp->read(&entity->pedestalInvertedPower, sizeof(Sint32), 1);
						fp->read(&entity->pedestalInGround, sizeof(Sint32), 1);
						fp->read(&entity->pedestalLockOrb, sizeof(Sint32), 1);
						break;
					case 9:
						fp->read(&entity->teleporterX, sizeof(Sint32), 1);
						fp->read(&entity->teleporterY, sizeof(Sint32), 1);
						fp->read(&entity->teleporterType, sizeof(Sint32), 1);
						break;
					case 10:
						fp->read(&entity->ceilingTileModel, sizeof(Sint32), 1);
						break;
					case 11:
						fp->read(&entity->spellTrapType, sizeof(Sint32), 1);
						fp->read(&entity->spellTrapRefire, sizeof(Sint32), 1);
						fp->read(&entity->spellTrapLatchPower, sizeof(Sint32), 1);
						fp->read(&entity->spellTrapFloorTile, sizeof(Sint32), 1);
						fp->read(&entity->spellTrapRefireRate, sizeof(Sint32), 1);
						break;
					case 12:
						if ( entity->sprite == 60 ) // chair
						{
							if ( editorVersion >= 25 )
							{
								fp->read(&entity->furnitureDir, sizeof(Sint32), 1);
							}
							else
							{
								// don't read data, set default.
								setSpriteAttributes(entity, nullptr, nullptr);
							}
						}
						else
						{
							fp->read(&entity->furnitureDir, sizeof(Sint32), 1);
						}
						break;
					case 13:
						fp->read(&entity->floorDecorationModel, sizeof(Sint32), 1);
						fp->read(&entity->floorDecorationRotation, sizeof(Sint32), 1);
						fp->read(&entity->floorDecorationHeightOffset, sizeof(Sint32), 1);
						if ( editorVersion >= 25 )
						{
							fp->read(&entity->floorDecorationXOffset, sizeof(Sint32), 1);
							fp->read(&entity->floorDecorationYOffset, sizeof(Sint32), 1);
							for ( int i = 8; i < 60; ++i )
							{
								fp->read(&entity->skill[i], sizeof(Sint32), 1);
							}
						}
						break;
					case 14:
						fp->read(&entity->soundSourceToPlay, sizeof(Sint32), 1);
						fp->read(&entity->soundSourceVolume, sizeof(Sint32), 1);
						fp->read(&entity->soundSourceLatchOn, sizeof(Sint32), 1);
						fp->read(&entity->soundSourceDelay, sizeof(Sint32), 1);
						fp->read(&entity->soundSourceOrigin, sizeof(Sint32), 1);
						break;
					case 15:
						fp->read(&entity->lightSourceAlwaysOn, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceBrightness, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceInvertPower, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceLatchOn, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceRadius, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceFlicker, sizeof(Sint32), 1);
						fp->read(&entity->lightSourceDelay, sizeof(Sint32), 1);
						break;
					case 16:
					{
						fp->read(&entity->textSourceColorRGB, sizeof(Sint32), 1);
						fp->read(&entity->textSourceVariables4W, sizeof(Sint32), 1);
						fp->read(&entity->textSourceDelay, sizeof(Sint32), 1);
						fp->read(&entity->textSourceIsScript, sizeof(Sint32), 1);
						for ( int i = 4; i < 60; ++i )
						{
							fp->read(&entity->skill[i], sizeof(Sint32), 1);
						}
						break;
					}
					case 17:
						fp->read(&entity->signalInputDirection, sizeof(Sint32), 1);
						fp->read(&entity->signalActivateDelay, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerInterval, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerRepeatCount, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerLatchInput, sizeof(Sint32), 1);
						break;
					case 18:
						fp->read(&entity->portalCustomSprite, sizeof(Sint32), 1);
						fp->read(&entity->portalCustomSpriteAnimationFrames, sizeof(Sint32), 1);
						fp->read(&entity->portalCustomZOffset, sizeof(Sint32), 1);
						fp->read(&entity->portalCustomLevelsToJump, sizeof(Sint32), 1);
						fp->read(&entity->portalNotSecret, sizeof(Sint32), 1);
						fp->read(&entity->portalCustomRequiresPower, sizeof(Sint32), 1);
						for ( int i = 11; i <= 18; ++i )
						{
							fp->read(&entity->skill[i], sizeof(Sint32), 1);
						}
						break;
					case 19:
						if ( editorVersion >= 25 )
						{
							fp->read(&entity->furnitureDir, sizeof(Sint32), 1);
							fp->read(&entity->furnitureTableSpawnChairs, sizeof(Sint32), 1);
							fp->read(&entity->furnitureTableRandomItemChance, sizeof(Sint32), 1);
						}
						else
						{
							// don't read data, set default.
							setSpriteAttributes(entity, nullptr, nullptr);
						}
						break;
					case 20:
						fp->read(&entity->skill[11], sizeof(Sint32), 1);
						fp->read(&entity->skill[12], sizeof(Sint32), 1);
						fp->read(&entity->skill[15], sizeof(Sint32), 1);
						for ( int i = 40; i <= 52; ++i )
						{
							fp->read(&entity->skill[i], sizeof(Sint32), 1);
						}
						break;
					case 21:
						if ( editorVersion >= 26 )
						{
							fp->read(&entity->doorForceLockedUnlocked, sizeof(Sint32), 1);
							fp->read(&entity->doorDisableLockpicks, sizeof(Sint32), 1);
							fp->read(&entity->doorDisableOpening, sizeof(Sint32), 1);
						}
						break;
					case 22:
						if ( editorVersion >= 26 )
						{
							fp->read(&entity->gateDisableOpening, sizeof(Sint32), 1);
						}
						break;
					case 23:
						if ( editorVersion >= 26 )
						{
							fp->read(&entity->playerStartDir, sizeof(Sint32), 1);
						}
						break;
					case 24:
						fp->read(&entity->statueDir, sizeof(Sint32), 1);
						fp->read(&entity->statueId, sizeof(Sint32), 1);
						break;
					case 25:
						fp->read(&entity->shrineDir, sizeof(Sint32), 1);
						fp->read(&entity->shrineZ, sizeof(Sint32), 1);
						if ( editorVersion >= 27 )
						{
							fp->read(&entity->shrineDestXOffset, sizeof(Sint32), 1);
							fp->read(&entity->shrineDestYOffset, sizeof(Sint32), 1);
						}
						break;
					case 26:
						fp->read(&entity->shrineDir, sizeof(Sint32), 1);
						fp->read(&entity->shrineZ, sizeof(Sint32), 1);
						break;
					case 27:
						fp->read(&entity->colliderDecorationModel, sizeof(Sint32), 1);
						fp->read(&entity->colliderDecorationRotation, sizeof(Sint32), 1);
						fp->read(&entity->colliderDecorationHeightOffset, sizeof(Sint32), 1);
						fp->read(&entity->colliderDecorationXOffset, sizeof(Sint32), 1);
						fp->read(&entity->colliderDecorationYOffset, sizeof(Sint32), 1);
						fp->read(&entity->colliderHasCollision, sizeof(Sint32), 1);
						fp->read(&entity->colliderSizeX, sizeof(Sint32), 1);
						fp->read(&entity->colliderSizeY, sizeof(Sint32), 1);
						fp->read(&entity->colliderMaxHP, sizeof(Sint32), 1);
						fp->read(&entity->colliderDiggable, sizeof(Sint32), 1);
						fp->read(&entity->colliderDamageTypes, sizeof(Sint32), 1);
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		if ( entity->behavior == actMonster || entity->behavior == actPlayer )
		{
			entity->addToCreatureList(creatureList);
		}

		fp->read(&x, sizeof(Sint32), 1);
		fp->read(&y, sizeof(Sint32), 1);
		entity->x = x;
		entity->y = y;
		mapHashData += (sprite * c);
	}

	FileIO::close(fp);

	if ( destmap == &map )
	{
		nummonsters = 0;
		minotaurlevel = 0;

#if defined (USE_FMOD) || defined(USE_OPENAL)
		if ( strcmp(oldmapname, map.name) )
		{
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_DEFAULT )
			{
				levelmusicplaying = false;
			}
		}
#endif

		// create new lightmap
		if ( lightmap != NULL )
		{
			free(lightmap);
		}
		if ( lightmapSmoothed )
		{
			free(lightmapSmoothed);
		}

		lightmap = (int*) malloc(sizeof(Sint32) * destmap->width * destmap->height);
		lightmapSmoothed = (int*)malloc(sizeof(Sint32) * (destmap->width + 2) * (destmap->height + 2));
		if ( strncmp(map.name, "Hell", 4) )
		{
	        memset(lightmap, 0, sizeof(Sint32) * map.width * map.height);
	        memset(lightmapSmoothed, 0, sizeof(Sint32) * (map.width + 2) * (map.height + 2));
		}
		else
		{
			for (c = 0; c < destmap->width * destmap->height; c++ )
			{
				lightmap[c] = 32;
#ifndef EDITOR
				if ( svFlags & SV_FLAG_CHEATS )
				{
					lightmap[c] = *cvar_hell_ambience;
				}
#endif
			}
			for (c = 0; c < (destmap->width + 2) * (destmap->height + 2); c++ )
			{
				lightmapSmoothed[c] = 32;
#ifndef EDITOR
				if ( svFlags & SV_FLAG_CHEATS )
				{
					lightmapSmoothed[c] = *cvar_hell_ambience;
				}
#endif
			}
		}

		// reset minimap
		for ( x = 0; x < MINIMAP_MAX_DIMENSION; x++ )
		{
			for ( y = 0; y < MINIMAP_MAX_DIMENSION; y++ )
			{
				minimap[y][x] = 0;
			}
		}

		// reset cameras
		for (int c = 0; c < MAXPLAYERS; ++c) {
			auto& camera = cameras[c];
			if ( game )
			{
				camera.x = -32;
				camera.y = -32;
				camera.z = 0;
				camera.ang = 3 * PI / 2;
				camera.vang = 0;
			}
			else
			{
				camera.x = 2;
				camera.y = 2;
				camera.z = 0;
				camera.ang = 0;
				camera.vang = 0;
			}
		}

		// shoparea
		if ( shoparea )
		{
			free(shoparea);
		}
		shoparea = (bool*) malloc(sizeof(bool) * destmap->width * destmap->height);
		for ( x = 0; x < destmap->width; x++ )
		{
			for ( y = 0; y < destmap->height; y++ )
			{
				shoparea[y + x * destmap->height] = false;
			}
		}
	}

	std::string mapShortName = filename2;
	size_t found = mapShortName.rfind("/");
	if ( found != std::string::npos )
	{
		mapShortName = mapShortName.substr(found + 1);
	}
	else
	{
		found = mapShortName.rfind("\\");
		if ( found != std::string::npos )
		{
			mapShortName = mapShortName.substr(found + 1);
		}
	}

	for ( c = 0; c < 512; c++ )
	{
		keystatus[c] = 0;
	}

	if ( checkMapHash != nullptr )
	{
		auto it = mapHashes.find(mapShortName);
		if ( it != mapHashes.end() && ((*it).second == mapHashData || (*it).second == -1) )
		{
			//printlog("MAP HASH SUCCESS");
			*checkMapHash = 1;
		}
		else
		{
			printlog("Notice: Unable to verify map %s hash %d.", filename2, mapHashData);
			*checkMapHash = 0;
		}
	}

    size_t size = std::min(mapShortName.size(), sizeof(destmap->filename) - 1);
    memcpy(destmap->filename, mapShortName.c_str(), size);
    destmap->filename[size] = '\0';

	return numentities;
}

/*-------------------------------------------------------------------------------

	saveMap

	Saves a map to the given filename

-------------------------------------------------------------------------------*/

int saveMap(const char* filename2)
{
	File* fp;
	Uint32 numentities = 0;
	node_t* node;
	Entity* entity;
	char filename[256];
	Sint32 x, y;
	Stat* myStats;

	if ( filename2 != NULL && strcmp(filename2, "") )
	{
		if ( !PHYSFS_isInit() )
		{
			strcpy(filename, "maps/");
			strcat(filename, filename2);
		}
		else
		{
			strcpy(filename, filename2);
		}

		if ( strstr(filename, ".lmp") == NULL )
		{
			strcat(filename, ".lmp");
		}

		if ((fp = openDataFile(filename, "wb")) == NULL)
		{
			printlog("warning: failed to open file '%s' for map saving!\n", filename);
			return 1;
		}

		fp->write("BARONY LMPV2.7", sizeof(char), strlen("BARONY LMPV2.0")); // magic code
		fp->write(map.name, sizeof(char), 32); // map filename
		fp->write(map.author, sizeof(char), 32); // map author
		fp->write(&map.width, sizeof(Uint32), 1); // map width
		fp->write(&map.height, sizeof(Uint32), 1); // map height
		fp->write(&map.skybox, sizeof(Uint32), 1); // map skybox
		fp->write(map.flags, sizeof(Sint32), MAPFLAGS); // map flags
		fp->write(map.tiles, sizeof(Sint32), map.width * map.height * MAPLAYERS);
		for (node = map.entities->first; node != nullptr; node = node->next)
		{
			++numentities;
		}
		fp->write(&numentities, sizeof(Uint32), 1); // number of entities on the map
		for (node = map.entities->first; node != nullptr; node = node->next)
		{
			entity = (Entity*) node->element;
			fp->write(&entity->sprite, sizeof(Sint32), 1);

			switch ( checkSpriteType(entity->sprite) )
			{
				case 1:
					// monsters
					myStats = entity->getStats();
					fp->write(&myStats->sex, sizeof(sex_t), 1);
					fp->write(&myStats->name, sizeof(char[128]), 1);
					fp->write(&myStats->HP, sizeof(Sint32), 1);
					fp->write(&myStats->MAXHP, sizeof(Sint32), 1);
					fp->write(&myStats->OLDHP, sizeof(Sint32), 1);
					fp->write(&myStats->MP, sizeof(Sint32), 1);
					fp->write(&myStats->MAXMP, sizeof(Sint32), 1);
					fp->write(&myStats->STR, sizeof(Sint32), 1);
					fp->write(&myStats->DEX, sizeof(Sint32), 1);
					fp->write(&myStats->CON, sizeof(Sint32), 1);
					fp->write(&myStats->INT, sizeof(Sint32), 1);
					fp->write(&myStats->PER, sizeof(Sint32), 1);
					fp->write(&myStats->CHR, sizeof(Sint32), 1);
					fp->write(&myStats->LVL, sizeof(Sint32), 1);
					fp->write(&myStats->GOLD, sizeof(Sint32), 1);

					fp->write(&myStats->RANDOM_MAXHP, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_HP, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_MAXMP, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_MP, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_STR, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_CON, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_DEX, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_INT, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_PER, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_CHR, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_LVL, sizeof(Sint32), 1);
					fp->write(&myStats->RANDOM_GOLD, sizeof(Sint32), 1);

					fp->write(&myStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM);
					fp->write(&myStats->MISC_FLAGS, sizeof(Sint32), 32);
					break;
				case 2:
					// chests
					fp->write(&entity->yaw, sizeof(real_t), 1);
					fp->write(&entity->skill[9], sizeof(Sint32), 1);
					fp->write(&entity->chestLocked, sizeof(Sint32), 1);
					break;
				case 3:
					// items
					fp->write(&entity->skill[10], sizeof(Sint32), 1);
					fp->write(&entity->skill[11], sizeof(Sint32), 1);
					fp->write(&entity->skill[12], sizeof(Sint32), 1);
					fp->write(&entity->skill[13], sizeof(Sint32), 1);
					fp->write(&entity->skill[15], sizeof(Sint32), 1);
					fp->write(&entity->skill[16], sizeof(Sint32), 1);
					break;
				case 4:
					fp->write(&entity->skill[0], sizeof(Sint32), 1);
					fp->write(&entity->skill[1], sizeof(Sint32), 1);
					fp->write(&entity->skill[2], sizeof(Sint32), 1);
					fp->write(&entity->skill[3], sizeof(Sint32), 1);
					fp->write(&entity->skill[4], sizeof(Sint32), 1);
					fp->write(&entity->skill[5], sizeof(Sint32), 1);
					break;
				case 5:
					fp->write(&entity->yaw, sizeof(real_t), 1);
					fp->write(&entity->crystalNumElectricityNodes, sizeof(Sint32), 1);
					fp->write(&entity->crystalTurnReverse, sizeof(Sint32), 1);
					fp->write(&entity->crystalSpellToActivate, sizeof(Sint32), 1);
					break;
				case 6:
					fp->write(&entity->leverTimerTicks, sizeof(Sint32), 1);
					break;
				case 7:
					fp->write(&entity->boulderTrapRefireAmount, sizeof(Sint32), 1);
					fp->write(&entity->boulderTrapRefireDelay, sizeof(Sint32), 1);
					fp->write(&entity->boulderTrapPreDelay, sizeof(Sint32), 1);
					break;
				case 8:
					fp->write(&entity->pedestalOrbType, sizeof(Sint32), 1);
					fp->write(&entity->pedestalHasOrb, sizeof(Sint32), 1);
					fp->write(&entity->pedestalInvertedPower, sizeof(Sint32), 1);
					fp->write(&entity->pedestalInGround, sizeof(Sint32), 1);
					fp->write(&entity->pedestalLockOrb, sizeof(Sint32), 1);
					break;
				case 9:
					fp->write(&entity->teleporterX, sizeof(Sint32), 1);
					fp->write(&entity->teleporterY, sizeof(Sint32), 1);
					fp->write(&entity->teleporterType, sizeof(Sint32), 1);
					break;
				case 10:
					fp->write(&entity->ceilingTileModel, sizeof(Sint32), 1);
					break;
				case 11:
					fp->write(&entity->spellTrapType, sizeof(Sint32), 1);
					fp->write(&entity->spellTrapRefire, sizeof(Sint32), 1);
					fp->write(&entity->spellTrapLatchPower, sizeof(Sint32), 1);
					fp->write(&entity->spellTrapFloorTile, sizeof(Sint32), 1);
					fp->write(&entity->spellTrapRefireRate, sizeof(Sint32), 1);
					break;
				case 12:
					fp->write(&entity->furnitureDir, sizeof(Sint32), 1);
					break;
				case 13:
					fp->write(&entity->floorDecorationModel, sizeof(Sint32), 1);
					fp->write(&entity->floorDecorationRotation, sizeof(Sint32), 1);
					fp->write(&entity->floorDecorationHeightOffset, sizeof(Sint32), 1);
					fp->write(&entity->floorDecorationXOffset, sizeof(Sint32), 1);
					fp->write(&entity->floorDecorationYOffset, sizeof(Sint32), 1);
					for ( int i = 8; i < 60; ++i )
					{
						fp->write(&entity->skill[i], sizeof(Sint32), 1);
					}
					break;
				case 14:
					fp->write(&entity->soundSourceToPlay, sizeof(Sint32), 1);
					fp->write(&entity->soundSourceVolume, sizeof(Sint32), 1);
					fp->write(&entity->soundSourceLatchOn, sizeof(Sint32), 1);
					fp->write(&entity->soundSourceDelay, sizeof(Sint32), 1);
					fp->write(&entity->soundSourceOrigin, sizeof(Sint32), 1);
					break;
				case 15:
					fp->write(&entity->lightSourceAlwaysOn, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceBrightness, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceInvertPower, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceLatchOn, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceRadius, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceFlicker, sizeof(Sint32), 1);
					fp->write(&entity->lightSourceDelay, sizeof(Sint32), 1);
					break;
				case 16:
				{
					fp->write(&entity->textSourceColorRGB, sizeof(Sint32), 1);
					fp->write(&entity->textSourceVariables4W, sizeof(Sint32), 1);
					fp->write(&entity->textSourceDelay, sizeof(Sint32), 1);
					fp->write(&entity->textSourceIsScript, sizeof(Sint32), 1);
					for ( int i = 4; i < 60; ++i )
					{
						fp->write(&entity->skill[i], sizeof(Sint32), 1);
					}
					break;
				}
				case 17:
					fp->write(&entity->signalInputDirection, sizeof(Sint32), 1);
					fp->write(&entity->signalActivateDelay, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerInterval, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerRepeatCount, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerLatchInput, sizeof(Sint32), 1);
					break;
				case 18:
					fp->write(&entity->portalCustomSprite, sizeof(Sint32), 1);
					fp->write(&entity->portalCustomSpriteAnimationFrames, sizeof(Sint32), 1);
					fp->write(&entity->portalCustomZOffset, sizeof(Sint32), 1);
					fp->write(&entity->portalCustomLevelsToJump, sizeof(Sint32), 1);
					fp->write(&entity->portalNotSecret, sizeof(Sint32), 1);
					fp->write(&entity->portalCustomRequiresPower, sizeof(Sint32), 1);
					for ( int i = 11; i <= 18; ++i )
					{
						fp->write(&entity->skill[i], sizeof(Sint32), 1);
					}
					break;
				case 19:
					fp->write(&entity->furnitureDir, sizeof(Sint32), 1);
					fp->write(&entity->furnitureTableSpawnChairs, sizeof(Sint32), 1);
					fp->write(&entity->furnitureTableRandomItemChance, sizeof(Sint32), 1);
					break;
				case 20:
					fp->write(&entity->skill[11], sizeof(Sint32), 1);
					fp->write(&entity->skill[12], sizeof(Sint32), 1);
					fp->write(&entity->skill[15], sizeof(Sint32), 1);
					for ( int i = 40; i <= 52; ++i )
					{
						fp->write(&entity->skill[i], sizeof(Sint32), 1);
					}
					break;
				case 21:
					fp->write(&entity->doorForceLockedUnlocked, sizeof(Sint32), 1);
					fp->write(&entity->doorDisableLockpicks, sizeof(Sint32), 1);
					fp->write(&entity->doorDisableOpening, sizeof(Sint32), 1);
					break;
				case 22:
					fp->write(&entity->gateDisableOpening, sizeof(Sint32), 1);
					break;
				case 23:
					fp->write(&entity->playerStartDir, sizeof(Sint32), 1);
					break;
				case 24:
					fp->write(&entity->statueDir, sizeof(Sint32), 1);
					fp->write(&entity->statueId, sizeof(Sint32), 1);
					break;
				case 25:
					fp->write(&entity->shrineDir, sizeof(Sint32), 1);
					fp->write(&entity->shrineZ, sizeof(Sint32), 1);
					fp->write(&entity->shrineDestXOffset, sizeof(Sint32), 1);
					fp->write(&entity->shrineDestYOffset, sizeof(Sint32), 1);
					break;
				case 26:
					fp->write(&entity->shrineDir, sizeof(Sint32), 1);
					fp->write(&entity->shrineZ, sizeof(Sint32), 1);
					break;
				case 27:
					fp->write(&entity->colliderDecorationModel, sizeof(Sint32), 1);
					fp->write(&entity->colliderDecorationRotation, sizeof(Sint32), 1);
					fp->write(&entity->colliderDecorationHeightOffset, sizeof(Sint32), 1);
					fp->write(&entity->colliderDecorationXOffset, sizeof(Sint32), 1);
					fp->write(&entity->colliderDecorationYOffset, sizeof(Sint32), 1);
					fp->write(&entity->colliderHasCollision, sizeof(Sint32), 1);
					fp->write(&entity->colliderSizeX, sizeof(Sint32), 1);
					fp->write(&entity->colliderSizeY, sizeof(Sint32), 1);
					fp->write(&entity->colliderMaxHP, sizeof(Sint32), 1);
					fp->write(&entity->colliderDiggable, sizeof(Sint32), 1);
					fp->write(&entity->colliderDamageTypes, sizeof(Sint32), 1);
					break;
				default:
					break;
			}

			x = entity->x;
			y = entity->y;
			fp->write(&x, sizeof(Sint32), 1);
			fp->write(&y, sizeof(Sint32), 1);
		}
		FileIO::close(fp);
		return 0;
	}
	else
	{
		return 1;
	}
}

/*-------------------------------------------------------------------------------

	readFile

	Simply reads the contents of an entire file into a char array

-------------------------------------------------------------------------------*/

char* readFile(char* filename)
{
	long input_file_size = 0;
	char* file_contents = NULL;
	File* input_file = openDataFile(filename, "rb");
	if (!input_file) {
		printlog("Open failed: %s", strerror(errno));
		goto out_input_file;
	}

	input_file_size = input_file->size();
	if (input_file_size > (1<<30)) {
		printlog("Unreasonable size: %ld", input_file_size);
		goto out_input_file;
	}
	
	input_file->seek(0, File::SeekMode::SET);
	file_contents = static_cast<char*>(malloc((input_file_size + 1) * sizeof(char)));
	input_file->read(file_contents, sizeof(char), input_file_size);
	file_contents[input_file_size] = 0;

out_input_file:
	FileIO::close(input_file);
	return file_contents;
}

/*-------------------------------------------------------------------------------

	directoryContents

	Stores the contents of a directory in a list

-------------------------------------------------------------------------------*/

std::list<std::string> directoryContents(const char* directory, bool includeSubdirectory, bool includeFiles)
{
	std::list<std::string> list;
	char fullPath[PATH_MAX];
	completePath(fullPath, directory);
	DIR* dir = opendir(fullPath);
	struct dirent* entry = NULL;

	if ( !dir )
	{
		printlog( "[directoryContents()] Failed to open directory \"%s\".\n", directory);
		return list;
	}

	struct stat cur;
	char curPath[PATH_MAX];
	while ((entry = readdir(dir)) != NULL)
	{
		strcpy(curPath, fullPath);
		strcat(curPath, entry->d_name);

		if (stat(curPath, &cur) != 0)
		{
			continue;
		}
		if ( includeFiles )
		{
			if ((cur.st_mode & S_IFMT) == S_IFREG)
			{
				list.push_back(entry->d_name);
			}
		}
		if ( includeSubdirectory )
		{
			if ((cur.st_mode & S_IFMT) == S_IFDIR)
			{
				list.push_back(entry->d_name);
			}
		}
	}

	closedir(dir);

	return list;
}

std::vector<std::string> getLinesFromDataFile(std::string filename)
{
	std::vector<std::string> lines;
#ifdef NINTENDO
	std::string filepath(filename);
#else
	std::string filepath(datadir);
	filepath += "/";
	filepath += filename;
#endif
	std::ifstream file(filepath);
	if ( !file )
	{
		std::ifstream file(filename); // check absolute path.
		if ( !file)
		{
			printlog("Error: Failed to open file \"%s\"", filename.c_str());
			return lines;
		}
		else
		{
			std::string line;
			while ( std::getline(file, line) )
			{
				if ( !line.empty() )
				{
					lines.push_back(line);
				}
			}
			file.close();
		}
	}
	else
	{
		std::string line;
		while ( std::getline(file, line) )
		{
			if ( !line.empty() )
			{
				lines.push_back(line);
			}
		}
		file.close();
	}

	return lines;
}

int physfsLoadMapFile(int levelToLoad, Uint32 seed, bool useRandSeed, int* checkMapHash)
{
	std::string mapsDirectory; // store the full file path here.
	std::string line = "";
	if ( loadCustomNextMap.compare("") != 0 )
	{
		line = "map: " + loadCustomNextMap;
		loadCustomNextMap = "";
	}
	else
	{
		if ( !secretlevel )
		{
			mapsDirectory = PHYSFS_getRealDir(LEVELSFILE);
			mapsDirectory.append(PHYSFS_getDirSeparator()).append(LEVELSFILE);
		}
		else
		{
			mapsDirectory = PHYSFS_getRealDir(SECRETLEVELSFILE);
			mapsDirectory.append(PHYSFS_getDirSeparator()).append(SECRETLEVELSFILE);
		}
		printlog("Maps directory: %s", mapsDirectory.c_str());
		std::vector<std::string> levelsList = getLinesFromDataFile(mapsDirectory);
		line = levelsList.front();
		int levelsCounted = 0;
		if ( levelToLoad > 0 ) // if level == 0, then load up the first map.
		{
			for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end() && levelsCounted <= levelToLoad; ++i )
			{
				// process i, iterate through all the map levels until currentlevel.
				line = *i;
				if ( line[0] == '\n' )
				{
					continue;
				}
				++levelsCounted;
			}
		}
	}
	std::size_t found = line.find(' ');
	char tempstr[1024];
	if ( found != std::string::npos )
	{
		std::string mapType = line.substr(0, found);
		std::string mapName;
		mapName = line.substr(found + 1, line.find('\n'));
		std::size_t carriageReturn = mapName.find('\r');
		if ( carriageReturn != std::string::npos )
		{
			mapName.erase(carriageReturn);
			printlog("%s", mapName.c_str());
		}
		if ( mapType.compare("map:") == 0 )
		{
			strncpy(tempstr, mapName.c_str(), mapName.length());
			tempstr[mapName.length()] = '\0';
			mapName = physfsFormatMapName(tempstr);
			if ( useRandSeed )
			{
			    mapseed = local_rng.rand();
			}
			if ( checkMapHash )
			{
				return loadMap(mapName.c_str(), &map, map.entities, map.creatures, checkMapHash);
			}
			else
			{
				return loadMap(mapName.c_str(), &map, map.entities, map.creatures);
			}
		}
		else if ( mapType.compare("gen:") == 0 )
		{
			std::size_t secretChanceFound = mapName.find(" secret%: ");
			std::size_t darkmapChanceFound = mapName.find(" darkmap%: ");
			std::size_t minotaurChanceFound = mapName.find(" minotaur%: ");
			std::size_t disableNormalExitFound = mapName.find(" noexit");
			std::string parameterStr = "";
			std::tuple<int, int, int, int> mapParameters = std::make_tuple(-1, -1, -1, 0);
			if ( secretChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(secretChanceFound + strlen(" secret%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) = -1;
				}
			}
			if ( darkmapChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(darkmapChanceFound + strlen(" darkmap%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) = -1;
				}
			}
			if ( minotaurChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(minotaurChanceFound + strlen(" minotaur%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) = -1;
				}
			}
			if ( disableNormalExitFound != std::string::npos )
			{
				std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) = 1;
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));

			strncpy(tempstr, mapName.c_str(), mapName.length());
			tempstr[mapName.length()] = '\0';
			if ( useRandSeed )
			{
				return generateDungeon(tempstr, local_rng.rand(), mapParameters);
			}
			else
			{
				return generateDungeon(tempstr, seed, mapParameters);
			}
		}
		//printlog("%s", mapName.c_str());
	}
	return 0;
}

std::list<std::string> physfsGetFileNamesInDirectory(const char* dir)
{
	std::list<std::string> filenames;
	char **rc = PHYSFS_enumerateFiles(dir);
	if ( rc == NULL )
	{
		printlog("[PhysFS]: Error: Failed to enumerate filenames in directory '%s': %s", dir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return filenames;
	}
	char **i;
	std::string file;
	for ( i = rc; *i != NULL; i++ )
	{
		file = *i;
		//printlog(" * We've got [%s].\n", file.c_str());
		filenames.push_back(file);
	}
	PHYSFS_freeList(rc);
	return filenames;
}

std::string physfsFormatMapName(char const * const levelfilename)
{
	std::string fullMapPath;
	std::string mapFileName = "maps/";
	mapFileName.append(levelfilename);
	if ( mapFileName.find(".lmp") == std::string::npos )
	{
		mapFileName.append(".lmp");
	}
	//printlog("format map name: %s", mapFileName.c_str());
	if ( PHYSFS_getRealDir(mapFileName.c_str()) != NULL )
	{
		//printlog("format map name: %s", mapFileName.c_str());
		fullMapPath = PHYSFS_getRealDir(mapFileName.c_str());
		fullMapPath.append(PHYSFS_getDirSeparator()).append(mapFileName);
	}
	return fullMapPath;
}

bool physfsSearchModelsToUpdate()
{
	if ( !PHYSFS_getRealDir("models/models.txt") )
	{
		printlog("error: could not find file: %s", "models/models.txt");
		return false;
	}
	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(name, PATH_MAX);
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string modelRealDir = PHYSFS_getRealDir(name);
			if ( modelRealDir.compare("./") != 0 )
			{
				FileIO::close(fp);
				return true;
			}
		}
	}
	FileIO::close(fp);
	return false;
}

bool physfsModelIndexUpdate(int &start, int &end, bool freePreviousModels)
{
	if ( !PHYSFS_getRealDir("models/models.txt") )
	{
		printlog("error: could not find file: %s", "models/models.txt");
		return false;
	}
	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	char modelName[PATH_MAX];
	int startnum = 0;
	int endnum = nummodels;
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File *fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(modelName, PATH_MAX);
		bool modelHasBeenModified = false;
		// has this model index been modified?
		std::vector<int>::iterator it = gamemods_modelsListModifiedIndexes.end();
		if ( !gamemods_modelsListModifiedIndexes.empty() )
		{
			it = std::find(gamemods_modelsListModifiedIndexes.begin(), 
				gamemods_modelsListModifiedIndexes.end(), c);
			if ( it != gamemods_modelsListModifiedIndexes.end() )
			{
				modelHasBeenModified = true; // found the model in the vector.
			}
		}

		if ( !PHYSFS_getRealDir(modelName) )
		{
			printlog("error: could not find file: %s", modelName);
			continue;
		}
		std::string modelPath = PHYSFS_getRealDir(modelName);
		if ( modelHasBeenModified || modelPath.compare("./") != 0 )
		{
			if ( !modelHasBeenModified )
			{
				// add this model index to say we've modified it as the base dir is not default.
				gamemods_modelsListModifiedIndexes.push_back(c);
			}
			else
			{
				if ( modelPath.compare("./") == 0 )
				{
					// model returned to base directory, remove from the modified index list.
					gamemods_modelsListModifiedIndexes.erase(it);
				}
			}

			if ( c < nummodels )
			{
				if ( models[c] != NULL )
				{
					if ( models[c]->data )
					{
						free(models[c]->data);
					}
					free(models[c]);
				}
			}
			else
			{
				printlog("[PhysFS]: WARNING: Loading a new model: %d outside normal nummodels: %d range - Need special handling case to free model after use", c, nummodels);
			}
			models[c] = loadVoxel(modelName);

			// this index is not found in the normal models folder.
			// store the lowest found model number inside startnum.
			if ( startnum == 0 || c < startnum )
			{
				startnum = c;
			}

			// store the higher end model num in endnum.
			if ( endnum == nummodels )
			{
				endnum = c + 1;
			}
			else if ( c + 1 > endnum )
			{
				endnum = c + 1;
			}
		}
	}
	if ( startnum == endnum )
	{
		endnum = std::min(static_cast<int>(nummodels), endnum + 1); // if both indices are the same, then models won't load.
	}
	printlog("[PhysFS]: Models file not in default directory... reloading models from index %d to %d\n", startnum, endnum);
	start = startnum;
	end = endnum;

	// now free polymodels as we'll be loading them up later.
	if ( freePreviousModels )
	{
		for ( int c = std::max(1, start); c < end && c < nummodels; ++c )
		{
			// cannot free index 0 - null object
			if ( polymodels[c].faces )
			{
				free(polymodels[c].faces);
			}
			if ( polymodels[c].vbo )
			{
				glDeleteBuffers(1, &polymodels[c].vbo);
			}
			if ( polymodels[c].colors )
			{
				glDeleteBuffers(1, &polymodels[c].colors);
			}
			if ( polymodels[c].va )
			{
				glDeleteVertexArrays(1, &polymodels[c].va);
			}
			if ( polymodels[c].colors_shifted )
			{
				glDeleteBuffers(1, &polymodels[c].colors_shifted);
			}
			if ( polymodels[c].grayscale_colors )
			{
				glDeleteVertexArrays(1, &polymodels[c].grayscale_colors);
			}
			if ( polymodels[c].grayscale_colors_shifted )
			{
				glDeleteBuffers(1, &polymodels[c].grayscale_colors_shifted);
			}
		}
	}

	FileIO::close(fp);
	return true;
}

bool physfsSearchSoundsToUpdate()
{
	if ( no_sound )
	{
		return false;
	}
	if ( !PHYSFS_getRealDir("sound/sounds.txt") )
	{
		printlog("error: could not find file: %s", "sound/sounds.txt");
		return false;
	}
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	if ( soundsDirectory.compare("./") != 0 )
	{
		return true;
	}
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	File* fp = openDataFile(soundsDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(name, PATH_MAX);
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string soundRealDir = PHYSFS_getRealDir(name);
			if ( soundRealDir.compare("./") != 0 )
			{
				FileIO::close(fp);
				return true;
			}
		}
	}
	FileIO::close(fp);
	return false;
}

void physfsReloadSounds(bool reloadAll)
{
	if ( no_sound )
	{
		return;
	}
	if ( !PHYSFS_getRealDir("sound/sounds.txt") )
	{
		printlog("error: could not find file: %s", "sound/sounds.txt");
		return;
	}
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	File* fp = openDataFile(soundsDirectory.c_str(), "rb");
	char name[PATH_MAX];

	printlog("freeing sounds and loading modded sounds...\n");
	if ( reloadAll )
	{
#ifdef SOUND
		if ( sounds != nullptr )
		{
			for ( int c = 0; c < numsounds; c++ )
			{
				if ( sounds[c] != nullptr )
				{
#ifdef USE_FMOD
					sounds[c]->release();    //Free the sound in FMOD
#endif
#ifdef USE_OPENAL
					OPENAL_Sound_Release(sounds[c]); //Free the sound in OPENAL
#endif
					sounds[c] = nullptr;
				}
			}
		}
#endif
	}

	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(name, PATH_MAX);
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string soundRealDir = PHYSFS_getRealDir(name);
			if ( reloadAll || soundRealDir.compare("./") != 0 )
			{
				std::string soundFile = soundRealDir;
				soundFile.append(PHYSFS_getDirSeparator()).append(name);
#ifdef USE_FMOD
				if ( !reloadAll )
				{
					sounds[c]->release();
					sounds[c] = nullptr;
				}
				fmod_result = fmod_system->createSound(soundFile.c_str(), FMOD_DEFAULT | FMOD_3D, nullptr, &sounds[c]); //TODO: FMOD_SOFTWARE -> FMOD_DEFAULT?
				if ( FMODErrorCheck() )
				{
					printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", name, c + 1);
				}
#endif
#ifdef USE_OPENAL
				if ( !reloadAll )
				{
					OPENAL_Sound_Release(sounds[c]);
				}
				OPENAL_CreateSound(soundFile.c_str(), true, &sounds[c]);
#endif 
			}
		}
	}
	FileIO::close(fp);
}

bool physfsSearchSpritesToUpdate() //TODO: NX PORT: Any changes needed here?
{
	if ( !PHYSFS_getRealDir("images/sprites.txt") )
	{
		printlog("error: could not find file: %s", "images/sprites.txt");
		return false;
	}
	std::string spritesDirectory = PHYSFS_getRealDir("images/sprites.txt");
	spritesDirectory.append(PHYSFS_getDirSeparator()).append("images/sprites.txt");
	File* fp = openDataFile(spritesDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, PATH_MAX);
		while ( fp->getc() != '\n' )
		{
			if ( fp->eof() )
			{
				break;
			}
		}

		if ( PHYSFS_getRealDir(name) != nullptr )
		{
			std::string spritesRealDir = PHYSFS_getRealDir(name);
			if ( spritesRealDir.compare("./") != 0 )
			{
				FileIO::close(fp);
				printlog("[PhysFS]: Found modified sprite in sprites/ directory, reloading all sprites...");
				return true;
			}
		}
	}
	FileIO::close(fp);
	return false;
}

void physfsReloadSprites(bool reloadAll) //TODO: NX PORT: Any changes needed here?
{
	if ( !PHYSFS_getRealDir("images/sprites.txt") )
	{
		printlog("error: could not find file: %s", "images/sprites.txt");
		return;
	}
	std::string spritesDirectory = PHYSFS_getRealDir("images/sprites.txt");
	spritesDirectory.append(PHYSFS_getDirSeparator()).append("images/sprites.txt");
	printlog("[PhysFS]: Loading sprites from directory %s...\n", spritesDirectory.c_str());
	File* fp = openDataFile(spritesDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, PATH_MAX);
		while ( fp->getc() != '\n' )
		{
			if ( fp->eof() )
			{
				break;
			}
		}

		if ( PHYSFS_getRealDir(name) != nullptr )
		{
			std::string spritesRealDir = PHYSFS_getRealDir(name);
			if ( reloadAll || spritesRealDir.compare("./") != 0 )
			{
				std::string spriteFile = spritesRealDir;
				spriteFile.append(PHYSFS_getDirSeparator()).append(name);
				if ( sprites[c] )
				{
					SDL_FreeSurface(sprites[c]);
				}
				char fullname[PATH_MAX];
				strncpy(fullname, spriteFile.c_str(), PATH_MAX - 1);
				sprites[c] = loadImage(fullname);
				if ( nullptr != sprites[c]  )
				{
					//Whee
				}
				else
				{
					printlog("warning: failed to load '%s' listed at line %d in %s\n", name, c + 1, spritesDirectory.c_str());
					if ( 0 == c )
					{
						printlog("sprite 0 cannot be NULL!\n");
						FileIO::close(fp);
						return;
					}
				}
			}
		}
	}
	FileIO::close(fp);
}

bool physfsSearchTilesToUpdate()
{
	if ( !PHYSFS_getRealDir("images/tiles.txt") )
	{
		printlog("error: could not find file: %s", "images/tiles.txt");
		return false;
	}
	std::string tilesDirectory = PHYSFS_getRealDir("images/tiles.txt");
	tilesDirectory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
	File* fp = openDataFile(tilesDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(name, PATH_MAX);
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string tileRealDir = PHYSFS_getRealDir(name);
			if ( tileRealDir.compare("./") != 0 )
			{
				FileIO::close(fp);
				printlog("[PhysFS]: Found modified tile in tiles/ directory, reloading all tiles...");
				return true;
			}
		}
	}
	FileIO::close(fp);
	return false;
}

void physfsReloadTiles(bool reloadAll)
{
	if ( !PHYSFS_getRealDir("images/tiles.txt") )
	{
		printlog("error: could not find file: %s", "images/tiles.txt");
		return;
	}
	std::string tilesDirectory = PHYSFS_getRealDir("images/tiles.txt");
	tilesDirectory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
	printlog("[PhysFS]: Loading tiles from directory %s...\n", tilesDirectory.c_str());
	File* fp = openDataFile(tilesDirectory.c_str(), "rb");
	char name[PATH_MAX];

	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(name, PATH_MAX);
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string tileRealDir = PHYSFS_getRealDir(name);
			if ( reloadAll || tileRealDir.compare("./") != 0 )
			{
				std::string tileFile = tileRealDir;
				tileFile.append(PHYSFS_getDirSeparator()).append(name);
				if ( tiles[c] )
				{
					SDL_FreeSurface(tiles[c]);
				}
				char fullname[PATH_MAX];
				strncpy(fullname, tileFile.c_str(), PATH_MAX - 1);
				tiles[c] = loadImage(fullname);
				animatedtiles[c] = false;
				lavatiles[c] = false;
				swimmingtiles[c] = false;
				if ( tiles[c] != NULL )
				{
					size_t found = tileFile.find(".png");
					if ( found != string::npos && found != 0 )
					{
						if ( tileFile.at(found - 1) >= '0' && tileFile.at(found - 1) <= '9' )
						{
							// animated tiles if the tile name ends in a number 0-9.
							animatedtiles[c] = true;
						}
					}
					if ( strstr(name, "Lava") || strstr(name, "lava") )
					{
						lavatiles[c] = true;
					}
					if ( strstr(name, "Water") || strstr(name, "water") || strstr(name, "swimtile") || strstr(name, "Swimtile") )
					{
						swimmingtiles[c] = true;
					}
				}
				else
				{
					printlog("warning: failed to load '%s' listed at line %d in %s\n", name, c + 1, tilesDirectory.c_str());
					if ( c == 0 )
					{
						printlog("tile 0 cannot be NULL!\n");
						FileIO::close(fp);
						return;
					}
				}
			}
		}
	}
	FileIO::close(fp);
}

bool physfsIsMapLevelListModded()
{
	std::string mapsDirectory = PHYSFS_getRealDir(LEVELSFILE);
	if ( mapsDirectory.compare("./") != 0 )
	{
		//return true;
	}
	mapsDirectory.append(PHYSFS_getDirSeparator()).append(LEVELSFILE);

	std::vector<std::string> levelsList = getLinesFromDataFile(mapsDirectory);
	if ( levelsList.empty() )
	{
		return false;
	}
	std::string line = levelsList.front();
	int levelsCounted = 0;
	for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end(); ++i )
	{
		// process i, iterate through all the map levels until currentlevel.
		line = *i;
		if ( line[0] == '\n' )
		{
			continue;
		}
		std::size_t found = line.find(' ');
		if ( found != std::string::npos )
		{
			std::string mapType = line.substr(0, found);
			std::string mapName;
			mapName = line.substr(found + 1, line.find('\n'));
			std::size_t carriageReturn = mapName.find('\r');
			if ( carriageReturn != std::string::npos )
			{
				mapName.erase(carriageReturn);
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));
			if ( mapName.compare(officialLevelsTxtOrder.at(levelsCounted)) != 0 )
			{
				return true;
			}
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}
		}
		++levelsCounted;
	}

	mapsDirectory = PHYSFS_getRealDir(SECRETLEVELSFILE);
	if ( mapsDirectory.compare("./") != 0 )
	{
		//return true;
	}
	mapsDirectory.append(PHYSFS_getDirSeparator()).append(SECRETLEVELSFILE);

	levelsList = getLinesFromDataFile(mapsDirectory);
	if ( levelsList.empty() )
	{
		return false;
	}
	line = levelsList.front();
	levelsCounted = 0;
	for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end(); ++i )
	{
		// process i, iterate through all the map levels until currentlevel.
		line = *i;
		if ( line[0] == '\n' )
		{
			continue;
		}
		std::size_t found = line.find(' ');
		if ( found != std::string::npos )
		{
			std::string mapType = line.substr(0, found);
			std::string mapName;
			mapName = line.substr(found + 1, line.find('\n'));
			std::size_t carriageReturn = mapName.find('\r');
			if ( carriageReturn != std::string::npos )
			{
				mapName.erase(carriageReturn);
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));
			if ( mapName.compare(officialSecretlevelsTxtOrder.at(levelsCounted)) != 0 )
			{
				return true;
			}
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}
		}
		++levelsCounted;
	}
	return false;
}

bool physfsSearchItemSpritesToUpdate()
{
	for ( int c = 0; c < NUMITEMS; ++c )
	{
		for ( int x = 0; x < list_Size(&items[c].images); x++ )
		{
			node_t* node = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node->element;
			std::string itemImgDir;
			if ( PHYSFS_getRealDir(string->data) != NULL )
			{
				itemImgDir = PHYSFS_getRealDir(string->data);
				if ( itemImgDir.compare("./") != 0 )
				{
					printlog("[PhysFS]: Found modified item sprite in items/items.txt file, reloading all item sprites...");
					return true;
				}
			}
		}
	}
	return false;
}

void physfsReloadItemSprites(bool reloadAll)
{
	for ( int c = 0; c < NUMITEMS; ++c )
	{
		bool reloadImg = reloadAll;
		if ( !reloadAll )
		{
			for ( int x = 0; x < list_Size(&items[c].images); x++ )
			{
				node_t* node = list_Node(&items[c].images, x);
				string_t* string = (string_t*)node->element;
				std::string itemImgDir;
				if ( PHYSFS_getRealDir(string->data) != NULL )
				{
					itemImgDir = PHYSFS_getRealDir(string->data);
					if ( itemImgDir.compare("./") != 0 )
					{
						reloadImg = true;
					}
				}
			}
		}
		if ( reloadImg )
		{
			// free the image data.
			//list_FreeAll(&items[c].images);
			node_t* node, *nextnode;
			for ( node = items[c].surfaces.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				SDL_Surface** surface = (SDL_Surface**)node->element;
				if ( surface )
				{
					if ( *surface )
					{
						SDL_FreeSurface(*surface);
					}
				}
			}
			list_FreeAll(&items[c].surfaces);

			// now reload the image data.
			for ( int x = 0; x < list_Size(&items[c].images); x++ )
			{
				SDL_Surface** surface = (SDL_Surface**)malloc(sizeof(SDL_Surface*));
				node_t* node = list_AddNodeLast(&items[c].surfaces);
				node->element = surface;
				node->deconstructor = &defaultDeconstructor;
				node->size = sizeof(SDL_Surface*);

				node_t* node2 = list_Node(&items[c].images, x);
				string_t* string = (string_t*)node2->element;
				std::string itemImgDir;
				if ( PHYSFS_getRealDir(string->data) != NULL )
				{
					itemImgDir = PHYSFS_getRealDir(string->data);
					itemImgDir.append(PHYSFS_getDirSeparator()).append(string->data);
				}
				else
				{
					itemImgDir = string->data;
				}
				char imgFileChar[256];
				strncpy(imgFileChar, itemImgDir.c_str(), 255);
				*surface = loadImage(imgFileChar);
			}
		}
	}
}

bool physfsSearchItemsTxtToUpdate()
{
	if ( !PHYSFS_getRealDir("items/items.txt") )
	{
		printlog("error: could not find file: %s", "items/items.txt");
		return false;
	}
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items.txt");
	if ( itemsTxtDirectory.compare("./") != 0 )
	{
		printlog("[PhysFS]: Found modified items/items.txt file, reloading all item information...");
		return true;
	}
	return false;
}

bool physfsSearchItemsGlobalTxtToUpdate()
{
	if ( !PHYSFS_getRealDir("items/items_global.txt") )
	{
		printlog("error: could not find file: %s", "items/items_global.txt");
		return false;
	}
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items_global.txt");
	if ( itemsTxtDirectory.compare("./") != 0 )
	{
		printlog("[PhysFS]: Found modified items/items_global.txt file, reloading item spawn levels...");
		return true;
	}
	return false;
}

void physfsReloadItemsTxt()
{
	if ( !PHYSFS_getRealDir("items/items.txt") )
	{
		printlog("error: could not find file: %s", "items/items.txt");
		return;
	}
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items.txt");
	itemsTxtDirectory.append(PHYSFS_getDirSeparator()).append("items/items.txt");
	File* fp = openDataFile(itemsTxtDirectory.c_str(), "rb");
	char buffer[PATH_MAX];

	for ( int c = 0; !fp->eof() && c < NUMITEMS; ++c )
	{
		//if ( c > ARTIFACT_BOW )
		//{
		//	int newItems = c - ARTIFACT_BOW - 1;
		//	items[c].name_identified = language[2200 + newItems * 2];
		//	items[c].name_unidentified = language[2201 + newItems * 2];
		//}
		//else
		//{
		//	items[c].name_identified = language[1545 + c * 2];
		//	items[c].name_unidentified = language[1546 + c * 2];
		//}
		items[c].index = fp->geti();
		items[c].fpindex = fp->geti();
		items[c].variations = fp->geti();
		fp->gets2(buffer, PATH_MAX);
		size_t len = strlen(buffer) - 1U;
		if (buffer[len] == '\n' || buffer[len] == '\r')
		{
			buffer[len] = '\0';
		}
		if ( !strcmp(buffer, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(buffer, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(buffer, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(buffer, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(buffer, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(buffer, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(buffer, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(buffer, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(buffer, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(buffer, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(buffer, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(buffer, "THROWN") )
		{
			items[c].category = THROWN;
		}
		else if ( !strcmp(buffer, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		items[c].weight = fp->geti();
		items[c].value = fp->geti();

		list_FreeAll(&items[c].images);

		while ( 1 )
		{
			string_t* string = (string_t*)malloc(sizeof(string_t));
			string->data = (char*)malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			int x = 0;
			bool fileend = false;
			while ( (string->data[x] = fp->getc()) != '\n' )
			{
				if ( fp->eof() )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}

	FileIO::close(fp);
}

bool physfsSearchMonsterLimbFilesToUpdate()
{
	bool requiresUpdate = false;
	for ( int c = 1; c < NUMMONSTERS; c++ )
	{
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( PHYSFS_getRealDir(filename) == NULL ) // some monsters don't have limbs.
		{
			continue;
		}
		std::string limbsDir = PHYSFS_getRealDir(filename);
		if ( limbsDir.compare("./") != 0 )
		{
			printlog("[PhysFS]: Found modified limbs.txt file for monster %s, reloading all limb information...", monstertypename[c]);
			requiresUpdate = true;
		}
	}
	return requiresUpdate;
}

void physfsReloadMonsterLimbFiles()
{
	int x;
	File* fp;
	for ( int c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( x = 0; x < 20; x++ )
		{
			limbs[c][x][0] = 0;
			limbs[c][x][1] = 0;
			limbs[c][x][2] = 0;
		}

		// open file
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( PHYSFS_getRealDir(filename) == NULL ) // some monsters don't have limbs
		{
			continue;
		}
		std::string limbsDir = PHYSFS_getRealDir(filename);
		limbsDir.append(PHYSFS_getDirSeparator()).append(filename);
		if ( (fp = openDataFile(limbsDir.c_str(), "rb")) == NULL )
		{
			continue;
		}

		// read file
		int line;
		for ( line = 1; fp->eof() == 0; line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fp->gets2(data, 256);

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", limbsDir.c_str(), line);
				continue;
			}
			if ( sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb offsets!\n", limbsDir.c_str(), line);
				continue;
			}
		}
		// close file
		FileIO::close(fp);
	}
}

bool physfsSearchSystemImagesToUpdate()
{
	bool requireReload = false;
	systemResourceImagesToReload.clear();

	for ( std::vector<std::pair<SDL_Surface**, std::string>>::const_iterator it = systemResourceImages.begin(); it != systemResourceImages.end(); ++it )
	{
		std::pair<SDL_Surface**, std::string> line = *it;
		std::string imgFile = line.second;
		if ( PHYSFS_getRealDir(imgFile.c_str()) != NULL)
		{
			std::string imgDir = PHYSFS_getRealDir(imgFile.c_str());
			if ( imgDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified %s file, reloading system image...", imgFile.c_str());
				requireReload = true;
				systemResourceImagesToReload.push_back(line);
			}
		}
	}
	return requireReload;
}

void physfsReloadSystemImages()
{
	if ( !systemResourceImagesToReload.empty() )
	{
		for ( std::vector<std::pair<SDL_Surface**, std::string>>::const_iterator it = systemResourceImagesToReload.begin(); it != systemResourceImagesToReload.end(); ++it )
		{
			std::pair<SDL_Surface**, std::string> line = *it;
			if ( *(line.first) ) // SDL_Surface* pointer exists
			{
				// load a new image, getting the VFS system location.
				if ( !PHYSFS_getRealDir(line.second.c_str()) )
				{
					printlog("error: could not find file: %s", line.second.c_str());
					continue;
				}
				std::string filepath = PHYSFS_getRealDir(line.second.c_str());
				filepath.append(PHYSFS_getDirSeparator()).append(line.second);

				char filepathChar[1024];
				strncpy(filepathChar, filepath.c_str(), 1023);

				SDL_FreeSurface(*(line.first));
				*(line.first) = loadImage(filepathChar);

				if ( !(*(line.first)))
				{
					printlog("[PhysFS]: Error: Failed to reload %s!", filepath.c_str());
				}
			}
		}
	}
}

size_t FileBase::write(const void* src, size_t size, size_t count)
{
	if (mode != FileMode::WRITE || nullptr == src)
	{
		return 0U;
	}
	return 1U; //Input validation passed.
}

size_t FileBase::read(void* buffer, size_t size, size_t count)
{
	if (mode != FileMode::READ || nullptr == buffer)
	{
		return 0U;
	}
	return 1U; //Input validation passed.
}