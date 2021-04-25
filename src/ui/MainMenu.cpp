#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../draw.hpp"
#include "../sound.hpp"

static Frame* main_menu_frame = nullptr;
static int buttons_height = 0;
static Uint32 menu_ticks = 0u;
static float main_menu_cursor_bob = 0.f;
static int main_menu_cursor_x = 0;
static int main_menu_cursor_y = 0;

static const char* bigfont_outline = "fonts/pixelmix.ttf#18#2";
static const char* bigfont_no_outline = "fonts/pixelmix.ttf#18#0";
static const char* smallfont_outline = "fonts/pixel_maz.ttf#32#2";
static const char* smallfont_no_outline = "fonts/pixel_maz.ttf#48#2";

/******************************************************************************/

void recordsAdventureArchives(Button& button) {
	playSound(139, 64); // click sound
}

void recordsLeaderboards(Button& button) {
	playSound(139, 64); // click sound
}

void recordsDungeonCompendium(Button& button) {
	playSound(139, 64); // click sound
}

void recordsStoryIntroduction(Button& button) {
	playSound(139, 64); // click sound
}

void recordsBackToMainMenu(Button& button) {
	playSound(139, 64); // click sound

	assert(main_menu_frame);

	// revert notification section
	auto notification = main_menu_frame->findFrame("notification"); assert(notification);
	auto image = notification->findImage("background"); assert(image);
	image->path = "images/ui/Main Menus/Main/UI_MainMenu_EXNotification.png";
	notification->setSize(SDL_Rect{
		(Frame::virtualScreenX - 236 * 2) / 2,
		notification->getSize().y,
		236 * 2,
		49 * 2
		});
	notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
	image->pos = notification->getActualSize();
	notification->remove("text");

	// enable banners
	for (int c = 0; c < 2; ++c) {
		std::string name = std::string("banner") + std::to_string(c + 1);
		auto banner = main_menu_frame->findFrame(name.c_str());
		banner->setDisabled(false);
	}

	// delete existing buttons
	auto old_buttons = main_menu_frame->findFrame("buttons");
	old_buttons->removeSelf();

	// put original options back
	struct Option {
		const char* name;
		void (*callback)(Button&);
	};
#ifdef NINTENDO
	Option options[] = {
		{"PLAY GAME", mainPlayGame},
		{"HALL OF RECORDS", mainHallOfRecords},
		{"SETTINGS", mainSettings}
	};
#else
	Option options[] = {
		{"PLAY GAME", mainPlayGame},
		{"PLAY MODDED GAME", mainPlayModdedGame},
		{"HALL OF RECORDS", mainHallOfRecords},
		{"SETTINGS", mainSettings},
		{"QUIT", mainQuit}
	};
#endif
	const int num_options = sizeof(options) / sizeof(options[0]);

	int y = buttons_height;

	auto buttons = main_menu_frame->addFrame("buttons");
	buttons->setSize(SDL_Rect{0, y, Frame::virtualScreenX, 36 * num_options});
	buttons->setActualSize(SDL_Rect{0, 0, buttons->getSize().w, buttons->getSize().h});
	buttons->setHollow(true);
	buttons->setBorder(0);
	for (int c = 0; c < num_options; ++c) {
		auto button = buttons->addButton(options[c].name);
		button->setCallback(options[c].callback);
		button->setBorder(8);
		button->setHJustify(Button::justify_t::LEFT);
		button->setVJustify(Button::justify_t::CENTER);
		button->setText(options[c].name);
		button->setFont(smallfont_no_outline);
		button->setBackground("images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
		button->setColor(makeColor(255, 255, 255, 127));
		button->setHighlightColor(makeColor(255, 255, 255, 255));
		button->setTextColor(makeColor(180, 180, 180, 255));
		button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		button->setSize(SDL_Rect{
			(Frame::virtualScreenX - 164 * 2) / 2,
			y - buttons->getSize().y,
			164 * 2,
			16 * 2
			});
		int back = c - 1 < 0 ? num_options - 1 : c - 1;
		int forward = c + 1 >= num_options ? 0 : c + 1;
		button->setWidgetDown(options[forward].name);
		button->setWidgetUp(options[back].name);
		y += button->getSize().h;
		y += 4;
	}
	y += 16;

	auto records = buttons->findButton("HALL OF RECORDS");
	if (records) {
		records->select();
	}
}

/******************************************************************************/

void mainPlayGame(Button& button) {
	playSound(139, 64); // click sound
}

void mainPlayModdedGame(Button& button) {
	playSound(139, 64); // click sound
}

void mainHallOfRecords(Button& button) {
	playSound(139, 64); // click sound

	assert(main_menu_frame);

	// change "notification" section into Hall of Records banner
	auto notification = main_menu_frame->findFrame("notification"); assert(notification);
	auto image = notification->findImage("background"); assert(image);
	image->path = "images/ui/Main Menus/AdventureArchives/UI_AdventureArchives_TitleGraphic00.png";
	notification->setSize(SDL_Rect{
		(Frame::virtualScreenX - 204 * 2) / 2,
		notification->getSize().y,
		204 * 2,
		43 * 2
		});
	notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
	image->pos = notification->getActualSize();

	// add banner text to notification
	auto banner_text = notification->addField("text", 64);
	banner_text->setJustify(Field::justify_t::CENTER);
	banner_text->setText("HALL OF RECORDS");
	banner_text->setFont(smallfont_no_outline);
	banner_text->setColor(makeColor(180, 135, 27, 255));
	banner_text->setSize(SDL_Rect{19 * 2, 15 * 2, 166 * 2, 12 * 2});

	// disable banners
	for (int c = 0; c < 2; ++c) {
		std::string name = std::string("banner") + std::to_string(c + 1);
		auto banner = main_menu_frame->findFrame(name.c_str());
		banner->setDisabled(true);
	}

	// delete existing buttons
	auto old_buttons = main_menu_frame->findFrame("buttons");
	old_buttons->removeSelf();

	struct Option {
		const char* name;
		void (*callback)(Button&);
	};
	Option options[] = {
		{"ADVENTURE ARCHIVES", recordsAdventureArchives},
		{"DUNGEON COMPENDIUM", recordsDungeonCompendium},
		{"STORY INTRODUCTION", recordsStoryIntroduction},
		{"BACK TO MAIN MENU", recordsBackToMainMenu}
	};
	const int num_options = sizeof(options) / sizeof(options[0]);

	int y = buttons_height;

	auto buttons = main_menu_frame->addFrame("buttons");
	buttons->setSize(SDL_Rect{0, y, Frame::virtualScreenX, 36 * (num_options + 1)});
	buttons->setActualSize(SDL_Rect{0, 0, buttons->getSize().w, buttons->getSize().h});
	buttons->setHollow(true);
	buttons->setBorder(0);
	for (int c = 0; c < num_options; ++c) {
		auto button = buttons->addButton(options[c].name);
		button->setCallback(options[c].callback);
		button->setBorder(8);
		button->setHJustify(Button::justify_t::LEFT);
		button->setVJustify(Button::justify_t::CENTER);
		button->setText(options[c].name);
		button->setFont(smallfont_no_outline);
		button->setBackground("images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
		button->setColor(makeColor(255, 255, 255, 127));
		button->setHighlightColor(makeColor(255, 255, 255, 255));
		button->setTextColor(makeColor(180, 180, 180, 255));
		button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		button->setSize(SDL_Rect{
			(Frame::virtualScreenX - 164 * 2) / 2,
			y - buttons->getSize().y,
			164 * 2,
			16 * 2
			});
		int back = c - 1 < 0 ? num_options - 1 : c - 1;
		int forward = c + 1 >= num_options ? 0 : c + 1;
		button->setWidgetDown(options[forward].name);
		button->setWidgetUp(options[back].name);
		button->setWidgetBack("BACK TO MAIN MENU");
		y += button->getSize().h;
		y += 4;
		if (c == num_options - 2) {
			y += button->getSize().h;
			y += 4;
		}
	}
	y += 16;

	auto archives = buttons->findButton("ADVENTURE ARCHIVES");
	if (archives) {
		archives->select();
	}
}

void mainSettings(Button& button) {
	playSound(139, 64); // click sound
}

void mainQuit(Button& button) {
	playSound(139, 64); // click sound
}

/******************************************************************************/

void doMainMenu() {
	if (!main_menu_frame) {
		createMainMenu();
	}
	assert(main_menu_frame);
	if (menu_ticks < ticks) {
		++menu_ticks;

		// move cursor to current menu selection
		auto cursor = main_menu_frame->findImage("cursor"); assert(cursor);
		auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
		for (auto button : buttons->getButtons()) {
			if (button->isSelected()) {
				main_menu_cursor_x = button->getSize().x - 80;
				main_menu_cursor_y = button->getSize().y - 9 + buttons->getSize().y;
			}
		}
		
		// bob cursor
		const float bobrate = (float)PI * 2.f / (float)TICKS_PER_SECOND;
		main_menu_cursor_bob += bobrate;
		if (main_menu_cursor_bob >= (float)PI * 2.f) {
			main_menu_cursor_bob -= (float)PI * 2.f;
		}

		// update cursor position
		int diff = main_menu_cursor_y - cursor->pos.y;
		if (diff > 0) {
			diff = std::max(1, diff / 4);
		} else if (diff < 0) {
			diff = std::min(-1, diff / 4);
		}
		cursor->pos = SDL_Rect{
			main_menu_cursor_x + (int)(sinf(main_menu_cursor_bob) * 16.f) - 16,
			diff + cursor->pos.y,
			37 * 2,
			23 * 2
		};
	}
}

void createMainMenu() {
	main_menu_frame = gui->addFrame("main_menu");

	auto frame = main_menu_frame;
	frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	frame->setActualSize(SDL_Rect{0, 0, frame->getSize().w, frame->getSize().h});
	frame->setHollow(true);
	frame->setBorder(0);

	int y = 16;

	auto title_img = Image::get("images/system/title.png");
	auto title = frame->addImage(
		SDL_Rect{
			(int)(Frame::virtualScreenX - (int)title_img->getWidth() * 2.0 / 3.0) / 2,
			y,
			(int)(title_img->getWidth() * 2.0 / 3.0),
			(int)(title_img->getHeight() * 2.0 / 3.0)
		},
		makeColor(255, 255, 255, 255),
		title_img->getName(),
		"title"
	);
	y += title->pos.h;

	auto notification = frame->addFrame("notification");
	notification->setSize(SDL_Rect{
		(Frame::virtualScreenX - 236 * 2) / 2,
		y,
		236 * 2,
		49 * 2
	});
	notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
	notification->addImage(notification->getActualSize(), 0xffffffff,
		"images/ui/Main Menus/Main/UI_MainMenu_EXNotification.png", "background");
	y += notification->getSize().h;
	y += 16;

	struct Option {
		const char* name;
		void (*callback)(Button&);
	};
#ifdef NINTENDO
	Option options[] = {
		{"PLAY GAME", mainPlayGame},
		{"HALL OF RECORDS", mainHallOfRecords},
		{"SETTINGS", mainSettings}
	};
#else
	Option options[] = {
		{"PLAY GAME", mainPlayGame},
		{"PLAY MODDED GAME", mainPlayModdedGame},
		{"HALL OF RECORDS", mainHallOfRecords},
		{"SETTINGS", mainSettings},
		{"QUIT", mainQuit}
	};
#endif

	const int num_options = sizeof(options) / sizeof(options[0]);
	
	buttons_height = y;

	auto buttons = frame->addFrame("buttons");
	buttons->setSize(SDL_Rect{0, y, Frame::virtualScreenX, 36 * num_options});
	buttons->setActualSize(SDL_Rect{0, 0, buttons->getSize().w, buttons->getSize().h});
	buttons->setHollow(true);
	buttons->setBorder(0);
	for (int c = 0; c < num_options; ++c) {
		auto button = buttons->addButton(options[c].name);
		button->setCallback(options[c].callback);
		button->setBorder(8);
		button->setHJustify(Button::justify_t::LEFT);
		button->setVJustify(Button::justify_t::CENTER);
		button->setText(options[c].name);
		button->setFont(smallfont_no_outline);
		button->setBackground("images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
		button->setColor(makeColor(255, 255, 255, 127));
		button->setHighlightColor(makeColor(255, 255, 255, 255));
		button->setTextColor(makeColor(180, 180, 180, 255));
		button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		button->setSize(SDL_Rect{
			(Frame::virtualScreenX - 164 * 2) / 2,
			y - buttons->getSize().y,
			164 * 2,
			16 * 2
		});
		int back = c - 1 < 0 ? num_options - 1 : c - 1;
		int forward = c + 1 >= num_options ? 0 : c + 1;
		button->setWidgetDown(options[forward].name);
		button->setWidgetUp(options[back].name);
		y += button->getSize().h;
		y += 4;
	}
	y += 16;

	auto play = buttons->findButton("PLAY GAME");
	if (play) {
		play->select();
	}

	frame->addImage(
		SDL_Rect{0, 0, 0, 0},
		0xffffffff,
		"images/ui/Main Menus/UI_Pointer_Spear00.png",
		"cursor"
	);

	for (int c = 0; c < 2; ++c) {
		std::string name = std::string("banner") + std::to_string(c + 1);
		auto banner = frame->addFrame(name.c_str());
		banner->setSize(SDL_Rect{
			(Frame::virtualScreenX - 472) / 2,
			y,
			472,
			76
		});
		banner->setActualSize(SDL_Rect{0, 0, banner->getSize().w, banner->getSize().h});
		std::string background = std::string("images/ui/Main Menus/Main/UI_MainMenu_EXBanner") + std::to_string(c + 1) + std::string(".png");
		banner->addImage(banner->getActualSize(), 0xffffffff, background.c_str());
		y += banner->getSize().h;
		y += 16;
	}

	auto copyright = frame->addField("copyright", 64);
	copyright->setFont(bigfont_outline);
	copyright->setText(u8"Copyright \u00A9 2021, Turning Wheel LLC");
	copyright->setJustify(Field::justify_t::CENTER);
	copyright->setSize(SDL_Rect{
		(Frame::virtualScreenX - 512) / 2,
		Frame::virtualScreenY - 50,
		512,
		50
	});
	copyright->setColor(0xffffffff);

	auto version = frame->addField("version", 32);
	version->setFont(smallfont_outline);
	version->setText(VERSION);
	version->setHJustify(Field::justify_t::RIGHT);
	version->setVJustify(Field::justify_t::BOTTOM);
	version->setSize(SDL_Rect{
		Frame::virtualScreenX - 200,
		Frame::virtualScreenY - 54,
		200,
		50
	});
	version->setColor(0xffffffff);

	int num_online_players = 1337;
	std::string online_players_text = std::string("Players online: ") + std::to_string(num_online_players);
	auto online_players = frame->addField("online_players", 32);
	online_players->setFont(smallfont_outline);
	online_players->setText(online_players_text.c_str());
	online_players->setHJustify(Field::justify_t::RIGHT);
	online_players->setVJustify(Field::justify_t::TOP);
	online_players->setSize(SDL_Rect{
		Frame::virtualScreenX - 200,
		4,
		200,
		50
	});
	online_players->setColor(0xffffffff);
}

void destroyMainMenu() {
	gui->remove("main_menu");
	main_menu_frame = nullptr;
}
