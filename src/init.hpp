#pragma once

int initApp(const char* title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode();
void generatePolyModels(int start, int end);
void generateVBOs(int start, int end);
int loadLanguage(const char* lang);
int reloadLanguage();
void freeLanguages();
