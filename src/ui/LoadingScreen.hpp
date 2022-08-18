#pragma once

#include "../main.hpp"

void createLoadingScreen(real_t progress);
void createLevelLoadScreen(real_t progress);
void updateLoadingScreen(real_t progress);
void doLoadingScreen();
void destroyLoadingScreen();

extern Uint32 loadingticks;