#pragma once

class Button;

void settingsUI(Button&);
void settingsVideo(Button&);
void settingsAudio(Button&);
void settingsControls(Button&);
void settingsGame(Button&);

void recordsAdventureArchives(Button&);
void recordsLeaderboards(Button&);
void recordsDungeonCompendium(Button&);
void recordsStoryIntroduction(Button&);
void recordsCredits(Button&);
void recordsBackToMainMenu(Button&);

void mainPlayGame(Button&);
void mainPlayModdedGame(Button&);
void mainHallOfRecords(Button&);
void mainSettings(Button&);
void mainQuit(Button&);

void doMainMenu();
void createMainMenu();
void destroyMainMenu();
