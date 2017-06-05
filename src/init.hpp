#pragma once

int initApp(char* title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode();
void generatePolyModels();
void generateVBOs();
int loadLanguage(char* lang);
int reloadLanguage();
