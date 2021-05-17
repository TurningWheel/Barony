#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../draw.hpp"

static Frame* main_menu_frame = nullptr;

void doMainMenu() {
	if (!main_menu_frame) {
		createMainMenu();
	}
}

void createMainMenu() {
	main_menu_frame = gui->addFrame("main_menu");

	static const char* bigfont_outline = "fonts/pixelmix.ttf#18#2";
	static const char* bigfont_no_outline = "fonts/pixelmix.ttf#18#0";
	static const char* smallfont_outline = "fonts/pixel_maz.ttf#32#2";
	static const char* smallfont_no_outline = "fonts/pixel_maz.ttf#32#2";

	auto frame = main_menu_frame;
	frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	frame->setActualSize(SDL_Rect{0, 0, frame->getSize().w, frame->getSize().h});
	frame->setHollow(true);
	frame->setBorder(0);

	int y = 16;

	auto title_img = Image::get("images/system/title.png");
	auto title = frame->addImage(
		SDL_Rect{
			(Frame::virtualScreenX - (int)title_img->getWidth()) / 2,
			y,
			(int)title_img->getWidth(),
			(int)title_img->getHeight()
		},
		makeColor(255, 255, 255, 255),
		title_img->getName(),
		"title"
	);
	y += title->pos.h;

	auto notification = frame->addFrame("notification");
	notification->setSize(SDL_Rect{
		(Frame::virtualScreenX - 256) / 2,
		y,
		256,
		64
	});
	notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
	y += notification->getSize().h;
	y += 16;

#ifdef NINTENDO
	const char* options[] = {
		"Play Game",
		"Hall of Records",
		"Settings",
		"Extras"
	};
#else
	const char* options[] = {
		"Play Game",
		"Play Modded Game",
		"Hall of Records",
		"Settings",
		"Extras"
		"Quit"
	};
#endif

	/*auto option_cursor = frame->addImage(
		SDL_Rect{
		},
		0xffffffff,
		""
	);*/

	int num_options = sizeof(options) / sizeof(options[0]);
	for (int c = 0; c < num_options; ++c) {
		auto button = frame->addButton(options[c]);
		button->setText(options[c]);
		button->setFont(smallfont_no_outline);
		button->setTextColor(0xffffffff);
		button->setHighlightColor(makeColor(255, 255, 32, 255));
		button->setSize(SDL_Rect{
			(Frame::virtualScreenX - 164 * 2) / 2,
			y,
			164 * 2,
			16 * 2
		});
		button->setBackground("images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
		int back = c - 1 < 0 ? num_options - 1 : c - 1;
		int forward = c + 1 >= num_options ? 0 : c + 1;
		button->setWidgetDown(options[forward]);
		button->setWidgetUp(options[back]);
		y += button->getSize().h;
		y += 4;
	}
	y += 16;

	auto play = frame->findButton("Play Game");
	if (play) {
		play->select();
	}

	for (int c = 0; c < 2; ++c) {
		std::string name = std::string("banner") + std::to_string(c + 1);
		auto banner = frame->addFrame(name.c_str());
		banner->setSize(SDL_Rect{
			(Frame::virtualScreenX - 440) / 2,
			y,
			256,
			64
		});
		banner->setActualSize(SDL_Rect{0, 0, banner->getSize().w, banner->getSize().h});
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
