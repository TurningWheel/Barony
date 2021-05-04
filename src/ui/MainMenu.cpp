#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Slider.hpp"

#include "../draw.hpp"
#include "../engine/audio/sound.hpp"

enum FadeDestination {
	None = 0,
	RootMainMenu = 1,
	IntroStoryScreen = 2,
};

static Frame* main_menu_frame = nullptr;
static int buttons_height = 0;
static Uint32 menu_ticks = 0u;
static float main_menu_cursor_bob = 0.f;
static int main_menu_cursor_x = 0;
static int main_menu_cursor_y = 0;
static FadeDestination main_menu_fade_destination = FadeDestination::None;

static int story_text_pause = 0;
static int story_text_scroll = 0;
static int story_text_section = 0;
static bool story_text_end = false;

static std::string settings_tab;

static const char* bigfont_outline = "fonts/pixelmix.ttf#18#2";
static const char* bigfont_no_outline = "fonts/pixelmix.ttf#18#0";
static const char* smallfont_outline = "fonts/pixel_maz.ttf#32#2";
static const char* smallfont_no_outline = "fonts/pixel_maz.ttf#32#2";
static const char* menu_option_font = "fonts/pixel_maz.ttf#48#2";

static const char* intro_text =
	u8"Long ago, the bustling town of Hamlet was the envy of all its neighbors,\nfor it was the most thriving city in all the land.#"
	u8"Its prosperity was unmatched for generations until the evil Baron Herx came\nto power.#"
	u8"The Baron, in his endless greed, forced the people to dig the hills for gold,\nthough the ground had never given such treasure before.#"
	u8"Straining under the yoke of their master, the people planned his demise.\nThey tricked him with a promise of gold and sealed him within the mines.#"
	u8"Free of their cruel master, the people returned to their old way of life.\nBut disasters shortly began to befall the village.#"
	u8"Monsters and other evils erupted from the ground, transforming the village\ninto a ghost town.#"
	u8"Many adventurers have descended into the mines to break the Baron's curse,\nbut none have returned.#"
	u8"The town of Hamlet cries for redemption, and only a hero can save it\nfrom its curse...";

static void storyScreen() {
	main_menu_frame->addImage(
		main_menu_frame->getSize(),
		0xffffffff,
		"images/ui/Main Menus/Story/intro1.png",
		"backdrop"
	);

	story_text_pause = 0;
	story_text_scroll = 0;
	story_text_section = 0;
	story_text_end = false;

	auto back_button = main_menu_frame->addButton("back");
	back_button->setText("Skip story  ");
	back_button->setColor(makeColor(0, 0, 0, 0));
	back_button->setHighlightColor(makeColor(0, 0, 0, 0));
	back_button->setBorderColor(makeColor(0, 0, 0, 0));
	back_button->setTextColor(0xffffffff);
	back_button->setTextHighlightColor(0xffffffff);
	back_button->setFont(smallfont_outline);
	back_button->setJustify(Button::justify_t::RIGHT);
	back_button->setSize(SDL_Rect{Frame::virtualScreenX - 400, Frame::virtualScreenY - 50, 400, 50});
	back_button->setCallback([](Button& b){
		fadeout = true;
		main_menu_fade_destination = FadeDestination::RootMainMenu;
		});
	back_button->setWidgetBack("back");
	back_button->select();

	auto font = Font::get(bigfont_outline); assert(font);

	auto textbox1 = main_menu_frame->addFrame("story_text_box");
	textbox1->setSize(SDL_Rect{100, Frame::virtualScreenY - font->height() * 4, Frame::virtualScreenX - 200, font->height() * 3});
	textbox1->setActualSize(SDL_Rect{0, 0, textbox1->getSize().w, textbox1->getSize().h});
	textbox1->setColor(makeColor(0, 0, 0, 127));
	textbox1->setBorder(0);

	auto textbox2 = textbox1->addFrame("story_text_box");
	textbox2->setScrollBarsEnabled(false);
	textbox2->setSize(SDL_Rect{0, font->height() / 2, Frame::virtualScreenX - 200, font->height() * 2});
	textbox2->setActualSize(SDL_Rect{0, 0, textbox2->getSize().w, font->height() * 16});
	textbox2->setHollow(true);
	textbox2->setBorder(0);

	auto field = textbox2->addField("text", 1024);
	field->setFont(bigfont_outline);
	field->setSize(textbox2->getActualSize());
	field->setHJustify(Field::justify_t::CENTER);
	field->setVJustify(Field::justify_t::TOP);
	field->setColor(makeColor(255, 255, 255, 255));

	textbox1->setTickCallback([](Widget& widget){
		auto textbox1 = static_cast<Frame*>(&widget);
		auto story_font = Font::get(bigfont_outline); assert(story_font);
		if (story_text_scroll > 0) {
			auto textbox2 = textbox1->findFrame("story_text_box");
			assert(textbox2);
			auto size = textbox2->getActualSize();
			++size.y;
			textbox2->setActualSize(size);
			--story_text_scroll;
			if (story_text_section % 2 == 0) {
				auto backdrop = main_menu_frame->findImage("backdrop");
				if (backdrop) {
					Uint8 c = 255 * (fabs(story_text_scroll - story_font->height()) / story_font->height());
					backdrop->color = makeColor(c, c, c, 255);
					if (c == 0) {
						char c = backdrop->path[backdrop->path.size() - 5];
						backdrop->path[backdrop->path.size() - 5] = c + 1;
					}
				}
			}
		} else {
			if (story_text_pause > 0) {
				--story_text_pause;
				if (story_text_pause == 0) {
					if (story_text_end == true) {
						fadeout = true;
						main_menu_fade_destination = FadeDestination::RootMainMenu;
					} else {
						story_text_scroll = story_font->height() * 2;
					}
				}
			} else {
				if (menu_ticks % 2 == 0) {
					auto textbox2 = textbox1->findFrame("story_text_box");
					assert(textbox2);
					auto text = textbox2->findField("text");
					assert(text);
					size_t len = strlen(text->getText());
					if (len < strlen(intro_text)) {
						char buf[1024] = { '\0' };
						strcpy(buf, text->getText());
						char c = intro_text[len];
						if (c == '#') {
							++story_text_section;
							story_text_pause = TICKS_PER_SECOND * 5;
							c = '\n';
						}
						buf[len] = c;
						buf[len + 1] = '\0';
						text->setText(buf);
					} else {
						story_text_pause = TICKS_PER_SECOND * 5;
						story_text_end = true;
					}
				}
			}
		}
	});
}

static void updateMenuCursor(Widget& widget) {
	Frame* buttons = static_cast<Frame*>(&widget);
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
	auto cursor = main_menu_frame->findImage("cursor");
	if (cursor) {
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

/******************************************************************************/

static int settingsAddSubHeader(Frame& frame, int y, const char* name, const char* text) {
	std::string fullname = std::string("subheader_") + name;
	auto image = frame.addImage(
		SDL_Rect{0, y, 1080, 42},
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_SubHeading_Backing00.png",
		(fullname + "_image").c_str()
	);
	auto field = frame.addField((fullname + "_field").c_str(), 128);
	field->setSize(image->pos);
	field->setFont(bigfont_outline);
	field->setText(text);
	field->setJustify(Field::justify_t::CENTER);
	return image->pos.h + 6;
}

static int settingsAddOption(Frame& frame, int y, const char* name, const char* text) {
	std::string fullname = std::string("setting_") + name;
	auto image = frame.addImage(
		SDL_Rect{0, y, 382, 52},
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_Left_Backing00.png",
		(fullname + "_image").c_str()
	);
	auto field = frame.addField((fullname + "_field").c_str(), 128);
	auto size = image->pos; size.x += 12; size.w -= 12;
	field->setSize(size);
	field->setFont(bigfont_outline);
	field->setText(text);
	field->setHJustify(Field::justify_t::LEFT);
	field->setVJustify(Field::justify_t::CENTER);
	return size.h + 10;
}

static int settingsAddBooleanOption(
	Frame& frame,
	int y,
	const char* name,
	const char* text,
	bool on,
	void (*callback)(Button&))
{
	std::string fullname = std::string("setting_") + name;
	int result = settingsAddOption(frame, y, name, text);
	auto button = frame.addButton((fullname + "_button").c_str());
	button->setSize(SDL_Rect{
		390,
		y + 2,
		158,
		48});
	button->setFont(smallfont_outline);
	button->setText("Off      On");
	button->setJustify(Button::justify_t::CENTER);
	button->setCallback(callback);
	button->setPressed(on);
	button->setBackground("images/ui/Main Menus/Settings/Settings_SwitchOff00.png");
	button->setBackgroundActivated("images/ui/Main Menus/Settings/Settings_SwitchOn00.png");
	button->setStyle(Button::style_t::STYLE_TOGGLE);
	button->setHighlightColor(makeColor(255,255,255,255));
	button->setColor(makeColor(127,127,127,255));
	button->setTextHighlightColor(makeColor(255,255,255,255));
	button->setTextColor(makeColor(127,127,127,255));
	return result;
}

static int settingsAddBooleanWithCustomizeOption(
	Frame& frame,
	int y,
	const char* name,
	const char* text,
	bool on,
	void (*callback)(Button&),
	void (*customize_callback)(Button&))
{
	std::string fullname = std::string("setting_") + name;
	int result = settingsAddBooleanOption(frame, y, name, text, on, callback);
	auto button = frame.addButton((fullname + "_customize_button").c_str());
	button->setSize(SDL_Rect{
		574,
		y + 4,
		158,
		44});
	button->setFont(smallfont_outline);
	button->setText("Customize");
	button->setJustify(Button::justify_t::CENTER);
	button->setCallback(customize_callback);
	button->setBackground("images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
	button->setHighlightColor(makeColor(255,255,255,255));
	button->setColor(makeColor(127,127,127,255));
	button->setTextHighlightColor(makeColor(255,255,255,255));
	button->setTextColor(makeColor(127,127,127,255));
	return result;
}

static int settingsAddCustomize(
	Frame& frame,
	int y,
	const char* name,
	const char* text,
	void (*callback)(Button&))
{
	std::string fullname = std::string("setting_") + name;
	int result = settingsAddOption(frame, y, name, text);
	auto button = frame.addButton((fullname + "_customize_button").c_str());
	button->setSize(SDL_Rect{
		390,
		y + 4,
		158,
		44});
	button->setFont(smallfont_outline);
	button->setText("Customize");
	button->setJustify(Button::justify_t::CENTER);
	button->setCallback(callback);
	button->setBackground("images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
	button->setHighlightColor(makeColor(255,255,255,255));
	button->setColor(makeColor(127,127,127,255));
	button->setTextHighlightColor(makeColor(255,255,255,255));
	button->setTextColor(makeColor(127,127,127,255));
	return result;
}

static int settingsAddDropdown(
	Frame& frame,
	int y,
	const char* name,
	const char* text,
	const std::vector<const char*>& items,
	void (*callback)(Button&))
{
	std::string fullname = std::string("setting_") + name;
	int result = settingsAddOption(frame, y, name, text);
	auto button = frame.addButton((fullname + "_dropdown").c_str());
	button->setSize(SDL_Rect{
		390,
		y + 4,
		158,
		44});
	button->setFont(smallfont_outline);
	button->setText(items[0]);
	button->setJustify(Button::justify_t::CENTER);
	button->setCallback(callback);
	button->setBackground("images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
	button->setHighlightColor(makeColor(255,255,255,255));
	button->setColor(makeColor(127,127,127,255));
	button->setTextHighlightColor(makeColor(255,255,255,255));
	button->setTextColor(makeColor(127,127,127,255));
	return result;
}

static int settingsAddSlider(
	Frame& frame,
	int y,
	const char* name,
	const char* text,
	float value,
	float minValue,
	float maxValue,
	void (*callback)(Slider&))
{
	std::string fullname = std::string("setting_") + name;
	int result = settingsAddOption(frame, y, name, text);
	auto box = frame.addImage(
		SDL_Rect{402, y + 4, 132, 44},
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_Value_Backing00.png",
		(fullname + "_box").c_str()
	);
	auto field = frame.addField((fullname + "_text").c_str(), 8);
	field->setSize(box->pos);
	field->setJustify(Field::justify_t::CENTER);
	field->setFont(smallfont_outline);
	auto slider = frame.addSlider((fullname + "_slider").c_str());
	slider->setMinValue(minValue);
	slider->setMaxValue(maxValue);
	slider->setValue(value);
	slider->setRailSize(SDL_Rect{field->getSize().x + field->getSize().w + 32, y + 14, 450, 24});
	slider->setHandleSize(SDL_Rect{0, 0, 52, 42});
	slider->setCallback(callback);
	slider->setHandleImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Slide00.png");
	slider->setRailImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Backing00.png");
	if (callback) {
		(*callback)(*slider);
	}
	return result;
}

static Frame* settingsSubwindowSetup(Button& button) {
	if (settings_tab == button.getName()) {
		return nullptr;
	}
	playSound(139, 64); // click sound
	settings_tab = button.getName();

	assert(main_menu_frame);
	auto settings = main_menu_frame->findFrame("settings"); assert(settings);
	auto settings_subwindow = settings->findFrame("settings_subwindow");
	if (settings_subwindow) {
		settings_subwindow->removeSelf();
	}
	settings_subwindow = settings->addFrame("settings_subwindow");
	settings_subwindow->setSize(SDL_Rect{8 * 2, 71 * 2, 547 * 2, 224 * 2});
	settings_subwindow->setActualSize(SDL_Rect{0, 0, 547 * 2, 224 * 2});
	settings_subwindow->setHollow(true);
	settings_subwindow->setBorder(0);
	auto rock_background = settings_subwindow->addImage(
		settings_subwindow->getActualSize(),
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_BGTile00.png",
		"background"
	);
	rock_background->tiled = true;
	return settings_subwindow;
}

static void settingsSubwindowFinalize(Frame& frame, int y) {
	frame.setActualSize(SDL_Rect{0, 0, 547 * 2, std::max(224 * 2, y)});
	auto rock_background = frame.findImage("background"); assert(rock_background);
	rock_background->pos = frame.getActualSize();
}

void settingsUI(Button& button) {
	Frame* settings_subwindow;
	if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
		return;
	}

	int y = 0;

	y += settingsAddSubHeader(*settings_subwindow, y, "inventory", "Inventory Options");
	y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "add_items_to_hotbar", "Add Items to Hotbar", false, nullptr, nullptr);
	y += settingsAddCustomize(*settings_subwindow, y, "inventory_sorting", "Inventory Sorting", nullptr);
#ifndef NINTENDO
	y += settingsAddBooleanOption(*settings_subwindow, y, "use_on_release", "Use on Release", true, nullptr);
#endif

	y += settingsAddSubHeader(*settings_subwindow, y, "hud", "HUD Options");
	y += settingsAddCustomize(*settings_subwindow, y, "minimap_settings", "Minimap Settings", nullptr);
	y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "show_messages", "Show Messages", true, nullptr, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "show_player_nametags", "Show Player Nametags", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "show_hud", "Show HUD", true, nullptr);
#ifndef NINTENDO
	y += settingsAddBooleanOption(*settings_subwindow, y, "show_ip_address", "Show IP Address", true, nullptr);
#endif

	settingsSubwindowFinalize(*settings_subwindow, y);
}

void settingsVideo(Button& button) {
	Frame* settings_subwindow;
	if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
		return;
	}

	int y = 0;

	y += settingsAddSubHeader(*settings_subwindow, y, "accessibility", "Accessibility");
	y += settingsAddBooleanOption(*settings_subwindow, y, "content_control", "Content Control", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "colorblind_mode", "Colorblind Mode", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "arachnophobia_filter", "Arachnophobia Filter", true, nullptr);

	y += settingsAddSubHeader(*settings_subwindow, y, "effects", "Effects");
	y += settingsAddBooleanOption(*settings_subwindow, y, "shaking", "Shaking", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "bobbing", "Bobbing", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "light_flicker", "Light Flicker", true, nullptr);

	y += settingsAddSubHeader(*settings_subwindow, y, "display", "Display");
#ifndef NINTENDO
	y += settingsAddDropdown(*settings_subwindow, y, "resolution", "Resolution", {"1280 x 720", "1920 x 1080"}, nullptr);
	y += settingsAddDropdown(*settings_subwindow, y, "window_mode", "Window Mode", {"Fullscreen", "Borderless", "Windowed"}, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "vsync", "Vertical Sync", true, nullptr);
#endif
	y += settingsAddBooleanOption(*settings_subwindow, y, "splitmode", "Vertical Splitscreen", false, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "gamma", "Gamma", 100, 50, 200, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "fov", "Field of View", 65, 40, 100, nullptr);
#ifndef NINTENDO
	y += settingsAddSlider(*settings_subwindow, y, "fps", "FPS limit", 30, 60, 300, nullptr);
#endif

	settingsSubwindowFinalize(*settings_subwindow, y);
}

void settingsAudio(Button& button) {
	Frame* settings_subwindow;
	if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
		return;
	}
	int y = 0;

	y += settingsAddSubHeader(*settings_subwindow, y, "volume", "Volume");
	y += settingsAddSlider(*settings_subwindow, y, "master_volume", "Master Volume", 50, 0, 100, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "gameplay_volume", "Gameplay Volume", 50, 0, 100, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "ambient_volume", "Ambient Volume", 50, 0, 100, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "environment_volume", "Environment Volume", 50, 0, 100, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "music_volume", "Music Volume", 50, 0, 100, nullptr);

	y += settingsAddSubHeader(*settings_subwindow, y, "options", "Options");
	y += settingsAddBooleanOption(*settings_subwindow, y, "minimap_pings", "Minimap Pings", true, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "player_monster_sounds", "Player Monster Sounds", true, nullptr);
#ifndef NINTENDO
	y += settingsAddBooleanOption(*settings_subwindow, y, "out_of_focus_audio", "Out-of-Focus Audio", true, nullptr);
#endif

	settingsSubwindowFinalize(*settings_subwindow, y);
}

void settingsControls(Button& button) {
	Frame* settings_subwindow;
	if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
		return;
	}

	int y = 0;

#ifndef NINTENDO
	y += settingsAddSubHeader(*settings_subwindow, y, "mouse_and_keyboard", "Mouse & Keyboard");
	y += settingsAddCustomize(*settings_subwindow, y, "mouse_and_key_bindings", "Bindings", nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "numkeys_in_inventory", "Number Keys in Inventory", true, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "mousespeed", "Mouse Sensitivity", 20, 0, 100, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "reverse_mouse", "Reverse Mouse", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "smooth_mouse", "Smooth Mouse", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "rotation_speed_limit", "Rotation Speed Limit", true, nullptr);
#endif

	y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Controller Settings");
	y += settingsAddCustomize(*settings_subwindow, y, "controller_bindings", "Bindings", nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_x", "Turn Sensitivity X", 20, 0, 100, nullptr);
	y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_y", "Turn Sensitivity Y", 20, 0, 100, nullptr);

	settingsSubwindowFinalize(*settings_subwindow, y);
}

void settingsGame(Button& button) {
	Frame* settings_subwindow;
	if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
		return;
	}

	int y = 0;

	y += settingsAddSubHeader(*settings_subwindow, y, "game", "Game Settings");
	y += settingsAddBooleanOption(*settings_subwindow, y, "classic_mode", "Classic Mode", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "hardcore_mode", "Hardcore Mode", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "friendly_fire", "Friendly Fire", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "keep_inventory", "Keep Inventory on Death", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "hunger", "Hunger", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "minotaur", "Minotaur", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "random_traps", "Random Traps", false, nullptr);
	y += settingsAddBooleanOption(*settings_subwindow, y, "extra_life", "Extra Life", false, nullptr);
#ifndef NINTENDO
	y += settingsAddBooleanOption(*settings_subwindow, y, "cheats", "Cheats", false, nullptr);
#endif

	settingsSubwindowFinalize(*settings_subwindow, y);
}

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

	destroyMainMenu();
	main_menu_frame = gui->addFrame("main_menu");
	main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
	main_menu_frame->setHollow(true);
	main_menu_frame->setBorder(0);
	main_menu_frame->setTickCallback([](Widget&){++menu_ticks;});

	fadeout = true;
	main_menu_fade_destination = FadeDestination::IntroStoryScreen;
}

void recordsCredits(Button& button) {
	playSound(139, 64); // click sound

	destroyMainMenu();
	main_menu_frame = gui->addFrame("main_menu");
	main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
	main_menu_frame->setHollow(true);
	main_menu_frame->setBorder(0);
	main_menu_frame->setTickCallback([](Widget&){++menu_ticks;});

	auto back_button = main_menu_frame->addButton("back");
	back_button->setText("Return to Main Menu  ");
	back_button->setColor(makeColor(0, 0, 0, 0));
	back_button->setHighlightColor(makeColor(0, 0, 0, 0));
	back_button->setBorderColor(makeColor(0, 0, 0, 0));
	back_button->setTextColor(0xffffffff);
	back_button->setTextHighlightColor(0xffffffff);
	back_button->setFont(smallfont_outline);
	back_button->setJustify(Button::justify_t::RIGHT);
	back_button->setSize(SDL_Rect{Frame::virtualScreenX - 400, Frame::virtualScreenY - 50, 400, 50});
	back_button->setCallback([](Button& b){
		destroyMainMenu();
		createMainMenu();
		mainHallOfRecords(b);
		auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
		auto credits = buttons->findButton("CREDITS"); assert(credits);
		credits->select();
	});
	back_button->setWidgetBack("back");
	back_button->select();

	auto font = Font::get(bigfont_outline); assert(font);
		
	auto credits = main_menu_frame->addFrame("credits");
	credits->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	credits->setActualSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY + font->height() * 80});
	credits->setHollow(true);
	credits->setBorder(0);
	credits->setTickCallback([](Widget& widget){
		auto credits = static_cast<Frame*>(&widget);
		auto size = credits->getActualSize();
		size.y += 1;
		if (size.y >= size.h) {
			size.y = 0;
		}
		credits->setActualSize(size);
		});

	// titles
	auto text1 = credits->addField("text1", 1024);
	text1->setFont(bigfont_outline);
	text1->setColor(makeColor(255, 191, 32, 255));
	text1->setHJustify(Field::justify_t::CENTER);
	text1->setVJustify(Field::justify_t::TOP);
	text1->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 80});
	text1->setText(
		u8"Project lead, programming, and design\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Music and sound design\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Programming\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Art and design\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Programming and design\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Additional art\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Additional writing\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Special thanks\n"
		u8" \n"
		u8" \n"
		u8" \n"
		u8" \n"
		u8" \n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"A big shout-out to our open-source community!\n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"Barony is a product of Turning Wheel LLC\n"
		u8" \n"
		u8" \n"
		u8" \n \n \n \n \n"
		u8"This game is dedicated to all of our friends, family, and fans\n"
		u8"who encouraged and supported us on our journey to finish it.\n"
		u8" \n"
		u8" \n"
	);

	// entries
	auto text2 = credits->addField("text2", 1024);
	text2->setFont(bigfont_outline);
	text2->setColor(0xffffffff);
	text2->setHJustify(Field::justify_t::CENTER);
	text2->setVJustify(Field::justify_t::TOP);
	text2->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 80});
	text2->setText(
		u8" \n"
		u8"Sheridan Rathbun\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Chris Kukla\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Ciprian Elies\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Josiah Colborn\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Benjamin Potter\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Matthew Griebner\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Frasier Panton\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Our Kickstarter Backers\n"
		u8"Sterling Rathbun\n"
		u8"Kevin White\n"
		u8"Jesse Riddle\n"
		u8"Julian Seeger\n"
		u8"Mathias Golinelli\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Learn more at http://www.github.com/TurningWheel/Barony\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8"Copyright \u00A9 2021, all rights reserved\n"
		u8"http://www.baronygame.com/\n"
		u8" \n \n \n \n \n"
		u8" \n"
		u8" \n"
		u8" \n"
		u8"Thank you!\n"
	);
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
	buttons->setTickCallback(updateMenuCursor);
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
		button->setFont(menu_option_font);
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
	banner_text->setFont(menu_option_font);
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
		{"CREDITS", recordsCredits},
		{"BACK TO MAIN MENU", recordsBackToMainMenu}
	};
	const int num_options = sizeof(options) / sizeof(options[0]);

	int y = buttons_height;

	auto buttons = main_menu_frame->addFrame("buttons");
	buttons->setTickCallback(updateMenuCursor);
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
		button->setFont(menu_option_font);
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

	settings_tab = "";

	auto settings = main_menu_frame->addFrame("settings");
	settings->setSize(SDL_Rect{(Frame::virtualScreenX - 1126) / 2, (Frame::virtualScreenY - 718) / 2, 1126, 718});
	settings->setActualSize(SDL_Rect{0, 0, settings->getSize().w, settings->getSize().h});
	settings->setHollow(true);
	settings->setBorder(0);
	settings->addImage(
		SDL_Rect{
			(settings->getActualSize().w - 553 * 2) / 2,
			0,
			553 * 2,
			357 * 2
		},
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_Window01.png",
		"background"
	);
	auto timber = settings->addImage(
		SDL_Rect{0, 66 * 2, 1126, 586},
		0xffffffff,
		"images/ui/Main Menus/Settings/Settings_TimberEdge00.png",
		"timber"
	);
	timber->ontop = true;

	settings->setTickCallback([](Widget& widget){
		auto settings = static_cast<Frame*>(&widget);
		const char* tabs[] = {
			"UI",
			"Video",
			"Audio",
			"Controls"
		};
		for (auto name : tabs) {
			auto button = settings->findButton(name);
			if (button) {
				if (name == settings_tab) {
					button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
				} else {
					button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
				}
			}
		}
		});

	static const char* pixel_maz_outline = "fonts/pixel_maz.ttf#46#2";

	auto window_title = settings->addField("window_title", 64);
	window_title->setFont(pixel_maz_outline);
	window_title->setSize(SDL_Rect{394, 26, 338, 24});
	window_title->setJustify(Field::justify_t::CENTER);
	window_title->setText("SETTINGS");

	auto tab_left = settings->addButton("tab_left");
	tab_left->setBackground("images/ui/Main Menus/Settings/Settings_Button_L00.png");
	tab_left->setSize(SDL_Rect{32, 68, 38, 58});
	tab_left->setColor(makeColor(255, 255, 255, 191));
	tab_left->setHighlightColor(makeColor(255, 255, 255, 255));
	tab_left->setWidgetBack("discard_and_exit");
	tab_left->setWidgetPageLeft("tab_left");
	tab_left->setWidgetPageRight("tab_right");
	tab_left->setWidgetRight("UI");
	tab_left->setWidgetDown("restore_defaults");

	auto tab_right = settings->addButton("tab_right");
	tab_right->setBackground("images/ui/Main Menus/Settings/Settings_Button_R00.png");
	tab_right->setSize(SDL_Rect{1056, 68, 38, 58});
	tab_right->setColor(makeColor(255, 255, 255, 191));
	tab_right->setHighlightColor(makeColor(255, 255, 255, 255));
	tab_right->setWidgetBack("discard_and_exit");
	tab_right->setWidgetPageLeft("tab_left");
	tab_right->setWidgetPageRight("tab_right");
	tab_right->setWidgetLeft("Controls");
	tab_right->setWidgetDown("confirm_and_exit");

	struct Option {
		const char* name;
		void (*callback)(Button&);
	};
	Option tabs[] = {
		{"UI", settingsUI},
		{"Video", settingsVideo},
		{"Audio", settingsAudio},
		{"Controls", settingsControls}
	};
	int num_options = sizeof(tabs) / sizeof(tabs[0]);
	for (int c = 0; c < num_options; ++c) {
		auto button = settings->addButton(tabs[c].name);
		button->setCallback(tabs[c].callback);
		button->setText(tabs[c].name);
		button->setFont(pixel_maz_outline);
		button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
		button->setBackgroundActivated("images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
		button->setSize(SDL_Rect{76 + (272 - 76) * c, 64, 184, 64});
		button->setColor(makeColor(255, 255, 255, 191));
		button->setHighlightColor(makeColor(255, 255, 255, 255));
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		if (c > 0) {
			button->setWidgetLeft(tabs[c - 1].name);
		} else {
			button->setWidgetLeft("tab_left");
		}
		if (c < num_options - 1) {
			button->setWidgetRight(tabs[c + 1].name);
		} else {
			button->setWidgetRight("tab_right");
		}
		button->setWidgetBack("discard_and_exit");
		if (c <= num_options / 2) {
			button->setWidgetDown("restore_defaults");
		} else if (c == num_options - 2) {
			button->setWidgetDown("discard_and_exit");
		} else if (c == num_options - 1) {
			button->setWidgetDown("confirm_and_exit");
		}
	}
	auto first_tab = settings->findButton(tabs[0].name);
	if (first_tab) {
		first_tab->select();
		first_tab->activate();
	}

	auto tooltip = settings->addField("tooltip", 256);
	tooltip->setSize(SDL_Rect{92, 590, 948, 32});
	tooltip->setFont(smallfont_no_outline);
	tooltip->setText("Blah blah blah this is what the option does. Fe fi fo fum.");

	auto restore_defaults = settings->addButton("restore_defaults");
	restore_defaults->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
	restore_defaults->setSize(SDL_Rect{84, 630, 164, 62});
	restore_defaults->setText("Restore\nDefaults");
	restore_defaults->setJustify(Button::justify_t::CENTER);
	restore_defaults->setFont(smallfont_outline);
	restore_defaults->setColor(makeColor(255, 255, 255, 191));
	restore_defaults->setHighlightColor(makeColor(255, 255, 255, 255));
	restore_defaults->setWidgetBack("discard_and_exit");
	restore_defaults->setWidgetPageLeft("tab_left");
	restore_defaults->setWidgetPageRight("tab_right");
	restore_defaults->setWidgetUp("UI");
	restore_defaults->setWidgetRight("discard_and_exit");

	auto discard_and_exit = settings->addButton("discard_and_exit");
	discard_and_exit->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
	discard_and_exit->setSize(SDL_Rect{700, 630, 164, 62});
	discard_and_exit->setText("Discard\n& Exit");
	discard_and_exit->setJustify(Button::justify_t::CENTER);
	discard_and_exit->setFont(smallfont_outline);
	discard_and_exit->setColor(makeColor(255, 255, 255, 191));
	discard_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
	discard_and_exit->setCallback([](Button&){
		assert(main_menu_frame);
		auto settings = main_menu_frame->findFrame("settings");
		if (settings) {
			settings->removeSelf();
		}
		auto buttons = main_menu_frame->findFrame("buttons");
		if (buttons) {
			auto settings_button = buttons->findButton("SETTINGS");
			if (settings_button) {
				settings_button->select();
			}
		}
	});
	discard_and_exit->setWidgetBack("discard_and_exit");
	discard_and_exit->setWidgetPageLeft("tab_left");
	discard_and_exit->setWidgetPageRight("tab_right");
	discard_and_exit->setWidgetUp("Controls");
	discard_and_exit->setWidgetLeft("restore_defaults");
	discard_and_exit->setWidgetRight("confirm_and_exit");

	auto confirm_and_exit = settings->addButton("confirm_and_exit");
	confirm_and_exit->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
	confirm_and_exit->setSize(SDL_Rect{880, 630, 164, 62});
	confirm_and_exit->setText("Confirm\n& Exit");
	confirm_and_exit->setJustify(Button::justify_t::CENTER);
	confirm_and_exit->setFont(smallfont_outline);
	confirm_and_exit->setColor(makeColor(255, 255, 255, 191));
	confirm_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
	confirm_and_exit->setCallback([](Button&){
		assert(main_menu_frame);
		auto settings = main_menu_frame->findFrame("settings");
		if (settings) {
			settings->removeSelf();
		}
		auto buttons = main_menu_frame->findFrame("buttons");
		if (buttons) {
			auto settings_button = buttons->findButton("SETTINGS");
			if (settings_button) {
				settings_button->select();
			}
		}
	});
	confirm_and_exit->setWidgetBack("discard_and_exit");
	confirm_and_exit->setWidgetPageLeft("tab_left");
	confirm_and_exit->setWidgetPageRight("tab_right");
	confirm_and_exit->setWidgetUp("tab_right");
	confirm_and_exit->setWidgetLeft("discard_and_exit");
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

	if (main_menu_fade_destination) {
		if (fadeout && fadealpha >= 255) {
			if (main_menu_fade_destination == FadeDestination::RootMainMenu) {
				destroyMainMenu();
				createMainMenu();
			}
			if (main_menu_fade_destination == FadeDestination::IntroStoryScreen) {
				storyScreen();
			}
			fadeout = false;
			main_menu_fade_destination = FadeDestination::None;
		}
	}
}

void createMainMenu() {
	main_menu_frame = gui->addFrame("main_menu");

	auto frame = main_menu_frame;
	frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	frame->setActualSize(SDL_Rect{0, 0, frame->getSize().w, frame->getSize().h});
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setTickCallback([](Widget&){++menu_ticks;});

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
	buttons->setTickCallback(updateMenuCursor);
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
		button->setFont(menu_option_font);
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
		if (main_menu_cursor_x == 0 && main_menu_cursor_y == 0) {
			main_menu_cursor_x = play->getSize().x - 80;
			main_menu_cursor_y = play->getSize().y - 9 + buttons->getSize().y;
		}
	}

	frame->addImage(
		SDL_Rect{
			main_menu_cursor_x + (int)(sinf(main_menu_cursor_bob) * 16.f) - 16,
			main_menu_cursor_y,
			37 * 2,
			23 * 2
		},
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
	main_menu_frame->removeSelf();
	main_menu_frame = nullptr;
}
