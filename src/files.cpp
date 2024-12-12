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
#include <thread>
#include <future>

#include "main.hpp"
#include "files.hpp"
#include "engine/audio/sound.hpp"
#include "entity.hpp"
#include "book.hpp"
#include "menu.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "init.hpp"
#include "mod_tools.hpp"
#include "ui/LoadingScreen.hpp"
#ifdef EDITOR
#include "editor.hpp"
#endif

char datadir[PATH_MAX];
char outputdir[PATH_MAX];
const char* holidayThemeDirs[HolidayTheme::THEME_MAX] = {
    "",
    "themes/scarony/",
    "themes/merry/"
};

#ifndef EDITOR
ConsoleVariable<int> cvar_forceHoliday("/force_holiday", 0);
ConsoleVariable<bool> cvar_disableHoliday("/disable_holiday", false);
#endif

HolidayTheme getCurrentHoliday(bool force) {
#ifdef EDITOR
    return HolidayTheme::THEME_NONE;
#else
    if (*cvar_disableHoliday && !force) {
        return HolidayTheme::THEME_NONE;
    }
    if (*cvar_forceHoliday) {
        const int holiday = std::clamp(*cvar_forceHoliday, 0, (int)HolidayTheme::THEME_MAX - 1);
        return static_cast<HolidayTheme>(holiday);
    }
    static bool gotTime = false;
    static int year, month, day;
    if (!gotTime) {
        gotTime = true;
        getTimeAndDate(getTime(), &year, &month, &day,
            nullptr, nullptr, nullptr);
    }
    if (month == 10 || (month == 11 && day == 1)) {
        return HolidayTheme::THEME_HALLOWEEN;
    }
    else if (month == 12 || (month == 1 && day == 1)) {
        return HolidayTheme::THEME_XMAS;
    }
    else {
        return HolidayTheme::THEME_NONE;
    }
#endif
}

bool isCurrentHoliday(bool force) {
    return getCurrentHoliday(force) != HolidayTheme::THEME_NONE;
}

std::unordered_map<std::string, int> mapHashes = {
    { "boss.lmp", 2376307 },
    { "bramscastle.lmp", 2944609 },
    { "caves.lmp", 777612 },
    { "caves00.lmp", 70935 },
    { "caves01.lmp", 13350 },
    { "caves01a.lmp", 5 },
    { "caves01b.lmp", 898 },
    { "caves01c.lmp", 1238 },
    { "caves01d.lmp", 709 },
    { "caves01e.lmp", 5 },
    { "caves01f.lmp", 955 },
    { "caves02.lmp", 6995 },
    { "caves03.lmp", 11883 },
    { "caves04.lmp", 15294 },
    { "caves05.lmp", 10359 },
    { "caves06.lmp", 8376 },
    { "caves07.lmp", 9198 },
    { "caves08.lmp", 6873 },
    { "caves09.lmp", 102539 },
    { "caves09a.lmp", 2836 },
    { "caves09b.lmp", 6610 },
    { "caves09c.lmp", 4989 },
    { "caves09d.lmp", 6168 },
    { "caves09e.lmp", 3999 },
    { "caves10.lmp", 28899 },
    { "caves11.lmp", 35066 },
    { "caves12.lmp", 39802 },
    { "caves13.lmp", 45478 },
    { "caves13a.lmp", 1855 },
    { "caves13b.lmp", 1678 },
    { "caves13c.lmp", 6637 },
    { "caves13d.lmp", 3386 },
    { "caves13e.lmp", 2892 },
    { "caves14.lmp", 37757 },
    { "caves15.lmp", 26569 },
    { "caves16.lmp", 142126 },
    { "caves17.lmp", 31734 },
    { "caves18.lmp", 36806 },
    { "caves19.lmp", 25878 },
    { "caves20.lmp", 33315 },
    { "caves21.lmp", 16169 },
    { "caves22.lmp", 26212 },
    { "caves23.lmp", 35367 },
    { "caves24.lmp", 82144 },
    { "caves24a.lmp", 175812 },
    { "caves24b.lmp", 293908 },
    { "caves24c.lmp", 266809 },
    { "caves24d.lmp", 70970 },
    { "caves25.lmp", 19691 },
    { "caves26.lmp", 41694 },
    { "caves27.lmp", 10622 },
    { "caves28.lmp", 8712 },
    { "caveslair.lmp", 4869964 },
    { "cavessecret.lmp", 198959 },
    { "cavestocitadel.lmp", 276221 },
    { "citadel.lmp", 729069 },
    { "citadel00.lmp", 23997 },
    { "citadel01.lmp", 30094 },
    { "citadel01a.lmp", 2025 },
    { "citadel01b.lmp", 2224 },
    { "citadel01c.lmp", 2144 },
    { "citadel01d.lmp", 2377 },
    { "citadel01e.lmp", 582 },
    { "citadel01f.lmp", 204 },
    { "citadel01g.lmp", 6 },
    { "citadel02.lmp", 22718 },
    { "citadel02a.lmp", 4 },
    { "citadel02b.lmp", 4 },
    { "citadel02c.lmp", 4 },
    { "citadel02d.lmp", 4 },
    { "citadel02e.lmp", 4 },
    { "citadel02f.lmp", 4 },
    { "citadel02g.lmp", 4 },
    { "citadel03.lmp", 25112 },
    { "citadel04.lmp", 28361 },
    { "citadel05.lmp", 29081 },
    { "citadel06.lmp", 26711 },
    { "citadel07.lmp", 28380 },
    { "citadel08.lmp", 26768 },
    { "citadel09.lmp", 31320 },
    { "citadel09a.lmp", 5545 },
    { "citadel09b.lmp", 7174 },
    { "citadel09c.lmp", 6969 },
    { "citadel10.lmp", 31323 },
    { "citadel10a.lmp", 5557 },
    { "citadel10b.lmp", 10195 },
    { "citadel10c.lmp", 6072 },
    { "citadel11.lmp", 31330 },
    { "citadel11a.lmp", 4930 },
    { "citadel11b.lmp", 7722 },
    { "citadel11c.lmp", 8961 },
    { "citadel11d.lmp", 4179 },
    { "citadel12.lmp", 31330 },
    { "citadel12a.lmp", 5545 },
    { "citadel12b.lmp", 6497 },
    { "citadel12c.lmp", 7446 },
    { "citadel13.lmp", 62357 },
    { "citadel13a.lmp", 37133 },
    { "citadel13b.lmp", 12526 },
    { "citadel14.lmp", 121487 },
    { "citadel14a.lmp", 282262 },
    { "citadel14b.lmp", 51979 },
    { "citadel14c.lmp", 67253 },
    { "citadel14d.lmp", 43379 },
    { "citadel15.lmp", 62368 },
    { "citadel15a.lmp", 12141 },
    { "citadel15b.lmp", 24927 },
    { "citadel16.lmp", 55383 },
    { "citadel16a.lmp", 11376 },
    { "citadel16b.lmp", 14784 },
    { "citadel17.lmp", 112499 },
    { "citadel17a.lmp", 7279 },
    { "citadel17b.lmp", 1729 },
    { "citadel17c.lmp", 6786 },
    { "citadel17d.lmp", 10055 },
    { "citadel18.lmp", 31650 },
    { "citadel19.lmp", 13068 },
    { "citadel20.lmp", 14119 },
    { "citadel21.lmp", 72586 },
    { "citadelsecret.lmp", 104229 },
    { "gnomishmines.lmp", 603055 },
    { "greatcastle.lmp", 9978472 },
    { "hamlet.lmp", 8390565 },
    { "hell.lmp", 3511919 },
    { "hell00.lmp", 28651 },
    { "hell01.lmp", 22399 },
    { "hell01a.lmp", 12949 },
    { "hell01b.lmp", 18166 },
    { "hell01c.lmp", 19627 },
    { "hell01d.lmp", 16459 },
    { "hell01e.lmp", 36334 },
    { "hell01f.lmp", 18020 },
    { "hell02.lmp", 22399 },
    { "hell02a.lmp", 3763 },
    { "hell02b.lmp", 1670 },
    { "hell02c.lmp", 1910 },
    { "hell02d.lmp", 4375 },
    { "hell02e.lmp", 14071 },
    { "hell03.lmp", 50240 },
    { "hell03a.lmp", 12761 },
    { "hell03b.lmp", 40402 },
    { "hell03c.lmp", 40234 },
    { "hell03d.lmp", 49944 },
    { "hell03e.lmp", 15617 },
    { "hell04.lmp", 22972 },
    { "hell04a.lmp", 10679 },
    { "hell04b.lmp", 8986 },
    { "hell04c.lmp", 5239 },
    { "hell04d.lmp", 31329 },
    { "hell04e.lmp", 4952 },
    { "hell04f.lmp", 9569 },
    { "hell05.lmp", 25407 },
    { "hell05a.lmp", 7189 },
    { "hell05b.lmp", 3665 },
    { "hell05c.lmp", 32373 },
    { "hell05d.lmp", 22036 },
    { "hell05e.lmp", 10065 },
    { "hell06.lmp", 22609 },
    { "hell06a.lmp", 8241 },
    { "hell06b.lmp", 10302 },
    { "hell06c.lmp", 8056 },
    { "hell06d.lmp", 2877 },
    { "hell06e.lmp", 7147 },
    { "hell07.lmp", 36892 },
    { "hell07a.lmp", 17548 },
    { "hell07b.lmp", 28580 },
    { "hell07c.lmp", 32585 },
    { "hell07d.lmp", 13911 },
    { "hell07e.lmp", 12105 },
    { "hell08.lmp", 22399 },
    { "hell08a.lmp", 10072 },
    { "hell08b.lmp", 6364 },
    { "hell08c.lmp", 9463 },
    { "hell08d.lmp", 25955 },
    { "hell08e.lmp", 6529 },
    { "hell09.lmp", 120552 },
    { "hell09a.lmp", 42748 },
    { "hell09b.lmp", 42531 },
    { "hell09c.lmp", 34090 },
    { "hell09d.lmp", 39055 },
    { "hell09e.lmp", 41828 },
    { "hell09f.lmp", 31239 },
    { "hell10.lmp", 105532 },
    { "hell10a.lmp", 71620 },
    { "hell10b.lmp", 73741 },
    { "hell10c.lmp", 73427 },
    { "hell10d.lmp", 84123 },
    { "hell10e.lmp", 75177 },
    { "hell11.lmp", 154132 },
    { "hell11a.lmp", 39188 },
    { "hell11b.lmp", 960950 },
    { "hell11c.lmp", 297188 },
    { "hell11d.lmp", 311336 },
    { "hell11e.lmp", 45064 },
    { "hell12.lmp", 102711 },
    { "hell12a.lmp", 372802 },
    { "hell12b.lmp", 202404 },
    { "hell12c.lmp", 98709 },
    { "hell12d.lmp", 49665 },
    { "hell12e.lmp", 118405 },
    { "hell13.lmp", 89151 },
    { "hell13a.lmp", 8688 },
    { "hell13b.lmp", 79713 },
    { "hell13c.lmp", 82953 },
    { "hell13d.lmp", 58989 },
    { "hell13e.lmp", 156881 },
    { "hell14.lmp", 159494 },
    { "hell14a.lmp", 185994 },
    { "hell14b.lmp", 111381 },
    { "hell14c.lmp", 1156179 },
    { "hell14d.lmp", 1080797 },
    { "hell14e.lmp", 43268 },
    { "hell15.lmp", 186638 },
    { "hell15a.lmp", 277620 },
    { "hell15b.lmp", 16905 },
    { "hell15c.lmp", 110319 },
    { "hell15d.lmp", 24877 },
    { "hell15e.lmp", 39287 },
    { "hell16.lmp", 102356 },
    { "hell16a.lmp", 20283 },
    { "hell16b.lmp", 69443 },
    { "hell16c.lmp", 49121 },
    { "hell16d.lmp", 34319 },
    { "hell16e.lmp", 340840 },
    { "hell17.lmp", 102551 },
    { "hell17a.lmp", 47111 },
    { "hell17b.lmp", 62511 },
    { "hell17c.lmp", 696123 },
    { "hell17d.lmp", 65619 },
    { "hell17e.lmp", 54446 },
    { "hell18.lmp", 107665 },
    { "hell18a.lmp", 34227 },
    { "hell18b.lmp", 35552 },
    { "hell18c.lmp", 30816 },
    { "hell18d.lmp", 66616 },
    { "hell18e.lmp", 99723 },
    { "hell18f.lmp", 44282 },
    { "hell19.lmp", 46447 },
    { "hell19a.lmp", 13345 },
    { "hell19b.lmp", 6927 },
    { "hell19c.lmp", 13221 },
    { "hell19d.lmp", 19694 },
    { "hell19e.lmp", 27110 },
    { "hell20.lmp", 118897 },
    { "hell20a.lmp", 40697 },
    { "hell20b.lmp", 39985 },
    { "hell20c.lmp", 67615 },
    { "hell20d.lmp", 23685 },
    { "hell20e.lmp", 53263 },
    { "hell21.lmp", 90546 },
    { "hell21a.lmp", 30584 },
    { "hell21b.lmp", 139850 },
    { "hell21c.lmp", 58279 },
    { "hell21d.lmp", 47900 },
    { "hell21e.lmp", 57606 },
    { "hell22.lmp", 157265 },
    { "hell22a.lmp", 481602 },
    { "hell22b.lmp", 955291 },
    { "hell22c.lmp", 232845 },
    { "hell22d.lmp", 89909 },
    { "hell22e.lmp", 122584 },
    { "hell23.lmp", 21689 },
    { "hell23a.lmp", 9093 },
    { "hell23b.lmp", 33348 },
    { "hell23c.lmp", 27068 },
    { "hell23d.lmp", 25263 },
    { "hell23e.lmp", 28828 },
    { "hell24.lmp", 51865 },
    { "hell24a.lmp", 79994 },
    { "hell24b.lmp", 68952 },
    { "hell24c.lmp", 186384 },
    { "hell24d.lmp", 72132 },
    { "hell24e.lmp", 142110 },
    { "hell25.lmp", 50764 },
    { "hell25a.lmp", 89577 },
    { "hell25b.lmp", 28086 },
    { "hell25c.lmp", 23221 },
    { "hell25d.lmp", 33416 },
    { "hell25e.lmp", 13553 },
    { "hell26.lmp", 102672 },
    { "hell26a.lmp", 27154 },
    { "hell26b.lmp", 37682 },
    { "hell26c.lmp", 97908 },
    { "hell26d.lmp", 94776 },
    { "hell26e.lmp", 138670 },
    { "hell27.lmp", 17187 },
    { "hell27a.lmp", 3756 },
    { "hell27b.lmp", 3838 },
    { "hell27c.lmp", 4407 },
    { "hell27d.lmp", 2879 },
    { "hell27e.lmp", 6 },
    { "hell28.lmp", 25405 },
    { "hell28a.lmp", 11985 },
    { "hell28b.lmp", 12416 },
    { "hell28c.lmp", 11255 },
    { "hell28d.lmp", 11550 },
    { "hell28e.lmp", 14389 },
    { "hell29.lmp", 102673 },
    { "hell29a.lmp", 45392 },
    { "hell29b.lmp", 92088 },
    { "hell29c.lmp", 124389 },
    { "hell29d.lmp", 144983 },
    { "hell29e.lmp", 48684 },
    { "hell29f.lmp", 167923 },
    { "hellboss.lmp", 4459727 },
	{ "baphoexit.lmp", 4309647 },
    { "labyrinth.lmp", 397402 },
    { "labyrinth00.lmp", 119759 },
    { "labyrinth01.lmp", 32319 },
    { "labyrinth01a.lmp", 64189 },
    { "labyrinth01b.lmp", 65337 },
    { "labyrinth01c.lmp", 43746 },
    { "labyrinth01d.lmp", 26536 },
    { "labyrinth01e.lmp", 19636 },
    { "labyrinth02.lmp", 40567 },
    { "labyrinth02a.lmp", 420 },
    { "labyrinth02b.lmp", 38920 },
    { "labyrinth02c.lmp", 16008 },
    { "labyrinth02d.lmp", 24253 },
    { "labyrinth02e.lmp", 63921 },
    { "labyrinth02f.lmp", 126844 },
    { "labyrinth03.lmp", 149405 },
    { "labyrinth03a.lmp", 16472 },
    { "labyrinth03b.lmp", 111509 },
    { "labyrinth03c.lmp", 50370 },
    { "labyrinth03d.lmp", 236316 },
    { "labyrinth03e.lmp", 82737 },
    { "labyrinth03f.lmp", 191286 },
    { "labyrinth04.lmp", 109784 },
    { "labyrinth04a.lmp", 66827 },
    { "labyrinth04b.lmp", 80896 },
    { "labyrinth04c.lmp", 107797 },
    { "labyrinth04d.lmp", 89364 },
    { "labyrinth04e.lmp", 91803 },
    { "labyrinth04f.lmp", 67426 },
    { "labyrinth05.lmp", 110121 },
    { "labyrinth05a.lmp", 53007 },
    { "labyrinth05b.lmp", 47308 },
    { "labyrinth05c.lmp", 66118 },
    { "labyrinth05d.lmp", 53922 },
    { "labyrinth05e.lmp", 66820 },
    { "labyrinth06.lmp", 39633 },
    { "labyrinth06a.lmp", 16213 },
    { "labyrinth06b.lmp", 41773 },
    { "labyrinth06c.lmp", 52793 },
    { "labyrinth06d.lmp", 33467 },
    { "labyrinth06e.lmp", 35252 },
    { "labyrinth07.lmp", 39633 },
    { "labyrinth07a.lmp", 16244 },
    { "labyrinth07b.lmp", 64137 },
    { "labyrinth07c.lmp", 43007 },
    { "labyrinth07d.lmp", 49006 },
    { "labyrinth07e.lmp", 48286 },
    { "labyrinth08.lmp", 91975 },
    { "labyrinth08a.lmp", 126344 },
    { "labyrinth08b.lmp", 261965 },
    { "labyrinth08c.lmp", 116805 },
    { "labyrinth08d.lmp", 93423 },
    { "labyrinth08e.lmp", 166877 },
    { "labyrinth09.lmp", 70907 },
    { "labyrinth09a.lmp", 42123 },
    { "labyrinth09b.lmp", 123680 },
    { "labyrinth09c.lmp", 158047 },
    { "labyrinth09d.lmp", 41246 },
    { "labyrinth09e.lmp", 53113 },
    { "labyrinth10.lmp", 70911 },
    { "labyrinth10a.lmp", 37919 },
    { "labyrinth10b.lmp", 52942 },
    { "labyrinth10c.lmp", 104651 },
    { "labyrinth10d.lmp", 41875 },
    { "labyrinth10e.lmp", 77554 },
    { "labyrinth11.lmp", 102011 },
    { "labyrinth11a.lmp", 124995 },
    { "labyrinth11b.lmp", 44855 },
    { "labyrinth11c.lmp", 121035 },
    { "labyrinth11d.lmp", 245955 },
    { "labyrinth11e.lmp", 92172 },
    { "labyrinth12.lmp", 108662 },
    { "labyrinth12a.lmp", 248043 },
    { "labyrinth12b.lmp", 75322 },
    { "labyrinth12c.lmp", 251882 },
    { "labyrinth12d.lmp", 285746 },
    { "labyrinth12e.lmp", 93600 },
    { "labyrinth13.lmp", 42829 },
    { "labyrinth13a.lmp", 30982 },
    { "labyrinth13b.lmp", 21962 },
    { "labyrinth13c.lmp", 31714 },
    { "labyrinth13d.lmp", 9987 },
    { "labyrinth13e.lmp", 20182 },
    { "labyrinth13f.lmp", 12578 },
    { "labyrinth14.lmp", 67079 },
    { "labyrinth14a.lmp", 129808 },
    { "labyrinth14b.lmp", 45415 },
    { "labyrinth14c.lmp", 44746 },
    { "labyrinth14d.lmp", 100542 },
    { "labyrinth14e.lmp", 31548 },
    { "labyrinth15.lmp", 287900 },
    { "labyrinth15a.lmp", 22984 },
    { "labyrinth15b.lmp", 104822 },
    { "labyrinth15c.lmp", 114256 },
    { "labyrinth15d.lmp", 104887 },
    { "labyrinth15e.lmp", 64426 },
    { "labyrinth16.lmp", 92449 },
    { "labyrinth16a.lmp", 12211 },
    { "labyrinth16b.lmp", 62760 },
    { "labyrinth16c.lmp", 77205 },
    { "labyrinth16d.lmp", 39180 },
    { "labyrinth16e.lmp", 30724 },
    { "labyrinth17.lmp", 358170 },
    { "labyrinth17a.lmp", 27749 },
    { "labyrinth17b.lmp", 43026 },
    { "labyrinth17c.lmp", 33891 },
    { "labyrinth17d.lmp", 70771 },
    { "labyrinth17e.lmp", 123853 },
    { "labyrinth18.lmp", 142769 },
    { "labyrinth18a.lmp", 108928 },
    { "labyrinth18b.lmp", 103153 },
    { "labyrinth18c.lmp", 69372 },
    { "labyrinth18d.lmp", 68465 },
    { "labyrinth18e.lmp", 99163 },
    { "labyrinth19.lmp", 56735 },
    { "labyrinth19a.lmp", 3275 },
    { "labyrinth19b.lmp", 15120 },
    { "labyrinth19c.lmp", 37342 },
    { "labyrinth19d.lmp", 30150 },
    { "labyrinth19e.lmp", 63796 },
    { "labyrinth20.lmp", 68928 },
    { "labyrinth20a.lmp", 9243 },
    { "labyrinth20b.lmp", 68327 },
    { "labyrinth20c.lmp", 59151 },
    { "labyrinth20d.lmp", 42530 },
    { "labyrinth20e.lmp", 76531 },
    { "labyrinth21.lmp", 53319 },
    { "labyrinth21a.lmp", 3161 },
    { "labyrinth21b.lmp", 38170 },
    { "labyrinth21c.lmp", 12316 },
    { "labyrinth21d.lmp", 25530 },
    { "labyrinth21e.lmp", 60942 },
    { "labyrinth22.lmp", 35351 },
    { "labyrinth22a.lmp", 10229 },
    { "labyrinth22b.lmp", 12801 },
    { "labyrinth22c.lmp", 12265 },
    { "labyrinth22d.lmp", 10832 },
    { "labyrinth22e.lmp", 15763 },
    { "labyrinth23.lmp", 84812 },
    { "labyrinth23a.lmp", 60989 },
    { "labyrinth23b.lmp", 46318 },
    { "labyrinth23c.lmp", 38896 },
    { "labyrinth23d.lmp", 233512 },
    { "labyrinth23e.lmp", 81611 },
    { "labyrinth24.lmp", 55073 },
    { "labyrinth24a.lmp", 188853 },
    { "labyrinth24b.lmp", 74991 },
    { "labyrinth24c.lmp", 39266 },
    { "labyrinth24d.lmp", 17045 },
    { "labyrinth24e.lmp", 23087 },
    { "labyrinth25.lmp", 60492 },
    { "labyrinth25a.lmp", 11150 },
    { "labyrinth25b.lmp", 6878 },
    { "labyrinth25c.lmp", 11057 },
    { "labyrinth25d.lmp", 7201 },
    { "labyrinth25e.lmp", 4705 },
    { "labyrinth26.lmp", 10430 },
    { "labyrinth26a.lmp", 4453 },
    { "labyrinth26b.lmp", 5457 },
    { "labyrinth26c.lmp", 8932 },
    { "labyrinth26d.lmp", 3908 },
    { "labyrinth26e.lmp", 4013 },
    { "labyrinth27.lmp", 10433 },
    { "labyrinth27a.lmp", 4453 },
    { "labyrinth27b.lmp", 8752 },
    { "labyrinth27c.lmp", 4702 },
    { "labyrinth27d.lmp", 3343 },
    { "labyrinth27e.lmp", 4163 },
    { "labyrinth28.lmp", 20971 },
    { "labyrinth28a.lmp", 5511 },
    { "labyrinth28b.lmp", 4837 },
    { "labyrinth28c.lmp", 8471 },
    { "labyrinth28d.lmp", 5907 },
    { "labyrinth28e.lmp", 5629 },
    { "labyrinth29.lmp", 20516 },
    { "labyrinth29a.lmp", 2709 },
    { "labyrinth29b.lmp", 14281 },
    { "labyrinth29c.lmp", 12255 },
    { "labyrinth29d.lmp", 13714 },
    { "labyrinth29e.lmp", 27632 },
    { "labyrinth30.lmp", 67775 },
    { "labyrinth30a.lmp", 250787 },
    { "labyrinth30b.lmp", 76890 },
    { "labyrinth30c.lmp", 42344 },
    { "labyrinth30d.lmp", 122312 },
    { "labyrinth30e.lmp", 32283 },
    { "labyrinth31.lmp", 65567 },
    { "labyrinth31a.lmp", 18532 },
    { "labyrinth31b.lmp", 26136 },
    { "labyrinth31c.lmp", 44440 },
    { "labyrinth31d.lmp", 23865 },
    { "labyrinth31e.lmp", 21255 },
    { "labyrinth32.lmp", 49841 },
    { "labyrinth32a.lmp", 30907 },
    { "labyrinth32b.lmp", 35643 },
    { "labyrinth32c.lmp", 25911 },
    { "labyrinth32d.lmp", 5279 },
    { "labyrinth32e.lmp", 26968 },
    { "labyrinthsecret.lmp", 73135 },
    { "labyrinthtoruins.lmp", 137530 },
    { "mainmenu1.lmp", -1 },
    { "mainmenu2.lmp", -1 },
    { "mainmenu3.lmp", -1 },
    { "mainmenu4.lmp", -1 },
    { "mainmenu5.lmp", -1 },
    { "mainmenu6.lmp", -1 },
    { "mainmenu7.lmp", -1 },
    { "mainmenu8.lmp", -1 },
    { "mainmenu9.lmp", -1 },
    { "mine.lmp", 80741 },
    { "mine00.lmp", 12890 },
    { "mine01.lmp", 52889 },
    { "mine01a.lmp", 4094 },
    { "mine01b.lmp", 5156 },
    { "mine01c.lmp", 5524 },
    { "mine01d.lmp", 2780 },
    { "mine01e.lmp", 33508 },
    { "mine01f.lmp", 2436 },
    { "mine01g.lmp", 15844 },
    { "mine02.lmp", 16102 },
    { "mine02a.lmp", 1964 },
    { "mine02b.lmp", 2050 },
    { "mine02c.lmp", 3796 },
    { "mine02d.lmp", 19759 },
    { "mine02e.lmp", 2052 },
    { "mine02f.lmp", 375 },
    { "mine02g.lmp", 862 },
    { "mine03.lmp", 17657 },
    { "mine03a.lmp", 3440 },
    { "mine03b.lmp", 49719 },
    { "mine03c.lmp", 3274 },
    { "mine03d.lmp", 1451 },
    { "mine03e.lmp", 1516 },
    { "mine03f.lmp", 6794 },
    { "mine03g.lmp", 1331 },
    { "mine04.lmp", 21711 },
    { "mine04a.lmp", 2264 },
    { "mine04b.lmp", 6415 },
    { "mine04c.lmp", 4200 },
    { "mine04d.lmp", 2925 },
    { "mine04e.lmp", 914 },
    { "mine04f.lmp", 1042 },
    { "mine04g.lmp", 2415 },
    { "mine05.lmp", 33835 },
    { "mine05a.lmp", 4057 },
    { "mine05b.lmp", 8869 },
    { "mine05c.lmp", 3943 },
    { "mine05d.lmp", 6466 },
    { "mine05e.lmp", 3486 },
    { "mine06.lmp", 79441 },
    { "mine06a.lmp", 12895 },
    { "mine06b.lmp", 58604 },
    { "mine06c.lmp", 28704 },
    { "mine06d.lmp", 81643 },
    { "mine06e.lmp", 25051 },
    { "mine07.lmp", 107931 },
    { "mine07a.lmp", 13341 },
    { "mine07b.lmp", 24443 },
    { "mine07c.lmp", 49306 },
    { "mine07d.lmp", 17195 },
    { "mine07e.lmp", 13843 },
    { "mine08.lmp", 50089 },
    { "mine08a.lmp", 2824 },
    { "mine08b.lmp", 6387 },
    { "mine08c.lmp", 3666 },
    { "mine08d.lmp", 27312 },
    { "mine08e.lmp", 1614 },
    { "mine09.lmp", 57257 },
    { "mine09a.lmp", 10581 },
    { "mine09b.lmp", 10445 },
    { "mine09c.lmp", 46588 },
    { "mine09d.lmp", 30484 },
    { "mine09e.lmp", 10147 },
    { "mine10.lmp", 117885 },
    { "mine10a.lmp", 178759 },
    { "mine10b.lmp", 70918 },
    { "mine10c.lmp", 21147 },
    { "mine10d.lmp", 65233 },
    { "mine10e.lmp", 61077 },
    { "mine11.lmp", 74234 },
    { "mine11a.lmp", 7475 },
    { "mine11b.lmp", 7909 },
    { "mine11c.lmp", 4787 },
    { "mine11d.lmp", 18375 },
    { "mine11e.lmp", 55599 },
    { "mine12.lmp", 30195 },
    { "mine12a.lmp", 2408 },
    { "mine12b.lmp", 1064 },
    { "mine12c.lmp", 1064 },
    { "mine12d.lmp", 1064 },
    { "mine12e.lmp", 1066 },
    { "mine12f.lmp", 1123 },
    { "mine12g.lmp", 3173 },
    { "mine12h.lmp", 51714 },
    { "mine12i.lmp", 29186 },
    { "mine13.lmp", 43438 },
    { "mine13a.lmp", 6070 },
    { "mine13b.lmp", 6864 },
    { "mine13c.lmp", 94568 },
    { "mine13d.lmp", 14907 },
    { "mine13e.lmp", 14220 },
    { "mine14.lmp", 54278 },
    { "mine14a.lmp", 8371 },
    { "mine14b.lmp", 10552 },
    { "mine14c.lmp", 32460 },
    { "mine14d.lmp", 12894 },
    { "mine14e.lmp", 6706 },
    { "mine15.lmp", 44160 },
    { "mine15a.lmp", 14464 },
    { "mine15b.lmp", 16268 },
    { "mine15c.lmp", 15070 },
    { "mine15d.lmp", 11959 },
    { "mine15e.lmp", 6270 },
    { "mine16.lmp", 47683 },
    { "mine16a.lmp", 5567 },
    { "mine16b.lmp", 4516 },
    { "mine16c.lmp", 11200 },
    { "mine16d.lmp", 15776 },
    { "mine16e.lmp", 22921 },
    { "mine16f.lmp", 34277 },
    { "mine17.lmp", 18741 },
    { "mine17a.lmp", 4756 },
    { "mine17b.lmp", 6251 },
    { "mine17c.lmp", 6627 },
    { "mine17d.lmp", 5883 },
    { "mine17e.lmp", 7393 },
    { "mine17f.lmp", 7827 },
    { "mine18.lmp", 48006 },
    { "mine18a.lmp", 10791 },
    { "mine18b.lmp", 11978 },
    { "mine18c.lmp", 26618 },
    { "mine18d.lmp", 32406 },
    { "mine18e.lmp", 11426 },
    { "mine19.lmp", 21099 },
    { "mine19a.lmp", 2014 },
    { "mine19b.lmp", 2706 },
    { "mine19c.lmp", 2448 },
    { "mine19d.lmp", 2500 },
    { "mine19e.lmp", 4188 },
    { "mine20.lmp", 16836 },
    { "mine20a.lmp", 968 },
    { "mine20b.lmp", 1473 },
    { "mine20c.lmp", 2087 },
    { "mine20d.lmp", 1077 },
    { "mine20e.lmp", 10418 },
    { "mine21.lmp", 26317 },
    { "mine21a.lmp", 4635 },
    { "mine21b.lmp", 4623 },
    { "mine21c.lmp", 4872 },
    { "mine21d.lmp", 5830 },
    { "mine21e.lmp", 5158 },
    { "mine22.lmp", 20095 },
    { "mine22a.lmp", 937 },
    { "mine22b.lmp", 1229 },
    { "mine22c.lmp", 1229 },
    { "mine22d.lmp", 1353 },
    { "mine22e.lmp", 2587 },
    { "mine22f.lmp", 28494 },
    { "mine23.lmp", 20477 },
    { "mine23a.lmp", 2453 },
    { "mine23b.lmp", 2528 },
    { "mine23c.lmp", 3226 },
    { "mine23d.lmp", 1998 },
    { "mine23e.lmp", 14561 },
    { "mine23f.lmp", 3490 },
    { "mine24.lmp", 18353 },
    { "mine24a.lmp", 2206 },
    { "mine24b.lmp", 2573 },
    { "mine24c.lmp", 1544 },
    { "mine24d.lmp", 868 },
    { "mine24e.lmp", 1541 },
    { "mine25.lmp", 11992 },
    { "mine25a.lmp", 1512 },
    { "mine25b.lmp", 1099 },
    { "mine25c.lmp", 10042 },
    { "mine25d.lmp", 475 },
    { "mine25e.lmp", 945 },
    { "mine26.lmp", 6642 },
    { "mine26a.lmp", 646 },
    { "mine26b.lmp", 568 },
    { "mine26c.lmp", 650 },
    { "mine26d.lmp", 729 },
    { "mine26e.lmp", 1912 },
    { "mine27.lmp", 6783 },
    { "mine27a.lmp", 647 },
    { "mine27b.lmp", 646 },
    { "mine27c.lmp", 719 },
    { "mine27d.lmp", 727 },
    { "mine27e.lmp", 2659 },
    { "mine28.lmp", 12641 },
    { "mine28a.lmp", 1867 },
    { "mine28b.lmp", 1981 },
    { "mine28c.lmp", 2279 },
    { "mine28d.lmp", 2763 },
    { "mine28e.lmp", 2148 },
    { "mine29.lmp", 7173 },
    { "mine29a.lmp", 631 },
    { "mine29b.lmp", 663 },
    { "mine29c.lmp", 853 },
    { "mine29d.lmp", 787 },
    { "mine29e.lmp", 777 },
    { "mine30.lmp", 56963 },
    { "mine30a.lmp", 24107 },
    { "mine30b.lmp", 40738 },
    { "mine30c.lmp", 21350 },
    { "mine30d.lmp", 9993 },
    { "mine30e.lmp", 9790 },
    { "mine31.lmp", 38812 },
    { "mine31a.lmp", 13079 },
    { "mine31b.lmp", 14258 },
    { "mine31c.lmp", 13828 },
    { "mine31d.lmp", 6625 },
    { "mine31e.lmp", 8149 },
    { "mine32.lmp", 7983 },
    { "mine32a.lmp", 2775 },
    { "mine32b.lmp", 2944 },
    { "mine32c.lmp", 1755 },
    { "mine32d.lmp", 266 },
    { "mine32e.lmp", 1916 },
    { "mine33.lmp", 12331 },
    { "mine33a.lmp", 63 },
    { "mine33b.lmp", 366 },
    { "mine33c.lmp", 564 },
    { "mine33d.lmp", 848 },
    { "mine33e.lmp", 298 },
    { "mine33f.lmp", 258 },
    { "mine33g.lmp", 2972 },
    { "minesecret.lmp", 25068 },
    { "minetoswamp.lmp", 132973 },
    { "minetown.lmp", 842756 },
    { "minotaur.lmp", 4409895 },
    { "mysticlibrary.lmp", 360623 },
    { "ruins.lmp", 92624 },
    { "ruins00.lmp", 6373 },
	{ "ruins01.lmp", 149157 },
	{ "ruins01a.lmp", 103231 },
	{ "ruins01b.lmp", 156192 },
	{ "ruins01c.lmp", 24298 },
	{ "ruins01d.lmp", 15632 },
	{ "ruins01e.lmp", 57306 },
	{ "ruins02.lmp", 149481 },
	{ "ruins02a.lmp", 97513 },
	{ "ruins02b.lmp", 38538 },
	{ "ruins02c.lmp", 59764 },
	{ "ruins02d.lmp", 174860 },
	{ "ruins02e.lmp", 74840 },
	{ "ruins03.lmp", 79141 },
	{ "ruins03a.lmp", 16088 },
	{ "ruins03b.lmp", 44743 },
	{ "ruins03c.lmp", 46486 },
	{ "ruins03d.lmp", 28068 },
	{ "ruins03e.lmp", 17496 },
	{ "ruins04.lmp", 60569 },
	{ "ruins04a.lmp", 11190 },
	{ "ruins04b.lmp", 24108 },
	{ "ruins04c.lmp", 6640 },
	{ "ruins04d.lmp", 9275 },
	{ "ruins04e.lmp", 13245 },
	{ "ruins05.lmp", 138563 },
	{ "ruins05a.lmp", 15194 },
	{ "ruins05b.lmp", 16728 },
	{ "ruins05c.lmp", 65468 },
	{ "ruins05d.lmp", 77080 },
	{ "ruins05e.lmp", 39438 },
	{ "ruins06.lmp", 52488 },
	{ "ruins06a.lmp", 10005 },
	{ "ruins06b.lmp", 11882 },
	{ "ruins06c.lmp", 10656 },
	{ "ruins06d.lmp", 12742 },
	{ "ruins06e.lmp", 56033 },
	{ "ruins07.lmp", 102161 },
	{ "ruins07a.lmp", 2150 },
	{ "ruins07b.lmp", 81411 },
	{ "ruins07c.lmp", 114854 },
	{ "ruins07d.lmp", 45084 },
	{ "ruins07e.lmp", 42387 },
	{ "ruins08.lmp", 69920 },
	{ "ruins08a.lmp", 12627 },
	{ "ruins08b.lmp", 16729 },
	{ "ruins08c.lmp", 39399 },
	{ "ruins08d.lmp", 11756 },
	{ "ruins08e.lmp", 11813 },
	{ "ruins09.lmp", 69810 },
	{ "ruins09a.lmp", 12624 },
	{ "ruins09b.lmp", 9891 },
	{ "ruins09c.lmp", 12694 },
	{ "ruins09d.lmp", 3048 },
	{ "ruins09e.lmp", 51819 },
	{ "ruins10.lmp", 44004 },
	{ "ruins10a.lmp", 10773 },
	{ "ruins10b.lmp", 9221 },
	{ "ruins10c.lmp", 9242 },
	{ "ruins10d.lmp", 32823 },
	{ "ruins10e.lmp", 18179 },
	{ "ruins11.lmp", 55530 },
	{ "ruins11a.lmp", 11046 },
	{ "ruins11b.lmp", 8791 },
	{ "ruins11c.lmp", 39502 },
	{ "ruins11d.lmp", 25593 },
	{ "ruins11e.lmp", 13047 },
	{ "ruins12.lmp", 24121 },
	{ "ruins12a.lmp", 6262 },
	{ "ruins12b.lmp", 4804 },
	{ "ruins12c.lmp", 2207 },
	{ "ruins12d.lmp", 7859 },
	{ "ruins12e.lmp", 4042 },
	{ "ruins13.lmp", 213163 },
	{ "ruins13a.lmp", 46245 },
	{ "ruins13b.lmp", 143588 },
	{ "ruins13c.lmp", 176468 },
	{ "ruins13d.lmp", 12169 },
	{ "ruins13e.lmp", 113050 },
	{ "ruins14.lmp", 48408 },
	{ "ruins14a.lmp", 608 },
	{ "ruins14b.lmp", 891 },
	{ "ruins14c.lmp", 2483 },
	{ "ruins14d.lmp", 8723 },
	{ "ruins14e.lmp", 11032 },
	{ "ruins15.lmp", 48408 },
	{ "ruins15a.lmp", 614 },
	{ "ruins15b.lmp", 893 },
	{ "ruins15c.lmp", 4910 },
	{ "ruins15d.lmp", 8664 },
	{ "ruins15e.lmp", 776 },
	{ "ruins16.lmp", 42383 },
	{ "ruins16a.lmp", 4493 },
	{ "ruins16b.lmp", 4400 },
	{ "ruins16c.lmp", 4800 },
	{ "ruins16d.lmp", 1228 },
	{ "ruins16e.lmp", 3028 },
	{ "ruins17.lmp", 36976 },
	{ "ruins17a.lmp", 6521 },
	{ "ruins17b.lmp", 5539 },
	{ "ruins17c.lmp", 5269 },
	{ "ruins17d.lmp", 3739 },
	{ "ruins17e.lmp", 6023 },
	{ "ruins18.lmp", 49405 },
	{ "ruins18a.lmp", 12111 },
	{ "ruins18b.lmp", 7985 },
	{ "ruins18c.lmp", 15650 },
	{ "ruins18d.lmp", 2869 },
	{ "ruins18e.lmp", 13637 },
	{ "ruins19.lmp", 105023 },
	{ "ruins19a.lmp", 2469 },
	{ "ruins19b.lmp", 42444 },
	{ "ruins19c.lmp", 8925 },
	{ "ruins19d.lmp", 59478 },
	{ "ruins19e.lmp", 359371 },
	{ "ruins20.lmp", 239795 },
	{ "ruins20a.lmp", 39623 },
	{ "ruins20b.lmp", 152619 },
	{ "ruins20c.lmp", 416612 },
	{ "ruins20d.lmp", 57511 },
	{ "ruins20e.lmp", 369845 },
	{ "ruins21.lmp", 71662 },
	{ "ruins21a.lmp", 4165 },
	{ "ruins21b.lmp", 8883 },
	{ "ruins21c.lmp", 4264 },
	{ "ruins21d.lmp", 27792 },
	{ "ruins21e.lmp", 18263 },
	{ "ruins22.lmp", 218758 },
	{ "ruins22a.lmp", 81278 },
	{ "ruins22b.lmp", 192781 },
	{ "ruins22c.lmp", 40728 },
	{ "ruins22d.lmp", 72781 },
	{ "ruins22e.lmp", 60361 },
	{ "ruins23.lmp", 2830 },
	{ "ruins23a.lmp", 102 },
	{ "ruins23b.lmp", 380 },
	{ "ruins23c.lmp", 223 },
	{ "ruins23d.lmp", 223 },
	{ "ruins23e.lmp", 104 },
	{ "ruins24.lmp", 118224 },
	{ "ruins24a.lmp", 30518 },
	{ "ruins24b.lmp", 63399 },
	{ "ruins24c.lmp", 62535 },
	{ "ruins24d.lmp", 248223 },
	{ "ruins24e.lmp", 36120 },
	{ "ruins25.lmp", 73445 },
	{ "ruins25a.lmp", 4058 },
	{ "ruins25b.lmp", 3436 },
	{ "ruins25c.lmp", 7994 },
	{ "ruins25d.lmp", 33091 },
	{ "ruins25e.lmp", 4050 },
	{ "ruins26.lmp", 73990 },
	{ "ruins26a.lmp", 18063 },
	{ "ruins26b.lmp", 14285 },
	{ "ruins26c.lmp", 46076 },
	{ "ruins26d.lmp", 66885 },
	{ "ruins26e.lmp", 50935 },
	{ "ruins27.lmp", 9308 },
	{ "ruins27a.lmp", 1234 },
	{ "ruins27b.lmp", 2634 },
	{ "ruins27c.lmp", 1307 },
	{ "ruins27d.lmp", 3126 },
	{ "ruins27e.lmp", 3019 },
	{ "ruins28.lmp", 5501 },
	{ "ruins28a.lmp", 483 },
	{ "ruins28b.lmp", 697 },
	{ "ruins28c.lmp", 10324 },
	{ "ruins28d.lmp", 439 },
	{ "ruins28e.lmp", 250 },
	{ "ruins28f.lmp", 443 },
	{ "ruins28g.lmp", 441 },
	{ "ruins29.lmp", 46557 },
	{ "ruins29a.lmp", 9562 },
	{ "ruins29b.lmp", 19612 },
	{ "ruins29c.lmp", 7999 },
	{ "ruins29d.lmp", 7931 },
	{ "ruins29e.lmp", 21346 },
	{ "ruins30.lmp", 58106 },
	{ "ruins30a.lmp", 46892 },
	{ "ruins30b.lmp", 7881 },
	{ "ruins30c.lmp", 6702 },
	{ "ruins30d.lmp", 23092 },
	{ "ruins30e.lmp", 70869 },
	{ "ruins31.lmp", 62838 },
	{ "ruins31a.lmp", 41977 },
	{ "ruins31b.lmp", 12094 },
	{ "ruins31c.lmp", 13341 },
	{ "ruins31d.lmp", 39298 },
	{ "ruins31e.lmp", 24223 },
	{ "ruins32.lmp", 29693 },
	{ "ruins32a.lmp", 15412 },
	{ "ruins32b.lmp", 3907 },
	{ "ruins32c.lmp", 8961 },
	{ "ruins32d.lmp", 1852 },
	{ "ruins32e.lmp", 8710 },
	{ "ruins33.lmp", 15907 },
	{ "ruins33a.lmp", 2594 },
	{ "ruins33b.lmp", 905 },
	{ "ruins33c.lmp", 1426 },
	{ "ruins33d.lmp", 1786 },
	{ "ruins33e.lmp", 1426 },
	{ "ruins33f.lmp", 1788 },
	{ "ruins33g.lmp", 1426 },
	{ "ruins34.lmp", 5437 },
	{ "ruins34a.lmp", 1790 },
	{ "ruins34b.lmp", 8 },
	{ "ruins34c.lmp", 1477 },
	{ "ruins34d.lmp", 58 },
	{ "ruins34e.lmp", 129 },
    { "ruinssecret.lmp", 66694 },
    { "sanctum.lmp", 6476194 },
    { "shop-roomgen00.lmp", 11844 },
    { "shop-roomgen01.lmp", 9917 },
    { "shop-roomgen02.lmp", 40191 },
    { "shop-roomgen02a.lmp", 4754 },
    { "shop-roomgen02b.lmp", 5250 },
    { "shop-roomgen02c.lmp", 5481 },
    { "shop-roomgen02d.lmp", 4467 },
    { "shop-roomgen02e.lmp", 4575 },
    { "shop-roomgen02f.lmp", 4025 },
    { "shop-roomgen03.lmp", 5324 },
    { "shop-roomgen04.lmp", 6945 },
    { "shop-roomgen05.lmp", 12261 },
    { "shop-roomgen06.lmp", 12925 },
    { "shop-roomgen07.lmp", 10729 },
    { "shop-roomgen08.lmp", 12489 },
    { "shop-roomgen09.lmp", 9209 },
    { "shop-roomgen10.lmp", 11533 },
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
    { "shop05.lmp", 13570 },
    { "shop06.lmp", 16915 },
    { "shop07.lmp", 13185 },
    { "shop08.lmp", 18483 },
    { "shop09.lmp", 10094 },
    { "shop10.lmp", 16181 },
    { "sokoban.lmp", 140500 },
    { "start.lmp", 648033 },
    { "swamp.lmp", 124034 },
    { "swamp00.lmp", 13629 },
    { "swamp01.lmp", 29459 },
    { "swamp01a.lmp", 5303 },
    { "swamp01b.lmp", 6563 },
    { "swamp01c.lmp", 10334 },
    { "swamp01d.lmp", 5571 },
    { "swamp01e.lmp", 4060 },
    { "swamp02.lmp", 39297 },
    { "swamp02a.lmp", 9131 },
    { "swamp02b.lmp", 11457 },
    { "swamp02c.lmp", 8877 },
    { "swamp02d.lmp", 7327 },
    { "swamp02e.lmp", 4203 },
    { "swamp03.lmp", 35148 },
    { "swamp03a.lmp", 6837 },
    { "swamp03b.lmp", 25871 },
    { "swamp03c.lmp", 3520 },
    { "swamp03d.lmp", 7686 },
    { "swamp03e.lmp", 15932 },
    { "swamp04.lmp", 51827 },
    { "swamp04a.lmp", 6931 },
    { "swamp04b.lmp", 5875 },
    { "swamp04c.lmp", 4610 },
    { "swamp04d.lmp", 7818 },
    { "swamp04e.lmp", 18431 },
    { "swamp05.lmp", 20558 },
    { "swamp05a.lmp", 2611 },
    { "swamp05b.lmp", 2966 },
    { "swamp05c.lmp", 5344 },
    { "swamp05d.lmp", 1610 },
    { "swamp05e.lmp", 2746 },
    { "swamp06.lmp", 60048 },
    { "swamp06a.lmp", 12354 },
    { "swamp06b.lmp", 31135 },
    { "swamp06c.lmp", 39770 },
    { "swamp06d.lmp", 60075 },
    { "swamp06e.lmp", 6324 },
    { "swamp07.lmp", 91130 },
    { "swamp07a.lmp", 12340 },
    { "swamp07b.lmp", 7290 },
    { "swamp07c.lmp", 20314 },
    { "swamp07d.lmp", 45805 },
    { "swamp07e.lmp", 43344 },
    { "swamp08.lmp", 31288 },
    { "swamp08a.lmp", 3734 },
    { "swamp08b.lmp", 5429 },
    { "swamp08c.lmp", 9406 },
    { "swamp08d.lmp", 31893 },
    { "swamp08e.lmp", 2913 },
    { "swamp09.lmp", 29565 },
    { "swamp09a.lmp", 1516 },
    { "swamp09b.lmp", 6762 },
    { "swamp09c.lmp", 6705 },
    { "swamp09d.lmp", 4955 },
    { "swamp09e.lmp", 3428 },
    { "swamp10.lmp", 27653 },
    { "swamp10a.lmp", 2322 },
    { "swamp10b.lmp", 5975 },
    { "swamp10c.lmp", 5260 },
    { "swamp10d.lmp", 6911 },
    { "swamp10e.lmp", 2344 },
    { "swamp10f.lmp", 3680 },
    { "swamp11.lmp", 64926 },
    { "swamp11a.lmp", 6851 },
    { "swamp11b.lmp", 11835 },
    { "swamp11c.lmp", 26869 },
    { "swamp11d.lmp", 46682 },
    { "swamp11e.lmp", 37481 },
    { "swamp12.lmp", 52448 },
    { "swamp12a.lmp", 7504 },
    { "swamp12b.lmp", 14581 },
    { "swamp12c.lmp", 8585 },
    { "swamp12d.lmp", 12097 },
    { "swamp12e.lmp", 13987 },
    { "swamp13.lmp", 30293 },
    { "swamp13a.lmp", 6911 },
    { "swamp13b.lmp", 10109 },
    { "swamp13c.lmp", 9103 },
    { "swamp13d.lmp", 5933 },
    { "swamp13e.lmp", 6334 },
    { "swamp14.lmp", 41242 },
    { "swamp14a.lmp", 9092 },
    { "swamp14b.lmp", 4317 },
    { "swamp14c.lmp", 14974 },
    { "swamp14d.lmp", 19525 },
    { "swamp14e.lmp", 6608 },
    { "swamp15.lmp", 352 },
    { "swamp16.lmp", 14028 },
    { "swamp16a.lmp", 1377 },
    { "swamp16b.lmp", 1411 },
    { "swamp16c.lmp", 856 },
    { "swamp16d.lmp", 2052 },
    { "swamp16e.lmp", 1154 },
    { "swamp17.lmp", 31463 },
    { "swamp17a.lmp", 5507 },
    { "swamp17b.lmp", 10976 },
    { "swamp17c.lmp", 9420 },
    { "swamp17d.lmp", 6550 },
    { "swamp17e.lmp", 4447 },
    { "swamp18.lmp", 18634 },
    { "swamp18a.lmp", 934 },
    { "swamp18b.lmp", 3264 },
    { "swamp18c.lmp", 5153 },
    { "swamp18d.lmp", 2974 },
    { "swamp18e.lmp", 1429 },
    { "swamp18f.lmp", 3207 },
    { "swamp18g.lmp", 3646 },
    { "swamp19.lmp", 20258 },
    { "swamp19a.lmp", 3302 },
    { "swamp19b.lmp", 1808 },
    { "swamp19c.lmp", 3136 },
    { "swamp19d.lmp", 3705 },
    { "swamp19e.lmp", 4800 },
    { "swamp20.lmp", 33017 },
    { "swamp20a.lmp", 4569 },
    { "swamp20b.lmp", 10215 },
    { "swamp20c.lmp", 5654 },
    { "swamp20d.lmp", 26165 },
    { "swamp20e.lmp", 6173 },
    { "swamp21.lmp", 16295 },
    { "swamp21a.lmp", 1887 },
    { "swamp21b.lmp", 3323 },
    { "swamp21c.lmp", 6881 },
    { "swamp21d.lmp", 25271 },
    { "swamp21e.lmp", 3401 },
    { "swamp22.lmp", 55275 },
    { "swamp22a.lmp", 13094 },
    { "swamp22b.lmp", 5215 },
    { "swamp22c.lmp", 16634 },
    { "swamp22d.lmp", 8710 },
    { "swamp22e.lmp", 26057 },
    { "swamp23.lmp", 63299 },
    { "swamp23a.lmp", 9089 },
    { "swamp23b.lmp", 18498 },
    { "swamp23c.lmp", 26586 },
    { "swamp23d.lmp", 8888 },
    { "swamp23e.lmp", 14992 },
    { "swamp24.lmp", 36395 },
    { "swamp24a.lmp", 5045 },
    { "swamp24b.lmp", 4267 },
    { "swamp24c.lmp", 5682 },
    { "swamp24d.lmp", 5928 },
    { "swamp24e.lmp", 12981 },
    { "swamp25.lmp", 34758 },
    { "swamp25a.lmp", 5060 },
    { "swamp25b.lmp", 6237 },
    { "swamp25c.lmp", 12252 },
    { "swamp25d.lmp", 5226 },
    { "swamp25e.lmp", 5717 },
    { "swamp25f.lmp", 10354 },
    { "swamp26.lmp", 40227 },
    { "swamp26a.lmp", 3747 },
    { "swamp26b.lmp", 2398 },
    { "swamp26c.lmp", 1005 },
    { "swamp26d.lmp", 3129 },
    { "swamp26e.lmp", 1262 },
    { "swamp27.lmp", 42021 },
    { "swamp27a.lmp", 4975 },
    { "swamp27b.lmp", 3725 },
    { "swamp27c.lmp", 2059 },
    { "swamp27d.lmp", 5312 },
    { "swamp27e.lmp", 4425 },
    { "swamp28.lmp", 6067 },
    { "swamp28a.lmp", 676 },
    { "swamp28b.lmp", 524 },
    { "swamp28c.lmp", 2016 },
    { "swamp28d.lmp", 1353 },
    { "swamp28e.lmp", 1457 },
    { "swamp28f.lmp", 338 },
    { "swamp29.lmp", 6111 },
    { "swamp29a.lmp", 591 },
    { "swamp29b.lmp", 596 },
    { "swamp29c.lmp", 1336 },
    { "swamp29d.lmp", 2619 },
    { "swamp29e.lmp", 1391 },
    { "swamp29f.lmp", 1249 },
    { "swamp29g.lmp", 1249 },
    { "swamp30.lmp", 54169 },
    { "swamp30a.lmp", 17250 },
    { "swamp30b.lmp", 69206 },
    { "swamp30c.lmp", 20925 },
    { "swamp30d.lmp", 18887 },
    { "swamp30e.lmp", 37453 },
    { "swamp31.lmp", 24287 },
    { "swamp31a.lmp", 7582 },
    { "swamp31b.lmp", 9169 },
    { "swamp31c.lmp", 2026 },
    { "swamp31d.lmp", 5365 },
    { "swamp31e.lmp", 35837 },
    { "swamp32.lmp", 30796 },
    { "swamp32a.lmp", 5496 },
    { "swamp32b.lmp", 6551 },
    { "swamp32c.lmp", 4638 },
    { "swamp32d.lmp", 6124 },
    { "swamp32e.lmp", 9867 },
    { "swamp33.lmp", 31993 },
    { "swamp33a.lmp", 9437 },
    { "swamp33b.lmp", 15085 },
    { "swamp33c.lmp", 6699 },
    { "swamp33d.lmp", 7910 },
    { "swamp33e.lmp", 10948 },
    { "swamp34.lmp", 36795 },
    { "swamp34a.lmp", 6929 },
    { "swamp34b.lmp", 68279 },
    { "swamp34c.lmp", 12514 },
    { "swamp34d.lmp", 5546 },
    { "swamp34e.lmp", 17278 },
    { "swampsecret.lmp", 24027 },
    { "swamptolabyrinth.lmp", 213824 },
    { "temple.lmp", 6911955 },
    { "tutorial1.lmp", 709674 },
    { "tutorial10.lmp", 1504767 },
    { "tutorial2.lmp", 1313718 },
    { "tutorial3.lmp", 1235301 },
    { "tutorial4.lmp", 1926157 },
    { "tutorial5.lmp", 873881 },
    { "tutorial6.lmp", 2024462 },
    { "tutorial7.lmp", 4059721 },
    { "tutorial8.lmp", 1721791 },
    { "tutorial9.lmp", 3166055 },
    { "tutorial_hub.lmp", 22944284 },
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
    { "underworld02e.lmp", 10867 },
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
    { "underworld09.lmp", 76902 },
    { "underworld09a.lmp", 29965 },
    { "underworld09b.lmp", 44375 },
    { "underworld09c.lmp", 35307 },
    { "underworld09d.lmp", 27376 },
    { "underworld09e.lmp", 30058 },
    { "underworld10.lmp", 119996 },
    { "underworld10a.lmp", 135977 },
    { "underworld10b.lmp", 47378 },
    { "underworld10c.lmp", 37212 },
    { "underworld10d.lmp", 41839 },
    { "underworld10e.lmp", 41223 },
    { "underworld10f.lmp", 50632 },
    { "underworld11.lmp", 118540 },
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
    { "underworld15d.lmp", 22683 },
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
    { "underworld28c.lmp", 7944 },
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
    { "warpzone.lmp", 3133088 },
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
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid[texnum]));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels));
	SDL_UnlockSurface(image);
}


bool completePath(char *dest, const char * const filename, const char *base) {
	if (!(filename && filename[0])) {
		return false;
	}

	// Already absolute
	if (filename[0] == '/') {
		strcpy(dest, filename);
		return true;
	}

#ifdef NINTENDO
	// Already absolute on nintendo
	if (strncmp(filename, base, strlen(base)) == 0) {
		strcpy(dest, filename);
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

    if (base == datadir && isCurrentHoliday()) {
        const auto holiday = getCurrentHoliday();
        const auto holiday_dir = holidayThemeDirs[holiday];
        assert(holiday_dir[0]);
        snprintf(dest, PATH_MAX, "%s/%s%s", base, holiday_dir, filename);
        if (!dataPathExists(dest, false)) {
            snprintf(dest, PATH_MAX, "%s/%s", base, filename);
        }
    } else {
        snprintf(dest, PATH_MAX, "%s/%s", base, filename);
    }
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
	GL_CHECK_ERR(glLoadTexture(allsurfaces[imgref], imgref));

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

constexpr float hellAmbience = 32.f;
#ifndef EDITOR
static ConsoleVariable<float> cvar_hell_ambience("/hell_ambience", hellAmbience);
#endif

/*-------------------------------------------------------------------------------

	loadMap

	Loads a map from the given filename

-------------------------------------------------------------------------------*/

bool verifyMapHash(const char* filename, int hash, bool *fileExistsInTable) {
	auto r = strrchr(filename, '/');
	auto it = mapHashes.find(r ? (r + 1) : filename);
	const int canonical = it != mapHashes.end() ? it->second : -1;
	if ( fileExistsInTable )
	{
		*fileExistsInTable = it != mapHashes.end();
	}
	const bool result = it != mapHashes.end() && (canonical == hash || canonical == -1 || hash == -1);
	if (!result) {
		printlog("map '%s' failed hash check (%d should be %d)", filename, hash, canonical);
	}
	return result;
}

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
	if ( strncmp(valid_data, "BARONY LMPV2.9", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.9 version of editor - chest mimic chance, and gates, pressure plate triggers
		editorVersion = 29;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.8", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.8 version of editor - teleport shrine dest x update
		editorVersion = 28;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.7", strlen("BARONY LMPV2.0")) == 0 )
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

	if ( destmap->trapexcludelocations )
	{
		free(destmap->trapexcludelocations);
		destmap->trapexcludelocations = nullptr;
	}
	if ( destmap->monsterexcludelocations )
	{
		free(destmap->monsterexcludelocations);
		destmap->monsterexcludelocations = nullptr;
	}
	if ( destmap->lootexcludelocations )
	{
		free(destmap->lootexcludelocations);
		destmap->lootexcludelocations = nullptr;
	}

	if ( destmap == &map )
	{
		// remove old lights
		list_FreeAll(&light_l);
		// remove old world UI
		if ( destmap->worldUI )
		{
			list_FreeAll(map.worldUI);
		}
		destmap->liquidSfxPlayedTiles.clear();
	}
	if ( destmap->tiles != nullptr )
	{
		free(destmap->tiles);
		destmap->tiles = nullptr;
	}
	if ( destmap == &map )
	{
#ifdef EDITOR
		if ( camera.vismap != nullptr )
		{
			free(camera.vismap);
			camera.vismap = nullptr;
		}
#endif
		if ( menucam.vismap != nullptr )
		{
			free(menucam.vismap);
			menucam.vismap = nullptr;
		}
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( cameras[i].vismap != nullptr )
			{
				free(cameras[i].vismap);
				cameras[i].vismap = nullptr;
			}
		}
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
	if ( destmap == &map )
	{
#ifdef EDITOR
		camera.vismap = (bool*)malloc(sizeof(bool) * destmap->width * destmap->height);
        memset(camera.vismap, 0, sizeof(bool) * destmap->height * destmap->width);
#endif
		menucam.vismap = (bool*)malloc(sizeof(bool) * destmap->width * destmap->height);
        memset(menucam.vismap, 0, sizeof(bool) * destmap->height * destmap->width);
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			cameras[i].vismap = (bool*)malloc(sizeof(bool) * destmap->width * destmap->height);
            memset(cameras[i].vismap, 0, sizeof(bool) * destmap->height * destmap->width);
		}
	}
	fp->read(destmap->tiles, sizeof(Sint32), destmap->width * destmap->height * MAPLAYERS);
	fp->read(&numentities, sizeof(Uint32), 1); // number of entities on the map

    const int mapsize = destmap->width * destmap->height * MAPLAYERS;
	for ( int c = 0; c < mapsize; ++c )
	{
		mapHashData += destmap->tiles[c];
	}
 
    // new as of july 30 2023
    // fix animated tiles so they always start on the correct index
    constexpr int numTileAtlases = sizeof(AnimatedTile::indices) / sizeof(AnimatedTile::indices[0]);
    for (int c = 0; c < mapsize; ++c) {
        int& tile = destmap->tiles[c];
        if (animatedtiles[tile]) {
            auto find = tileAnimations.find(tile);
            if (find == tileAnimations.end()) {
                // this is not the correct index!
                for (const auto& pair : tileAnimations) {
                    const auto& animation = pair.second;
                    for (int i = 0; i < numTileAtlases; ++i) {
                        if (animation.indices[i] == tile) {
                            tile = animation.indices[0];
                        }
                    }
                }
            }
        }
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
			case 28:
			case 29:
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
						if ( editorVersion >= 29 )
						{
							fp->read(&entity->yaw, sizeof(real_t), 1);
							fp->read(&entity->skill[9], sizeof(Sint32), 1);
							fp->read(&entity->chestLocked, sizeof(Sint32), 1);
							fp->read(&entity->chestMimicChance, sizeof(Sint32), 1);
						}
						else
						{
							setSpriteAttributes(entity, nullptr, nullptr);
							fp->read(&entity->yaw, sizeof(real_t), 1);
							fp->read(&entity->skill[9], sizeof(Sint32), 1);
							fp->read(&entity->chestLocked, sizeof(Sint32), 1);
						}
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
						if ( editorVersion >= 29 )
						{
							fp->read(&entity->skill[9], sizeof(Sint32), 1);
						}
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
						if ( editorVersion >= 28 )
						{
							fp->read(&entity->ceilingTileModel, sizeof(Sint32), 1);
							fp->read(&entity->ceilingTileDir, sizeof(Sint32), 1);
							fp->read(&entity->ceilingTileAllowTrap, sizeof(Sint32), 1);
							fp->read(&entity->ceilingTileBreakable, sizeof(Sint32), 1);
						}
						else
						{
							setSpriteAttributes(entity, nullptr, nullptr);
							fp->read(&entity->ceilingTileModel, sizeof(Sint32), 1);
						}
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
						if ( editorVersion >= 29 )
						{
							fp->read(&entity->signalInvertOutput, sizeof(Sint32), 1);
						}
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
					case 28:
						fp->read(&entity->signalInputDirection, sizeof(Sint32), 1);
						fp->read(&entity->signalActivateDelay, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerInterval, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerRepeatCount, sizeof(Sint32), 1);
						fp->read(&entity->signalTimerLatchInput, sizeof(Sint32), 1);
						fp->read(&entity->signalInvertOutput, sizeof(Sint32), 1);
						break;
					case 29:
						if ( editorVersion >= 29 )
						{
							fp->read(&entity->pressurePlateTriggerType, sizeof(Sint32), 1);
						}
						else
						{
							// don't read data, set default.
							setSpriteAttributes(entity, nullptr, nullptr);
						}
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
			if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_TUTORIAL
				&& gameModeManager.getMode() != GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
			{
				levelmusicplaying = false;
			}
		}
#endif

		// create new lightmap
        for (int c = 0; c < MAXPLAYERS + 1; ++c) {
            auto& lightmap = lightmaps[c];
            auto& lightmapSmoothed = lightmapsSmoothed[c];
            lightmap.resize(destmap->width * destmap->height);
            lightmapSmoothed.resize((destmap->width + 2) * (destmap->height + 2));
            if ( strncmp(map.name, "Hell", 4) )
            {
                memset(lightmap.data(), 0, sizeof(vec4_t) * map.width * map.height);
                memset(lightmapSmoothed.data(), 0, sizeof(vec4_t) * (map.width + 2) * (map.height + 2));
            }
            else
            {
                for (int c = 0; c < destmap->width * destmap->height; c++ )
                {
                    lightmap[c].x = hellAmbience;
                    lightmap[c].y = hellAmbience;
                    lightmap[c].z = hellAmbience;
#ifndef EDITOR
                    if ( svFlags & SV_FLAG_CHEATS )
                    {
                        lightmap[c].x = *cvar_hell_ambience;
                        lightmap[c].y = *cvar_hell_ambience;
                        lightmap[c].z = *cvar_hell_ambience;
                    }
#endif
                }
                for (int c = 0; c < (destmap->width + 2) * (destmap->height + 2); c++ )
                {
                    lightmapSmoothed[c].x = hellAmbience;
                    lightmapSmoothed[c].y = hellAmbience;
                    lightmapSmoothed[c].z = hellAmbience;
#ifndef EDITOR
                    if ( svFlags & SV_FLAG_CHEATS )
                    {
                        lightmapSmoothed[c].x = *cvar_hell_ambience;
                        lightmapSmoothed[c].y = *cvar_hell_ambience;
                        lightmapSmoothed[c].z = *cvar_hell_ambience;
                    }
#endif
                }
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
		*checkMapHash = mapHashData;
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

		fp->write("BARONY LMPV2.9", sizeof(char), strlen("BARONY LMPV2.0")); // magic code
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
					fp->write(&entity->chestMimicChance, sizeof(Sint32), 1);
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
					// summon trap
					fp->write(&entity->skill[0], sizeof(Sint32), 1);
					fp->write(&entity->skill[1], sizeof(Sint32), 1);
					fp->write(&entity->skill[2], sizeof(Sint32), 1);
					fp->write(&entity->skill[3], sizeof(Sint32), 1);
					fp->write(&entity->skill[4], sizeof(Sint32), 1);
					fp->write(&entity->skill[5], sizeof(Sint32), 1);
					fp->write(&entity->skill[9], sizeof(Sint32), 1);
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
					fp->write(&entity->ceilingTileDir, sizeof(Sint32), 1);
					fp->write(&entity->ceilingTileAllowTrap, sizeof(Sint32), 1);
					fp->write(&entity->ceilingTileBreakable, sizeof(Sint32), 1);
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
					fp->write(&entity->signalInvertOutput, sizeof(Sint32), 1);
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
				case 28:
					fp->write(&entity->signalInputDirection, sizeof(Sint32), 1);
					fp->write(&entity->signalActivateDelay, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerInterval, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerRepeatCount, sizeof(Sint32), 1);
					fp->write(&entity->signalTimerLatchInput, sizeof(Sint32), 1);
					fp->write(&entity->signalInvertOutput, sizeof(Sint32), 1);
					break;
				case 29:
					fp->write(&entity->pressurePlateTriggerType, sizeof(Sint32), 1);
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

std::list<std::string> directoryContents(const char* directory, bool includeSubdirectory, bool includeFiles, const char* base)
{
	std::list<std::string> list;
	char fullPath[PATH_MAX];
	completePath(fullPath, directory, base);
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
		if (entry->d_name[0] == '.') {
			if (!entry->d_name[1] || entry->d_name[1] == '.') {
				continue;
			}
		}
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
#ifdef EDITOR
				if ( gameModeManager.currentSession.seededRun.seed == 0 )
#else
				if ( gameModeManager.currentSession.seededRun.seed == 0 && !*cvar_map_sequence_rng )
#endif
				{
					mapseed = local_rng.rand();
				}
				else
				{
					int rng_cycles = std::max(0, currentlevel + (secretlevel ? 100 : 0));
					while ( rng_cycles > 0 )
					{
						map_sequence_rng.rand(); // dummy advance
						--rng_cycles;
					}
					mapseed = map_sequence_rng.rand();
				}
			}
			return loadMap(mapName.c_str(), &map, map.entities, map.creatures, checkMapHash);
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

bool physfsModelIndexUpdate(int &start, int &end)
{
	if ( !PHYSFS_getRealDir("models/models.txt") )
	{
		printlog("error: could not find file: %s", "models/models.txt");
		return false;
	}
	
	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");

	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	if ( !fp )
	{
		return false;
	}
	char modelName[PATH_MAX];
	int startnum = 0;
	int endnum = nummodels;
	for ( int c = 0; !fp->eof(); c++ )
	{
		fp->gets2(modelName, PATH_MAX);
		bool modelHasBeenModified = false;
		// has this model index been modified?
		std::vector<int>::iterator it = Mods::modelsListModifiedIndexes.end();
		if ( !Mods::modelsListModifiedIndexes.empty() )
		{
			it = std::find(Mods::modelsListModifiedIndexes.begin(),
				Mods::modelsListModifiedIndexes.end(), c);
			if ( it != Mods::modelsListModifiedIndexes.end() )
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
				Mods::modelsListModifiedIndexes.push_back(c);
			}
			else
			{
				if ( modelPath.compare("./") == 0 )
				{
					// model returned to base directory, remove from the modified index list.
					Mods::modelsListModifiedIndexes.erase(it);
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

	FileIO::close(fp);
	return true;
}

/*-------------------------------------------------------------------------------

	generatePolyModels

	processes voxel models and turns them into polygon-based models (surface
	optimized)

-------------------------------------------------------------------------------*/

void saveModelCache() {
	File* model_cache;
	const std::string cache_path = std::string(outputdir) + "/models.cache";
	if (model_cache = openDataFile(cache_path.c_str(), "wb")) {
		char modelCacheHeader[32] = "BARONY";
		strcat(modelCacheHeader, VERSION);
		model_cache->write(&modelCacheHeader, sizeof(char), strlen(modelCacheHeader));
		for (size_t model_index = 0; model_index < nummodels; model_index++) {
			polymodel_t* cur = &polymodels[model_index];
			model_cache->write(&cur->numfaces, sizeof(cur->numfaces), 1);
			model_cache->write(cur->faces, sizeof(polytriangle_t), cur->numfaces);
		}
		FileIO::close(model_cache);
	}
}

#ifndef EDITOR
#include "interface/consolecommand.hpp"
static ConsoleCommand ccmd_writeModelCache("/write_model_cache", "",
	[](int argc, const char** argv){
	saveModelCache();
	});
#endif

void generatePolyModels(int start, int end, bool forceCacheRebuild)
{
	const bool generateAll = start == 0 && end == nummodels;
    constexpr auto LARGEST_POLYMODEL_FACES_ALLOWED = (1<<17); // 131072

	if ( generateAll )
	{
		if (polymodels) {
			for (int c = 0; c < nummodels; ++c) {
				if (polymodels[c].faces) {
					free(polymodels[c].faces);
					polymodels[c].faces = nullptr;
				}
			}
			free(polymodels);
			polymodels = nullptr;
		}
		polymodels = (polymodel_t*)malloc(sizeof(polymodel_t) * nummodels);
        memset(polymodels, 0, sizeof(polymodel_t) * nummodels);
		if ( useModelCache && !forceCacheRebuild )
		{
#ifndef NINTENDO
            std::string cache_path;
            if (isCurrentHoliday()) {
                const auto holiday = getCurrentHoliday();
                switch (holiday) {
                case HolidayTheme::THEME_NONE:
                    cache_path = std::string(outputdir) + "/models.cache";
                    break;
                default:
                    cache_path = "models.cache";
                    break;         
                }
            } else {
                cache_path = std::string(outputdir) + "/models.cache";
            }
#else
			std::string cache_path = "models.cache";
#endif
			auto model_cache = openDataFile(cache_path.c_str(), "rb");
			if ( model_cache )
			{
				printlog("loading model cache...\n");
				char polymodelsVersionStr[7] = "v0.0.0";
				char modelsCacheHeader[7] = "000000";
				model_cache->read(&modelsCacheHeader, sizeof(char), strlen("BARONY"));

				if ( !strcmp(modelsCacheHeader, "BARONY") )
				{
					// we're using the new polymodels file.
					model_cache->read(&polymodelsVersionStr, sizeof(char), strlen(VERSION));
					printlog("[MODEL CACHE]: Using updated version format %s.", polymodelsVersionStr);
					if ( strncmp(polymodelsVersionStr, VERSION, strlen(VERSION)) )
					{
						// different version.
						printlog("[MODEL CACHE]: Detected outdated version number %s - current is %s. Upgrading cache...", polymodelsVersionStr, VERSION);
                        FileIO::close(model_cache);
                        goto generate;
					}
				}
				else
				{
					printlog("[MODEL CACHE]: Detected legacy cache without embedded version data, upgrading cache to %s...", VERSION);
					FileIO::close(model_cache);
					goto generate;
				}
                
                for ( size_t model_index = 0; model_index < nummodels; model_index++ ) {
                    updateLoadingScreen(30 + ((real_t)model_index / nummodels) * 30.0);
                    polymodel_t* cur = &polymodels[model_index];
  
                    size_t readsize;
                    readsize = model_cache->read(&cur->numfaces, sizeof(cur->numfaces), 1);
                    if (readsize == 1) {
                        readsize = 0;
                        if (cur->numfaces && cur->numfaces <= LARGEST_POLYMODEL_FACES_ALLOWED) {
                            cur->faces = (polytriangle_t*)calloc(sizeof(polytriangle_t), cur->numfaces);
                            if (cur->faces) {
                                readsize = model_cache->read(polymodels[model_index].faces, sizeof(polytriangle_t), cur->numfaces);
                            }
                        }
                        if (!readsize || readsize != cur->numfaces) {
                            printlog("[MODEL CACHE]: Error loading model cache, rebuilding...");
                            FileIO::close(model_cache);
                            goto generate;
                        }
                    } else {
                        printlog("[MODEL CACHE]: Error loading model cache, rebuilding...");
                        FileIO::close(model_cache);
                        goto generate;
                    }
                }
                
                printlog("successfully loaded model cache.\n");
                FileIO::close(model_cache);
                return;
			}
		}
	}

	printlog("generating poly models...\n");
 
 generate:

	Sint32 x, y, z;
	Sint32 c, i;
	Uint32 index, indexdown[3];
	Uint8 newcolor, oldcolor;
	bool buildingquad;
	polyquad_t* quad1, * quad2;
	Uint32 numquads;
	list_t quads;
	quads.first = NULL;
	quads.last = NULL;

	if ( !polymodels )
	{
		polymodels = (polymodel_t*)malloc(sizeof(polymodel_t) * nummodels);
		memset(polymodels, 0, sizeof(polymodel_t) * nummodels);
	}

	for ( c = start; c < end; ++c )
	{
		updateLoadingScreen(30 + ((real_t)(c - start) / (end - start)) * 30.0);
		numquads = 0;
		polymodels[c].numfaces = 0;
		voxel_t* model = models[c];
		if ( !model )
		{
			continue;
		}
		indexdown[0] = model->sizez * model->sizey;
		indexdown[1] = model->sizez;
		indexdown[2] = 1;

		// find front faces
		for ( x = models[c]->sizex - 1; x >= 0; x-- )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( y = 0; y < models[c]->sizey; y++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( x < models[c]->sizex - 1 )
							if ( models[c]->data[index + indexdown[0]] >= 0 && models[c]->data[index + indexdown[0]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f + 1;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f + 1;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( x == models[c]->sizex - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[0]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 0;
								quad1->vertex[0].x = x - model->sizex / 2.f + 1;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f + 1;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f + 1;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f + 1;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find back faces
		for ( x = 0; x < models[c]->sizex; x++ )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( y = 0; y < models[c]->sizey; y++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( x > 0 )
							if ( models[c]->data[index - indexdown[0]] >= 0 && models[c]->data[index - indexdown[0]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( x == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[0]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 1;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find right faces
		for ( y = models[c]->sizey - 1; y >= 0; y-- )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( y < models[c]->sizey - 1 )
							if ( models[c]->data[index + indexdown[1]] >= 0 && models[c]->data[index + indexdown[1]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f + 1;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f + 1;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( y == models[c]->sizey - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[1]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 2;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f + 1;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f + 1;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;
					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f + 1;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f + 1;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find left faces
		for ( y = 0; y < models[c]->sizey; y++ )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( y > 0 )
							if ( models[c]->data[index - indexdown[1]] >= 0 && models[c]->data[index - indexdown[1]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( y == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[1]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 3;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;
					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find bottom faces
		for ( z = models[c]->sizez - 1; z >= 0; z-- )
		{
			for ( y = 0; y < models[c]->sizey; y++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( z < models[c]->sizez - 1 )
							if ( models[c]->data[index + indexdown[2]] >= 0 && models[c]->data[index + indexdown[2]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f + 1;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].y++;
												quad2->vertex[3].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( z == models[c]->sizez - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[2]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 4;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f + 1;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f + 1;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].y++;
										quad2->vertex[3].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find top faces
		for ( z = 0; z < models[c]->sizez; z++ )
		{
			for ( y = 0; y < models[c]->sizey; y++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( z > 0 )
							if ( models[c]->data[index - indexdown[2]] >= 0 && models[c]->data[index - indexdown[2]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f + 1;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].y++;
												quad2->vertex[1].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( z == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[2]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*)calloc(1, sizeof(polyquad_t));
								quad1->side = 5;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f + 1;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f + 1;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].y++;
										quad2->vertex[1].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// translate quads into triangles
        if (polymodels[c].faces) {
            free(polymodels[c].faces);
			polymodels[c].faces = nullptr;
        }
		polymodels[c].faces = (polytriangle_t*)malloc(sizeof(polytriangle_t) * polymodels[c].numfaces);
		for ( uint64_t i = 0; i < polymodels[c].numfaces; i++ )
		{
			node_t* node = list_Node(&quads, (int)i / 2);
			polyquad_t* quad = (polyquad_t*)node->element;
            auto& face = polymodels[c].faces[i];
            switch (quad->side) {
            case 0: face.normal = { 1.f,  0.f,  0.f}; break; // front
            case 1: face.normal = {-1.f,  0.f,  0.f}; break; // back
            case 2: face.normal = { 0.f,  1.f,  0.f}; break; // right
            case 3: face.normal = { 0.f, -1.f,  0.f}; break; // left
            case 4: face.normal = { 0.f,  0.f,  1.f}; break; // bottom
            case 5: face.normal = { 0.f,  0.f, -1.f}; break; // top
            default: printlog("[MODELS] this should never happen!"); assert(0); break;
            }
			face.r = quad->r;
			face.g = quad->g;
			face.b = quad->b;
			if ( i % 2 )
			{
				face.vertex[0] = quad->vertex[0];
				face.vertex[1] = quad->vertex[1];
				face.vertex[2] = quad->vertex[2];
			}
			else
			{
				face.vertex[0] = quad->vertex[0];
				face.vertex[1] = quad->vertex[2];
				face.vertex[2] = quad->vertex[3];
			}
		}

		// free up quads for the next model
		list_FreeAll(&quads);
	}
#ifndef NINTENDO
    if (!isCurrentHoliday() && useModelCache) {
		saveModelCache();
    }
#endif

    uint64_t greatest = 0;
    for (uint32_t c = 0; c < nummodels; ++c) {
        greatest = std::max(greatest, polymodels[c].numfaces);
    }
    printlog("greatest number of faces on any model: %lld", greatest);
}

void reloadModels(int start, int end) {
	start = std::clamp(start, 0, (int)nummodels - 1);
	end = std::clamp(end, 0, (int)nummodels);

	if ( start >= end ) {
		return;
	}

	//messagePlayer(clientnum, Language::get(2354));
#ifndef EDITOR
	messagePlayer(clientnum, MESSAGE_MISC, Language::get(2355), start, end);
#endif

	loading = true;
	if ( intro ) {
		createLoadingScreen(5);
	}
	else {
		createLevelLoadScreen(5);
	}
	doLoadingScreen();

	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, sizeof(name));
		if ( c >= start && c < end ) {
			if (polymodels[c].vao) {
				GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
			}
            if (polymodels[c].positions) {
                GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
            }
            if (polymodels[c].colors) {
                GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
            }
            if (polymodels[c].normals) {
                GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
            }
		}
	}

	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done, start, end]() {
		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
		modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
		File* fp = openDataFile(modelsDirectory.c_str(), "rb");
		for ( int c = 0; !fp->eof(); c++ )
		{
			char name[128];
			fp->gets2(name, sizeof(name));
			if ( c >= start && c < end )
			{
				if ( models[c] != NULL )
				{
					if ( models[c]->data )
					{
						free(models[c]->data);
					}
					free(models[c]);
					if ( polymodels[c].faces )
					{
						free(polymodels[c].faces);
						polymodels[c].faces = nullptr;
					}
					models[c] = loadVoxel(name);
				}
			}
		}
		FileIO::close(fp);
		generatePolyModels(start, end, true);
		loading_done = true;
		return 0;
		});
	while ( !loading_done )
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	generateVBOs(start, end);
	destroyLoadingScreen();
	loading = false;
}

void generateVBOs(int start, int end)
{
	const int count = end - start;

	std::unique_ptr<GLuint[]> vaos(new GLuint[count]);
	GL_CHECK_ERR(glGenVertexArrays(count, vaos.get()));

	std::unique_ptr<GLuint[]> position_vbos(new GLuint[count]);
	GL_CHECK_ERR(glGenBuffers(count, position_vbos.get()));

	std::unique_ptr<GLuint[]> color_vbos(new GLuint[count]);
	GL_CHECK_ERR(glGenBuffers(count, color_vbos.get()));

	std::unique_ptr<GLuint[]> normal_vbos(new GLuint[count]);
	GL_CHECK_ERR(glGenBuffers(count, normal_vbos.get()));

	for ( uint64_t c = (uint64_t)start; c < (uint64_t)end; ++c )
	{
		polymodel_t* model = &polymodels[c];
		std::unique_ptr<GLfloat[]> positions(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> colors(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> normals(new GLfloat[9 * model->numfaces]);
		for ( uint64_t i = 0; i < (uint64_t)model->numfaces; i++ )
		{
			const polytriangle_t* face = &model->faces[i];
			for ( uint64_t vert_index = 0; vert_index < 3; vert_index++ )
			{
				const uint64_t data_index = i * 9 + vert_index * 3;
				const vertex_t* vert = &face->vertex[vert_index];

				positions[data_index] = vert->x;
				positions[data_index + 1] = -vert->z;
				positions[data_index + 2] = vert->y;

				colors[data_index] = face->r / 255.f;
				colors[data_index + 1] = face->g / 255.f;
				colors[data_index + 2] = face->b / 255.f;
    
				normals[data_index] = face->normal.x;
				normals[data_index + 1] = -face->normal.z;
				normals[data_index + 2] = face->normal.y;
			}
		}
		model->vao = vaos[c - start];
		model->positions = position_vbos[c - start];
		model->colors = color_vbos[c - start];
		model->normals = normal_vbos[c - start];

		// NOTE: OpenGL 2.1 does not support vertex array objects!
#ifdef VERTEX_ARRAYS_ENABLED
		GL_CHECK_ERR(glBindVertexArray(model->vao));
#endif

		// position data
		GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, model->positions));
		GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, positions.get(), GL_STATIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
		GL_CHECK_ERR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
		GL_CHECK_ERR(glEnableVertexAttribArray(0));
#endif

		// color data
		GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, model->colors));
		GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, colors.get(), GL_STATIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
		GL_CHECK_ERR(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
		GL_CHECK_ERR(glEnableVertexAttribArray(1));
#endif

		// normal data
		GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, model->normals));
		GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, normals.get(), GL_STATIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
		GL_CHECK_ERR(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
		GL_CHECK_ERR(glEnableVertexAttribArray(2));
#endif

#ifndef VERTEX_ARRAYS_ENABLED
		GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif

		const int current = (int)c - start;
		updateLoadingScreen(80 + (10 * current) / count);
		doLoadingScreen();
	}
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
		bool soundHasBeenModified = false;
		// has this sound index been modified?
		std::vector<int>::iterator it = Mods::soundsListModifiedIndexes.end();
		if ( !Mods::soundsListModifiedIndexes.empty() )
		{
			it = std::find(Mods::soundsListModifiedIndexes.begin(),
				Mods::soundsListModifiedIndexes.end(), c);
			if ( it != Mods::soundsListModifiedIndexes.end() )
			{
				soundHasBeenModified = true; // found the sound in the vector.
			}
		}

		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string soundRealDir = PHYSFS_getRealDir(name);
			if ( soundHasBeenModified || reloadAll || soundRealDir.compare("./") != 0 )
			{
				std::string soundFile = soundRealDir;
				soundFile.append(PHYSFS_getDirSeparator()).append(name);

				if ( !soundHasBeenModified )
				{
					// add this sound index to say we've modified it as the base dir is not default.
					Mods::soundsListModifiedIndexes.push_back(c);
				}
				else
				{
					if ( soundRealDir.compare("./") == 0 )
					{
						// model returned to base directory, remove from the modified index list.
						Mods::soundsListModifiedIndexes.erase(it);
					}
				}

#ifdef USE_FMOD
				if ( !reloadAll )
				{
					sounds[c]->release();
					sounds[c] = nullptr;
				}
				FMOD_MODE flags = FMOD_DEFAULT | FMOD_3D | FMOD_LOWMEM;
				if ( c == 133 || c == 672 || c == 135 || c == 155 || c == 149 )
				{
					flags |= FMOD_LOOP_NORMAL;
				}
				fmod_result = fmod_system->createSound(soundFile.c_str(), flags, nullptr, &sounds[c]); //TODO: FMOD_SOFTWARE -> FMOD_DEFAULT?
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
				if (Mods::isLoading) {
					updateLoadingScreen(20.f + (c / (float)numsounds) * 10.f);
				}
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

	/*int numsprites = 0;
	for ( numsprites = 0; !fp->eof(); numsprites++ )
	{
		while ( fp->getc() != '\n' ) if ( fp->eof() )
		{
			break;
		}
	}*/

	for ( int c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, PATH_MAX);
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
		if (Mods::isLoading) {
			doLoadingScreen();
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
    // load tiles.txt
	if (!PHYSFS_getRealDir("images/tiles.txt")) {
		printlog("error: could not find file: %s", "images/tiles.txt");
	} else {
        std::string directory = PHYSFS_getRealDir("images/tiles.txt");
        directory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
        printlog("[PhysFS]: Loading tiles from directory %s...\n", directory.c_str());
        File* fp = openDataFile(directory.c_str(), "rb");
        if (!fp) {
            printlog("error: could not open file: %s", "images/tiles.txt");
        } else {
            for ( int c = 0; !fp->eof(); c++ )
            {
				if (Mods::isLoading) {
					doLoadingScreen();
				}
                char name[PATH_MAX];
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
                            printlog("warning: failed to load '%s' listed at line %d in %s\n",
                                name, c + 1, directory.c_str());
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
    }
 
    // load animated.txt
	if (!PHYSFS_getRealDir("images/animated.txt")) {
		printlog("error: could not find file: %s", "images/animated.txt");
	} else {
        std::string directory = PHYSFS_getRealDir("images/animated.txt");
        directory.append(PHYSFS_getDirSeparator()).append("images/animated.txt");
        printlog("[PhysFS]: Loading tile animations from directory %s...\n", directory.c_str());
        File* fp = openDataFile(directory.c_str(), "rb");
        if (!fp) {
            printlog("error: could not open file: %s", "images/animated.txt");
        } else {
            tileAnimations.clear();
            for (int c = 0; !fp->eof(); ++c) {
                AnimatedTile animation;
                char line[PATH_MAX];
                fp->gets2(line, PATH_MAX);
                
                // extract animation frames
                constexpr int numIndices = sizeof(animation.indices) / sizeof(animation.indices[0]);
                char *str = line, *end;
                int index = 0;
                do {
                    animation.indices[index] = (int)strtol(str, &end, 10);
                    str = end + 1;
                    ++index;
                } while (end && *end == ' ' && index < numIndices);
                tileAnimations.insert({animation.indices[0], animation});
            }
            FileIO::close(fp);
        }
    }
 
    // generate tile texture atlas
	generateTileTextures();
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
			if ( levelsCounted >= officialLevelsTxtOrder.size() )
			{
				return true;
			}
			if ( mapName.compare(officialLevelsTxtOrder.at(levelsCounted)) != 0 )
			{
				return true;
			}
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			/*if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}*/
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
			if ( levelsCounted >= officialSecretlevelsTxtOrder.size() )
			{
				return true;
			}
			if ( mapName.compare(officialSecretlevelsTxtOrder.at(levelsCounted)) != 0 )
			{
				return true;
			}
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			/*if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}*/
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
	printlog("loading model offsets...\n");
	for ( int c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( int x = 0; x < 20; x++ )
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
		File* fp;

		if ( PHYSFS_getRealDir(filename) )
		{
			std::string limbsDir = PHYSFS_getRealDir(filename);
			limbsDir.append(PHYSFS_getDirSeparator()).append(filename);
			if ( (fp = openDataFile(limbsDir.c_str(), "rb")) == NULL )
			{
				continue;
			}
		}
		else
		{
			if ( (fp = openDataFile(filename, "rb")) == NULL )
			{
				continue;
			}
		}

		// read file
		int line;
		for ( line = 1; !fp->eof(); line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fp->gets(data, 256);

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
				continue;
			}
			if ( sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
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
	Mods::systemResourceImagesToReload.clear();

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
				Mods::systemResourceImagesToReload.push_back(line);
			}
		}
	}
	return requireReload;
}

void physfsReloadSystemImages()
{
	if ( !Mods::systemResourceImagesToReload.empty() )
	{
		for ( std::vector<std::pair<SDL_Surface**, std::string>>::const_iterator it = Mods::systemResourceImagesToReload.begin(); it != Mods::systemResourceImagesToReload.end(); ++it )
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
