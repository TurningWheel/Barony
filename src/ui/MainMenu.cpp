#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Slider.hpp"
#include "Text.hpp"

#include "../net.hpp"
#include "../player.hpp"
#include "../stat.hpp"
#include "../menu.hpp"
#include "../scores.hpp"
#include "../mod_tools.hpp"
#include "../interface/interface.hpp"
#include "../draw.hpp"
#include "../engine/audio/sound.hpp"

#include <cassert>
#include <functional>

namespace MainMenu {
	// ALL NEW menu options:
	bool arachnophobia_filter = false;
	bool vertical_splitscreen = false;
	float master_volume = 100.f;

	enum class FadeDestination : Uint8 {
		None = 0,
		RootMainMenu = 1,
		IntroStoryScreen = 2,
		HallOfTrials = 3,
		GameStart = 4,
	};

	static Frame* main_menu_frame = nullptr;
	static int main_menu_buttons_height = 0;
	static Uint32 main_menu_ticks = 0u;
	static float main_menu_cursor_bob = 0.f;
	static int main_menu_cursor_x = 0;
	static int main_menu_cursor_y = 0;
	static FadeDestination main_menu_fade_destination = FadeDestination::None;

	static const char* bigfont_outline = "fonts/pixelmix.ttf#18#2";
	static const char* bigfont_no_outline = "fonts/pixelmix.ttf#18#0";
	static const char* smallfont_outline = "fonts/pixel_maz.ttf#14#2";
	static const char* smallfont_no_outline = "fonts/pixel_maz.ttf#14#2";
	static const char* menu_option_font = "fonts/pixel_maz.ttf#24#2";

	static inline void soundToggleMenu() {
		playSound(500, 48);
	}

	static inline void soundMove() {
		playSound(495, 48);
	}

	static inline void soundActivate() {
		playSound(493, 48);
	}

	static inline void soundCancel() {
		playSound(499, 48);
	}

	static inline void soundToggle() {
		playSound(492, 48);
	}

	static inline void soundCheckmark() {
		playSound(494, 48);
	}

	static inline void soundSlider() {
#ifdef NINTENDO
		playSound(497, 48);
#endif
	}

	static inline void soundWarning() {
		playSound(496, 48);
	}

	static inline void soundError() {
		playSound(498, 48);
	}

/******************************************************************************/

	static void updateMenuCursor(Widget& widget) {
		Frame* buttons = static_cast<Frame*>(&widget);
		bool buttonSelected = false;
		for (auto button : buttons->getButtons()) {
			if (button->isSelected()) {
				main_menu_cursor_x = button->getSize().x - 80;
				main_menu_cursor_y = button->getSize().y - 9 + buttons->getSize().y;
				buttonSelected = true;
				break;
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
			cursor->disabled = !buttonSelected;
			int diff = main_menu_cursor_y - cursor->pos.y;
			if (diff > 0) {
				diff = std::max(1, diff / 2);
			} else if (diff < 0) {
				diff = std::min(-1, diff / 2);
			}
			cursor->pos = SDL_Rect{
				main_menu_cursor_x + (int)(sinf(main_menu_cursor_bob) * 16.f) - 16,
				diff + cursor->pos.y,
				37 * 2,
				23 * 2
			};
		}
	}

	static void tickMainMenu(Widget& widget) {
		++main_menu_ticks;
	}

	static void updateSliderArrows(Frame& frame) {
		bool drawSliders = false;
		auto selectedWidget = frame.findSelectedWidget();
		if (selectedWidget && selectedWidget->getType() == Widget::WIDGET_SLIDER) {
			auto slider = static_cast<Slider*>(selectedWidget);
			if (slider->isActivated() && slider->getOrientation() == Slider::SLIDER_HORIZONTAL) {
				drawSliders = true;
				auto left = frame.findImage("slider_left");
				if (left) {
					left->pos.x = slider->getHandleSize().x - 32;
					left->pos.y = slider->getRailSize().y + slider->getRailSize().h / 2 - 22;
					if (left->disabled) {
						left->disabled = false;
						soundActivate();
					}
				}
				auto right = frame.findImage("slider_right");
				if (right) {
					right->pos.x = slider->getHandleSize().x + slider->getHandleSize().w + 2;
					right->pos.y = slider->getRailSize().y + slider->getRailSize().h / 2 - 22;
					if (right->disabled) {
						right->disabled = false;
					}
				}
			}
		}
		if (drawSliders == false) {
			auto left = frame.findImage("slider_left");
			if (left) {
				left->disabled = true;
			}
			auto right = frame.findImage("slider_right");
			if (right) {
				right->disabled = true;
			}
		}
	}

	static Button* createBackWidget(Frame* parent, void (*callback)(Button&)) {
		auto back = parent->addFrame("back");
		back->setSize(SDL_Rect{5, 5, 66, 36});
		back->setActualSize(SDL_Rect{0, 0, 66, 36});
		back->setColor(0);
		back->setBorderColor(0);
		back->setBorder(0);
		auto backdrop = back->addImage(
			back->getActualSize(),
			0xffffffff,
			"images/ui/BackButton/UI_ButtonBack_00.png",
			"backdrop"
		);

		auto back_button = back->addButton("back_button");
		back_button->setSize(SDL_Rect{10, 12, 48, 20});
		back_button->setColor(0);
		back_button->setBorderColor(0);
		back_button->setHighlightColor(0);
		back_button->setBorder(0);
		back_button->setText("Back");
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::LEFT);
		back_button->setVJustify(Button::justify_t::CENTER);
		back_button->setCallback(callback);
		/*back_button->setTickCallback([](Widget& widget) {
			auto button = static_cast<Button*>(&widget);
			auto frame = static_cast<Frame*>(button->getParent());
			auto backdrop = frame->findImage("backdrop");
			if (button->isSelected()) {
				backdrop->color = makeColor(255, 255, 255, 255);
			} else {
				backdrop->color = makeColor(127, 127, 127, 255);
			}
			});*/

		return back_button;
	}

/******************************************************************************/

	inline void InventorySorting::save() {
		auto_hotbar_categories[0] = hotbarWeapons;
		auto_hotbar_categories[1] = hotbarArmor;
		auto_hotbar_categories[2] = hotbarAmulets;
		auto_hotbar_categories[3] = hotbarBooks;
		auto_hotbar_categories[4] = hotbarTools;
		auto_hotbar_categories[5] = hotbarThrown;
		auto_hotbar_categories[6] = hotbarGems;
		auto_hotbar_categories[7] = hotbarPotions;
		auto_hotbar_categories[8] = hotbarScrolls;
		auto_hotbar_categories[9] = hotbarStaves;
		auto_hotbar_categories[10] = hotbarFood;
		auto_hotbar_categories[11] = hotbarSpells;
		autosort_inventory_categories[0] = sortWeapons;
		autosort_inventory_categories[1] = sortArmor;
		autosort_inventory_categories[2] = sortAmulets;
		autosort_inventory_categories[3] = sortBooks;
		autosort_inventory_categories[4] = sortTools;
		autosort_inventory_categories[5] = sortThrown;
		autosort_inventory_categories[6] = sortGems;
		autosort_inventory_categories[7] = sortPotions;
		autosort_inventory_categories[8] = sortScrolls;
		autosort_inventory_categories[9] = sortStaves;
		autosort_inventory_categories[10] = sortFood;
		autosort_inventory_categories[11] = sortEquipped;
	}

	inline InventorySorting InventorySorting::load() {
		InventorySorting inventory_sorting;
		inventory_sorting.hotbarWeapons = auto_hotbar_categories[0];
		inventory_sorting.hotbarArmor = auto_hotbar_categories[1];
		inventory_sorting.hotbarAmulets = auto_hotbar_categories[2];
		inventory_sorting.hotbarBooks = auto_hotbar_categories[3];
		inventory_sorting.hotbarTools = auto_hotbar_categories[4];
		inventory_sorting.hotbarThrown = auto_hotbar_categories[5];
		inventory_sorting.hotbarGems = auto_hotbar_categories[6];
		inventory_sorting.hotbarPotions = auto_hotbar_categories[7];
		inventory_sorting.hotbarScrolls = auto_hotbar_categories[8];
		inventory_sorting.hotbarStaves = auto_hotbar_categories[9];
		inventory_sorting.hotbarFood = auto_hotbar_categories[10];
		inventory_sorting.hotbarSpells = auto_hotbar_categories[11];
		inventory_sorting.sortWeapons = autosort_inventory_categories[0];
		inventory_sorting.sortArmor = autosort_inventory_categories[1];
		inventory_sorting.sortAmulets = autosort_inventory_categories[2];
		inventory_sorting.sortBooks = autosort_inventory_categories[3];
		inventory_sorting.sortTools = autosort_inventory_categories[4];
		inventory_sorting.sortThrown = autosort_inventory_categories[5];
		inventory_sorting.sortGems = autosort_inventory_categories[6];
		inventory_sorting.sortPotions = autosort_inventory_categories[7];
		inventory_sorting.sortScrolls = autosort_inventory_categories[8];
		inventory_sorting.sortStaves = autosort_inventory_categories[9];
		inventory_sorting.sortFood = autosort_inventory_categories[10];
		inventory_sorting.sortEquipped = autosort_inventory_categories[11];
		return inventory_sorting;
	}

	inline InventorySorting InventorySorting::reset() {
		return InventorySorting();
	}

/******************************************************************************/

	static int story_text_pause = 0;
	static int story_text_scroll = 0;
	static int story_text_section = 0;
	static float story_text_writer = 0.f;
	static bool story_text_end = false;

	static const char* intro_text =
	u8"Long ago, the bustling town of Hamlet was the envy of all its neighbors,\nfor it was the most thriving city in all the land.#"
	u8"Its prosperity was unmatched for generations until the evil Baron Herx came\nto power.#"
	u8"The Baron, in his endless greed, forced the people to dig the hills for gold,\nthough the ground had never given such treasure before.#"
	u8"Straining under the yoke of their master, the people planned his demise.\nThey tricked him with a promise of gold and sealed him within the mines.#"
	u8"Free of their cruel master, the people returned to their old way of life.\nBut disasters shortly began to befall the village.#"
	u8"Monsters and other evils erupted from the ground, transforming the village\ninto a ghost town.#"
	u8"Many adventurers have descended into the mines to break the Baron's curse,\nbut none have returned.#"
	u8"The town of Hamlet cries for redemption, and only a hero can save it\nfrom its curse...";

	static void createStoryScreen() {
		main_menu_frame->addImage(
			main_menu_frame->getSize(),
			0xffffffff,
			"images/ui/Main Menus/Story/intro1.png",
			"backdrop"
		);

		story_text_pause = 0;
		story_text_scroll = 0;
		story_text_section = 0;
		story_text_writer = 0.f;
		story_text_end = false;

		auto back_button = main_menu_frame->addButton("back");
		back_button->setText("Skip story");
		back_button->setColor(makeColor(0, 0, 0, 0));
		back_button->setHighlightColor(makeColor(0, 0, 0, 0));
		back_button->setBorderColor(makeColor(0, 0, 0, 0));
		back_button->setTextColor(0xffffffff);
		back_button->setTextHighlightColor(0xffffffff);
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::RIGHT);
		back_button->setVJustify(Button::justify_t::CENTER);
		back_button->setSize(SDL_Rect{Frame::virtualScreenX - 400, Frame::virtualScreenY - 70, 380, 50});
		back_button->setCallback([](Button& b){
			fadeout = true;
			main_menu_fade_destination = FadeDestination::RootMainMenu;
			});
		back_button->setWidgetBack("back");
		back_button->select();

		auto font = Font::get(bigfont_outline); assert(font);

		auto textbox1 = main_menu_frame->addFrame("story_text_box");
		textbox1->setSize(SDL_Rect{120, Frame::virtualScreenY - font->height() * 4, Frame::virtualScreenX - 240, font->height() * 3});
		textbox1->setActualSize(SDL_Rect{0, 0, textbox1->getSize().w, textbox1->getSize().h});
		textbox1->setColor(makeColor(0, 0, 0, 127));
		textbox1->setBorder(0);

		auto textbox2 = textbox1->addFrame("story_text_box");
		textbox2->setScrollBarsEnabled(false);
		textbox2->setAllowScrollBinds(false);
		textbox2->setSize(SDL_Rect{0, font->height() / 2, Frame::virtualScreenX - 240, font->height() * 2});
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
					--story_text_writer;
					if (story_text_writer <= 0.f) {
						story_text_writer = fmodf(story_text_writer, 1.f);
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
							} else if (c == ',') {
								story_text_writer += TICKS_PER_SECOND / 5.f;
							} else if (c == '.') {
								story_text_writer += TICKS_PER_SECOND / 2.f;
							} else {
								story_text_writer += TICKS_PER_SECOND / 30.f;
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

/******************************************************************************/

	static std::string settings_tab_name;
	static AllSettings allSettings;

	struct Setting {
		enum class Type : Uint8 {
			Boolean = 0,
			Slider = 1,
			Customize = 2,
			BooleanWithCustomize = 3,
			Dropdown = 4,
		};
		Type type;
		const char* name;
	};

	static void settingsSave() {
		auto_hotbar_new_items = allSettings.add_items_to_hotbar_enabled;
		allSettings.inventory_sorting.save();
		right_click_protect = !allSettings.use_on_release_enabled;
		disable_messages = !allSettings.show_messages_enabled;
		hide_playertags = !allSettings.show_player_nametags_enabled;
		nohud = !allSettings.show_hud_enabled;
		broadcast = !allSettings.show_ip_address_enabled;
		spawn_blood = !allSettings.content_control_enabled;
		colorblind = allSettings.colorblind_mode_enabled;
		arachnophobia_filter = allSettings.arachnophobia_filter_enabled;
		shaking = allSettings.shaking_enabled;
		bobbing = allSettings.bobbing_enabled;
		flickerLights = allSettings.light_flicker_enabled;
		xres = allSettings.resolution_x;
		yres = allSettings.resolution_y;
		verticalSync = allSettings.vsync_enabled;
		vertical_splitscreen = allSettings.vertical_split_enabled;
		vidgamma = allSettings.gamma / 100.f;
		fov = allSettings.fov;
		fpsLimit = allSettings.fps;
		master_volume = allSettings.master_volume;
		sfxvolume = (allSettings.gameplay_volume / 100.f) * 128.f;
		sfxAmbientVolume = (allSettings.ambient_volume / 100.f) * 128.f;
		sfxEnvironmentVolume = (allSettings.environment_volume / 100.f) * 128.f;
		musvolume = (allSettings.music_volume / 100.f) * 128.f;
		minimapPingMute = !allSettings.minimap_pings_enabled;
		mute_player_monster_sounds = !allSettings.player_monster_sounds_enabled;
		mute_audio_on_focus_lost = !allSettings.out_of_focus_audio_enabled;
		hotbar_numkey_quick_add = allSettings.numkeys_in_inventory_enabled;
		mousespeed = allSettings.mouse_sensitivity;
		reversemouse = allSettings.reverse_mouse_enabled;
		smoothmouse = allSettings.smooth_mouse_enabled;
		disablemouserotationlimit = !allSettings.rotation_speed_limit_enabled;
		gamepad_rightx_sensitivity = allSettings.turn_sensitivity_x * 10.f;
		gamepad_righty_sensitivity = allSettings.turn_sensitivity_y * 10.f;
		svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
		svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
		svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
		svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
		svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
		svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
		svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
		svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
		svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);

		// transmit server flags
		if ( !intro && multiplayer == SERVER ) {
			strcpy((char*)net_packet->data, "SVFL");
			SDLNet_Write32(svFlags, &net_packet->data[4]);
			net_packet->len = 8;
			for ( int c = 1; c < MAXPLAYERS; ++c ) {
				if ( client_disconnected[c] ) {
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
				messagePlayer(c, language[276]);
			}
			messagePlayer(clientnum, language[276]);
		}

		// update volume for sound groups
#ifdef USE_FMOD
		music_group->setVolume(musvolume / 128.f);
		sound_group->setVolume(sfxvolume / 128.f);
		soundAmbient_group->setVolume(sfxAmbientVolume / 128.f);
		soundEnvironment_group->setVolume(sfxEnvironmentVolume / 128.f);
#elif defined USE_OPENAL
		OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
		OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
		OPENAL_ChannelGroup_SetVolume(soundAmbient_group, sfxAmbientVolume / 128.f);
		OPENAL_ChannelGroup_SetVolume(soundEnvironment_group, sfxEnvironmentVolume / 128.f);
#endif

		// write config file
		saveConfig("default.cfg");
	}

	static void settingsReset() {
		allSettings.add_items_to_hotbar_enabled = true;
		allSettings.inventory_sorting = InventorySorting::reset();
		allSettings.use_on_release_enabled = true;
		allSettings.show_messages_enabled = true;
		allSettings.show_player_nametags_enabled = true;
		allSettings.show_hud_enabled = true;
		allSettings.show_ip_address_enabled = true;
		allSettings.content_control_enabled = false;
		allSettings.colorblind_mode_enabled = false;
		allSettings.arachnophobia_filter_enabled = false;
		allSettings.shaking_enabled = true;
		allSettings.bobbing_enabled = true;
		allSettings.light_flicker_enabled = true;
		allSettings.resolution_x = 1280;
		allSettings.resolution_y = 720;
		allSettings.vsync_enabled = true;
		allSettings.vertical_split_enabled = false;
		allSettings.gamma = 100.f;
		allSettings.fov = 65;
		allSettings.fps = 60;
		allSettings.master_volume = 100.f;
		allSettings.gameplay_volume = 100.f;
		allSettings.ambient_volume = 100.f;
		allSettings.environment_volume = 100.f;
		allSettings.music_volume = 100.f;
		allSettings.minimap_pings_enabled = true;
		allSettings.player_monster_sounds_enabled = true;
		allSettings.out_of_focus_audio_enabled = true;
		allSettings.numkeys_in_inventory_enabled = true;
		allSettings.mouse_sensitivity = 32.f;
		allSettings.reverse_mouse_enabled = false;
		allSettings.smooth_mouse_enabled = false;
		allSettings.rotation_speed_limit_enabled = true;
		allSettings.turn_sensitivity_x = 50.f;
		allSettings.turn_sensitivity_y = 50.f;
		allSettings.classic_mode_enabled = false;
		allSettings.hardcore_mode_enabled = false;
		allSettings.friendly_fire_enabled = true;
		allSettings.keep_inventory_enabled = false;
		allSettings.hunger_enabled = true;
		allSettings.minotaur_enabled = true;
		allSettings.random_traps_enabled = true;
		allSettings.extra_life_enabled = false;
		allSettings.cheats_enabled = false;
	}

	void settingsCustomizeInventorySorting(Button&);

	static void inventorySortingDefaults(Button& button) {
		soundActivate();
		allSettings.inventory_sorting = InventorySorting::reset();
		auto window = main_menu_frame->findFrame("inventory_sorting_window"); assert(window);
		window->removeSelf();
		settingsCustomizeInventorySorting(button);
	}

	static void inventorySortingDiscard(Button& button) {
		soundCancel();
		auto window = main_menu_frame->findFrame("inventory_sorting_window"); assert(window);
		window->removeSelf();
		auto settings = main_menu_frame->findFrame("settings");
		if (settings) {
			auto settings_subwindow = settings->findFrame("settings_subwindow");
			if (settings_subwindow) {
				auto inventory_sorting_customize = settings_subwindow->findButton("setting_inventory_sorting_customize_button");
				if (inventory_sorting_customize) {
					inventory_sorting_customize->select();
				}
			}
		}
		allSettings.inventory_sorting = InventorySorting::load();
	}

	static void inventorySortingConfirm(Button& button) {
		soundActivate();
		auto window = main_menu_frame->findFrame("inventory_sorting_window"); assert(window);
		window->removeSelf();
		auto settings = main_menu_frame->findFrame("settings");
		if (settings) {
			auto settings_subwindow = settings->findFrame("settings_subwindow");
			if (settings_subwindow) {
				auto inventory_sorting_customize = settings_subwindow->findButton("setting_inventory_sorting_customize_button");
				if (inventory_sorting_customize) {
					inventory_sorting_customize->select();
				}
			}
		}
		allSettings.inventory_sorting.save();
	}

	void settingsCustomizeInventorySorting(Button& button) {
		soundActivate();

		auto window = main_menu_frame->addFrame("inventory_sorting_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 978) / 2,
			(Frame::virtualScreenY - 718) / 2,
			978,
			718
			});
		window->setActualSize(SDL_Rect{0, 0, window->getSize().w, window->getSize().h});
		window->setColor(0);
		window->setBorder(0);
		window->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			updateSliderArrows(*frame);
			});

		// slider arrows
		auto sliderLeft = window->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;
		auto sliderRight = window->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;

		// banner
		auto banner = window->addImage(
			SDL_Rect{14, 4, 950, 50},
			makeColor(50, 56, 67, 255),
			"images/system/white.png",
			"banner_area"
		);
		auto banner_text = window->addField("banner_text", 64);
		banner_text->setSize(banner->pos);
		banner_text->setText("AUTOMATIC INVENTORY BEHAVIOR");
		banner_text->setFont(bigfont_outline);
		banner_text->setJustify(Field::justify_t::CENTER);

		auto populate_text = window->addField("populate_text", 64);
		populate_text->setSize(SDL_Rect{20, 64, 512, 64});
		populate_text->setText("Populate Hotbar");
		populate_text->setFont(bigfont_outline);
		populate_text->setVJustify(Field::justify_t::TOP);
		populate_text->setHJustify(Field::justify_t::LEFT);

		auto sort_text = window->addField("sort_text", 64);
		sort_text->setSize(SDL_Rect{16, 64, 942, 64});
		sort_text->setText("Auto-Sort Order");
		sort_text->setFont(bigfont_outline);
		sort_text->setVJustify(Field::justify_t::TOP);
		sort_text->setHJustify(Field::justify_t::CENTER);

		// background
		window->addImage(
			SDL_Rect{18, 54, 942, 658},
			0xffffffff,
			"images/system/white.png",
			"rock_background_dimmer"
		);
		auto rock_background = window->addImage(
			SDL_Rect{18, 54, 942, 658},
			makeColor(127, 127, 127, 251),
			"images/ui/Main Menus/Settings/Settings_BGTile00.png",
			"rock_background"
		);
		rock_background->tiled = true;
		auto window_frame = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_Window00.png",
			"window_frame"
		);
		window_frame->ontop = true;

		// bottom buttons
		struct Option {
			const char* name;
			void (*callback)(Button&);
		};
		Option options[] = {
			{"Defaults", inventorySortingDefaults},
			{"Discard", inventorySortingDiscard},
			{"Confirm", inventorySortingConfirm},
		};
		const int num_options = sizeof(options) / sizeof(options[0]);
		for (int c = 0; c < num_options; ++c) {
			auto button = window->addButton(options[c].name);
			button->setSize(SDL_Rect{412 + (582 - 412) * c, 638, 164, 62});
			button->setBackground("images/ui/Main Menus/Settings/AutoSort/Button_Basic00.png");
			button->setText(options[c].name);
			button->setFont(bigfont_outline);
			button->setColor(makeColor(127, 127, 127, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			if (c > 0) {
				button->setWidgetLeft(options[c - 1].name);
			}
			if (c < num_options - 1) {
				button->setWidgetRight(options[c + 1].name);
			}
			button->setWidgetUp("sort_slider10");
			button->setCallback(options[c].callback);
			button->setWidgetBack("Discard");
			button->addWidgetAction("MenuAlt1", "Defaults");
			button->addWidgetAction("MenuStart", "Confirm");
		}
		auto first_button = window->findButton(options[0].name); assert(first_button);
		first_button->select();
		first_button->setWidgetLeft("hotbar_button11");

		// hotbar toggle buttons
		void (*hotbar_callbacks[12])(Button&) = {
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarWeapons = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarArmor = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarAmulets = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarBooks = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarTools = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarThrown = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarGems = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarPotions = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarScrolls = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarStaves = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarFood = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarSpells = button.isPressed(); },
		};
		const int num_hotbar_buttons = sizeof(hotbar_callbacks) / sizeof(hotbar_callbacks[0]);
		for (int c = num_hotbar_buttons - 1; c >= 0; --c) {
			auto button = window->addButton((std::string("hotbar_button") + std::to_string(c)).c_str());
			button->setSize(SDL_Rect{62, 88 + c * 50, 48, 48});
			button->setBackground("images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_Backing00.png");
			button->setIcon("images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_X00.png");
			button->setStyle(Button::style_t::STYLE_CHECKBOX);
			button->setCallback(hotbar_callbacks[c]);
			button->setBorder(0);
			button->setTickCallback([](Widget& widget){
				auto button = static_cast<Button*>(&widget);
				if (button->isSelected()) {
					button->setBackground("images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_Selected00.png");
				} else {
					button->setBackground("images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_Backing00.png");
				}
				});
			if (c > 0) {
				button->setWidgetUp((std::string("hotbar_button") + std::to_string(c - 1)).c_str());
			}
			if (c < num_hotbar_buttons - 1) {
				button->setWidgetDown((std::string("hotbar_button") + std::to_string(c + 1)).c_str());
			}
			if (c == num_hotbar_buttons - 1) {
				button->setWidgetRight("Defaults");
			} else {
				button->setWidgetRight((std::string("sort_slider") + std::to_string(c)).c_str());
			}
			switch (c) {
			case 0: button->setPressed(allSettings.inventory_sorting.hotbarWeapons); break;
			case 1: button->setPressed(allSettings.inventory_sorting.hotbarArmor); break;
			case 2: button->setPressed(allSettings.inventory_sorting.hotbarAmulets); break;
			case 3: button->setPressed(allSettings.inventory_sorting.hotbarBooks); break;
			case 4: button->setPressed(allSettings.inventory_sorting.hotbarTools); break;
			case 5: button->setPressed(allSettings.inventory_sorting.hotbarThrown); break;
			case 6: button->setPressed(allSettings.inventory_sorting.hotbarGems); break;
			case 7: button->setPressed(allSettings.inventory_sorting.hotbarPotions); break;
			case 8: button->setPressed(allSettings.inventory_sorting.hotbarScrolls); break;
			case 9: button->setPressed(allSettings.inventory_sorting.hotbarStaves); break;
			case 10: button->setPressed(allSettings.inventory_sorting.hotbarFood); break;
			case 11: button->setPressed(allSettings.inventory_sorting.hotbarSpells); break;
			default: break;
			}
			button->setWidgetBack("Discard");
			button->addWidgetAction("MenuAlt1", "Defaults");
			button->addWidgetAction("MenuStart", "Confirm");
		}

		// inventory sort sliders
		void (*sort_slider_callbacks[11])(Slider&) = {
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortWeapons = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortArmor = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortAmulets = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortBooks = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortTools = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortThrown = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortGems = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortPotions = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortScrolls = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortStaves = slider.getValue(); },
			[](Slider& slider){ soundSlider(); allSettings.inventory_sorting.sortFood = slider.getValue(); },
			//[](Slider& slider){ allSettings.inventory_sorting.sortEquipped = slider.getValue(); }, // Hey, we don't have enough room for this
		};
		const int num_sliders = sizeof(sort_slider_callbacks) / sizeof(sort_slider_callbacks[0]);
		for (int c = num_sliders - 1; c >= 0; --c) {
			auto slider = window->addSlider((std::string("sort_slider") + std::to_string(c)).c_str());
			slider->setMaxValue(6.f);
			slider->setMinValue(-6.f);
			slider->setRailSize(SDL_Rect{158, 100 + 50 *c, 724, 24});
			slider->setHandleSize(SDL_Rect{0, 0, 52, 54});
			slider->setRailImage("images/ui/Main Menus/Settings/AutoSort/transparent.png");
			slider->setHandleImage("images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png");
			slider->setCallback(sort_slider_callbacks[c]);
			switch (c) {
			case 0: slider->setValue(allSettings.inventory_sorting.sortWeapons); break;
			case 1: slider->setValue(allSettings.inventory_sorting.sortArmor); break;
			case 2: slider->setValue(allSettings.inventory_sorting.sortAmulets); break;
			case 3: slider->setValue(allSettings.inventory_sorting.sortBooks); break;
			case 4: slider->setValue(allSettings.inventory_sorting.sortTools); break;
			case 5: slider->setValue(allSettings.inventory_sorting.sortThrown); break;
			case 6: slider->setValue(allSettings.inventory_sorting.sortGems); break;
			case 7: slider->setValue(allSettings.inventory_sorting.sortPotions); break;
			case 8: slider->setValue(allSettings.inventory_sorting.sortScrolls); break;
			case 9: slider->setValue(allSettings.inventory_sorting.sortStaves); break;
			case 10: slider->setValue(allSettings.inventory_sorting.sortFood); break;
			default: break;
			}
			if (c > 0) {
				slider->setWidgetUp((std::string("sort_slider") + std::to_string(c - 1)).c_str());
			}
			if (c < num_sliders - 1) {
				slider->setWidgetDown((std::string("sort_slider") + std::to_string(c + 1)).c_str());
			}
			else {
				slider->setWidgetDown("Defaults");
			}
			slider->setWidgetLeft((std::string("hotbar_button") + std::to_string(c)).c_str());
			const char* icon_img_path = nullptr;
			switch (c) {
			case 0: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Sword00.png"; break;
			case 1: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Helmet00.png"; break;
			case 2: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Necklace00.png"; break;
			case 3: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Book00.png"; break;
			case 4: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Torch00.png"; break;
			case 5: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Chakram00.png"; break;
			case 6: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Gem00.png"; break;
			case 7: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Potion00.png"; break;
			case 8: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Scroll00.png"; break;
			case 9: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Staff00.png"; break;
			case 10: icon_img_path = "images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Bread00.png"; break;
			default: break;
			}
			auto icon = window->addImage(
				SDL_Rect{0, 0, 52, 54},
				0xffffffff,
				icon_img_path ? icon_img_path : "",
				(std::string("sort_slider_img") + std::to_string(c)).c_str()
			);
			icon->ontop = true;
			icon->disabled = true;
			slider->setTickCallback([](Widget& widget){
				Slider* slider = static_cast<Slider*>(&widget);
				slider->setValue((int)slider->getValue());
				auto window = main_menu_frame->findFrame("inventory_sorting_window");
				if (window) {
					auto number = std::string(slider->getName()).substr(sizeof("sort_slider") - 1);
					int c = atoi(number.c_str());
					auto icon = window->findImage((std::string("sort_slider_img") + std::to_string(c)).c_str()); assert(icon);
					icon->disabled = false;
					icon->pos.x = slider->getHandleSize().x;
					icon->pos.y = slider->getHandleSize().y;
					if (slider->isSelected()) {
						slider->setHandleImage("images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBrown00.png");
					} else {
						slider->setHandleImage("images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png");
					}
				}
				});
			slider->setWidgetBack("Discard");
			slider->addWidgetAction("MenuAlt1", "Defaults");
			slider->addWidgetAction("MenuStart", "Confirm");
		}
		window->addImage(
			SDL_Rect{126, 100, 788, 524},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_Sliders_BackingALL00.png",
			"slider_backing"
		);
		window->addImage(
			SDL_Rect{188, 98, 664, 526},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_Sliders_Spacers01.png",
			"slider_spacers"
		);

		// spell box
		window->addImage(
			SDL_Rect{134, 638, 52, 54},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png",
			"spellbox"
		);
		window->addImage(
			SDL_Rect{134, 638, 52, 54},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Magic00.png",
			"spellbox_icon"
		);
	}

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
		Text* text_image = Text::get(text, field->getFont());
		int w = text_image->getWidth();
		auto fleur_left = frame.addImage(
			SDL_Rect{ (1080 - w) / 2 - 26 - 8, y + 6, 26, 30 },
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_left").c_str()
		);
		auto fleur_right = frame.addImage(
			SDL_Rect{ (1080 + w) / 2 + 8, y + 6, 26, 30 },
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_right").c_str()
		);
		return image->pos.h + 6;
	}

	static int settingsAddOption(Frame& frame, int y, const char* name, const char* text, const char* tip) {
		std::string fullname = std::string("setting_") + name;
		auto image = frame.addImage(
			SDL_Rect{0, y, 382, 52},
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_Left_Backing00.png",
			(fullname + "_image").c_str()
		);
		auto field = frame.addField((fullname + "_field").c_str(), 128);
		auto size = image->pos; size.x += 24; size.w -= 24;
		field->setSize(size);
		field->setFont(bigfont_outline);
		field->setText(text);
		field->setHJustify(Field::justify_t::LEFT);
		field->setVJustify(Field::justify_t::CENTER);
		field->setGuide(tip);
		return size.h + 10;
	}

	static int settingsAddBooleanOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool on,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
		auto button = frame.addButton((fullname + "_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y + 2,
			158,
			48});
		button->setFont(smallfont_outline);
		button->setText("Off          On");
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
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddBooleanWithCustomizeOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool on,
		void (*callback)(Button&),
		void (*customize_callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddBooleanOption(frame, y, name, text, tip, on, callback);
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
		button->setWidgetLeft((fullname + "_button").c_str());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		auto boolean = frame.findButton((fullname + "_button").c_str()); assert(boolean);
		boolean->setWidgetRight((fullname + "_customize_button").c_str());
		boolean->setWidgetBack("discard_and_exit");
		boolean->setWidgetPageLeft("tab_left");
		boolean->setWidgetPageRight("tab_right");
		boolean->addWidgetAction("MenuAlt1", "restore_defaults");
		boolean->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddCustomize(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
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
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddDropdown(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		const std::vector<const char*>& items,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
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
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddSlider(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		float value,
		float minValue,
		float maxValue,
		bool percent,
		void (*callback)(Slider&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
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
		if (percent) {
			field->setTickCallback([](Widget& widget){
				auto field = static_cast<Field*>(&widget); assert(field);
				auto frame = static_cast<Frame*>(widget.getParent());
				auto name = std::string(widget.getName());
				auto setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_text") - 1) - (sizeof("setting_") - 1));
				auto slider = frame->findSlider((std::string("setting_") + setting + std::string("_slider")).c_str()); assert(slider);
				char buf[8]; snprintf(buf, sizeof(buf), "%d%%", (int)slider->getValue());
				field->setText(buf);
				});
		} else {
			field->setTickCallback([](Widget& widget){
				auto field = static_cast<Field*>(&widget); assert(field);
				auto frame = static_cast<Frame*>(widget.getParent());
				auto name = std::string(widget.getName());
				auto setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_text") - 1) - (sizeof("setting_") - 1));
				auto slider = frame->findSlider((std::string("setting_") + setting + std::string("_slider")).c_str()); assert(slider);
				char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)slider->getValue());
				field->setText(buf);
				});
		}
		auto slider = frame.addSlider((fullname + "_slider").c_str());
		slider->setMinValue(minValue);
		slider->setMaxValue(maxValue);
		slider->setValue(value);
		slider->setRailSize(SDL_Rect{field->getSize().x + field->getSize().w + 32, y + 14, 450, 24});
		slider->setHandleSize(SDL_Rect{0, 0, 52, 42});
		slider->setCallback(callback);
		slider->setColor(makeColor(127,127,127,255));
		slider->setHighlightColor(makeColor(255,255,255,255));
		slider->setHandleImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Slide00.png");
		slider->setRailImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Backing00.png");
		slider->setWidgetBack("discard_and_exit");
		slider->setWidgetPageLeft("tab_left");
		slider->setWidgetPageRight("tab_right");
		slider->addWidgetAction("MenuAlt1", "restore_defaults");
		slider->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static Frame* settingsSubwindowSetup(Button& button) {
		if (settings_tab_name == button.getName()) {
			return nullptr;
		}
		soundActivate();
		settings_tab_name = button.getName();

		assert(main_menu_frame);
		auto settings = main_menu_frame->findFrame("settings"); assert(settings);
		auto settings_subwindow = settings->findFrame("settings_subwindow");
		if (settings_subwindow) {
			settings_subwindow->removeSelf();
		}
		settings_subwindow = settings->addFrame("settings_subwindow");
		settings_subwindow->setScrollBarsEnabled(false);
		settings_subwindow->setSize(SDL_Rect{16, 71 * 2, 547 * 2, 224 * 2});
		settings_subwindow->setActualSize(SDL_Rect{0, 0, 547 * 2, 224 * 2});
		settings_subwindow->setColor(0);
		settings_subwindow->setBorder(0);
		settings_subwindow->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			auto& images = frame->getImages();
			for (auto image : images) {
				if (image->path == "images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png") {
					image->path = "images/ui/Main Menus/Settings/Settings_Left_Backing00.png";
				}
			}
			auto selectedWidget = widget.findSelectedWidget();
			if (selectedWidget) {
				std::string setting;
				auto name = std::string(selectedWidget->getName());
				if (selectedWidget->getType() == Widget::WIDGET_SLIDER) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_slider") - 1) - (sizeof("setting_") - 1));
				} else if (selectedWidget->getType() == Widget::WIDGET_BUTTON) {
					auto button = static_cast<Button*>(selectedWidget);
					auto customize = "images/ui/Main Menus/Settings/Settings_Button_Customize00.png";
					if (strcmp(button->getBackground(), customize) == 0) {
						setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_customize_button") - 1) - (sizeof("setting_") - 1));
					} else {
						setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_button") - 1) - (sizeof("setting_") - 1));
					}
				}
				if (!setting.empty()) {
					auto image = frame->findImage((std::string("setting_") + setting + std::string("_image")).c_str());
					if (image) {
						image->path = "images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png";
					}
					auto field = frame->findField((std::string("setting_") + setting + std::string("_field")).c_str());
					if (field) {
						auto settings = static_cast<Frame*>(frame->getParent());
						auto tooltip = settings->findField("tooltip"); assert(tooltip);
						tooltip->setText(field->getGuide());
					}
				}
			}
			updateSliderArrows(*frame);
			});
		auto rock_background = settings_subwindow->addImage(
			settings_subwindow->getActualSize(),
			makeColor(127, 127, 127, 251),
			"images/ui/Main Menus/Settings/Settings_BGTile00.png",
			"background"
		);
		rock_background->tiled = true;
		auto slider = settings_subwindow->addSlider("scroll_slider");
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{1038, 16, 30, 440});
		slider->setRailImage("images/ui/Main Menus/Settings/Settings_Slider_Backing00.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = 16 + actualSize.y;
			slider.setRailSize(railSize);
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = 16 + actualSize.y;
			slider->setRailSize(railSize);
			});
		auto sliderLeft = settings_subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;
		auto sliderRight = settings_subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;
		return settings_subwindow;
	}

	static std::pair<std::string, std::string> getFullSettingNames(const Setting& setting) {
		switch (setting.type) {
		case Setting::Type::Boolean:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_button"),
				std::string(""));
		case Setting::Type::Slider:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_slider"),
				std::string(""));
		case Setting::Type::Customize:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_customize_button"),
				std::string(""));
		case Setting::Type::BooleanWithCustomize:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_button"),
				std::string("setting_") + std::string(setting.name) + std::string("_customize_button"));
		default:
			assert(0 && "Unknown setting type!");
			return std::make_pair(
				std::string(""),
				std::string(""));
		}
	}

	static void settingsSelect(Frame& frame, const Setting& setting) {
		auto names = getFullSettingNames(setting);
		auto widget = frame.findWidget(names.first.c_str(), false); assert(widget);
		widget->select();
	}

	static void settingsSubwindowFinalize(Frame& frame, int y) {
		const int height = std::max(224 * 2, y);
		frame.setActualSize(SDL_Rect{0, 0, 547 * 2, height});
		auto rock_background = frame.findImage("background"); assert(rock_background);
		rock_background->pos = frame.getActualSize();
		auto slider = frame.findSlider("scroll_slider"); assert(slider);
		slider->setValue(0.f);
		slider->setMinValue(0.f);
		slider->setMaxValue(height - 224 * 2);
	}

	static void hookSettingToSetting(Frame& frame, const Setting& setting1, const Setting& setting2) {
		auto names1 = getFullSettingNames(setting1);
		auto names2 = getFullSettingNames(setting2);
		auto widget11 = frame.findWidget(names1.first.c_str(), false); assert(widget11);
		auto widget12 = names1.second.empty() ? nullptr : frame.findWidget(names1.second.c_str(), false);
		auto widget21 = frame.findWidget(names2.first.c_str(), false); assert(widget21);
		auto widget22 = names2.second.empty() ? nullptr : frame.findWidget(names2.second.c_str(), false);
		widget11->setWidgetDown(names2.first.c_str());
		widget21->setWidgetUp(names1.first.c_str());
		if (widget12) {
			if (widget22) {
				widget12->setWidgetDown(names2.second.c_str());
				widget22->setWidgetUp(names1.second.c_str());
			} else {
				widget12->setWidgetDown(names2.first.c_str());
			}
		} else {
			if (widget22) {
				widget22->setWidgetUp(names1.first.c_str());
			}
		}
	}

	static void hookSettings(Frame& frame, const std::vector<Setting>& settings) {
		for (auto it = settings.begin(); std::next(it) != settings.end(); ++it) {
			auto& setting1 = (*it);
			auto& setting2 = (*std::next(it));
			hookSettingToSetting(frame, setting1, setting2);
		}
	}

	void settingsUI(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			settingsSelect(*settings_subwindow, {Setting::Type::BooleanWithCustomize, "add_items_to_hotbar"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "inventory", "Inventory Options");
		y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "add_items_to_hotbar", "Add Items to Hotbar",
			"Automatically fill the hotbar with recently collected items.",
			allSettings.add_items_to_hotbar_enabled, [](Button& button){soundToggle(); allSettings.add_items_to_hotbar_enabled = button.isPressed();},
			settingsCustomizeInventorySorting);
		y += settingsAddCustomize(*settings_subwindow, y, "inventory_sorting", "Inventory Sorting",
			"Customize the way items are automatically sorted in your inventory.",
			settingsCustomizeInventorySorting);
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "use_on_release", "Use on Release",
			"Activate an item as soon as the Use key is released in the inventory window.",
			allSettings.use_on_release_enabled, [](Button& button){soundToggle(); allSettings.use_on_release_enabled = button.isPressed();});
#endif

		y += settingsAddSubHeader(*settings_subwindow, y, "hud", "HUD Options");
		y += settingsAddCustomize(*settings_subwindow, y, "minimap_settings", "Minimap Settings",
			"Customize the appearance of the in-game minimap.",
			nullptr);
		y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "show_messages", "Show Messages",
			"Customize which messages will be logged to the player, if any.",
			allSettings.show_messages_enabled, [](Button& button){soundToggle(); allSettings.show_messages_enabled = button.isPressed();},
			nullptr);
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_player_nametags", "Show Player Nametags",
			"Display the name of each player character above their avatar.",
			allSettings.show_player_nametags_enabled, [](Button& button){soundToggle(); allSettings.show_player_nametags_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_hud", "Show HUD",
			"Toggle the display of health and other status bars in game when the inventory is closed.",
			allSettings.show_hud_enabled, [](Button& button){soundToggle(); allSettings.show_hud_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_ip_address", "Show IP Address",
			"Hide the display of IP addresses and other location data for privacy purposes.",
			allSettings.show_ip_address_enabled, [](Button& button){soundToggle(); allSettings.show_ip_address_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::BooleanWithCustomize, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},
			{Setting::Type::Boolean, "use_on_release"},
			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},
			{Setting::Type::Boolean, "show_hud"},
			{Setting::Type::Boolean, "show_ip_address"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::BooleanWithCustomize, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},
			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},
			{Setting::Type::Boolean, "show_hud"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::BooleanWithCustomize, "add_items_to_hotbar"});
	}

	void settingsVideo(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "content_control"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "accessibility", "Accessibility");
		y += settingsAddBooleanOption(*settings_subwindow, y, "content_control", "Content Control",
			"Disable the appearance of blood and other explicit kinds of content in the game",
			allSettings.content_control_enabled, [](Button& button){soundToggle(); allSettings.content_control_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "colorblind_mode", "Colorblind Mode",
			"Change the appearance of certain UI elements to improve visibility for certain colorblind individuals.",
			allSettings.colorblind_mode_enabled, [](Button& button){soundToggle(); allSettings.colorblind_mode_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "arachnophobia_filter", "Arachnophobia Filter",
			"Replace all giant spiders in the game with hostile crustaceans.",
			allSettings.arachnophobia_filter_enabled, [](Button& button){soundToggle(); allSettings.arachnophobia_filter_enabled = button.isPressed();});

		y += settingsAddSubHeader(*settings_subwindow, y, "effects", "Effects");
		y += settingsAddBooleanOption(*settings_subwindow, y, "shaking", "Shaking",
			"Toggle the camera's ability to twist and roll when the player stumbles or receives damage.",
			allSettings.shaking_enabled, [](Button& button){soundToggle(); allSettings.shaking_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "bobbing", "Bobbing",
			"Toggle the camera's ability to bob steadily as the player moves.",
			allSettings.bobbing_enabled, [](Button& button){soundToggle(); allSettings.bobbing_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "light_flicker", "Light Flicker",
			"Toggle the flickering appearance of torches and other light fixtures in the game world.",
			allSettings.light_flicker_enabled, [](Button& button){soundToggle(); allSettings.light_flicker_enabled = button.isPressed();});

		y += settingsAddSubHeader(*settings_subwindow, y, "display", "Display");
#ifndef NINTENDO
		y += settingsAddDropdown(*settings_subwindow, y, "resolution", "Resolution", "Change the current window resolution.",
			{"1280 x 720", "1920 x 1080"},
			nullptr);
		y += settingsAddDropdown(*settings_subwindow, y, "window_mode", "Window Mode", "Change the current display mode.",
			{"Fullscreen", "Borderless", "Windowed"},
			nullptr);
		y += settingsAddBooleanOption(*settings_subwindow, y, "vsync", "Vertical Sync",
			"Prevent screen-tearing by locking the game's refresh rate to the current display.",
			allSettings.vsync_enabled, [](Button& button){soundToggle(); allSettings.vsync_enabled = button.isPressed();});
#endif
		y += settingsAddBooleanOption(*settings_subwindow, y, "vertical_split", "Vertical Splitscreen",
			"For splitscreen with two-players: divide the screen along a vertical line rather than a horizontal one.",
			allSettings.vertical_split_enabled, [](Button& button){soundToggle(); allSettings.vertical_split_enabled = button.isPressed();});
		y += settingsAddSlider(*settings_subwindow, y, "gamma", "Gamma",
			"Adjust the brightness of the visuals in-game.",
			allSettings.gamma, 50, 200, true, [](Slider& slider){soundSlider(); allSettings.gamma = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "fov", "Field of View",
			"Adjust the vertical field-of-view of the in-game camera.",
			allSettings.fov, 40, 100, false, [](Slider& slider){soundSlider(); allSettings.fov = slider.getValue();});
#ifndef NINTENDO
		y += settingsAddSlider(*settings_subwindow, y, "fps", "FPS limit",
			"Control the frame-rate limit of the game window.",
			allSettings.fps, 30, 300, false, [](Slider& slider){soundSlider(); allSettings.fps = slider.getValue();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "content_control"},
			{Setting::Type::Boolean, "colorblind_mode"},
			{Setting::Type::Boolean, "arachnophobia_filter"},
			{Setting::Type::Boolean, "shaking"},
			{Setting::Type::Boolean, "bobbing"},
			{Setting::Type::Boolean, "light_flicker"},
			{Setting::Type::Dropdown, "resolution"},
			{Setting::Type::Dropdown, "window_mode"},
			{Setting::Type::Boolean, "vsync"},
			{Setting::Type::Boolean, "vertical_split"},
			{Setting::Type::Slider, "gamma"},
			{Setting::Type::Slider, "fov"},
			{Setting::Type::Slider, "fps"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "content_control"},
			{Setting::Type::Boolean, "colorblind_mode"},
			{Setting::Type::Boolean, "arachnophobia_filter"},
			{Setting::Type::Boolean, "shaking"},
			{Setting::Type::Boolean, "bobbing"},
			{Setting::Type::Boolean, "light_flicker"},
			{Setting::Type::Boolean, "vertical_split"},
			{Setting::Type::Slider, "gamma"},
			{Setting::Type::Slider, "fov"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "content_control"});
	}

	void settingsAudio(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			settingsSelect(*settings_subwindow, {Setting::Type::Slider, "master_volume"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "volume", "Volume");
		y += settingsAddSlider(*settings_subwindow, y, "master_volume", "Master Volume",
			"Adjust the volume of all sound sources equally.",
			allSettings.master_volume, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.master_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "gameplay_volume", "Gameplay Volume",
			"Adjust the volume of most game sound effects.",
			allSettings.gameplay_volume, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.gameplay_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "ambient_volume", "Ambient Volume",
			"Adjust the volume of ominous subterranean sound-cues.",
			allSettings.ambient_volume, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.ambient_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "environment_volume", "Environment Volume",
			"Adjust the volume of flowing water and lava.",
			allSettings.environment_volume, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.environment_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "music_volume", "Music Volume",
			"Adjust the volume of the game's soundtrack.",
			allSettings.music_volume, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.music_volume = slider.getValue();});

		y += settingsAddSubHeader(*settings_subwindow, y, "options", "Options");
		y += settingsAddBooleanOption(*settings_subwindow, y, "minimap_pings", "Minimap Pings",
			"Toggle the ability to hear pings on the minimap",
			allSettings.minimap_pings_enabled, [](Button& button){soundToggle(); allSettings.minimap_pings_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "player_monster_sounds", "Player Monster Sounds",
			"Toggle the chance to emit monstrous mumbles when playing a non-human character.",
			allSettings.player_monster_sounds_enabled, [](Button& button){soundToggle(); allSettings.player_monster_sounds_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "out_of_focus_audio", "Out-of-Focus Audio",
			"Enable audio sources even when the game window is out-of-focus.",
			allSettings.out_of_focus_audio_enabled, [](Button& button){soundToggle(); allSettings.out_of_focus_audio_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Slider, "master_volume"},
			{Setting::Type::Slider, "gameplay_volume"},
			{Setting::Type::Slider, "ambient_volume"},
			{Setting::Type::Slider, "environment_volume"},
			{Setting::Type::Slider, "music_volume"},
			{Setting::Type::Boolean, "minimap_pings"},
			{Setting::Type::Boolean, "player_monster_sounds"},
			{Setting::Type::Boolean, "out_of_focus_audio"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Slider, "master_volume"},
			{Setting::Type::Slider, "gameplay_volume"},
			{Setting::Type::Slider, "ambient_volume"},
			{Setting::Type::Slider, "environment_volume"},
			{Setting::Type::Slider, "music_volume"},
			{Setting::Type::Boolean, "minimap_pings"},
			{Setting::Type::Boolean, "player_monster_sounds"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::Slider, "master_volume"});
	}

	void settingsControls(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			settingsSelect(*settings_subwindow, {Setting::Type::Customize, "bindings"});
			return;
		}
		int y = 0;

#ifndef NINTENDO
		y += settingsAddSubHeader(*settings_subwindow, y, "general", "General Settings");
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Modify controls for mouse, keyboard, gamepads, and other peripherals.",
			nullptr);

		y += settingsAddSubHeader(*settings_subwindow, y, "mouse_and_keyboard", "Mouse & Keyboard");
		y += settingsAddBooleanOption(*settings_subwindow, y, "numkeys_in_inventory", "Number Keys in Inventory",
			"Allow the player to bind inventory items to the hotbar using the number keys on their keyboard.",
			allSettings.numkeys_in_inventory_enabled, [](Button& button){soundToggle(); allSettings.numkeys_in_inventory_enabled = button.isPressed();});
		y += settingsAddSlider(*settings_subwindow, y, "mouse_sensitivity", "Mouse Sensitivity",
			"Control the speed by which mouse movement affects camera movement.",
			allSettings.mouse_sensitivity, 0, 100, false, [](Slider& slider){soundSlider(); allSettings.mouse_sensitivity = slider.getValue();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "reverse_mouse", "Reverse Mouse",
			"Reverse mouse up and down movement for controlling the orientation of the player.",
			allSettings.reverse_mouse_enabled, [](Button& button){soundToggle(); allSettings.reverse_mouse_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "smooth_mouse", "Smooth Mouse",
			"Smooth the movement of the mouse over a few frames of input.",
			allSettings.smooth_mouse_enabled, [](Button& button){soundToggle(); allSettings.smooth_mouse_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "rotation_speed_limit", "Rotation Speed Limit",
			"Limit how fast the player can rotate by moving the mouse.",
			allSettings.rotation_speed_limit_enabled, [](Button& button){soundToggle(); allSettings.rotation_speed_limit_enabled = button.isPressed();});
#endif

#ifdef NINTENDO
		y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Controller Settings");
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Modify controller bindings.",
			nullptr);
#else
		y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Gamepad Settings");
#endif
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_x", "Turn Sensitivity X",
			"Affect the horizontal sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_x, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.turn_sensitivity_x = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_y", "Turn Sensitivity Y",
			"Affect the vertical sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_y, 0, 100, true, [](Slider& slider){soundSlider(); allSettings.turn_sensitivity_y = slider.getValue();});

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Customize, "bindings"},
			{Setting::Type::Boolean, "numkeys_in_inventory"},
			{Setting::Type::Slider, "mouse_sensitivity"},
			{Setting::Type::Boolean, "reverse_mouse"},
			{Setting::Type::Boolean, "smooth_mouse"},
			{Setting::Type::Boolean, "rotation_speed_limit"},
			{Setting::Type::Slider, "turn_sensitivity_x"},
			{Setting::Type::Slider, "turn_sensitivity_y"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Customize, "bindings"},
			{Setting::Type::Slider, "turn_sensitivity_x"},
			{Setting::Type::Slider, "turn_sensitivity_y"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::Customize, "bindings"});
	}

	void settingsGame(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "classic_mode"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "game", "Game Settings");
		y += settingsAddBooleanOption(*settings_subwindow, y, "classic_mode", "Classic Mode",
			"Toggle this option to make the game end after the battle with Baron Herx.",
			allSettings.classic_mode_enabled, [](Button& button){soundToggle(); allSettings.classic_mode_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "hardcore_mode", "Hardcore Mode",
			"Greatly increases the difficulty of all combat encounters.",
			allSettings.hardcore_mode_enabled, [](Button& button){soundToggle(); allSettings.hardcore_mode_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "friendly_fire", "Friendly Fire",
			"Enable players to harm eachother and their allies.",
			allSettings.friendly_fire_enabled, [](Button& button){soundToggle(); allSettings.friendly_fire_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "keep_inventory", "Keep Inventory after Death",
			"When a player dies, they retain their inventory when revived on the next level.",
			allSettings.keep_inventory_enabled, [](Button& button){soundToggle(); allSettings.keep_inventory_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "hunger", "Hunger",
			"Toggle player hunger. When hunger is off, eating food heals the player directly.",
			allSettings.hunger_enabled, [](Button& button){soundToggle(); allSettings.hunger_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "minotaur", "Minotaur",
			"Toggle the minotaur's ability to spawn on many levels after a certain amount of time.",
			allSettings.minotaur_enabled, [](Button& button){soundToggle(); allSettings.minotaur_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "random_traps", "Random Traps",
			"Toggle the random placement of traps throughout each level.",
			allSettings.random_traps_enabled, [](Button& button){soundToggle(); allSettings.random_traps_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "extra_life", "Extra Life",
			"Start the game with an Amulet of Life-saving, to prevent one death.",
			allSettings.extra_life_enabled, [](Button& button){soundToggle(); allSettings.extra_life_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "cheats", "Cheats",
			"Toggle the ability to activate cheatcodes during gameplay.",
			allSettings.cheats_enabled, [](Button& button){soundToggle(); allSettings.cheats_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "classic_mode"},
			{Setting::Type::Boolean, "hardcore_mode"},
			{Setting::Type::Boolean, "friendly_fire"},
			{Setting::Type::Boolean, "keep_inventory"},
			{Setting::Type::Boolean, "hunger"},
			{Setting::Type::Boolean, "minotaur"},
			{Setting::Type::Boolean, "random_traps"},
			{Setting::Type::Boolean, "extra_life"},
			{Setting::Type::Boolean, "cheats"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "classic_mode"},
			{Setting::Type::Boolean, "hardcore_mode"},
			{Setting::Type::Boolean, "friendly_fire"},
			{Setting::Type::Boolean, "keep_inventory"},
			{Setting::Type::Boolean, "hunger"},
			{Setting::Type::Boolean, "minotaur"},
			{Setting::Type::Boolean, "random_traps"},
			{Setting::Type::Boolean, "extra_life"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "classic_mode"});
	}

/******************************************************************************/

	void recordsAdventureArchives(Button& button) {
		soundActivate();
	}

	void recordsLeaderboards(Button& button) {
		soundActivate();
	}

	void recordsDungeonCompendium(Button& button) {
		soundActivate();
	}

	void recordsStoryIntroduction(Button& button) {
		soundActivate();

		destroyMainMenu();
		createDummyMainMenu();

		fadeout = true;
		main_menu_fade_destination = FadeDestination::IntroStoryScreen;
	}

	void recordsCredits(Button& button) {
		soundActivate();

		destroyMainMenu();
		createDummyMainMenu();

		auto back_button = main_menu_frame->addButton("back");
		back_button->setText("Return to Main Menu");
		back_button->setColor(makeColor(0, 0, 0, 0));
		back_button->setHighlightColor(makeColor(0, 0, 0, 0));
		back_button->setBorderColor(makeColor(0, 0, 0, 0));
		back_button->setTextColor(0xffffffff);
		back_button->setTextHighlightColor(0xffffffff);
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::RIGHT);
		back_button->setVJustify(Button::justify_t::CENTER);
		back_button->setSize(SDL_Rect{Frame::virtualScreenX - 400, Frame::virtualScreenY - 70, 380, 50});
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
		credits->setScrollBarsEnabled(false);
		credits->setAllowScrollBinds(false);
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
		text1->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 81});
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
		text2->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 81});
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
			u8"Julian Seeger\n"
			u8"Mathias Golinelli\n"
			u8"Jesse Riddle\n"
			u8"Kevin White\n"
			u8"Desiree Colborn\n"
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
		soundCancel();

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

		int y = main_menu_buttons_height;

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

	static LobbyType currentLobbyType;

	enum class DLC {
		Base,
		MythsAndOutcasts,
		LegendsAndPariahs
	};

	struct Class {
		const char* name;
		DLC dlc;
		const char* image;
	};

	static const std::unordered_map<std::string, Class> classes = {
		{"random", {"Random", DLC::Base, "ClassSelect_Icon_Randomize_00.png"}},
		{"barbarian", {"Barbarian", DLC::Base, "ClassSelect_Icon_Barbarian_00.png"}},
		{"warrior", {"Warrior", DLC::Base, "ClassSelect_Icon_Warrior_00.png"}},
		{"healer", {"Healer", DLC::Base, "ClassSelect_Icon_Healer_00.png"}},
		{"rogue", {"Rogue", DLC::Base, "ClassSelect_Icon_Rogue_00.png"}},
		{"wanderer", {"Wanderer", DLC::Base, "ClassSelect_Icon_Wanderer_00.png"}},
		{"cleric", {"Cleric", DLC::Base, "ClassSelect_Icon_Cleric_00.png"}},
		{"merchant", {"Merchant", DLC::Base, "ClassSelect_Icon_Merchant_00.png"}},
		{"wizard", {"Wizard", DLC::Base, "ClassSelect_Icon_Wizard_00.png"}},
		{"arcanist", {"Arcanist", DLC::Base, "ClassSelect_Icon_Arcanist_00.png"}},
		{"joker", {"Joker", DLC::Base, "ClassSelect_Icon_Jester_00.png"}},
		{"sexton", {"Sexton", DLC::Base, "ClassSelect_Icon_Sexton_00.png"}},
		{"ninja", {"Ninja", DLC::Base, "ClassSelect_Icon_Ninja_00.png"}},
		{"monk", {"Monk", DLC::Base, "ClassSelect_Icon_Monk_00.png"}},
		{"conjurer", {"Conjurer", DLC::MythsAndOutcasts, "ClassSelect_Icon_Conjurer_00.png"}},
		{"accursed", {"Accursed", DLC::MythsAndOutcasts, "ClassSelect_Icon_Accursed_00.png"}},
		{"mesmer", {"Mesmer", DLC::MythsAndOutcasts, "ClassSelect_Icon_Mesmer_00.png"}},
		{"brewer", {"Brewer", DLC::MythsAndOutcasts, "ClassSelect_Icon_Brewer_00.png"}},
		{"mechanist", {"Mechanist", DLC::LegendsAndPariahs, "ClassSelect_Icon_Mechanist_00.png"}},
		{"punisher", {"Punisher", DLC::LegendsAndPariahs, "ClassSelect_Icon_Punisher_00.png"}},
		{"shaman", {"Shaman", DLC::LegendsAndPariahs, "ClassSelect_Icon_Shaman_00.png"}},
		{"hunter", {"Hunter", DLC::LegendsAndPariahs, "ClassSelect_Icon_Hunter_00.png"}},
	};

	static const char* classes_in_order[] = {
		"random", "barbarian", "warrior", "healer",
		"rogue", "wanderer", "cleric", "merchant",
		"wizard", "arcanist", "joker", "sexton",
		"ninja", "monk", "conjurer", "accursed",
		"mesmer", "brewer", "mechanist", "punisher",
		"shaman", "hunter"
	};

	constexpr int num_classes = sizeof(classes_in_order) / sizeof(classes_in_order[0]);

	std::vector<const char*> reducedClassList(int index) {
		std::vector<const char*> result;
		result.reserve(num_classes);
		result.emplace_back("random");
		for (int c = CLASS_BARBARIAN; c <= CLASS_HUNTER; ++c) {
			if (isCharacterValidFromDLC(*stats[index], c)) {
				result.emplace_back(classes_in_order[c + 1]);
			}
		}
		return result;
	}

	static auto male_button_fn = [](Button& button, int index) {
		auto card = static_cast<Frame*>(button.getParent());
		stats[index]->sex = MALE;
		if (stats[index]->playerRace == RACE_SUCCUBUS) {
			auto succubus = card->findButton("Succubus");
			if (succubus) {
				succubus->setPressed(false);
			}
			if (enabledDLCPack2) {
				stats[index]->playerRace = RACE_INCUBUS;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Incubus");
				}
				auto incubus = card->findButton("Incubus");
				if (incubus) {
					incubus->setPressed(true);
				}
				if (client_classes[index] == CLASS_MESMER && stats[index]->appearance == 0) {
					if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
						client_classes[index] = CLASS_PUNISHER;
						auto class_button = card->findButton("class");
						if (class_button) {
							class_button->setIcon("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_Punisher_00.png");
						}
					}
					stats[index]->clearStats();
					initClass(index);
				}
			} else {
				stats[index]->playerRace = RACE_HUMAN;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Human");
				}
				auto human = card->findButton("Human");
				if (human) {
					human->setPressed(true);
				}
			}
		}
	};

	static auto female_button_fn = [](Button& button, int index) {
		auto card = static_cast<Frame*>(button.getParent());
		stats[index]->sex = FEMALE;
		if (stats[index]->playerRace == RACE_INCUBUS) {
			auto incubus = card->findButton("Incubus");
			if (incubus) {
				incubus->setPressed(false);
			}
			if (enabledDLCPack1) {
				stats[index]->playerRace = RACE_SUCCUBUS;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Succubus");
				}
				auto succubus = card->findButton("Succubus");
				if (succubus) {
					succubus->setPressed(true);
				}
				if (client_classes[index] == CLASS_PUNISHER && stats[index]->appearance == 0) {
					if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
						client_classes[index] = CLASS_MESMER;
						auto class_button = card->findButton("class");
						if (class_button) {
							class_button->setIcon("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_Mesmer_00.png");
						}
					}
					stats[index]->clearStats();
					initClass(index);
				}
			} else {
				stats[index]->playerRace = RACE_HUMAN;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Human");
				}
				auto human = card->findButton("Human");
				if (human) {
					human->setPressed(true);
				}
			}
		}
	};

	static Frame* initCharacterCard(int index, int height) {
		auto lobby = main_menu_frame->findFrame("lobby");
		assert(lobby);

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{-2 + 320 * index, Frame::virtualScreenY - height, 324, height});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);
		card->setOwner(index);

		return card;
	}

	void characterCardGameSettingsMenu(int index) {
		bool local = currentLobbyType == LobbyType::LobbyLocal;

		auto card = initCharacterCard(index, 664);

		static void (*back_fn)(int) = [](int index){
			characterCardLobbySettingsMenu(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("custom_difficulty"); assert(button);
			button->select();
		};

		switch (index) {
		case 0: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(0);}); break;
		case 1: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(1);}); break;
		case 2: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(2);}); break;
		case 3: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(3);}); break;
		}

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDifficulty_Window_01.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("CUSTOM DIFFICULTY");
		header->setJustify(Field::justify_t::CENTER);

		const char* game_settings_text[] = {
			"Disable Hunger",
			"Disable Random\nMinotaurs",
			"Enable Life Saving\nAmulet",
			"Keep Items on Death",
			"Disable Random Traps",
			"Disable Friendly Fire",
			"Enable Classic\nEndings",
			"Enable Hardcore\nDifficulty",
#ifndef NINTENDO
			"Enable Cheats",
#endif
		};

		int num_settings = sizeof(game_settings_text) / sizeof(game_settings_text[0]);

		for (int c = 0; c < num_settings; ++c) {
			auto label = card->addField((std::string("label") + std::to_string(c)).c_str(), 128);
			label->setSize(SDL_Rect{48, 60 + 50 * c, 194, 64});
			label->setFont(smallfont_outline);
			label->setText(game_settings_text[c]);
			label->setColor(makeColor(166, 123, 81, 255));
			label->setHJustify(Field::justify_t::LEFT);
			label->setVJustify(Field::justify_t::CENTER);

			auto setting = card->addButton((std::string("setting") + std::to_string(c)).c_str());
			setting->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/Fill_Checked_00.png");
			setting->setStyle(Button::style_t::STYLE_CHECKBOX);
			setting->setSize(SDL_Rect{238, 66 + 50 * c, 44, 44});
			setting->setHighlightColor(0);
			setting->setBorderColor(0);
			setting->setBorder(0);
			setting->setColor(0);
			setting->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			setting->addWidgetAction("MenuStart", "confirm");
			setting->setWidgetBack("back_button");
			if (c > 0) {
				setting->setWidgetUp((std::string("setting") + std::to_string(c - 1)).c_str());
			}
			if (c < num_settings - 1) {
				setting->setWidgetDown((std::string("setting") + std::to_string(c + 1)).c_str());
			} else {
				setting->setWidgetDown("confirm");
			}
			if (c == 0) {
				setting->select();
			}

			switch (c) {
			case 0:
				setting->setPressed(!allSettings.hunger_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.hunger_enabled = !button.isPressed();});
				break;
			case 1:
				setting->setPressed(!allSettings.minotaur_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.minotaur_enabled = !button.isPressed();});
				break;
			case 2:
				setting->setPressed(allSettings.extra_life_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.extra_life_enabled = button.isPressed();});
				break;
			case 3:
				setting->setPressed(allSettings.keep_inventory_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.keep_inventory_enabled = button.isPressed();});
				break;
			case 4:
				setting->setPressed(!allSettings.random_traps_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.random_traps_enabled = !button.isPressed();});
				break;
			case 5:
				setting->setPressed(!allSettings.friendly_fire_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.friendly_fire_enabled = !button.isPressed();});
				break;
			case 6:
				setting->setPressed(allSettings.classic_mode_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.classic_mode_enabled = button.isPressed();});
				break;
			case 7:
				setting->setPressed(allSettings.hardcore_mode_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.hardcore_mode_enabled = button.isPressed();});
				break;
			case 8:
				setting->setPressed(allSettings.cheats_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.cheats_enabled = button.isPressed();});
				break;
			}
		}

		auto achievements = card->addField("achievements", 256);
		achievements->setSize(SDL_Rect{54, 526, 214, 50});
		achievements->setFont(smallfont_no_outline);
		achievements->setJustify(Field::justify_t::CENTER);
		achievements->setTickCallback([](Widget& widget){
			Field* achievements = static_cast<Field*>(&widget);
			if (allSettings.cheats_enabled ||
				allSettings.extra_life_enabled ||
				gamemods_disableSteamAchievements) {
				achievements->setColor(makeColor(180, 37, 37, 255));
				achievements->setText("ACHIEVEMENTS DISABLED");
			} else {
				achievements->setColor(makeColor(40, 180, 37, 255));
				achievements->setText("ACHIEVEMENTS ENABLED");
			}
			});
		(*achievements->getTickCallback())(*achievements);

		static auto confirmFlags = [](){
			soundActivate();
			svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
			svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
			svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
			svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
			svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
			svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
			svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
			svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
			svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
		};

		auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(127, 127, 127, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDiff_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 606, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		confirm->setWidgetUp((std::string("setting") + std::to_string(num_settings - 1)).c_str());
		switch (index) {
		case 0: confirm->setCallback([](Button&){confirmFlags(); back_fn(0);}); break;
		case 1: confirm->setCallback([](Button&){confirmFlags(); back_fn(1);}); break;
		case 2: confirm->setCallback([](Button&){confirmFlags(); back_fn(2);}); break;
		case 3: confirm->setCallback([](Button&){confirmFlags(); back_fn(3);}); break;
		}
	}

	void characterCardLobbySettingsMenu(int index) {
		bool local = currentLobbyType == LobbyType::LobbyLocal;

		auto card = initCharacterCard(index, 580);

		static void (*back_fn)(int) = [](int index){
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("game_settings"); assert(button);
			button->select();
		};

		switch (index) {
		case 0: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(0);}); break;
		case 1: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(1);}); break;
		case 2: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(2);}); break;
		case 3: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(3);}); break;
		}

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_Window_01.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("GAME SETTINGS");
		header->setJustify(Field::justify_t::CENTER);

		auto difficulty_header = card->addField("difficulty_header", 64);
		difficulty_header->setSize(SDL_Rect{78, 62, 166, 26});
		difficulty_header->setFont(smallfont_outline);
		difficulty_header->setText("DIFFICULTY SETTINGS");
		difficulty_header->setJustify(Field::justify_t::CENTER);

		auto easy_label = card->addField("easy_label", 64);
		easy_label->setSize(SDL_Rect{78, 102, 134, 26});
		easy_label->setFont(smallfont_outline);
		easy_label->setText("Practice");
		easy_label->setJustify(Field::justify_t::CENTER);
		easy_label->setColor(makeColor(83, 166, 98, 255));

		static const char* modes[] = {
			"easy", "normal", "hard", "custom"
		};

		static auto clear_difficulties = [](Button& button){
			Frame* frame = static_cast<Frame*>(button.getParent());
			for (auto mode : modes) {
				if (strcmp(button.getName(), mode) == 0) {
					continue;
				}
				auto other_button = frame->findButton(mode); assert(other_button);
				other_button->setPressed(false);
			}
		};

		auto easy = card->addButton("easy");
		easy->setSize(SDL_Rect{210, 100, 30, 30});
		easy->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		easy->setStyle(Button::style_t::STYLE_RADIO);
		easy->setBorder(0);
		easy->setColor(0);
		easy->setBorderColor(0);
		easy->setHighlightColor(0);
		easy->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		easy->addWidgetAction("MenuStart", "confirm");
		easy->setWidgetBack("back_button");
		easy->setWidgetDown("normal");
		easy->setCallback([](Button& button){
			clear_difficulties(button);
			svFlags = SV_FLAG_CLASSIC | SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING;
			});
		if (svFlags == (SV_FLAG_CLASSIC | SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING)) {
			easy->setPressed(true);
			easy->select();
		}

		auto normal_label = card->addField("normal_label", 64);
		normal_label->setSize(SDL_Rect{68, 140, 144, 26});
		normal_label->setFont(smallfont_outline);
		normal_label->setText("Normal");
		normal_label->setVJustify(Field::justify_t::TOP);
		normal_label->setHJustify(Field::justify_t::CENTER);
		normal_label->setColor(makeColor(206, 165, 40, 255));

		auto normal = card->addButton("normal");
		normal->setSize(SDL_Rect{210, 138, 30, 30});
		normal->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		normal->setStyle(Button::style_t::STYLE_RADIO);
		normal->setBorder(0);
		normal->setColor(0);
		normal->setBorderColor(0);
		normal->setHighlightColor(0);
		normal->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		normal->addWidgetAction("MenuStart", "confirm");
		normal->setWidgetBack("back_button");
		normal->setWidgetUp("easy");
		normal->setWidgetDown("hard");
		normal->setCallback([](Button& button){
			clear_difficulties(button);
			svFlags = SV_FLAG_FRIENDLYFIRE | SV_FLAG_KEEPINVENTORY | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS;
			});
		if (svFlags == (SV_FLAG_FRIENDLYFIRE | SV_FLAG_KEEPINVENTORY | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS)) {
			normal->setPressed(true);
			normal->select();
		}

		// TODO on non-english languages, normal text must be used

		/*auto hard_label = card->addField("hard_label", 64);
		hard_label->setSize(SDL_Rect{68, 178, 144, 26});
		hard_label->setFont(smallfont_outline);
		hard_label->setText("Nightmare");
		hard_label->setVJustify(Field::justify_t::TOP);
		hard_label->setHJustify(Field::justify_t::CENTER);
		hard_label->setColor(makeColor(124, 15, 10, 255));*/

		auto hard_label = card->addImage(
			SDL_Rect{84, 158, 120, 50},
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_Text_Nightmare_00.png",
			"hard_label"
		);

		auto hard = card->addButton("hard");
		hard->setSize(SDL_Rect{210, 176, 30, 30});
		hard->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		hard->setStyle(Button::style_t::STYLE_RADIO);
		hard->setBorder(0);
		hard->setColor(0);
		hard->setBorderColor(0);
		hard->setHighlightColor(0);
		hard->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		hard->addWidgetAction("MenuStart", "confirm");
		hard->setWidgetBack("back_button");
		hard->setWidgetUp("normal");
		hard->setWidgetDown("custom");
		hard->setCallback([](Button& button){
			clear_difficulties(button);
			svFlags = SV_FLAG_HARDCORE | SV_FLAG_FRIENDLYFIRE | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS;
			});
		if (svFlags == (SV_FLAG_HARDCORE | SV_FLAG_FRIENDLYFIRE | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS)) {
			hard->setPressed(true);
			hard->select();
		}

		auto custom_difficulty = card->addButton("custom_difficulty");
		custom_difficulty->setColor(makeColor(127, 127, 127, 255));
		custom_difficulty->setHighlightColor(makeColor(255, 255, 255, 255));
		custom_difficulty->setSize(SDL_Rect{84, 210, 114, 40});
		custom_difficulty->setBackground("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_GameSettings_Button_Custom_01.png");
		custom_difficulty->setFont(smallfont_outline);
		custom_difficulty->setText("Custom");
		custom_difficulty->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		custom_difficulty->addWidgetAction("MenuStart", "confirm");
		custom_difficulty->setWidgetBack("back_button");
		custom_difficulty->setWidgetUp("hard");
		custom_difficulty->setWidgetDown("invite");
		custom_difficulty->setWidgetRight("custom");
		switch (index) {
		case 0: custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameSettingsMenu(0);}); break;
		case 1: custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameSettingsMenu(1);}); break;
		case 2: custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameSettingsMenu(2);}); break;
		case 3: custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameSettingsMenu(3);}); break;
		}

		auto custom = card->addButton("custom");
		custom->setSize(SDL_Rect{210, 216, 30, 30});
		custom->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		custom->setStyle(Button::style_t::STYLE_RADIO);
		custom->setBorder(0);
		custom->setColor(0);
		custom->setBorderColor(0);
		custom->setHighlightColor(0);
		custom->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		custom->addWidgetAction("MenuStart", "confirm");
		custom->setWidgetBack("back_button");
		custom->setWidgetUp("hard");
		if (local) {
			custom->setWidgetDown("confirm");
		} else {
			custom->setWidgetDown("invite");
		}
		custom->setWidgetLeft("custom_difficulty");
		custom->setCallback([](Button& button){
			clear_difficulties(button);
			svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
			svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
			svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
			svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
			svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
			svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
			svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
			svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
			svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
			});
		if (!easy->isSelected() && !normal->isSelected() && !hard->isSelected()) {
			custom->setPressed(true);
			custom->select();
		}

		auto achievements = card->addField("achievements", 256);
		achievements->setSize(SDL_Rect{54, 260, 214, 50});
		achievements->setFont(smallfont_no_outline);
		achievements->setJustify(Field::justify_t::CENTER);
		achievements->setTickCallback([](Widget& widget){
			Field* achievements = static_cast<Field*>(&widget);
			if ((svFlags & SV_FLAG_CHEATS) ||
				(svFlags & SV_FLAG_LIFESAVING) ||
				gamemods_disableSteamAchievements) {
				achievements->setColor(makeColor(180, 37, 37, 255));
				achievements->setText("ACHIEVEMENTS DISABLED");
			} else {
				achievements->setColor(makeColor(40, 180, 37, 255));
				achievements->setText("ACHIEVEMENTS ENABLED");
			}
			});
		(*achievements->getTickCallback())(*achievements);

		auto multiplayer_header = card->addField("difficulty_header", 64);
		multiplayer_header->setSize(SDL_Rect{70, 328, 182, 34});
		multiplayer_header->setFont(smallfont_outline);
		multiplayer_header->setText("DIFFICULTY SETTINGS");
		multiplayer_header->setJustify(Field::justify_t::CENTER);

		auto invite_label = card->addField("invite_label", 64);
		invite_label->setSize(SDL_Rect{68, 390, 146, 26});
		invite_label->setFont(smallfont_outline);
		invite_label->setText("Invite Only");
		invite_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			invite_label->setColor(makeColor(70, 62, 59, 255));

			auto invite = card->addImage(
				SDL_Rect{214, 390, 26, 26},
				0xffffffff,
				"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"invite"
			);
		} else {
			invite_label->setColor(makeColor(166, 123, 81, 255));

			auto invite = card->addButton("invite");
			invite->setSize(SDL_Rect{212, 388, 30, 30});
			invite->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			invite->setStyle(Button::style_t::STYLE_RADIO);
			invite->setBorder(0);
			invite->setColor(0);
			invite->setBorderColor(0);
			invite->setHighlightColor(0);
			invite->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			invite->addWidgetAction("MenuStart", "confirm");
			invite->setWidgetBack("back_button");
			invite->setWidgetUp("custom");
			invite->setWidgetDown("friends");
			if (index != 0) {
				invite->setCallback([](Button&){soundError();});
			} else {
			}
		}

		auto friends_label = card->addField("friends_label", 64);
		friends_label->setSize(SDL_Rect{68, 428, 146, 26});
		friends_label->setFont(smallfont_outline);
		friends_label->setText("Friends Only");
		friends_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			friends_label->setColor(makeColor(70, 62, 59, 255));

			auto friends = card->addImage(
				SDL_Rect{214, 428, 26, 26},
				0xffffffff,
				"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"friends"
			);
		} else {
			friends_label->setColor(makeColor(166, 123, 81, 255));

			auto friends = card->addButton("friends");
			friends->setSize(SDL_Rect{212, 426, 30, 30});
			friends->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			friends->setStyle(Button::style_t::STYLE_RADIO);
			friends->setBorder(0);
			friends->setColor(0);
			friends->setBorderColor(0);
			friends->setHighlightColor(0);
			friends->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			friends->addWidgetAction("MenuStart", "confirm");
			friends->setWidgetBack("back_button");
			friends->setWidgetUp("invite");
			friends->setWidgetDown("open");
			if (index != 0) {
				friends->setCallback([](Button&){soundError();});
			} else {
			}
		}

		auto open_label = card->addField("open_label", 64);
		open_label->setSize(SDL_Rect{68, 466, 146, 26});
		open_label->setFont(smallfont_outline);
		open_label->setText("Open Lobby");
		open_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			open_label->setColor(makeColor(70, 62, 59, 255));

			auto open = card->addImage(
				SDL_Rect{214, 466, 26, 26},
				0xffffffff,
				"images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"open"
			);
		} else {
			open_label->setColor(makeColor(166, 123, 81, 255));

			auto open = card->addButton("open");
			open->setSize(SDL_Rect{212, 464, 30, 30});
			open->setIcon("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			open->setStyle(Button::style_t::STYLE_RADIO);
			open->setBorder(0);
			open->setColor(0);
			open->setBorderColor(0);
			open->setHighlightColor(0);
			open->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			open->addWidgetAction("MenuStart", "confirm");
			open->setWidgetBack("back_button");
			open->setWidgetUp("friends");
			open->setWidgetDown("confirm");
			if (index != 0) {
				open->setCallback([](Button&){soundError();});
			} else {
			}
		}

		auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(127, 127, 127, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 522, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		if (local) {
			confirm->setWidgetUp("custom");
		} else {
			confirm->setWidgetUp("open");
		}
		switch (index) {
		case 0: confirm->setCallback([](Button&){soundActivate(); back_fn(0);}); break;
		case 1: confirm->setCallback([](Button&){soundActivate(); back_fn(1);}); break;
		case 2: confirm->setCallback([](Button&){soundActivate(); back_fn(2);}); break;
		case 3: confirm->setCallback([](Button&){soundActivate(); back_fn(3);}); break;
		}
	}

	void characterCardRaceMenu(int index) {
		auto card = initCharacterCard(index, 488);

		static void (*back_fn)(int) = [](int index){
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("race"); assert(button);
			button->select();
		};

		switch (index) {
		case 0: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(0);}); break;
		case 1: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(1);}); break;
		case 2: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(2);}); break;
		case 3: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(3);}); break;
		}

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_Window_01.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("RACE SELECTION");
		header->setJustify(Field::justify_t::CENTER);

		static const char* dlcRaces1[] = {
			"Skeleton",
			"Vampire",
			"Succubus",
			"Goatman"
		};

		static const char* dlcRaces2[] = {
			"Automaton",
			"Incubus",
			"Goblin",
			"Insectoid"
		};

		static auto race_fn = [](Button& button, int index){
			Frame* frame = static_cast<Frame*>(button.getParent());
			std::vector<const char*> allRaces = { "Human" };
			allRaces.insert(allRaces.end(), std::begin(dlcRaces1), std::end(dlcRaces1));
			allRaces.insert(allRaces.end(), std::begin(dlcRaces2), std::end(dlcRaces2));
			for (int c = 0; c < allRaces.size(); ++c) {
				auto race = allRaces[c];
				if (strcmp(button.getName(), race) == 0) {
					stats[index]->playerRace = c;
					if (stats[index]->playerRace == RACE_SUCCUBUS) {
						stats[index]->sex = FEMALE;
					}
					else if (stats[index]->playerRace == RACE_INCUBUS) {
						stats[index]->sex = MALE;
					}
				} else {
					auto other_button = frame->findButton(race);
					other_button->setPressed(false);
				}
			}
		};

		auto human = card->addButton("Human");
		human->setSize(SDL_Rect{54, 80, 30, 30});
		human->setIcon("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/Fill_Round_00.png");
		human->setStyle(Button::style_t::STYLE_RADIO);
		human->setBorder(0);
		human->setColor(0);
		human->setBorderColor(0);
		human->setHighlightColor(0);
		human->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		human->addWidgetAction("MenuStart", "confirm");
		human->setWidgetBack("back_button");
		human->setWidgetRight("appearances");
		if (enabledDLCPack1) {
			human->setWidgetDown(dlcRaces1[0]);
		}
		else if (enabledDLCPack2) {
			human->setWidgetDown(dlcRaces2[0]);
		}
		else {
			human->setWidgetDown("disable_abilities");
		}
		switch (index) {
		case 0: human->setCallback([](Button& button){soundToggle(); race_fn(button, 0);}); break;
		case 1: human->setCallback([](Button& button){soundToggle(); race_fn(button, 1);}); break;
		case 2: human->setCallback([](Button& button){soundToggle(); race_fn(button, 2);}); break;
		case 3: human->setCallback([](Button& button){soundToggle(); race_fn(button, 3);}); break;
		}
		if (stats[index]->playerRace == RACE_HUMAN) {
			human->setPressed(true);
			human->select();
		}

		auto human_label = card->addField("human_label", 32);
		human_label->setColor(makeColor(166, 123, 81, 255));
		human_label->setText("Human");
		human_label->setFont(smallfont_outline);
		human_label->setSize(SDL_Rect{86, 78, 66, 36});
		human_label->setHJustify(Button::justify_t::LEFT);
		human_label->setVJustify(Button::justify_t::CENTER);

		auto appearances = card->addFrame("appearances");
		appearances->setSize(SDL_Rect{152, 78, 122, 36});
		appearances->setActualSize(SDL_Rect{0, 4, 122, 36});
		appearances->setFont(smallfont_outline);
		appearances->setBorder(0);
		appearances->setListOffset(SDL_Rect{12, 8, 0, 0});
		appearances->setScrollBarsEnabled(false);
		appearances->setAllowScrollBinds(false);
		appearances->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		appearances->addWidgetMovement("MenuListCancel", "appearances");
		appearances->addWidgetMovement("MenuListConfirm", "appearances");
		appearances->addWidgetAction("MenuStart", "confirm");
		appearances->setWidgetBack("back_button");
		appearances->setWidgetLeft("Human");
		if (enabledDLCPack2) {
			appearances->setWidgetDown(dlcRaces2[0]);
		}
		else if (enabledDLCPack1) {
			appearances->setWidgetDown(dlcRaces1[0]);
		}
		else {
			appearances->setWidgetDown("disable_abilities");
		}
		appearances->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			auto card = static_cast<Frame*>(frame->getParent());
			auto backdrop = frame->findImage("background"); assert(backdrop);
			auto box = frame->findImage("selection_box"); assert(box);
			box->disabled = !frame->isSelected();
			box->pos.y = frame->getActualSize().y;
			backdrop->pos.y = frame->getActualSize().y + 4;
			auto appearance_uparrow = card->findButton("appearance_uparrow");
			appearance_uparrow->setDisabled(!frame->isActivated());
			appearance_uparrow->setInvisible(!frame->isActivated());
			auto appearance_downarrow = card->findButton("appearance_downarrow");
			appearance_downarrow->setDisabled(!frame->isActivated());
			appearance_downarrow->setInvisible(!frame->isActivated());
			});

		auto appearance_backdrop = appearances->addImage(
			SDL_Rect{2, 4, 118, 28},
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_TextBack_00.png",
			"background"
		);

		auto appearance_selected = appearances->addImage(
			SDL_Rect{0, 0, 122, 36},
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_Textbox_00.png",
			"selection_box"
		);
		appearance_selected->disabled = true;
		appearance_selected->ontop = true;

		// TODO give these callbacks so they can be clicked:

		auto appearance_uparrow = card->addButton("appearance_uparrow");
		appearance_uparrow->setSize(SDL_Rect{198, 58, 32, 20});
		appearance_uparrow->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonUp_00.png");
		appearance_uparrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_uparrow->setColor(makeColor(223, 223, 223, 255));
		appearance_uparrow->setDisabled(true);
		appearance_uparrow->setInvisible(true);

		auto appearance_downarrow = card->addButton("appearance_downarrow");
		appearance_downarrow->setSize(SDL_Rect{198, 114, 32, 20});
		appearance_downarrow->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonDown_00.png");
		appearance_downarrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_downarrow->setColor(makeColor(223, 223, 223, 255));
		appearance_downarrow->setDisabled(true);
		appearance_downarrow->setInvisible(true);

		static const char* appearance_names[] = {
			"Landguard", "Northborn", "Firebrand", "Hardbred",
			"Highwatch", "Gloomforge", "Pyrebloom", "Snakestone",
			"Windclan", "Warblood", "Millbound", "Sunstalk",
			"Claymount", "Stormward", "Tradefell", "Nighthill",
			"Baytower", "Whetsong"
		};

		constexpr int num_appearances = sizeof(appearance_names) / sizeof(appearance_names[0]);

		static auto appearance_fn = [](Frame::entry_t& entry, int index){
			if (stats[index]->playerRace != RACE_HUMAN) {
				return;
			}
			stats[index]->appearance = std::stoi(entry.name);
		};

		for (int c = 0; c < num_appearances; ++c) {
			auto name = appearance_names[c];
			auto entry = appearances->addEntry(std::to_string(c).c_str(), true);
			entry->color = makeColor(166, 123, 81, 255);
			entry->text = name;
			switch (index) {
			case 0: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 0);}; break;
			case 1: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 1);}; break;
			case 2: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 2);}; break;
			case 3: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 3);}; break;
			}
		}

		if (enabledDLCPack1) {
			for (int c = 0; c < 4; ++c) {
				auto race = card->addButton(dlcRaces1[c]);
				race->setSize(SDL_Rect{38, 130 + 38 * c, 30, 30});
				race->setIcon("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/Fill_Round_00.png");
				race->setStyle(Button::style_t::STYLE_RADIO);
				race->setBorder(0);
				race->setColor(0);
				race->setBorderColor(0);
				race->setHighlightColor(0);
				race->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
				race->addWidgetAction("MenuStart", "confirm");
				race->setWidgetBack("back_button");
				if (enabledDLCPack2) {
					race->setWidgetRight(dlcRaces2[c]);
				}
				if (c > 0) {
					race->setWidgetUp(dlcRaces1[c - 1]);
				} else {
					race->setWidgetUp("Human");
				}
				if (c < 3) {
					race->setWidgetDown(dlcRaces1[c + 1]);
				} else {
					race->setWidgetDown("disable_abilities");
				}
				switch (index) {
				case 0: race->setCallback([](Button& button){soundToggle(); race_fn(button, 0);}); break;
				case 1: race->setCallback([](Button& button){soundToggle(); race_fn(button, 1);}); break;
				case 2: race->setCallback([](Button& button){soundToggle(); race_fn(button, 2);}); break;
				case 3: race->setCallback([](Button& button){soundToggle(); race_fn(button, 3);}); break;
				}

				if (stats[index]->playerRace == RACE_SKELETON + c) {
					race->setPressed(true);
					race->select();
				}

				auto label = card->addField((std::string(dlcRaces1[c]) + "_label").c_str(), 64);
				label->setSize(SDL_Rect{70, 132 + 38 * c, 104, 26});
				label->setVJustify(Field::justify_t::CENTER);
				label->setHJustify(Field::justify_t::LEFT);
				label->setColor(makeColor(180, 133, 13, 255));
				label->setFont(smallfont_outline);
				label->setText(dlcRaces1[c]);
			}
		} else {
			auto blockers = card->addImage(
				SDL_Rect{40, 132, 26, 140},
				0xffffffff,
				"images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_Blocker_00.png",
				"blockers"
			);
			for (int c = 0; c < 4; ++c) {
				auto label = card->addField((std::string(dlcRaces1[c]) + "_label").c_str(), 64);
				label->setSize(SDL_Rect{70, 132 + 38 * c, 104, 26});
				label->setVJustify(Field::justify_t::CENTER);
				label->setHJustify(Field::justify_t::LEFT);
				label->setColor(makeColor(70, 62, 59, 255));
				label->setFont(smallfont_outline);
				label->setText(dlcRaces1[c]);
			}
		}

		if (enabledDLCPack2) {
			for (int c = 0; c < 4; ++c) {
				auto race = card->addButton(dlcRaces2[c]);
				race->setSize(SDL_Rect{172, 130 + 38 * c, 30, 30});
				race->setIcon("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/Fill_Round_00.png");
				race->setStyle(Button::style_t::STYLE_RADIO);
				race->setBorder(0);
				race->setColor(0);
				race->setBorderColor(0);
				race->setHighlightColor(0);
				race->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
				race->addWidgetAction("MenuStart", "confirm");
				race->setWidgetBack("back_button");
				if (enabledDLCPack1) {
					race->setWidgetLeft(dlcRaces1[c]);
				}
				if (c > 0) {
					race->setWidgetUp(dlcRaces2[c - 1]);
				} else {
					race->setWidgetUp("appearances");
				}
				if (c < 3) {
					race->setWidgetDown(dlcRaces2[c + 1]);
				} else {
					race->setWidgetDown("disable_abilities");
				}
				switch (index) {
				case 0: race->setCallback([](Button& button){soundToggle(); race_fn(button, 0);}); break;
				case 1: race->setCallback([](Button& button){soundToggle(); race_fn(button, 1);}); break;
				case 2: race->setCallback([](Button& button){soundToggle(); race_fn(button, 2);}); break;
				case 3: race->setCallback([](Button& button){soundToggle(); race_fn(button, 3);}); break;
				}

				if (stats[index]->playerRace == RACE_AUTOMATON + c) {
					race->setPressed(true);
					race->select();
				}

				auto label = card->addField((std::string(dlcRaces2[c]) + "_label").c_str(), 64);
				label->setSize(SDL_Rect{202, 132 + 38 * c, 104, 26});
				label->setVJustify(Field::justify_t::CENTER);
				label->setHJustify(Field::justify_t::LEFT);
				label->setColor(makeColor(223, 44, 149, 255));
				label->setFont(smallfont_outline);
				label->setText(dlcRaces2[c]);
			}
		} else {
			auto blockers = card->addImage(
				SDL_Rect{174, 132, 26, 140},
				0xffffffff,
				"images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_Blocker_00.png",
				"blockers"
			);
			for (int c = 0; c < 4; ++c) {
				auto label = card->addField((std::string(dlcRaces2[c]) + "_label").c_str(), 64);
				label->setSize(SDL_Rect{202, 132 + 38 * c, 104, 26});
				label->setVJustify(Field::justify_t::CENTER);
				label->setHJustify(Field::justify_t::LEFT);
				label->setColor(makeColor(70, 62, 59, 255));
				label->setFont(smallfont_outline);
				label->setText(dlcRaces2[c]);
			}
		}

		auto disable_abilities_text = card->addField("disable_abilities_text", 256);
		disable_abilities_text->setSize(SDL_Rect{44, 274, 154, 64});
		disable_abilities_text->setFont(smallfont_outline);
		disable_abilities_text->setColor(makeColor(166, 123, 81, 255));
		disable_abilities_text->setText("Disable monster\nrace abilities");
		disable_abilities_text->setHJustify(Field::justify_t::LEFT);
		disable_abilities_text->setVJustify(Field::justify_t::CENTER);

		auto disable_abilities = card->addButton("disable_abilities");
		disable_abilities->setSize(SDL_Rect{198, 290, 32, 32});
		disable_abilities->setIcon("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/Fill_Checked_00.png");
		disable_abilities->setColor(0);
		disable_abilities->setBorderColor(0);
		disable_abilities->setBorder(0);
		disable_abilities->setHighlightColor(0);
		disable_abilities->setStyle(Button::style_t::STYLE_CHECKBOX);
		disable_abilities->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		disable_abilities->addWidgetAction("MenuStart", "confirm");
		disable_abilities->setWidgetBack("back_button");
		disable_abilities->setWidgetDown("show_race_info");
		if (enabledDLCPack2) {
			disable_abilities->setWidgetUp(dlcRaces2[sizeof(dlcRaces2) / sizeof(dlcRaces2[0]) - 1]);
		}
		else if (enabledDLCPack1) {
			disable_abilities->setWidgetUp(dlcRaces1[sizeof(dlcRaces1) / sizeof(dlcRaces1[0]) - 1]);
		}
		else {
			disable_abilities->setWidgetUp("appearances");
		}
		if (stats[index]->playerRace != RACE_HUMAN) {
			disable_abilities->setPressed(stats[index]->appearance != 0);
		}
		static auto disable_abilities_fn = [](Button& button, int index){
			soundCheckmark();
			if (stats[index]->playerRace != RACE_HUMAN) {
				stats[index]->appearance = button.isPressed() ? 1 : 0;
			}
		};
		switch (index) {
		case 0: disable_abilities->setCallback([](Button& button){disable_abilities_fn(button, 0);}); break;
		case 1: disable_abilities->setCallback([](Button& button){disable_abilities_fn(button, 1);}); break;
		case 2: disable_abilities->setCallback([](Button& button){disable_abilities_fn(button, 2);}); break;
		case 3: disable_abilities->setCallback([](Button& button){disable_abilities_fn(button, 3);}); break;
		}

		auto male_button = card->addButton("male");
		male_button->setColor(makeColor(127, 127, 127, 255));
		male_button->setHighlightColor(makeColor(255, 255, 255, 255));
		male_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMale_00.png");
		male_button->setSize(SDL_Rect{44, 344, 58, 52});
		male_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		male_button->addWidgetAction("MenuStart", "confirm");
		male_button->setWidgetBack("back_button");
		male_button->setWidgetUp("disable_abilities");
		male_button->setWidgetDown("confirm");
		male_button->setWidgetRight("female");
		switch (index) {
		case 0: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 0);}); break;
		case 1: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 1);}); break;
		case 2: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 2);}); break;
		case 3: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 3);}); break;
		}

		auto female_button = card->addButton("female");
		female_button->setColor(makeColor(127, 127, 127, 255));
		female_button->setHighlightColor(makeColor(255, 255, 255, 255));
		female_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemale_00.png");
		female_button->setSize(SDL_Rect{106, 344, 58, 52});
		female_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		female_button->addWidgetAction("MenuStart", "confirm");
		female_button->setWidgetBack("back_button");
		female_button->setWidgetUp("disable_abilities");
		female_button->setWidgetDown("confirm");
		female_button->setWidgetLeft("male");
		female_button->setWidgetRight("show_race_info");
		switch (index) {
		case 0: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 0);}); break;
		case 1: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 1);}); break;
		case 2: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 2);}); break;
		case 3: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 3);}); break;
		}

		auto show_race_info = card->addButton("show_race_info");
		show_race_info->setFont(smallfont_outline);
		show_race_info->setText("Show Race\nInfo");
		show_race_info->setColor(makeColor(127, 127, 127, 255));
		show_race_info->setHighlightColor(makeColor(255, 255, 255, 255));
		show_race_info->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonShowDetails_00.png");
		show_race_info->setSize(SDL_Rect{168, 344, 110, 52});
		show_race_info->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		show_race_info->addWidgetAction("MenuStart", "confirm");
		show_race_info->setWidgetBack("back_button");
		show_race_info->setWidgetUp("disable_abilities");
		show_race_info->setWidgetDown("confirm");
		show_race_info->setWidgetLeft("female");

		auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(127, 127, 127, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 430, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		confirm->setWidgetUp("female");
		switch (index) {
		case 0: confirm->setCallback([](Button&){soundActivate(); back_fn(0);}); break;
		case 1: confirm->setCallback([](Button&){soundActivate(); back_fn(1);}); break;
		case 2: confirm->setCallback([](Button&){soundActivate(); back_fn(2);}); break;
		case 3: confirm->setCallback([](Button&){soundActivate(); back_fn(3);}); break;
		}
	}

	void characterCardClassMenu(int index) {
		auto reduced_class_list = reducedClassList(index);
		auto card = initCharacterCard(index, 488);

		static void (*back_fn)(int) = [](int index){
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("class"); assert(button);
			button->select();
		};

		switch (index) {
		case 0: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(0);}); break;
		case 1: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(1);}); break;
		case 2: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(2);}); break;
		case 3: (void)createBackWidget(card,[](Button&){soundCancel(); back_fn(3);}); break;
		}

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Window_01.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{32, 14, 260, 38});
		header->setFont(smallfont_outline);
		header->setText("CLASS SELECTION");
		header->setJustify(Field::justify_t::CENTER);

		auto class_name_header = card->addField("class_name_header", 64);
		class_name_header->setSize(SDL_Rect{98, 70, 128, 26});
		class_name_header->setFont(smallfont_outline);
		class_name_header->setText("Fix this");
		class_name_header->setHJustify(Field::justify_t::CENTER);
		class_name_header->setVJustify(Field::justify_t::BOTTOM);

		auto textbox = card->addImage(
			SDL_Rect{46, 116, 186, 36},
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_SearchBar_00.png",
			"textbox"
		);

		static auto class_name_fn = [](Field& field, int index){
			int i = std::min(std::max(0, client_classes[index] + 1), num_classes);
			auto find = classes.find(classes_in_order[i]);
			if (find != classes.end()) {
				field.setText(find->second.name);
			}
		};

		auto class_name = card->addField("class_name", 64);
		class_name->setSize(SDL_Rect{48, 120, 182, 28});
		class_name->setHJustify(Field::justify_t::LEFT);
		class_name->setVJustify(Field::justify_t::CENTER);
		class_name->setFont(smallfont_outline);
		switch (index) {
		case 0: class_name->setTickCallback([](Widget& widget){class_name_fn(*static_cast<Field*>(&widget), 0);}); break;
		case 1: class_name->setTickCallback([](Widget& widget){class_name_fn(*static_cast<Field*>(&widget), 1);}); break;
		case 2: class_name->setTickCallback([](Widget& widget){class_name_fn(*static_cast<Field*>(&widget), 2);}); break;
		case 3: class_name->setTickCallback([](Widget& widget){class_name_fn(*static_cast<Field*>(&widget), 3);}); break;
		}
		(*class_name->getTickCallback())(*class_name);

		auto subframe = card->addFrame("subframe");
		subframe->setScrollBarsEnabled(false);
		subframe->setSize(SDL_Rect{34, 160, 226, 258});
		subframe->setActualSize(SDL_Rect{0, 0, 226, std::max(258, 6 + 54 * (int)(classes.size() / 4 + ((classes.size() % 4) ? 1 : 0)))});
		subframe->setHollow(true);
		subframe->setBorder(0);

		if (subframe->getActualSize().h > 258) {
			auto slider = card->addSlider("slider");
			slider->setRailSize(SDL_Rect{260, 160, 30, 266});
			slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
			slider->setRailImage("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_ScrollBar_00.png");
			slider->setHandleImage("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_ScrollBar_SliderB_00.png");
			slider->setOrientation(Slider::orientation_t::SLIDER_VERTICAL);
			slider->setMinValue(0.f);
			slider->setMaxValue(subframe->getActualSize().h - 258);
			slider->setCallback([](Slider& slider){
				Frame* frame = static_cast<Frame*>(slider.getParent());
				Frame* subframe = frame->findFrame("subframe"); assert(subframe);
				auto actualSize = subframe->getActualSize();
				actualSize.y = slider.getValue();
				subframe->setActualSize(actualSize);
				});
			slider->setTickCallback([](Widget& widget){
				Slider* slider = static_cast<Slider*>(&widget);
				Frame* frame = static_cast<Frame*>(slider->getParent());
				Frame* subframe = frame->findFrame("subframe"); assert(subframe);
				auto actualSize = subframe->getActualSize();
				slider->setValue(actualSize.y);
				});
		}

		auto class_info = card->addButton("class_info");
		class_info->setColor(makeColor(127, 127, 127, 255));
		class_info->setHighlightColor(makeColor(255, 255, 255, 255));
		class_info->setSize(SDL_Rect{236, 110, 48, 48});
		class_info->setBackground("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_MagnifyingGlass_00.png");
		class_info->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		class_info->addWidgetAction("MenuStart", "confirm");
		class_info->addWidgetAction("MenuAlt1", "class_info");
		class_info->setWidgetBack("back_button");

		const std::string prefix = "images/ui/Main Menus/Play/PlayerCreation/ClassSelection/";
		for (int c = reduced_class_list.size() - 1; c >= 0; --c) {
			auto name = reduced_class_list[c];
			auto find = classes.find(name);
			assert(find != classes.end());
			auto& full_class = find->second;
			auto button = subframe->addButton(name);
			switch (full_class.dlc) {
			case DLC::Base: button->setBackground((prefix + "ClassSelect_IconBGBase_00.png").c_str()); break;
			case DLC::MythsAndOutcasts: button->setBackground((prefix + "ClassSelect_IconBGMyths_00.png").c_str()); break;
			case DLC::LegendsAndPariahs: button->setBackground((prefix + "ClassSelect_IconBGLegends_00.png").c_str()); break;
			}
			button->setIcon((prefix + full_class.image).c_str());
			button->setSize(SDL_Rect{8 + (c % 4) * 54, 6 + (c / 4) * 54, 54, 54});
			button->setColor(makeColor(127, 127, 127, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			if (c > 0) {
				button->setWidgetLeft(reduced_class_list[c - 1]);
			}
			if (c < reduced_class_list.size() - 1) {
				button->setWidgetRight(reduced_class_list[c + 1]);
			}
			if (c > 3) {
				button->setWidgetUp(reduced_class_list[c - 4]);
			} else {
				button->setWidgetUp(reduced_class_list[0]);
			}
			if (c < reduced_class_list.size() - 4) {
				button->setWidgetDown(reduced_class_list[c + 4]);
			} else {
				button->setWidgetDown(reduced_class_list[reduced_class_list.size() - 1]);
			}
			button->addWidgetAction("MenuStart", "confirm");
			button->addWidgetAction("MenuAlt1", "class_info");
			button->setWidgetBack("back_button");

			static auto button_fn = [](Button& button, int index){
				soundActivate();
				int c = 0;
				for (; c < num_classes; ++c) {
					if (strcmp(button.getName(), classes_in_order[c]) == 0) {
						break;
					}
				}
				if (c >= 1) {
					--c; // exclude the random "class"
					client_classes[index] = c;
				} else {
					auto reduced_class_list = reducedClassList(index);
					auto random_class = reduced_class_list[(rand() % (reduced_class_list.size() - 1)) + 1];
					for (int c = 0; c < num_classes; ++c) {
						if (strcmp(random_class, classes_in_order[c]) == 0) {
							client_classes[index] = c;
							break;
						}
					}
				}
				stats[index]->clearStats();
				initClass(index);
			};

			switch (index) {
			case 0: button->setCallback([](Button& button){button_fn(button, 0);}); break;
			case 1: button->setCallback([](Button& button){button_fn(button, 0);}); break;
			case 2: button->setCallback([](Button& button){button_fn(button, 0);}); break;
			case 3: button->setCallback([](Button& button){button_fn(button, 0);}); break;
			}
		}

		auto first_button = subframe->findButton(reduced_class_list[0]); assert(first_button);
		first_button->select();

		auto confirm = card->addButton("confirm");
		confirm->setColor(makeColor(127, 127, 127, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		confirm->setSize(SDL_Rect{62, 430, 202, 52});
		confirm->setText("Confirm");
		confirm->setFont(bigfont_outline);
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->addWidgetAction("MenuAlt1", "class_info");
		confirm->setWidgetBack("back_button");
		switch (index) {
		case 0: confirm->setCallback([](Button&){soundActivate(); back_fn(0);}); break;
		case 1: confirm->setCallback([](Button&){soundActivate(); back_fn(1);}); break;
		case 2: confirm->setCallback([](Button&){soundActivate(); back_fn(2);}); break;
		case 3: confirm->setCallback([](Button&){soundActivate(); back_fn(3);}); break;
		}
	}

	void createCharacterCard(int index) {
		auto lobby = main_menu_frame->findFrame("lobby");
		assert(lobby);

		auto card = initCharacterCard(index, 346);

		if (currentLobbyType == LobbyType::LobbyLocal) {
			switch (index) {
			case 0: (void)createBackWidget(card,[](Button& button){soundCancel(); createStartButton(0);}); break;
			case 1: (void)createBackWidget(card,[](Button& button){soundCancel(); createStartButton(1);}); break;
			case 2: (void)createBackWidget(card,[](Button& button){soundCancel(); createStartButton(2);}); break;
			case 3: (void)createBackWidget(card,[](Button& button){soundCancel(); createStartButton(3);}); break;
			}
		} else {
			switch (index) {
			case 0: (void)createBackWidget(card,[](Button& button){soundCancel(); createInviteButton(0);}); break;
			case 1: (void)createBackWidget(card,[](Button& button){soundCancel(); createInviteButton(1);}); break;
			case 2: (void)createBackWidget(card,[](Button& button){soundCancel(); createInviteButton(2);}); break;
			case 3: (void)createBackWidget(card,[](Button& button){soundCancel(); createInviteButton(3);}); break;
			}
		}

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/Finalize_Window_01.png",
			"backdrop"
		);

		auto name_text = card->addField("name_text", 32);
		name_text->setSize(SDL_Rect{30, 30, 56, 36});
		name_text->setFont(smallfont_outline);
		name_text->setColor(makeColor(166, 123, 81, 255));
		name_text->setText("NAME:");
		name_text->setHJustify(Field::justify_t::RIGHT);
		name_text->setVJustify(Field::justify_t::CENTER);

		auto name_box = card->addImage(
			SDL_Rect{88, 30, 150, 36},
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/Finalize__NameField_00.png",
			"name_box"
		);

		auto name_field = card->addField("name", 128);
		name_field->setGuide((std::string("Enter a name for Player ") + std::to_string(index + 1)).c_str());
		name_field->setFont(smallfont_outline);
		name_field->setText(stats[0]->name);
		name_field->setSize(SDL_Rect{90, 34, 146, 28});
		name_field->setColor(makeColor(166, 123, 81, 255));
		name_field->setHJustify(Field::justify_t::LEFT);
		name_field->setVJustify(Field::justify_t::CENTER);
		name_field->setEditable(true);
		name_field->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		name_field->addWidgetAction("MenuStart", "ready");
		name_field->setWidgetBack("back_button");
		name_field->setWidgetRight("randomize_name");
		name_field->setWidgetDown("game_settings");
		static auto name_field_fn = [](const char* text, int index) {
			size_t len = strlen(text);
			len = std::min(sizeof(Stat::name) - 1, len);
			memcpy(stats[index]->name, text, len);
			stats[index]->name[len] = '\0';
		};
		switch (index) {
		case 0: name_field->setCallback([](Field& field){name_field_fn(field.getText(), 0);}); break;
		case 1: name_field->setCallback([](Field& field){name_field_fn(field.getText(), 1);}); break;
		case 2: name_field->setCallback([](Field& field){name_field_fn(field.getText(), 2);}); break;
		case 3: name_field->setCallback([](Field& field){name_field_fn(field.getText(), 3);}); break;
		}
		name_field->select();

		auto randomize_name = card->addButton("randomize_name");
		randomize_name->setColor(makeColor(127, 127, 127, 255));
		randomize_name->setHighlightColor(makeColor(255, 255, 255, 255));
		randomize_name->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_Randomize_00.png");
		randomize_name->setSize(SDL_Rect{244, 26, 40, 44});
		randomize_name->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		randomize_name->addWidgetAction("MenuStart", "ready");
		randomize_name->setWidgetBack("back_button");
		randomize_name->setWidgetLeft("name");
		randomize_name->setWidgetDown("game_settings");
		static auto randomize_name_fn = [](Button& button, int index) {
			auto& names = stats[index]->sex == sex_t::MALE ?
				randomPlayerNamesMale : randomPlayerNamesFemale;
			auto name = names[rand() % names.size()].c_str();
			name_field_fn(name, index);
			auto card = static_cast<Frame*>(button.getParent());
			auto field = card->findField("name"); assert(field);
			field->setText(name);
		};
		switch (index) {
		case 0: randomize_name->setCallback([](Button& button){soundActivate(); randomize_name_fn(button, 0);}); break;
		case 1: randomize_name->setCallback([](Button& button){soundActivate(); randomize_name_fn(button, 1);}); break;
		case 2: randomize_name->setCallback([](Button& button){soundActivate(); randomize_name_fn(button, 2);}); break;
		case 3: randomize_name->setCallback([](Button& button){soundActivate(); randomize_name_fn(button, 3);}); break;
		}
		
		auto game_settings = card->addButton("game_settings");
		game_settings->setSize(SDL_Rect{62, 76, 202, 52});
		game_settings->setColor(makeColor(127, 127, 127, 255));
		game_settings->setHighlightColor(makeColor(255, 255, 255, 255));
		game_settings->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		game_settings->setText("View Game Settings");
		game_settings->setFont(smallfont_outline);
		game_settings->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		game_settings->addWidgetAction("MenuStart", "ready");
		game_settings->setWidgetBack("back_button");
		game_settings->setWidgetUp("name");
		game_settings->setWidgetDown("male");
		switch (index) {
		case 0: game_settings->setCallback([](Button&){soundActivate(); characterCardLobbySettingsMenu(0);}); break;
		case 1: game_settings->setCallback([](Button&){soundActivate(); characterCardLobbySettingsMenu(1);}); break;
		case 2: game_settings->setCallback([](Button&){soundActivate(); characterCardLobbySettingsMenu(2);}); break;
		case 3: game_settings->setCallback([](Button&){soundActivate(); characterCardLobbySettingsMenu(3);}); break;
		}

		auto male_button = card->addButton("male");
		male_button->setColor(makeColor(127, 127, 127, 255));
		male_button->setHighlightColor(makeColor(255, 255, 255, 255));
		male_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_Male_00.png");
		male_button->setSize(SDL_Rect{42, 166, 58, 52});
		male_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		male_button->addWidgetAction("MenuStart", "ready");
		male_button->setWidgetBack("back_button");
		male_button->setWidgetRight("female");
		male_button->setWidgetUp("game_settings");
		male_button->setWidgetDown("class");
		switch (index) {
		case 0: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 0);}); break;
		case 1: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 1);}); break;
		case 2: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 2);}); break;
		case 3: male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, 3);}); break;
		}

		auto female_button = card->addButton("female");
		female_button->setColor(makeColor(127, 127, 127, 255));
		female_button->setHighlightColor(makeColor(255, 255, 255, 255));
		female_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_Female_00.png");
		female_button->setSize(SDL_Rect{104, 166, 58, 52});
		female_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		female_button->addWidgetAction("MenuStart", "ready");
		female_button->setWidgetBack("back_button");
		female_button->setWidgetLeft("male");
		female_button->setWidgetRight("race");
		female_button->setWidgetUp("game_settings");
		female_button->setWidgetDown("class");
		switch (index) {
		case 0: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 0);}); break;
		case 1: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 1);}); break;
		case 2: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 2);}); break;
		case 3: female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, 3);}); break;
		}

		auto race_button = card->addButton("race");
		race_button->setColor(makeColor(127, 127, 127, 255));
		race_button->setHighlightColor(makeColor(255, 255, 255, 255));
		race_button->setSize(SDL_Rect{166, 166, 108, 52});
		switch (stats[index]->playerRace) {
		case RACE_HUMAN: race_button->setText("Human"); break;
		case RACE_SKELETON: race_button->setText("Skeleton"); break;
		case RACE_VAMPIRE: race_button->setText("Vampire"); break;
		case RACE_SUCCUBUS: race_button->setText("Succubus"); break;
		case RACE_GOATMAN: race_button->setText("Goatman"); break;
		case RACE_AUTOMATON: race_button->setText("Automaton"); break;
		case RACE_INCUBUS: race_button->setText("Incubus"); break;
		case RACE_GOBLIN: race_button->setText("Goblin"); break;
		case RACE_INSECTOID: race_button->setText("Insectoid"); break;
		case RACE_RAT: race_button->setText("Rat"); break;
		case RACE_TROLL: race_button->setText("Troll"); break;
		case RACE_SPIDER: race_button->setText("Spider"); break;
		case RACE_IMP: race_button->setText("Imp"); break;
		}
		race_button->setFont(smallfont_outline);
		race_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_RaceBase_00.png");
		race_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		race_button->addWidgetAction("MenuStart", "ready");
		race_button->setWidgetBack("back_button");
		race_button->setWidgetLeft("female");
		race_button->setWidgetUp("game_settings");
		race_button->setWidgetDown("class");
		switch (index) {
		case 0: race_button->setCallback([](Button&){soundActivate(); characterCardRaceMenu(0);}); break;
		case 1: race_button->setCallback([](Button&){soundActivate(); characterCardRaceMenu(1);}); break;
		case 2: race_button->setCallback([](Button&){soundActivate(); characterCardRaceMenu(2);}); break;
		case 3: race_button->setCallback([](Button&){soundActivate(); characterCardRaceMenu(3);}); break;
		}

		static auto randomize_class_fn = [](Button& button, int index){
			soundActivate();
			auto reduced_class_list = reducedClassList(index);
			auto random_class = reduced_class_list[(rand() % (reduced_class_list.size() - 1)) + 1];
			for (int c = 0; c < num_classes; ++c) {
				if (strcmp(random_class, classes_in_order[c]) == 0) {
					client_classes[index] = c;
					stats[index]->clearStats();
					initClass(index);
					return;
				}
			}
		};

		auto randomize_class = card->addButton("randomize_class");
		randomize_class->setColor(makeColor(127, 127, 127, 255));
		randomize_class->setHighlightColor(makeColor(255, 255, 255, 255));
		randomize_class->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_Randomize_00.png");
		randomize_class->setSize(SDL_Rect{244, 230, 40, 44});
		randomize_class->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		randomize_class->addWidgetAction("MenuStart", "ready");
		randomize_class->setWidgetBack("back_button");
		randomize_class->setWidgetLeft("class");
		randomize_class->setWidgetDown("ready");
		randomize_class->setWidgetUp("race");
		switch (index) {
		case 0: randomize_class->setCallback([](Button& button){randomize_class_fn(button, 0);}); break;
		case 1: randomize_class->setCallback([](Button& button){randomize_class_fn(button, 1);}); break;
		case 2: randomize_class->setCallback([](Button& button){randomize_class_fn(button, 2);}); break;
		case 3: randomize_class->setCallback([](Button& button){randomize_class_fn(button, 3);}); break;
		}

		auto class_text = card->addField("class_text", 64);
		class_text->setSize(SDL_Rect{96, 236, 138, 32});
		static auto class_text_fn = [](Field& field, int index){
			auto find = classes.find(classes_in_order[client_classes[index] + 1]);
			if (find != classes.end()) {
				field.setText(find->second.name);
			}
		};
		class_text->setFont(smallfont_outline);
		class_text->setJustify(Field::justify_t::CENTER);
		switch (index) {
		case 0: class_text->setTickCallback([](Widget& widget){class_text_fn(*static_cast<Field*>(&widget), 0);}); break;
		case 1: class_text->setTickCallback([](Widget& widget){class_text_fn(*static_cast<Field*>(&widget), 1);}); break;
		case 2: class_text->setTickCallback([](Widget& widget){class_text_fn(*static_cast<Field*>(&widget), 2);}); break;
		case 3: class_text->setTickCallback([](Widget& widget){class_text_fn(*static_cast<Field*>(&widget), 3);}); break;
		}
		(*class_text->getTickCallback())(*class_text);

		static auto class_button_fn = [](Button& button, int index) {
			auto find = classes.find(classes_in_order[client_classes[index] + 1]);
			if (find != classes.end()) {
				auto& class_info = find->second;
				switch (class_info.dlc) {
				case DLC::Base:
					button.setBackground("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGBase_00.png");
					break;
				case DLC::MythsAndOutcasts:
					button.setBackground("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGMyths_00.png");
					break;
				case DLC::LegendsAndPariahs:
					button.setBackground("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGLegends_00.png");
					break;
				}
				button.setIcon((std::string("images/ui/Main Menus/Play/PlayerCreation/ClassSelection/") + find->second.image).c_str());
			}
		};

		auto class_button = card->addButton("class");
		class_button->setColor(makeColor(127, 127, 127, 255));
		class_button->setHighlightColor(makeColor(255, 255, 255, 255));
		class_button->setSize(SDL_Rect{46, 226, 52, 52});
		class_button->setBorder(0);
		class_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		class_button->addWidgetAction("MenuStart", "ready");
		class_button->setWidgetBack("back_button");
		class_button->setWidgetRight("randomize_class");
		class_button->setWidgetUp("male");
		class_button->setWidgetDown("ready");
		switch (index) {
		case 0:
			class_button->setTickCallback([](Widget& widget){class_button_fn(*static_cast<Button*>(&widget), 0);});
			class_button->setCallback([](Button&){soundActivate(); characterCardClassMenu(0);});
			break;
		case 1:
			class_button->setTickCallback([](Widget& widget){class_button_fn(*static_cast<Button*>(&widget), 1);});
			class_button->setCallback([](Button&){soundActivate(); characterCardClassMenu(1);});
			break;
		case 2:
			class_button->setTickCallback([](Widget& widget){class_button_fn(*static_cast<Button*>(&widget), 2);});
			class_button->setCallback([](Button&){soundActivate(); characterCardClassMenu(2);});
			break;
		case 3:
			class_button->setTickCallback([](Widget& widget){class_button_fn(*static_cast<Button*>(&widget), 3);});
			class_button->setCallback([](Button&){soundActivate(); characterCardClassMenu(3);});
			break;
		}
		(*class_button->getTickCallback())(*class_button);

		auto ready_button = card->addButton("ready");
		ready_button->setSize(SDL_Rect{62, 288, 202, 52});
		ready_button->setColor(makeColor(127, 127, 127, 255));
		ready_button->setHighlightColor(makeColor(255, 255, 255, 255));
		ready_button->setBackground("images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		ready_button->setFont(bigfont_outline);
		ready_button->setText("Ready");
		ready_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		ready_button->addWidgetAction("MenuStart", "ready");
		ready_button->setWidgetBack("back_button");
		ready_button->setWidgetUp("class");
		ready_button->setCallback([](Button& button){
			soundActivate();
			destroyMainMenu();
			createDummyMainMenu();
			main_menu_fade_destination = FadeDestination::GameStart;
			fadeout = true;
			});
	}

	void createStartButton(int index) {
		auto lobby = main_menu_frame->findFrame("lobby");
		assert(lobby);

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{20 + 320 * index, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);
		card->setOwner(index);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		static const char* banner_font = "fonts/pixel_maz.ttf#32#2";

		auto banner = card->addField("invite_banner", 64);
		banner->setText((std::string("PLAYER ") + std::to_string(index + 1)).c_str());
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);

		auto invite = card->addButton("invite_button");
		invite->setText("Press Start");
		invite->setFont(smallfont_outline);
		invite->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		invite->setVJustify(Field::justify_t::TOP);
		invite->setHJustify(Field::justify_t::CENTER);
		invite->setBorder(0);
		invite->setColor(0);
		invite->setBorderColor(0);
		invite->setHighlightColor(0);
		invite->setHideGlyphs(true);
		invite->setWidgetBack("back_button");
		invite->addWidgetAction("MenuStart", "invite_button");
		switch (index) {
		case 0: invite->setCallback([](Button&){createCharacterCard(0);}); break;
		case 1: invite->setCallback([](Button&){createCharacterCard(1);}); break;
		case 2: invite->setCallback([](Button&){createCharacterCard(2);}); break;
		case 3: invite->setCallback([](Button&){createCharacterCard(3);}); break;
		}
		invite->select();
	}

	void createInviteButton(int index) {
		auto lobby = main_menu_frame->findFrame("lobby");
		assert(lobby);

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{20 + 320 * index, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		static const char* banner_font = "fonts/pixel_maz.ttf#32#2";

		auto banner = card->addField("invite_banner", 64);
		banner->setText("INVITE");
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);

		auto invite = card->addButton("invite_button");
		invite->setText("Press to Invite");
		invite->setFont(smallfont_outline);
		invite->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		invite->setVJustify(Field::justify_t::TOP);
		invite->setHJustify(Field::justify_t::CENTER);
		invite->setBorder(0);
		invite->setColor(0);
		invite->setBorderColor(0);
		invite->setHighlightColor(0);
		invite->setCallback([](Button&){buttonInviteFriends(NULL);});
		invite->select();
	}

	void createLobby(LobbyType type) {
		destroyMainMenu();
		createDummyMainMenu();

		currentLobbyType = type;

		auto lobby = main_menu_frame->addFrame("lobby");
		lobby->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		lobby->setActualSize(SDL_Rect{0, 0, lobby->getSize().w, lobby->getSize().h});
		lobby->setHollow(true);
		lobby->setBorder(0);

		auto back_button = createBackWidget(lobby, [](Button&){
			soundCancel();
			destroyMainMenu();
			createMainMenu();
			});

		auto back_frame = back_button->getParent();
		back_frame->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget); assert(frame);
			auto lobby = static_cast<Frame*>(frame->getParent()); assert(lobby);
			bool allCardsClosed = true;
			for (int c = 0; c < 4; ++c) {
				auto card = lobby->findFrame((std::string("card") + std::to_string(c)).c_str()); assert(card);
				auto backdrop = card->findImage("backdrop"); assert(backdrop);
				if (backdrop->path != "images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png") {
					allCardsClosed = false;
					break;
				}
			}
			frame->setInvisible(!allCardsClosed);
			auto button = frame->findButton("back_button"); assert(button);
			button->setDisabled(!allCardsClosed);
			});

		createCharacterCard(0);
		if (type == LobbyType::LobbyLocal) {
			createStartButton(1);
			createStartButton(2);
			createStartButton(3);
		} else if (type == LobbyType::LobbyHosted) {
			createInviteButton(1);
			createInviteButton(2);
			createInviteButton(3);
		}
	}

	void createLobbyBrowser() {
		// TODO
	}

/******************************************************************************/

	static void createPlayWindow() {
		auto window = main_menu_frame->addFrame("play_game_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 218 * 2) / 2,
			(Frame::virtualScreenY - 130 * 2) / 2,
			218 * 2,
			130 * 2});
		window->setActualSize(SDL_Rect{0, 0, 218 * 2, 130 * 2});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/UI_PlayGame_Window_02.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{170, 24, 98, 18});
		banner_title->setText("PLAY GAME");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		bool continueAvailable = saveGameExists(true) || saveGameExists(false);

		auto hall_of_trials_button = window->addButton("hall_of_trials");
		hall_of_trials_button->setSize(SDL_Rect{134, 176, 168, 52});
		hall_of_trials_button->setBackground("images/ui/Main Menus/Play/UI_PlayMenu_Button_HallofTrials00.png");
		hall_of_trials_button->setHighlightColor(makeColor(255, 255, 255, 255));
		hall_of_trials_button->setColor(makeColor(127, 127, 127, 255));
		hall_of_trials_button->setText("HALL OF TRIALS");
		hall_of_trials_button->setFont(smallfont_outline);
		hall_of_trials_button->setWidgetSearchParent(window->getName());
		if (continueAvailable) {
			hall_of_trials_button->setWidgetUp("continue");
		} else {
			hall_of_trials_button->setWidgetUp("new");
		}
		hall_of_trials_button->setWidgetBack("back_button");
		hall_of_trials_button->setCallback([](Button&){
			soundActivate();
			destroyMainMenu();
			createDummyMainMenu();
			main_menu_fade_destination = FadeDestination::HallOfTrials;
			fadeout = true;
			});

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			assert(main_menu_frame);
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto play_button = buttons->findButton("PLAY GAME"); assert(play_button);
			play_button->select();
			});

		auto continue_button = window->addButton("continue");
		continue_button->setSize(SDL_Rect{39 * 2, 36 * 2, 66 * 2, 50 * 2});
		continue_button->setBackground("images/ui/Main Menus/Play/UI_PlayMenu_Button_ContinueB00.png");
		continue_button->setTextColor(makeColor(180, 180, 180, 255));
		continue_button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		continue_button->setText(" \nCONTINUE");
		continue_button->setFont(smallfont_outline);
		if (continueAvailable) {
			continue_button->setBackgroundHighlighted("images/ui/Main Menus/Play/UI_PlayMenu_Button_ContinueA00.png");
			continue_button->setCallback(playContinue);
		} else {
			continue_button->setCallback([](Button&){ soundError(); });
		}
		continue_button->setWidgetSearchParent(window->getName());
		continue_button->setWidgetRight("new");
		continue_button->setWidgetDown("hall_of_trials");
		continue_button->setWidgetBack("back_button");

		auto new_button = window->addButton("new");
		new_button->setSize(SDL_Rect{114 * 2, 36 * 2, 68 * 2, 56 * 2});
		new_button->setBackground("images/ui/Main Menus/Play/UI_PlayMenu_NewB00.png");
		new_button->setBackgroundHighlighted("images/ui/Main Menus/Play/UI_PlayMenu_NewA00.png");
		new_button->setTextColor(makeColor(180, 180, 180, 255));
		new_button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		new_button->setText(" \nNEW");
		new_button->setFont(smallfont_outline);
		new_button->setCallback(playNew);
		new_button->setWidgetSearchParent(window->getName());
		new_button->setWidgetLeft("continue");
		new_button->setWidgetDown("hall_of_trials");
		new_button->setWidgetBack("back_button");

		if (skipintro) {
			if (continueAvailable) {
				continue_button->select();
			} else {
				new_button->select();
			}
		} else {
			hall_of_trials_button->select();
		}
	}

	void playNew(Button& button) {
		soundActivate();

		allSettings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
		allSettings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
		allSettings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
		allSettings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
		allSettings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
		allSettings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
		allSettings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
		allSettings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
		allSettings.cheats_enabled = svFlags & SV_FLAG_CHEATS;

		// remove "Play Game" window
		auto frame = static_cast<Frame*>(button.getParent());
		frame->removeSelf();

		// create "Local or Network" window
		auto window = main_menu_frame->addFrame("local_or_network_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 436) / 2,
			(Frame::virtualScreenY - 240) / 2,
			436,
			240});
		window->setActualSize(SDL_Rect{0, 0, 436, 240});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/LocalOrNetwork/UI_LocalorNetwork_Window_00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{142, 24, 152, 18});
		banner_title->setText("NEW ADVENTURER");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			createPlayWindow();
			});

		auto join_button = window->addButton("join");
		join_button->setSize(SDL_Rect{220, 134, 164, 62});
		join_button->setBackground("images/ui/Main Menus/Play/LocalOrNetwork/UI_LocalorNetwork_Button_00.png");
		join_button->setHighlightColor(makeColor(255, 255, 255, 255));
		join_button->setColor(makeColor(127, 127, 127, 255));
		join_button->setText("Join Network\nParty");
		join_button->setFont(smallfont_outline);
		join_button->setWidgetSearchParent(window->getName());
		join_button->setWidgetBack("back_button");
		join_button->setWidgetUp("local");
		join_button->setWidgetLeft("host");

		auto host_button = window->addButton("host");
		host_button->setSize(SDL_Rect{52, 134, 164, 62});
		host_button->setBackground("images/ui/Main Menus/Play/LocalOrNetwork/UI_LocalorNetwork_Button_00.png");
		host_button->setHighlightColor(makeColor(255, 255, 255, 255));
		host_button->setColor(makeColor(127, 127, 127, 255));
		host_button->setText("Host Network\nParty");
		host_button->setFont(smallfont_outline);
		host_button->setWidgetSearchParent(window->getName());
		host_button->setWidgetBack("back_button");
		host_button->setWidgetUp("local");
		host_button->setWidgetRight("join");
		host_button->setCallback([](Button&){soundActivate(); createLobby(LobbyType::LobbyHosted);});

		auto local_button = window->addButton("local");
		local_button->setSize(SDL_Rect{134, 68, 168, 62});
		local_button->setBackground("images/ui/Main Menus/Play/LocalOrNetwork/UI_LocalorNetwork_Button_01.png");
		local_button->setHighlightColor(makeColor(255, 255, 255, 255));
		local_button->setColor(makeColor(127, 127, 127, 255));
		local_button->setText("Local Adventure");
		local_button->setFont(smallfont_outline);
		local_button->setWidgetSearchParent(window->getName());
		local_button->setWidgetBack("back_button");
		local_button->setWidgetDown("host");
		local_button->setWidgetRight("back");
		local_button->setCallback([](Button&){soundActivate(); createLobby(LobbyType::LobbyLocal);});

		local_button->select();
	}

	void playContinue(Button& button) {
		soundActivate();

		// TODO continue menu
	}

/******************************************************************************/

	void mainPlayGame(Button& button) {
		soundActivate();
		createPlayWindow();
	}

	void mainPlayModdedGame(Button& button) {
		// TODO add a mod selection menu or something here
		soundActivate();
		createPlayWindow();
	}

	void mainHallOfRecords(Button& button) {
		soundActivate();

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

		int y = main_menu_buttons_height;

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
		//soundActivate(); // not needed, activated tab will do this

		settings_tab_name = "";

		allSettings.add_items_to_hotbar_enabled = auto_hotbar_new_items;
		allSettings.inventory_sorting = InventorySorting::load();
		allSettings.use_on_release_enabled = !right_click_protect;
		allSettings.show_messages_enabled = !disable_messages;
		allSettings.show_player_nametags_enabled = !hide_playertags;
		allSettings.show_hud_enabled = !nohud;
		allSettings.show_ip_address_enabled = !broadcast;
		allSettings.content_control_enabled = !spawn_blood;
		allSettings.colorblind_mode_enabled = colorblind;
		allSettings.arachnophobia_filter_enabled = arachnophobia_filter;
		allSettings.shaking_enabled = shaking;
		allSettings.bobbing_enabled = bobbing;
		allSettings.light_flicker_enabled = flickerLights;
		allSettings.resolution_x = xres;
		allSettings.resolution_y = yres;
		allSettings.vsync_enabled = verticalSync;
		allSettings.vertical_split_enabled = vertical_splitscreen;
		allSettings.gamma = vidgamma * 100.f;
		allSettings.fov = fov;
		allSettings.fps = fpsLimit;
		allSettings.master_volume = master_volume;
		allSettings.gameplay_volume = (float)sfxvolume / 128.f * 100.f;
		allSettings.ambient_volume = (float)sfxAmbientVolume / 128.f * 100.f;
		allSettings.environment_volume = (float)sfxEnvironmentVolume / 128.f * 100.f;
		allSettings.music_volume = (float)musvolume / 128.f * 100.f;
		allSettings.minimap_pings_enabled = !minimapPingMute;
		allSettings.player_monster_sounds_enabled = !mute_player_monster_sounds;
		allSettings.out_of_focus_audio_enabled = !mute_audio_on_focus_lost;
		allSettings.numkeys_in_inventory_enabled = hotbar_numkey_quick_add;
		allSettings.mouse_sensitivity = mousespeed;
		allSettings.reverse_mouse_enabled = reversemouse;
		allSettings.smooth_mouse_enabled = smoothmouse;
		allSettings.rotation_speed_limit_enabled = !disablemouserotationlimit;
		allSettings.turn_sensitivity_x = gamepad_rightx_sensitivity / 10;
		allSettings.turn_sensitivity_y = gamepad_righty_sensitivity / 10;
		allSettings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
		allSettings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
		allSettings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
		allSettings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
		allSettings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
		allSettings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
		allSettings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
		allSettings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
		allSettings.cheats_enabled = svFlags & SV_FLAG_CHEATS;

		auto settings = main_menu_frame->addFrame("settings");
		settings->setSize(SDL_Rect{(Frame::virtualScreenX - 1126) / 2, (Frame::virtualScreenY - 718) / 2, 1126, 718});
		settings->setActualSize(SDL_Rect{0, 0, settings->getSize().w, settings->getSize().h});
		settings->setColor(0);
		settings->setBorder(0);
		settings->addImage(
			SDL_Rect{
				(settings->getActualSize().w - 553 * 2) / 2,
				0,
				553 * 2,
				357 * 2
			},
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_Window02.png",
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
					if (name == settings_tab_name) {
						button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
					} else {
						button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
					}
				}
			}
			});

		static const char* pixel_maz_outline = "fonts/pixel_maz.ttf#32#2";

		auto window_title = settings->addField("window_title", 64);
		window_title->setFont(pixel_maz_outline);
		window_title->setSize(SDL_Rect{394, 26, 338, 24});
		window_title->setJustify(Field::justify_t::CENTER);
		window_title->setText("SETTINGS");

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
		int num_tabs = sizeof(tabs) / sizeof(tabs[0]);
		for (int c = 0; c < num_tabs; ++c) {
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
			button->addWidgetAction("MenuAlt1", "restore_defaults");
			button->addWidgetAction("MenuStart", "confirm_and_exit");
			if (c > 0) {
				button->setWidgetLeft(tabs[c - 1].name);
			} else {
				button->setWidgetLeft("tab_left");
			}
			if (c < num_tabs - 1) {
				button->setWidgetRight(tabs[c + 1].name);
			} else {
				button->setWidgetRight("tab_right");
			}
			button->setWidgetBack("discard_and_exit");
			if (c <= num_tabs / 2) {
				button->setWidgetDown("restore_defaults");
			} else if (c == num_tabs - 2) {
				button->setWidgetDown("discard_and_exit");
			} else if (c == num_tabs - 1) {
				button->setWidgetDown("confirm_and_exit");
			}
		}
		auto first_tab = settings->findButton(tabs[0].name);
		if (first_tab) {
			first_tab->select();
			first_tab->activate();
		}

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
		tab_left->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_left->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_left->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			const char* tabs[] = {
				"UI",
				"Video",
				"Audio",
				"Controls"
			};
			const char* prevtab = nullptr;
			for (auto tab : tabs) {
				auto button = settings->findButton(tab); assert(button);
				const char* name = "images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png";
				if (strcmp(button->getBackground(), name) == 0) {
					if (prevtab) {
						auto prevbutton = settings->findButton(prevtab); assert(prevbutton);
						prevbutton->select();
						prevbutton->activate();
					}
					return; 
				}
				prevtab = tab;
			}
			});

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
		tab_right->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_right->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_right->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			const char* tabs[] = {
				"Controls",
				"Audio",
				"Video",
				"UI",
			};
			const char* nexttab = nullptr;
			for (auto tab : tabs) {
				auto button = settings->findButton(tab); assert(button);
				const char* name = "images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png";
				if (strcmp(button->getBackground(), name) == 0) {
					if (nexttab) {
						auto nextbutton = settings->findButton(nexttab); assert(nextbutton);
						nextbutton->select();
						nextbutton->activate();
					}
					return;
				}
				nexttab = tab;
			}
			});

		auto tooltip = settings->addField("tooltip", 256);
		tooltip->setSize(SDL_Rect{92, 590, 948, 32});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setText("");

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
		restore_defaults->addWidgetAction("MenuAlt1", "restore_defaults");
		restore_defaults->addWidgetAction("MenuStart", "confirm_and_exit");
		restore_defaults->setCallback([](Button& button){
			soundActivate();
			settingsReset();
			});

		auto discard_and_exit = settings->addButton("discard_and_exit");
		discard_and_exit->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		discard_and_exit->setSize(SDL_Rect{700, 630, 164, 62});
		discard_and_exit->setText("Discard\n& Exit");
		discard_and_exit->setJustify(Button::justify_t::CENTER);
		discard_and_exit->setFont(smallfont_outline);
		discard_and_exit->setColor(makeColor(255, 255, 255, 191));
		discard_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		discard_and_exit->setCallback([](Button&){
			soundCancel();
			assert(main_menu_frame);
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto settings_button = buttons->findButton("SETTINGS"); assert(settings_button);
			settings_button->select();
			auto settings = main_menu_frame->findFrame("settings");
			if (settings) {
				settings->removeSelf();
			}
			});
		discard_and_exit->setWidgetBack("discard_and_exit");
		discard_and_exit->setWidgetPageLeft("tab_left");
		discard_and_exit->setWidgetPageRight("tab_right");
		discard_and_exit->setWidgetUp("Controls");
		discard_and_exit->setWidgetLeft("restore_defaults");
		discard_and_exit->setWidgetRight("confirm_and_exit");
		discard_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		discard_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");

		auto confirm_and_exit = settings->addButton("confirm_and_exit");
		confirm_and_exit->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		confirm_and_exit->setSize(SDL_Rect{880, 630, 164, 62});
		confirm_and_exit->setText("Confirm\n& Exit");
		confirm_and_exit->setJustify(Button::justify_t::CENTER);
		confirm_and_exit->setFont(smallfont_outline);
		confirm_and_exit->setColor(makeColor(255, 255, 255, 191));
		confirm_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm_and_exit->setCallback([](Button&){
			soundActivate();
			assert(main_menu_frame);
			settingsSave();
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto settings_button = buttons->findButton("SETTINGS"); assert(settings_button);
			settings_button->select();
			auto settings = main_menu_frame->findFrame("settings");
			if (settings) {
				settings->removeSelf();
			}
			});
		confirm_and_exit->setWidgetBack("discard_and_exit");
		confirm_and_exit->setWidgetPageLeft("tab_left");
		confirm_and_exit->setWidgetPageRight("tab_right");
		confirm_and_exit->setWidgetUp("tab_right");
		confirm_and_exit->setWidgetLeft("discard_and_exit");
		confirm_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		confirm_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");
	}

	void mainQuit(Button& button) {
		soundActivate();
		// TODO
	}

/******************************************************************************/

	void doMainMenu() {
		if (!main_menu_frame) {
			createMainMenu();
		}

		assert(main_menu_frame);

		if (main_menu_fade_destination != FadeDestination::None) {
			if (fadeout && fadealpha >= 255) {
				if (main_menu_fade_destination == FadeDestination::RootMainMenu) {
					destroyMainMenu();
					createMainMenu();
					playMusic(intromusic[1], true, true, false);
				}
				if (main_menu_fade_destination == FadeDestination::IntroStoryScreen) {
					createStoryScreen();
					playMusic(sounds[501], false, true, false);
				}
				if (main_menu_fade_destination == FadeDestination::HallOfTrials) {
					multiplayer = SINGLE;
					numplayers = 0;
					gameModeManager.setMode(GameModeManager_t::GAME_MODE_TUTORIAL_INIT);
					if ( gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt )
					{
						gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
						gameModeManager.Tutorial.writeToDocument();
					}
					gameModeManager.Tutorial.startTutorial("");
					steamStatisticUpdate(STEAM_STAT_TUTORIAL_ENTERED, ESteamStatTypes::STEAM_STAT_INT, 1);
					doNewGame(false);
				}
				if (main_menu_fade_destination == FadeDestination::GameStart) {
					multiplayer = SINGLE;
					numplayers = 0;
					gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
					doNewGame(false);
				}
				fadeout = false;
				main_menu_fade_destination = FadeDestination::None;
			}
		}
	}

	void createMainMenu() {
		main_menu_frame = gui->addFrame("main_menu");

		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setHollow(true);
		main_menu_frame->setBorder(0);
		main_menu_frame->setTickCallback(tickMainMenu);

		int y = 16;

		auto title_img = Image::get("images/system/title.png");
		auto title = main_menu_frame->addImage(
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

		auto notification = main_menu_frame->addFrame("notification");
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

		main_menu_buttons_height = y;

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

		auto play = buttons->findButton("PLAY GAME");
		if (play) {
			play->select();
			if (main_menu_cursor_x == 0 && main_menu_cursor_y == 0) {
				main_menu_cursor_x = play->getSize().x - 80;
				main_menu_cursor_y = play->getSize().y - 9 + buttons->getSize().y;
			}
		}

		main_menu_frame->addImage(
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
			auto banner = main_menu_frame->addFrame(name.c_str());
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

		auto copyright = main_menu_frame->addField("copyright", 64);
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

		auto version = main_menu_frame->addField("version", 32);
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

#ifndef NINTENDO
		int num_online_players = 1337; // TODO change me!
		std::string online_players_text = std::string("Players online: ") + std::to_string(num_online_players);
		auto online_players = main_menu_frame->addField("online_players", 32);
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
#endif
	}

	void destroyMainMenu() {
		main_menu_frame->removeSelf();
		main_menu_frame = nullptr;
	}

	void createDummyMainMenu() {
		main_menu_frame = gui->addFrame("main_menu");
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setHollow(true);
		main_menu_frame->setBorder(0);
		main_menu_frame->setTickCallback(tickMainMenu);
	}
}