#pragma once

int initApp(char* title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode();
void generatePolyModels(int start, int end, bool forceCacheRebuild);
void generateVBOs(int start, int end);
int loadLanguage(char* lang);
int reloadLanguage();
void freeLanguages();
