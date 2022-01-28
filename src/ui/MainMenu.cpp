#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Slider.hpp"
#include "Text.hpp"
#include "GameUI.hpp"

#include "../init.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "../stat.hpp"
#include "../menu.hpp"
#include "../scores.hpp"
#include "../mod_tools.hpp"
#include "../interface/interface.hpp"
#include "../draw.hpp"
#include "../engine/audio/sound.hpp"
#include "../classdescriptions.hpp"
#include "../lobbies.hpp"

#include <cassert>
#include <functional>

namespace MainMenu {
    int pause_menu_owner = 0;

	// ALL NEW menu options:
	bool arachnophobia_filter = false;
	bool vertical_splitscreen = false;
	float master_volume = 100.f;
	bool cursor_delete_mode = false;
	Frame* main_menu_frame = nullptr;

	// If you want to add new player-visible bindings, ADD THEM HERE:
	// The first string in a binding is the name of the binding.
	// The second is the default Keyboard input.
    // The third is the default Gamepad input.
    // The fourth is the default Joystick input.
    static const char* emptyBinding = "[unbound]";
	static const char* hiddenBinding = "[hidden]";
	static const char* defaultBindings[][4] = {
		{"Attack", "Mouse1", "RightTrigger", emptyBinding},
		{"Use", "Mouse3", "ButtonA", emptyBinding},
		{"Cast Spell", "F", "ButtonRightBumper", emptyBinding},
		{"Block", "Space", "LeftTrigger", emptyBinding},
		{"Sneak", "Left Shift", "ButtonLeftBumper", emptyBinding},
		{"Character Status", "Tab", "ButtonBack", emptyBinding},
		{"Pause Game", hiddenBinding, "ButtonStart", emptyBinding},
		{"Spell List", "M", hiddenBinding, emptyBinding},
		{"Autosort Inventory", "R", emptyBinding, emptyBinding},
		{"Command NPC", "Q", "DpadY-", emptyBinding},
		{"Show NPC Commands", "C", "DpadX+", emptyBinding},
		{"Cycle NPCs", "E", "DpadX-", emptyBinding},
		{"Minimap Scale", "=", emptyBinding, emptyBinding},
		{"Toggle Minimap", "`", emptyBinding, emptyBinding},
		{"Hotbar Scroll Left", "MouseWheelUp", "ButtonX", emptyBinding},
		{"Hotbar Scroll Right", "MouseWheelDown", "ButtonB", emptyBinding},
		{"Hotbar Select", "Mouse2", "ButtonY", emptyBinding},
		{"Interact Tooltip Toggle", "T", "ButtonLeftStick", emptyBinding},
		{"Expand Inventory Tooltip", "X", hiddenBinding, emptyBinding },
		{"Quick Turn", emptyBinding, emptyBinding, emptyBinding },
		{"Chat", "Return", hiddenBinding, emptyBinding},
		{"Move Forward", "W", hiddenBinding, emptyBinding},
		{"Move Left", "A", hiddenBinding, emptyBinding},
		{"Move Backward", "S", hiddenBinding, emptyBinding},
		{"Move Right", "D", hiddenBinding, emptyBinding},
		{"Turn Left", "Left", hiddenBinding, emptyBinding},
		{"Turn Right", "Right", hiddenBinding, emptyBinding},
		{"Look Up", "Up", hiddenBinding, emptyBinding},
		{"Look Down", "Down", hiddenBinding, emptyBinding}
	};
	static const int numBindings = sizeof(defaultBindings) / sizeof(defaultBindings[0]);

	static int main_menu_buttons_height = 0;
	static Uint32 main_menu_ticks = 0u;
	static float main_menu_cursor_bob = 0.f;
	static int main_menu_cursor_x = 0;
	static int main_menu_cursor_y = 0;
	static FadeDestination main_menu_fade_destination = FadeDestination::None;

	enum class LobbyType {
	    None,
		LobbyLocal,
		LobbyLAN,
		LobbyOnline,
		LobbyJoined
	};

	static LobbyType currentLobbyType = LobbyType::None;
	static bool playersInLobby[4];

	void beginFade(FadeDestination fd) {
		main_menu_fade_destination = fd;
		fadeout = true;
		fadefinished = false;
	}

	static const char* bigfont_outline = "fonts/pixelmix.ttf#16#2";
	static const char* bigfont_no_outline = "fonts/pixelmix.ttf#16#0";
	static const char* smallfont_outline = "fonts/pixel_maz_multiline.ttf#16#2";
	static const char* smallfont_no_outline = "fonts/pixel_maz_multiline.ttf#16#0";
	static const char* menu_option_font = "fonts/pixel_maz.ttf#48#2";
	static const char* banner_font = "fonts/pixel_maz.ttf#64#2";

    // Inventory sorting options
	struct InventorySorting {
		bool hotbarWeapons = true;
		bool hotbarArmor = true;
		bool hotbarAmulets = true;
		bool hotbarBooks = true;
		bool hotbarTools = true;
		bool hotbarThrown = true;
		bool hotbarGems = false;
		bool hotbarPotions = true;
		bool hotbarScrolls = true;
		bool hotbarStaves = true;
		bool hotbarFood = true;
		bool hotbarSpells = true;
		int sortWeapons = 0;
		int sortArmor = 0;
		int sortAmulets = 0;
		int sortBooks = 0;
		int sortTools = 0;
		int sortThrown = 0;
		int sortGems = 0;
		int sortPotions = 0;
		int sortScrolls = 0;
		int sortStaves = 0;
		int sortFood = 0;
		int sortEquipped = 0;
		inline void save();
		static inline InventorySorting load();
		static inline InventorySorting reset();
		bool serialize(FileInterface*);
	};

    // Binding options
	struct Bindings {
		std::unordered_map<std::string, std::string> kb_mouse_bindings[4];
		std::unordered_map<std::string, std::string> gamepad_bindings[4];
		std::unordered_map<std::string, std::string> joystick_bindings[4];
		inline void save();
		static inline Bindings load();
		static inline Bindings reset();
		bool serialize(FileInterface*);
	};

    // Minimap options
	struct Minimap {
		int map_scale = 100;
		int icon_scale = 100;
		int foreground_opacity = 20;
		int background_opacity = 0;
		inline void save();
		static inline Minimap load();
		static inline Minimap reset();
		bool serialize(FileInterface*);
	};

    // Message options
	struct Messages {
		bool combat = true;
		bool status = true;
		bool inventory = true;
		bool equipment = true;
		bool world = true;
		bool chat = true;
		bool progression = true;
		bool interaction = true;
		bool inspection = true;
		bool hint = true;
		bool obituary = true;
		inline void save();
		static inline Messages load();
		static inline Messages reset();
		bool serialize(FileInterface*);
	};

    // All menu options combined
	struct AllSettings {
	    std::vector<std::pair<std::string, std::string>> mods;
	    bool crossplay_enabled;
		bool add_items_to_hotbar_enabled;
		InventorySorting inventory_sorting;
		bool use_on_release_enabled;
		Minimap minimap;
		bool show_messages_enabled;
		Messages show_messages;
		bool show_player_nametags_enabled;
		bool show_hud_enabled;
		bool show_ip_address_enabled;
		bool content_control_enabled;
		bool colorblind_mode_enabled;
		bool arachnophobia_filter_enabled;
		bool shaking_enabled;
		bool bobbing_enabled;
		bool light_flicker_enabled;
		int window_mode; // 0 = windowed, 1 = fullscreen, 2 = borderless
		int resolution_x;
		int resolution_y;
		bool vsync_enabled;
		bool vertical_split_enabled;
		float gamma;
		float fov;
		float fps;
		float master_volume;
		float gameplay_volume;
		float ambient_volume;
		float environment_volume;
		float music_volume;
		bool minimap_pings_enabled;
		bool player_monster_sounds_enabled;
		bool out_of_focus_audio_enabled;
		Bindings bindings;
		bool numkeys_in_inventory_enabled;
		float mouse_sensitivity;
		bool reverse_mouse_enabled;
		bool smooth_mouse_enabled;
		bool rotation_speed_limit_enabled;
		float turn_sensitivity_x;
		float turn_sensitivity_y;
		bool classic_mode_enabled;
		bool hardcore_mode_enabled;
		bool friendly_fire_enabled;
		bool keep_inventory_enabled;
		bool hunger_enabled;
		bool minotaur_enabled;
		bool random_traps_enabled;
		bool extra_life_enabled;
		bool cheats_enabled;
		bool skipintro;
		inline bool save(); // true if video needs restart
		static inline AllSettings load();
		static inline AllSettings reset();
		bool serialize(FileInterface*);
	};

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

	static inline void soundSlider(bool deafen_unless_gamepad = false) {
	    if (inputs.getVirtualMouse(clientnum)->draw_cursor && deafen_unless_gamepad) {
	        return;
	    }
	    playSound(497, 48);
	}

	static inline void soundWarning() {
		playSound(496, 48);
	}

	static inline void soundError() {
		playSound(498, 48);
	}

	static inline void soundDeleteSave() {
		playSound(153, 48);
	}

/******************************************************************************/

	static void settingsUI(Button&);
	static void settingsVideo(Button&);
	static void settingsAudio(Button&);
	static void settingsControls(Button&);
	static void settingsGame(Button&);

	static void recordsAdventureArchives(Button&);
	static void recordsLeaderboards(Button&);
	static void recordsDungeonCompendium(Button&);
	static void recordsStoryIntroduction(Button&);
	static void recordsCredits(Button&);
	static void recordsBackToMainMenu(Button&);

	static void playNew(Button&);
	static void playContinue(Button&);

	static void mainPlayGame(Button&);
	static void mainPlayModdedGame(Button&);
	static void mainHallOfRecords(Button&);
	static void mainAssignControllers(Button&);
	static void mainSettings(Button&);
	static void mainEditor(Button&);
	static void mainClose(Button&);
	static void mainEndLife(Button&);
	static void mainRestartGame(Button&);
	static void mainQuitToMainMenu(Button&);
	static void mainQuitToDesktop(Button&);

	static void characterCardGameSettingsMenu(int index);
	static void characterCardLobbySettingsMenu(int index);
	static void characterCardRaceMenu(int index);
	static void characterCardClassMenu(int index);

    static void createControllerPrompt(int index, bool show_player_text, void (*after_func)());
	static void createCharacterCard(int index);
	static void createStartButton(int index);
	static void createInviteButton(int index);
	static void createWaitingStone(int index);
	static void createLobby(LobbyType);
	static void createLobbyBrowser(Button&);

/******************************************************************************/

	static void setupSplitscreen() {
		if (multiplayer != SINGLE) {
			splitscreen = false;
			return;
		}

	    int playercount = 0;
        if (intro) {
		    clientnum = -1;
		    for (int c = 0; c < 4; ++c) {
			    if (playersInLobby[c]) {
				    clientnum = clientnum == -1 ? c : clientnum;
				    players[c]->bSplitscreen = true;
				    client_disconnected[c] = false;
				    ++playercount;
			    } else {
				    client_disconnected[c] = true;
				    players[c]->bSplitscreen = false;
			    }
		    }
		    if (clientnum == -1) {
		        // TODO loading a splitscreen game?
		        clientnum = 0;
		        players[0]->bSplitscreen = false;
		        client_disconnected[0] = false;
		        playercount = 1;
		    }
		} else {
		    for (int c = 0; c < 4; ++c) {
		        if (client_disconnected[c]) {
				    players[c]->bSplitscreen = false;
				    players[c]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
		        } else {
				    players[c]->bSplitscreen = true;
				    ++playercount;
		        }
		    }
		}
		splitscreen = playercount > 1;

		int c, playerindex;
		for (c = 0, playerindex = 0; c < 4; ++c, ++playerindex) {
			if (client_disconnected[c]) {
				--playerindex;
				continue;
			}
			if (vertical_splitscreen) {
				players[c]->splitScreenType = Player::SPLITSCREEN_VERTICAL;
			} else {
				players[c]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			}

			if (!splitscreen) {
				players[c]->camera().winx = 0;
				players[c]->camera().winy = 0;
				players[c]->camera().winw = xres;
				players[c]->camera().winh = yres;
			} else {
				if (playercount == 1) {
					players[c]->camera().winx = 0;
					players[c]->camera().winy = 0;
					players[c]->camera().winw = xres;
					players[c]->camera().winh = yres;
				} else if (playercount == 2) {
					if (players[c]->splitScreenType == Player::SPLITSCREEN_VERTICAL) {
						// divide screen vertically
						players[c]->camera().winx = playerindex * xres / 2;
						players[c]->camera().winy = 0;
						players[c]->camera().winw = xres / 2;
						players[c]->camera().winh = yres;
					} else {
						// divide screen horizontally
						players[c]->camera().winx = 0;
						players[c]->camera().winy = playerindex * yres / 2;
						players[c]->camera().winw = xres;
						players[c]->camera().winh = yres / 2;
					}
				} else if (playercount >= 3) {
					// divide screen into quadrants
					players[c]->camera().winx = (playerindex % 2) * xres / 2;
					players[c]->camera().winy = (playerindex / 2) * yres / 2;
					players[c]->camera().winw = xres / 2;
					players[c]->camera().winh = yres / 2;
				}
			}

			inputs.getVirtualMouse(c)->x = players[c]->camera_x1() + players[c]->camera_width() / 2;
			inputs.getVirtualMouse(c)->y = players[c]->camera_y1() + players[c]->camera_height() / 2;
		}
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
		const float bobrate = (float)PI * 2.f / (float)fpsLimit;
		main_menu_cursor_bob += bobrate;
		if (main_menu_cursor_bob >= (float)PI * 2.f) {
			main_menu_cursor_bob -= (float)PI * 2.f;
		}

		// update cursor position
		if (main_menu_frame) {
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
	}

	static void tickMainMenu(Widget& widget) {
		++main_menu_ticks;
	}

	static void updateSliderArrows(Frame& frame) {
		bool drawSliders = false;
		auto selectedWidget = frame.findSelectedWidget(0);
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

	static void updateSettingSelection(Frame& frame) {
		auto& images = frame.getImages();
		for (auto image : images) {
			if (image->path == "images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png") {
				image->path = "images/ui/Main Menus/Settings/Settings_Left_Backing00.png";
			}
			if (image->path == "images/ui/Main Menus/Settings/GenericWindow/Settings_Left_BackingSelect_Short00.png") {
				image->path = "images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png";
			}
		}
		auto selectedWidget = frame.findSelectedWidget(0);
		if (selectedWidget) {
			std::string setting;
			auto name = std::string(selectedWidget->getName());
			if (selectedWidget->getType() == Widget::WIDGET_SLIDER) {
				setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_slider") - 1) - (sizeof("setting_") - 1));
			} else if (selectedWidget->getType() == Widget::WIDGET_BUTTON) {
				auto button = static_cast<Button*>(selectedWidget);
				auto customize = "images/ui/Main Menus/Settings/Settings_Button_Customize00.png";
				auto binding = "images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonChoosing00.png";
				auto dropdown = "images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02.png";
				// TODO more sensible ways to identify these button types...
				auto boolean_button_text = "Off          On";
				if (strcmp(button->getBackground(), customize) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_customize_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getBackground(), binding) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_binding_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getBackground(), dropdown) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_dropdown_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getText(), boolean_button_text) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_button") - 1) - (sizeof("setting_") - 1));
				}
			}
			if (!setting.empty()) {
				auto image = frame.findImage((std::string("setting_") + setting + std::string("_image")).c_str());
				if (image && image->path == "images/ui/Main Menus/Settings/Settings_Left_Backing00.png") {
					image->path = "images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png";
				}
				else if (image && image->path == "images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png") {
					image->path = "images/ui/Main Menus/Settings/GenericWindow/Settings_Left_BackingSelect_Short00.png";
				}
				auto field = frame.findField((std::string("setting_") + setting + std::string("_field")).c_str());
				if (field) {
					static Widget* current_selected_widget = nullptr;
					if (current_selected_widget != selectedWidget) {
						current_selected_widget = selectedWidget;
						auto settings = static_cast<Frame*>(frame.getParent());
						auto tooltip = settings->findField("tooltip"); assert(tooltip);
						tooltip->setText(field->getGuide());
					}
				}
			}
		}
	}

	static Button* createBackWidget(Frame* parent, void (*callback)(Button&), SDL_Rect offset = SDL_Rect{4, 4, 0, 0}) {
		auto back = parent->addFrame("back");
		back->setSize(SDL_Rect{offset.x, offset.y, 66, 36});
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
		back_button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		back_button->setButtonsOffset(SDL_Rect{8, 0, 0, 0,});
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

	static void binaryPrompt(
		const char* window_text,
		const char* okay_text,
		const char* cancel_text,
		void (*okay_callback)(Button&),
		void (*cancel_callback)(Button&)
	) {
		if (main_menu_frame->findFrame("binary_prompt")) {
			return;
		}

		soundActivate();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto frame = dimmer->addFrame("binary_prompt");
		frame->setSize(SDL_Rect{(Frame::virtualScreenX - 364) / 2, (Frame::virtualScreenY - 176) / 2, 364, 176});
		frame->setActualSize(SDL_Rect{0, 0, 364, 176});
		frame->setColor(0);
		frame->setBorder(0);
		frame->addImage(
			frame->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Disconnect/UI_Disconnect_Window00.png",
			"background"
		);

		auto text = frame->addField("text", 128);
		text->setSize(SDL_Rect{30, 28, 304, 46});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);

		auto okay = frame->addButton("okay");
		okay->setSize(SDL_Rect{58, 78, 130, 52});
		okay->setBackground("images/ui/Main Menus/Disconnect/UI_Disconnect_Button_Abandon00.png");
		okay->setColor(makeColor(255, 255, 255, 255));
		okay->setHighlightColor(makeColor(255, 255, 255, 255));
		okay->setTextColor(makeColor(255, 255, 255, 255));
		okay->setTextHighlightColor(makeColor(255, 255, 255, 255));
		okay->setFont(smallfont_outline);
		okay->setText(okay_text);
		okay->setWidgetRight("cancel");
		okay->setWidgetBack("cancel");
		okay->setCallback(okay_callback);
		okay->select();

		auto cancel = frame->addButton("cancel");
		cancel->setSize(SDL_Rect{196, 78, 108, 52});
		cancel->setBackground("images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		cancel->setColor(makeColor(255, 255, 255, 255));
		cancel->setHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setTextColor(makeColor(255, 255, 255, 255));
		cancel->setTextHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setFont(smallfont_outline);
		cancel->setText(cancel_text);
		cancel->setWidgetLeft("okay");
		cancel->setWidgetBack("cancel");
		cancel->setCallback(cancel_callback);
	}

	static void monoPrompt(
		const char* window_text,
		const char* okay_text,
		void (*okay_callback)(Button&)
	) {
		if (main_menu_frame->findFrame("mono_prompt")) {
			return;
		}

		soundActivate();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto frame = dimmer->addFrame("mono_prompt");
		frame->setSize(SDL_Rect{(Frame::virtualScreenX - 364) / 2, (Frame::virtualScreenY - 176) / 2, 364, 176});
		frame->setActualSize(SDL_Rect{0, 0, 364, 176});
		frame->setColor(0);
		frame->setBorder(0);
		frame->addImage(
			frame->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Disconnect/UI_Disconnect_Window00.png",
			"background"
		);

		auto text = frame->addField("text", 128);
		text->setSize(SDL_Rect{30, 28, 304, 46});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);

		auto okay = frame->addButton("okay");
		okay->setSize(SDL_Rect{(frame->getActualSize().w - 108) / 2, 78, 108, 52});
		okay->setBackground("images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		okay->setColor(makeColor(255, 255, 255, 255));
		okay->setHighlightColor(makeColor(255, 255, 255, 255));
		okay->setTextColor(makeColor(255, 255, 255, 255));
		okay->setTextHighlightColor(makeColor(255, 255, 255, 255));
		okay->setFont(smallfont_outline);
		okay->setText(okay_text);
		okay->setWidgetBack("okay");
		okay->setCallback(okay_callback);
		okay->select();
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

	bool InventorySorting::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("hotbarWeapons", hotbarWeapons);
		file->property("hotbarArmor", hotbarArmor);
		file->property("hotbarAmulets", hotbarAmulets);
		file->property("hotbarBooks", hotbarBooks);
		file->property("hotbarTools", hotbarTools);
		file->property("hotbarThrown", hotbarThrown);
		file->property("hotbarGems", hotbarGems);
		file->property("hotbarPotions", hotbarPotions);
		file->property("hotbarScrolls", hotbarScrolls);
		file->property("hotbarStaves", hotbarStaves);
		file->property("hotbarFood", hotbarFood);
		file->property("hotbarSpells", hotbarSpells);
		file->property("sortWeapons", sortWeapons);
		file->property("sortArmor", sortArmor);
		file->property("sortAmulets", sortAmulets);
		file->property("sortBooks", sortBooks);
		file->property("sortTools", sortTools);
		file->property("sortThrown", sortThrown);
		file->property("sortGems", sortGems);
		file->property("sortPotions", sortPotions);
		file->property("sortScrolls", sortScrolls);
		file->property("sortStaves", sortStaves);
		file->property("sortFood", sortFood);
		file->property("sortEquipped", sortEquipped);
		return true;
	}

/******************************************************************************/

	static Bindings old_bindings;

	inline void Bindings::save() {
		for (int c = 0; c < 4; ++c) {
		    Input& input = Input::inputs[c];
			input.getKeyboardBindings().clear();
			input.setKeyboardBindings(kb_mouse_bindings[c]);
			input.getGamepadBindings().clear();
			input.setGamepadBindings(gamepad_bindings[c]);
			input.getJoystickBindings().clear();
			input.setJoystickBindings(joystick_bindings[c]);

			input.refresh();
		}
		old_bindings = *this;
	}

	inline Bindings Bindings::load() {
		return old_bindings;
	}

	inline Bindings Bindings::reset() {
		Bindings bindings;
		for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < numBindings; ++i) {
			    bindings.kb_mouse_bindings[c].emplace(defaultBindings[i][0], defaultBindings[i][1]);
			    bindings.gamepad_bindings[c].emplace(defaultBindings[i][0], defaultBindings[i][2]);
			    bindings.joystick_bindings[c].emplace(defaultBindings[i][0], defaultBindings[i][3]);
			}
		}
		return bindings;
	}

	bool Bindings::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		Uint32 num_players = 4;
		file->propertyName("players");
		file->beginArray(num_players);
		for (int c = 0; c < std::min(num_players, (Uint32)4); ++c) {
			file->beginObject();
			for (int j = 0; j < 3; ++j) {
				auto& bindings =
					j == 0 ? kb_mouse_bindings[c]:
					j == 1 ? gamepad_bindings[c]:
					joystick_bindings[c];
				file->propertyName(
					j == 0 ? "kb_mouse_bindings":
					j == 1 ? "gamepad_bindings":
					"joystick_bindings");
				if (file->isReading()) {
					bindings.clear();
				}
				Uint32 count = bindings.size();
				file->beginArray(count);
				if (file->isReading()) {
					for (Uint32 index = 0; index < count; ++index) {
						file->beginObject();
						std::string binding;
						file->property("binding", binding);
						std::string input;
						file->property("input", input);
						bindings.emplace(binding, input);
						file->endObject();
					}
				} else {
					for (auto& bind : bindings) {
						file->beginObject();
						std::string binding = bind.first;
						file->property("binding", binding);
						std::string input = bind.second;
						file->property("input", input);
						file->endObject();
					}
				}
				file->endArray();
			}
			file->endObject();
		}
		file->endArray();
		return true;
	}

/******************************************************************************/

	inline void Minimap::save() {
		minimapTransparencyForeground = 100 - foreground_opacity;
		minimapTransparencyBackground = 100 - background_opacity;
		minimapScale = map_scale;
		minimapObjectZoom = icon_scale;
	}

	inline Minimap Minimap::load() {
		Minimap minimap;
		minimap.foreground_opacity = 100 - minimapTransparencyForeground;
		minimap.background_opacity = 100 - minimapTransparencyBackground;
		minimap.map_scale = minimapScale;
		minimap.icon_scale = minimapObjectZoom;
		return minimap;
	}

	inline Minimap Minimap::reset() {
		return Minimap();
	}

	bool Minimap::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("map_scale", map_scale);
		file->property("icon_scale", icon_scale);
		file->property("foreground_opacity", foreground_opacity);
		file->property("background_opacity", background_opacity);
		return true;
	}

/******************************************************************************/

	inline void Messages::save() {
		messagesEnabled = 0;
		messagesEnabled |= combat ? MESSAGE_COMBAT : 0;
		messagesEnabled |= status ? MESSAGE_STATUS : 0;
		messagesEnabled |= inventory ? MESSAGE_INVENTORY : 0;
		messagesEnabled |= equipment ? MESSAGE_EQUIPMENT : 0;
		messagesEnabled |= world ? MESSAGE_WORLD : 0;
		messagesEnabled |= chat ? MESSAGE_CHAT : 0;
		messagesEnabled |= progression ? MESSAGE_PROGRESSION : 0;
		messagesEnabled |= interaction ? MESSAGE_INTERACTION : 0;
		messagesEnabled |= inspection ? MESSAGE_INSPECTION : 0;
		messagesEnabled |= hint ? MESSAGE_HINT : 0;
		messagesEnabled |= obituary ? MESSAGE_OBITUARY : 0;
		messagesEnabled |= MESSAGE_MISC;
#ifndef NDEBUG
        messagesEnabled |= MESSAGE_DEBUG;
#endif
	}

	inline Messages Messages::load() {
		Messages messages;
		messages.combat = messagesEnabled & MESSAGE_COMBAT;
		messages.status = messagesEnabled & MESSAGE_STATUS;
		messages.inventory = messagesEnabled & MESSAGE_INVENTORY;
		messages.equipment = messagesEnabled & MESSAGE_EQUIPMENT;
		messages.world = messagesEnabled & MESSAGE_WORLD;
		messages.chat = messagesEnabled & MESSAGE_CHAT;
		messages.progression = messagesEnabled & MESSAGE_PROGRESSION;
		messages.interaction = messagesEnabled & MESSAGE_INTERACTION;
		messages.inspection = messagesEnabled & MESSAGE_INSPECTION;
		messages.hint = messagesEnabled & MESSAGE_HINT;
		messages.obituary = messagesEnabled & MESSAGE_OBITUARY;
		return messages;
	}

	inline Messages Messages::reset() {
		return Messages();
	}

	bool Messages::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("combat", combat);
		file->property("status", status);
		file->property("inventory", inventory);
		file->property("equipment", equipment);
		file->property("world", world);
		file->property("chat", chat);
		file->property("progression", progression);
		file->property("interaction", interaction);
		file->property("inspection", inspection);
		file->property("hint", hint);
		file->property("obituary", obituary);
		return true;
	}

	/******************************************************************************/

	static AllSettings allSettings;

	inline bool AllSettings::save() {
	    bool result = false;

        gamemods_mountedFilepaths = mods;
        LobbyHandler.crossplayEnabled = crossplay_enabled;
		auto_hotbar_new_items = add_items_to_hotbar_enabled;
		inventory_sorting.save();
		right_click_protect = !use_on_release_enabled;
		minimap.save();
		disable_messages = !show_messages_enabled;
		show_messages.save();
		hide_playertags = !show_player_nametags_enabled;
		nohud = !show_hud_enabled;
		broadcast = !show_ip_address_enabled;
		spawn_blood = !content_control_enabled;
		colorblind = colorblind_mode_enabled;
		arachnophobia_filter = arachnophobia_filter_enabled;
		shaking = shaking_enabled;
		bobbing = bobbing_enabled;
		flickerLights = light_flicker_enabled;
		bool new_fullscreen, new_borderless;
		switch (allSettings.window_mode) {
		case 0:
			new_fullscreen = false;
			new_borderless = false;
			break;
		case 1:
			new_fullscreen = true;
			new_borderless = false;
			break;
		case 2:
			new_fullscreen = true;
			new_borderless = true;
			break;
		default:
			assert("Unknown video mode" && 0);
			break;
		}
		if (xres != resolution_x ||
		    yres != resolution_y ||
		    verticalSync != vsync_enabled ||
		    vertical_splitscreen != vertical_split_enabled ||
		    vidgamma != gamma / 100.f ||
		    new_fullscreen != fullscreen ||
		    new_borderless != borderless) {
		    result = true;
		}
		fullscreen = new_fullscreen;
		borderless = new_borderless;
		xres = std::max(resolution_x, 1280);
		yres = std::max(resolution_y, 720);
		verticalSync = vsync_enabled;
		vertical_splitscreen = vertical_split_enabled;
		vidgamma = gamma / 100.f;
		::fov = fov;
		fpsLimit = fps;
		MainMenu::master_volume = (master_volume / 200.f);
		sfxvolume = (gameplay_volume / 200.f);
		sfxAmbientVolume = (ambient_volume / 200.f);
		sfxEnvironmentVolume = (environment_volume / 200.f);
		musvolume = (music_volume / 200.f);
		minimapPingMute = !minimap_pings_enabled;
		mute_player_monster_sounds = !player_monster_sounds_enabled;
		mute_audio_on_focus_lost = !out_of_focus_audio_enabled;
		bindings.save();
		hotbar_numkey_quick_add = numkeys_in_inventory_enabled;
		mousespeed = mouse_sensitivity;
		reversemouse = reverse_mouse_enabled;
		smoothmouse = smooth_mouse_enabled;
		disablemouserotationlimit = !rotation_speed_limit_enabled;
		gamepad_rightx_sensitivity = turn_sensitivity_x * 10.f;
		gamepad_righty_sensitivity = turn_sensitivity_y * 10.f;
		svFlags = classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
		svFlags = hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
		svFlags = friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
		svFlags = keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
		svFlags = hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
		svFlags = minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
		svFlags = random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
		svFlags = extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
		svFlags = cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
		::skipintro = skipintro;

        // TODO crossplay settings
#ifdef USE_EOS
        /*
	    if ( LobbyHandler.settings_crossplayEnabled )
	    {
		    if ( !LobbyHandler.crossplayEnabled )
		    {
			    LobbyHandler.settings_crossplayEnabled = false;
			    EOS.CrossplayAccountManager.trySetupFromSettingsMenu = true;
		    }
	    }
	    else
	    {
		    if ( LobbyHandler.crossplayEnabled )
		    {
			    LobbyHandler.crossplayEnabled = false;
			    EOS.CrossplayAccountManager.logOut = true;
		    }
	    }
        */
#endif

	    return result;
    }

	inline AllSettings AllSettings::load() {
		AllSettings settings;
		settings.mods = gamemods_mountedFilepaths;
		settings.crossplay_enabled = LobbyHandler.crossplayEnabled;
		settings.add_items_to_hotbar_enabled = auto_hotbar_new_items;
		settings.inventory_sorting = InventorySorting::load();
		settings.use_on_release_enabled = !right_click_protect;
		settings.minimap = Minimap::load();
		settings.show_messages_enabled = !disable_messages;
		settings.show_messages = Messages::load();
		settings.show_player_nametags_enabled = !hide_playertags;
		settings.show_hud_enabled = !nohud;
		settings.show_ip_address_enabled = !broadcast;
		settings.content_control_enabled = !spawn_blood;
		settings.colorblind_mode_enabled = colorblind;
		settings.arachnophobia_filter_enabled = arachnophobia_filter;
		settings.shaking_enabled = shaking;
		settings.bobbing_enabled = bobbing;
		settings.light_flicker_enabled = flickerLights;
		settings.window_mode = fullscreen ? (borderless ? 2 : 1) : 0;
		settings.resolution_x = xres;
		settings.resolution_y = yres;
		settings.vsync_enabled = verticalSync;
		settings.vertical_split_enabled = vertical_splitscreen;
		settings.gamma = vidgamma * 100.f;
		settings.fov = ::fov;
		settings.fps = fpsLimit;
		settings.master_volume = MainMenu::master_volume * 200.f;
		settings.gameplay_volume = (float)sfxvolume * 200.f;
		settings.ambient_volume = (float)sfxAmbientVolume * 200.f;
		settings.environment_volume = (float)sfxEnvironmentVolume* 200.f;
		settings.music_volume = (float)musvolume * 200.f;
		settings.minimap_pings_enabled = !minimapPingMute;
		settings.player_monster_sounds_enabled = !mute_player_monster_sounds;
		settings.out_of_focus_audio_enabled = !mute_audio_on_focus_lost;
		settings.bindings = Bindings::load();
		settings.numkeys_in_inventory_enabled = hotbar_numkey_quick_add;
		settings.mouse_sensitivity = mousespeed;
		settings.reverse_mouse_enabled = reversemouse;
		settings.smooth_mouse_enabled = smoothmouse;
		settings.rotation_speed_limit_enabled = !disablemouserotationlimit;
		settings.turn_sensitivity_x = gamepad_rightx_sensitivity / 10;
		settings.turn_sensitivity_y = gamepad_righty_sensitivity / 10;
		settings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
		settings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
		settings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
		settings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
		settings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
		settings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
		settings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
		settings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
		settings.cheats_enabled = svFlags & SV_FLAG_CHEATS;
		settings.skipintro = ::skipintro;
		return settings;
	}

	inline AllSettings AllSettings::reset() {
		AllSettings settings;
		settings.mods = gamemods_mountedFilepaths;
		settings.crossplay_enabled = LobbyHandler.crossplayEnabled;
		settings.add_items_to_hotbar_enabled = true;
		settings.inventory_sorting = InventorySorting::reset();
		settings.use_on_release_enabled = true;
		settings.minimap = Minimap::reset();
		settings.show_messages_enabled = true;
		settings.show_messages = Messages::reset();
		settings.show_player_nametags_enabled = true;
		settings.show_hud_enabled = true;
		settings.show_ip_address_enabled = true;
		settings.content_control_enabled = false;
		settings.colorblind_mode_enabled = false;
		settings.arachnophobia_filter_enabled = false;
		settings.shaking_enabled = true;
		settings.bobbing_enabled = true;
		settings.light_flicker_enabled = true;
		settings.window_mode = 0;
		settings.resolution_x = 1280;
		settings.resolution_y = 720;
		settings.vsync_enabled = true;
		settings.vertical_split_enabled = false;
		settings.gamma = 100.f;
		settings.fov = 65;
		settings.fps = 60;
		settings.master_volume = 100.f;
		settings.gameplay_volume = 100.f;
		settings.ambient_volume = 100.f;
		settings.environment_volume = 100.f;
		settings.music_volume = 50.f;
		settings.minimap_pings_enabled = true;
		settings.player_monster_sounds_enabled = true;
		settings.out_of_focus_audio_enabled = true;
		settings.bindings = Bindings::reset();
		settings.numkeys_in_inventory_enabled = true;
		settings.mouse_sensitivity = 32.f;
		settings.reverse_mouse_enabled = false;
		settings.smooth_mouse_enabled = false;
		settings.rotation_speed_limit_enabled = true;
		settings.turn_sensitivity_x = 50.f;
		settings.turn_sensitivity_y = 50.f;
		settings.classic_mode_enabled = false;
		settings.hardcore_mode_enabled = false;
		settings.friendly_fire_enabled = true;
		settings.keep_inventory_enabled = false;
		settings.hunger_enabled = true;
		settings.minotaur_enabled = true;
		settings.random_traps_enabled = true;
		settings.extra_life_enabled = false;
		settings.cheats_enabled = false;
		settings.skipintro = false;
		return settings;
	}

	bool AllSettings::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
	    file->property("mods", mods);
		file->property("crossplay_enabled", crossplay_enabled);
		file->property("add_items_to_hotbar_enabled", add_items_to_hotbar_enabled);
		file->property("inventory_sorting", inventory_sorting);
		file->property("use_on_release_enabled", use_on_release_enabled);
		file->property("minimap", minimap);
		file->property("show_messages_enabled", show_messages_enabled);
		file->property("show_player_nametags_enabled", show_player_nametags_enabled);
		file->property("show_hud_enabled", show_hud_enabled);
		file->property("show_ip_address_enabled", show_ip_address_enabled);
		file->property("content_control_enabled", content_control_enabled);
		file->property("colorblind_mode_enabled", colorblind_mode_enabled);
		file->property("arachnophobia_filter_enabled", arachnophobia_filter_enabled);
		file->property("shaking_enabled", shaking_enabled);
		file->property("bobbing_enabled", bobbing_enabled);
		file->property("light_flicker_enabled", light_flicker_enabled);
		file->property("window_mode", window_mode);
		file->property("resolution_x", resolution_x);
		file->property("resolution_y", resolution_y);
		file->property("vsync_enabled", vsync_enabled);
		file->property("vertical_split_enabled", vertical_split_enabled);
		file->property("gamma", gamma);
		file->property("fov", fov);
		file->property("fps", fps);
		file->property("master_volume", master_volume);
		file->property("gameplay_volume", gameplay_volume);
		file->property("ambient_volume", ambient_volume);
		file->property("environment_volume", environment_volume);
		file->property("music_volume", music_volume);
		file->property("minimap_pings_enabled", minimap_pings_enabled);
		file->property("player_monster_sounds_enabled", player_monster_sounds_enabled);
		file->property("out_of_focus_audio_enabled", out_of_focus_audio_enabled);
		file->property("bindings", bindings);
		file->property("numkeys_in_inventory_enabled", numkeys_in_inventory_enabled);
		file->property("mouse_sensitivity", mouse_sensitivity);
		file->property("reverse_mouse_enabled", reverse_mouse_enabled);
		file->property("smooth_mouse_enabled", smooth_mouse_enabled);
		file->property("rotation_speed_limit_enabled", rotation_speed_limit_enabled);
		file->property("turn_sensitivity_x", turn_sensitivity_x);
		file->property("turn_sensitivity_y", turn_sensitivity_y);
		file->property("classic_mode_enabled", classic_mode_enabled);
		file->property("hardcore_mode_enabled", hardcore_mode_enabled);
		file->property("friendly_fire_enabled", friendly_fire_enabled);
		file->property("keep_inventory_enabled", keep_inventory_enabled);
		file->property("hunger_enabled", hunger_enabled);
		file->property("minotaur_enabled", minotaur_enabled);
		file->property("random_traps_enabled", random_traps_enabled);
		file->property("extra_life_enabled", extra_life_enabled);
		file->property("cheats_enabled", cheats_enabled);
		file->property("skipintro", skipintro);
		file->property("use_model_cache", useModelCache);
		file->property("debug_keys_enabled", enableDebugKeys);
		return true;
	}

/******************************************************************************/

	// Story text is formatted thus:
	// carat ^ advances the image index
	// pound # adds a pause
	// asterisk * followed by a single digit changes the text box size

	static void createStoryScreen(const char* file, void (*end_func)()) {
	    char filename[PATH_MAX];
	    (void)completePath(filename, file);

        struct Story {
            int version = 1;
            bool press_a_to_advance = false;
            std::vector<std::string> text;
            std::vector<std::string> images;

            bool serialize(FileInterface* file) {
                if (file->isReading()) {
                    text.clear();
                    images.clear();
                }
                file->property("version", version);
                file->property("press_a_to_advance", press_a_to_advance);
                file->property("text", text);
                file->property("images", images);
                return true;
            }
        };

        static Story story;
        static int story_text_chars;
        static int story_text_lines;
		static int story_text_pause;
		static int story_text_box_size;
		static float story_text_scroll;
		static float story_text_writer;
		static float story_text_box_scale;
		static bool story_text_adjust_box;
		static bool story_text_end;
		static int story_image_index;
		static float story_image_fade;
		static bool story_image_advanced;
		static void (*story_end_func)();
		static int story_skip;
		static float story_skip_timer;

		bool read_result = FileHelper::readObject(filename, story);
		if (!read_result) {
		    assert(0 && "Story file not found!");
		    return;
		}
		story_text_chars = 0;
		story_text_lines = 0;
		story_text_pause = 0;
		story_text_box_size = 2;
		story_text_scroll = 0.f;
		story_text_writer = 0.f;
		story_text_box_scale = -2.f;
		story_text_adjust_box = false;
		story_text_end = false;
		story_image_index = 0;
		story_image_fade = 0.f;
		story_image_advanced = false;
		story_end_func = end_func;
		story_skip = 0;
		story_skip_timer = 0.f;

		main_menu_frame->addImage(
			main_menu_frame->getSize(),
			0xffffffff,
			story.images[0].c_str(),
			"backdrop"
		);

		auto back_button = main_menu_frame->addButton("back");
		back_button->setHideSelectors(true);
		back_button->setText("Skip story");
		back_button->setColor(makeColor(0, 0, 0, 0));
		back_button->setHighlightColor(makeColor(0, 0, 0, 0));
		back_button->setBorderColor(makeColor(0, 0, 0, 0));
		back_button->setTextColor(0xffffffff);
		back_button->setTextHighlightColor(0xffffffff);
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::RIGHT);
		back_button->setVJustify(Button::justify_t::CENTER);
		back_button->setSize(SDL_Rect{Frame::virtualScreenX - 416, Frame::virtualScreenY - 70, 380, 50});
		back_button->setCallback([](Button& button){
		    ++story_skip;
		    story_skip_timer = (float)TICKS_PER_SECOND;
		    switch (story_skip) {
		    case 0: button.setText("Skip story"); break;
		    case 1: button.setText("Confirm (1)?"); break;
		    case 2: button.setText("Confirm (2)?"); break;
		    }
		    if (story_skip >= 3) {
			    story_end_func();
		    }
			});
		back_button->setWidgetBack("back");
		back_button->setGlyphPosition(Button::glyph_position_t::CENTERED_RIGHT);
		back_button->setButtonsOffset(SDL_Rect{16, 0, 0, 0,});

		auto font = Font::get(bigfont_outline); assert(font);

		auto next_button_func = [](Button&){
	        if (story_text_pause) {
		        story_text_pause = 0;
			    if (story_text_end == true) {
				    story_end_func();
			    } else {
	                auto font = Font::get(bigfont_outline); assert(font);
				    story_text_scroll = font->height() * story_text_box_size;
			    }
	        }
	        };

		auto textbox1 = main_menu_frame->addFrame("story_text_box");
		textbox1->setColor(makeColor(0, 0, 0, 127));
		textbox1->setBorder(0);
		if (!story.press_a_to_advance) {
		    textbox1->setHideSelectors(true);
		    textbox1->setHideGlyphs(true);
		    textbox1->select();
		    textbox1->setWidgetBack("back");
		} else {
		    auto next = main_menu_frame->addButton("next");
		    next->setHideSelectors(true);
		    next->setColor(makeColor(0, 0, 0, 0));
		    next->setHighlightColor(makeColor(0, 0, 0, 0));
		    next->setBorderColor(makeColor(0, 0, 0, 0));
		    next->setTextColor(0xffffffff);
		    next->setTextHighlightColor(0xffffffff);
		    next->setFont(smallfont_outline);
		    next->setHJustify(Button::justify_t::CENTER);
		    next->setVJustify(Button::justify_t::CENTER);
		    auto font = Font::get(bigfont_outline); assert(font);
		    next->setSize(SDL_Rect{
		        (Frame::virtualScreenX - 160) / 2,
		        (Frame::virtualScreenY - font->height()),
		        160,
		        font->height(),
		        });
		    next->setCallback(next_button_func);
			next->setTickCallback([](Widget& widget){
			    auto button = static_cast<Button*>(&widget);
                button->setInvisible(story_text_pause == 0);
		        button->setText(inputs.hasController(0) ? "" : "Continue...");
			    });
		    next->setWidgetBack("back");
		    next->select();
		    next->setGlyphPosition(Button::glyph_position_t::CENTERED_TOP);
		}

		auto textbox2 = textbox1->addFrame("story_text_box");
		textbox2->setScrollBarsEnabled(false);
		textbox2->setAllowScrollBinds(false);
		textbox2->setHollow(true);
		textbox2->setBorder(0);

		static auto change_box_size = [](float lines){
		    assert(main_menu_frame);
		    auto font = Font::get(bigfont_outline); assert(font);
		    auto textbox1 = main_menu_frame->findFrame("story_text_box");
		    textbox1->setSize(SDL_Rect{
		        160,
		        Frame::virtualScreenY - (int)(font->height() * std::max(lines + 2.f, 0.f)),
		        Frame::virtualScreenX - 320,
		        (int)(font->height() * std::max(lines + 1.f, 0.f)),
		        });
		    textbox1->setActualSize(SDL_Rect{
		        0,
		        0,
		        textbox1->getSize().w,
		        textbox1->getSize().h,
		        });
		    auto textbox2 = textbox1->findFrame("story_text_box");
		    textbox2->setSize(SDL_Rect{
		        font->height() / 2,
		        font->height() / 2,
		        Frame::virtualScreenX - 320 - font->height(),
		        (int)(font->height() * std::max(lines, 0.f)),
		        });
		    textbox2->setActualSize(SDL_Rect{
		        textbox2->getActualSize().x,
		        textbox2->getActualSize().y,
		        textbox2->getSize().w,
		        font->height() * 100,
		        });
		    };
		change_box_size(story_text_box_scale);

		static auto adjust_box_size = [](){
		    float f = story_text_box_size;
		    float diff = story_text_box_scale - f;
		    if (fabs(diff) < 0.1) {
		        story_text_box_scale -= diff;
		    } else {
		        story_text_box_scale -= diff / TICKS_PER_SECOND;
		    }
            change_box_size(story_text_box_scale);
		    };

		auto field = textbox2->addField("text", 1 << 16);
		field->setFont(bigfont_outline);
		field->setSize(textbox2->getActualSize());
		field->setHJustify(Field::justify_t::CENTER);
		field->setVJustify(Field::justify_t::TOP);
		field->setColor(makeColor(255, 255, 255, 255));

		textbox1->setTickCallback([](Widget& widget){
			const float inc = 1.f * ((float)TICKS_PER_SECOND / (float)fpsLimit);
			auto textbox1 = static_cast<Frame*>(&widget);
			auto story_font = Font::get(bigfont_outline); assert(story_font);
			auto backdrop = main_menu_frame->findImage("backdrop"); assert(backdrop);
			if (backdrop && !story_text_pause) {
				story_image_fade = std::max(0.f, story_image_fade - inc);
		        float factor = story_image_fade - story_font->height();
		        Uint8 c = 255 * (fabs(factor) / story_font->height());
		        backdrop->color = makeColor(c, c, c, 255);
		        if (factor <= 0.f && story_image_advanced) {
		            story_image_advanced = false;
			        story_image_index = (story_image_index + 1) % story.images.size();
			        backdrop->path = story.images[story_image_index];
		        }
			}
			if (story_text_scroll > 0.f) {
				int old_story_text_scroll = (int)story_text_scroll;
				story_text_scroll -= inc;
				if (story_text_scroll < 0.f) {
					story_text_scroll = 0.f;
				}
				if ((int)story_text_scroll != old_story_text_scroll) {
					auto textbox2 = textbox1->findFrame("story_text_box");
					assert(textbox2);
					auto size = textbox2->getActualSize();
					++size.y;
					textbox2->setActualSize(size);
				}
			} else {
				if (story_text_pause > 0) {
				    if (!story.press_a_to_advance) {
					    --story_text_pause;
				    }
					if (story_text_pause == 0) {
						if (story_text_end == true) {
							story_end_func();
						} else {
							story_text_scroll = story_font->height() * story_text_box_size;
						}
					}
				} else {
					story_text_writer -= 1.f;
					if (story_text_writer <= 0.f) {
						story_text_writer = fmodf(story_text_writer, 1.f);
						auto textbox2 = textbox1->findFrame("story_text_box");
						assert(textbox2);
						auto text = textbox2->findField("text");
						assert(text);
						size_t text_index = 0u;
						char* buf = const_cast<char*>(text->getText());
						int chars = story_text_chars;
						size_t len = strlen(buf);
					    for (;
					        text_index < story.text.size() && chars >= story.text[text_index].size();
					        chars -= story.text[text_index].size(), ++text_index);
						if (text_index < story.text.size()) {
							char pc = story.text[text_index][std::max(chars - 1, 0)];
							char c = story.text[text_index][chars];
							char nc = story.text[text_index][chars + 1];
							++story_text_chars;
							if (c == '\n') {
							    ++story_text_lines;
							    if (story_text_lines >= story_text_box_size) {
							        story_text_lines = 0;
								    story_text_pause = fpsLimit * 5;
							    }
							    if (nc == '^') {
							        story_image_advanced = true;
							        story_image_fade = story_font->height() * 2;
							    }
							} else if (c == '^') {
							    if (pc != '\n') {
							        story_image_advanced = true;
							        story_image_fade = story_font->height() * 2;
							    }
								return; // skip printing this character
							} else if (c == '#') {
								story_text_writer += fpsLimit / 10.f;
								return; // skip printing this character
							} else if (c == '*') {
								story_text_adjust_box = true;
								return; // skip printing this character
							} else if (story_text_adjust_box) {
								story_text_adjust_box = false;
								if (c >= '1' && c <= '9') {
								    story_text_box_size = (int)(c - '0');
								    return; // skip printing this character
								} else if (c == '0') {
								    story_text_box_size = -2;
								    return; // skip printing this character
								}
							} else {
								story_text_writer += fpsLimit / 30.f;
							}
							buf[len] = c;
							buf[len + 1] = '\0';
						} else {
							story_text_pause = fpsLimit * 5;
							story_text_end = true;
						}
					}
				}
			}
		    adjust_box_size();
		    if (story_skip_timer > 0.f) {
		        story_skip_timer -= inc;
		        if (story_skip_timer <= 0.f) {
		            story_skip_timer = 0.f;
		            story_skip = 0;
		            auto back_button = main_menu_frame->findButton("back");
		            back_button->setText("Skip story");
		        }
		    }
			});
	}

/******************************************************************************/

	static std::string settings_tab_name;

	struct Setting {
		enum class Type : Uint8 {
			Boolean = 0,
			Slider = 1,
			Customize = 2,
			BooleanWithCustomize = 3,
			Dropdown = 4,
			Binding = 5,
			//Field = 6,
		};
		Type type;
		const char* name;
	};

	void settingsApply() {
		bool reset_video = allSettings.save();

		// change video mode
		if (initialized && reset_video) {
		    int x = std::max(allSettings.resolution_x, 1280);
		    int y = std::max(allSettings.resolution_y, 720);
			if (!changeVideoMode(x, y)) {
				printlog("critical error! Attempting to abort safely...\n");
				mainloop = 0;
			}
		    if (!intro) {
		        setupSplitscreen();
		    }
		}

		// transmit server flags
		if ( initialized && !intro && multiplayer == SERVER ) {
			strcpy((char*)net_packet->data, "SVFL");
			SDLNet_Write32(svFlags, &net_packet->data[4]);
			net_packet->len = 8;
			if (multiplayer == SERVER) {
			    for ( int c = 1; c < MAXPLAYERS; ++c ) {
				    if ( client_disconnected[c] ) {
					    continue;
				    }
				    net_packet->address.host = net_clients[c - 1].host;
				    net_packet->address.port = net_clients[c - 1].port;
				    sendPacketSafe(net_sock, -1, net_packet, c - 1);
				    messagePlayer(c, MESSAGE_MISC, language[276]);
			    }
			}
			messagePlayer(clientnum, MESSAGE_MISC, language[276]);
		}

		// update volume for sound groups
		if (initialized) {
		    setGlobalVolume(master_volume, musvolume, sfxvolume, sfxAmbientVolume, sfxEnvironmentVolume);
		}
	}

	void settingsMount() {
		allSettings = AllSettings::load();
	}

	bool settingsSave() {
		return FileHelper::writeObject((std::string(outputdir) + "/config/config.json").c_str(), EFileFormat::Json, allSettings);
	}

	bool settingsLoad() {
		bool result = FileHelper::readObject((std::string(outputdir) + "/config/config.json").c_str(), allSettings);
		if (result) {
		    old_bindings = allSettings.bindings;
		}
		return result;
	}

	void settingsReset() {
		allSettings = AllSettings::reset();
		old_bindings = allSettings.bindings;
	}

	static void settingsCustomizeInventorySorting(Button&);

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

	static void settingsCustomizeInventorySorting(Button& button) {
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
			button->setColor(makeColor(255, 255, 255, 255));
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
			button->setHideKeyboardGlyphs(false);
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
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortWeapons = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortArmor = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortAmulets = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortBooks = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortTools = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortThrown = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortGems = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortPotions = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortScrolls = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortStaves = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortFood = slider.getValue(); },
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
					int x = slider->getHandleSize().x;
					int y = slider->getHandleSize().y;
					if (x || y) {
						icon->disabled = false;
						icon->pos.x = x;
						icon->pos.y = y;
					}
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

	static void settingsOpenDropdown(Button& button, const char* name, bool small_dropdown, void(*entry_func)(Frame::entry_t&)) {
		std::string dropdown_name = "setting_" + std::string(name) + "_dropdown";
		auto frame = static_cast<Frame*>(button.getParent());
		auto dropdown = frame->addFrame(dropdown_name.c_str()); assert(dropdown);
		dropdown->setSize(SDL_Rect{
			button.getSize().x,
			button.getSize().y,
			174,
			small_dropdown ? 181 : 362
			});
		dropdown->setActualSize(SDL_Rect{0, 0, dropdown->getSize().w, dropdown->getSize().h});
		dropdown->setColor(0);
		dropdown->setBorder(0);
		dropdown->setDropDown(true);

		auto background = dropdown->addImage(
			dropdown->getActualSize(),
			0xffffffff,
			small_dropdown ?
				"images/ui/Main Menus/Settings/Settings_Drop_ScrollBG01.png" :
				"images/ui/Main Menus/Settings/Settings_Drop_ScrollBG00.png",
			"background"
		);

		int border = 4;
		auto dropdown_list = dropdown->addFrame("list");
		dropdown_list->setSize(SDL_Rect{0, border, dropdown->getSize().w, dropdown->getSize().h - border*2});
		dropdown_list->setActualSize(SDL_Rect{0, 0, dropdown_list->getSize().w, dropdown_list->getSize().h});
		dropdown_list->setColor(0);
		dropdown_list->setBorder(0);
		dropdown_list->setFont(bigfont_outline);
		dropdown_list->setListOffset(SDL_Rect{0, 11, 0, 0});
		dropdown_list->setListJustify(Frame::justify_t::CENTER);
		dropdown_list->setScrollBarsEnabled(false);
		dropdown_list->addWidgetMovement("MenuListCancel", button.getName());

		for (int i = 0;; ++i) {
			auto str = std::string("__") + std::to_string(i);
			auto find = button.getWidgetActions().find(str);
			if (find != button.getWidgetActions().end()) {
				auto entry_name = find->second.c_str();
				auto entry = dropdown_list->addEntry(entry_name, false);
				entry->text = entry_name;
				entry->click = entry_func;
				entry->ctrlClick = entry_func;
				dropdown_list->resizeForEntries();
				auto size = dropdown_list->getActualSize();
				size.h += 14;
				dropdown_list->setActualSize(size);
				if (strcmp(button.getText(), entry_name) == 0) {
					dropdown_list->setSelection(i);
				}
			}
			else {
				auto str = std::string("~__") + std::to_string(i);
				auto find = button.getWidgetActions().find(str);
				if (find != button.getWidgetActions().end()) {
					auto entry_name = find->second.c_str();
					auto entry = dropdown_list->addEntry(entry_name, false);
					entry->text = entry_name;
					entry->click = [](Frame::entry_t&){soundError();};
					entry->ctrlClick = entry->click;
					entry->color = makeColor(127, 127, 127, 255);
					dropdown_list->resizeForEntries();
					auto size = dropdown_list->getActualSize();
					size.h += 14;
					dropdown_list->setActualSize(size);
					if (strcmp(button.getText(), entry_name) == 0) {
						dropdown_list->setSelection(i);
					}
				} else {
					break;
				}
			}
		}
		dropdown_list->scrollToSelection(true);
		dropdown_list->activate();

		auto selection = dropdown_list->addImage(
			SDL_Rect{8, 0, 158, 30},
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_Drop_SelectBacking00.png",
			"selection"
		);

		dropdown_list->setTickCallback([](Widget& widget){
			Frame* dropdown_list = static_cast<Frame*>(&widget); assert(dropdown_list);
			auto selection = dropdown_list->findImage("selection"); assert(selection);
			bool inFrame = dropdown_list->capturesMouse() || !inputs.getVirtualMouse(0)->draw_cursor;
			if (inFrame && dropdown_list->getSelection() >= 0 && dropdown_list->getSelection() < dropdown_list->getEntries().size()) {
				selection->disabled = false;
				int entrySize = 0;
				Font* _font = Font::get(bigfont_outline);
				if (_font != nullptr) {
					entrySize = _font->height();
					entrySize += entrySize / 2;
				}
				selection->pos.y = dropdown_list->getSelection() * entrySize + 8;
			} else {
				selection->disabled = true;
			}
			});
	}

	static void settingsResolution(Button& button) {
		settingsOpenDropdown(button, "resolution", false, [](Frame::entry_t& entry){
			soundActivate();
			int new_xres, new_yres;
			sscanf(entry.name.c_str(), "%d x %d", &new_xres, &new_yres);
			allSettings.resolution_x = new_xres;
			allSettings.resolution_y = new_yres;
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_resolution_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_resolution_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
			});
	}

	static void settingsWindowMode(Button& button) {
		settingsOpenDropdown(button, "window_mode", true, [](Frame::entry_t& entry){
			soundActivate();
			do {
				if (entry.name == "Windowed") {
					allSettings.window_mode = 0;
					break;
				}
				if (entry.name == "Fullscreen") {
					allSettings.window_mode = 1;
					break;
				}
				if (entry.name == "Borderless") {
					allSettings.window_mode = 2;
					break;
				}
			} while (0);
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_window_mode_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_window_mode_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
			});
	}

	static int settingsAddSubHeader(Frame& frame, int y, const char* name, const char* text, bool generic_window = false) {
		std::string fullname = std::string("subheader_") + name;
		auto image = frame.addImage(
			SDL_Rect{0, y, frame.getSize().w, 42},
			0xffffffff,
			generic_window?
			"images/ui/Main Menus/Settings/GenericWindow/UI_MM14_HeaderBacking00.png":
			"images/ui/Main Menus/Settings/Settings_SubHeading_Backing00.png",
			(fullname + "_image").c_str()
		);
		auto field = frame.addField((fullname + "_field").c_str(), 128);
		field->setSize(image->pos);
		field->setFont(bigfont_outline);
		field->setText(text);
		field->setJustify(Field::justify_t::CENTER);
		Text* text_image = Text::get(field->getText(), field->getFont(),
			field->getTextColor(), field->getOutlineColor());
		int w = text_image->getWidth();
		auto fleur_left = frame.addImage(
			SDL_Rect{ (image->pos.w - w) / 2 - 26 - 8, y + 6, 26, 30 },
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_left").c_str()
		);
		auto fleur_right = frame.addImage(
			SDL_Rect{ (image->pos.w + w) / 2 + 8, y + 6, 26, 30 },
			0xffffffff,
			"images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_right").c_str()
		);
		return image->pos.h + 6;
	}

	static int settingsAddOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool _short = false
	) {
		std::string fullname = std::string("setting_") + name;
		auto image = frame.addImage(
			SDL_Rect{0, y, _short ? 278 : 382, 52},
			0xffffffff,
			_short ?
			"images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png":
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

	static int settingsAddBinding(
		Frame& frame,
		int y,
		int player_index,
		int device_index,
		const char* binding,
		const char* tip,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + binding;
		int result = settingsAddOption(frame, y, binding, binding, tip);
		auto button = frame.addButton((fullname + "_binding_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y + 4,
			320,
			44});
		button->setFont(smallfont_outline);
		auto& bindings =
			device_index == 0 ? allSettings.bindings.kb_mouse_bindings[player_index]:
			device_index == 1 ? allSettings.bindings.gamepad_bindings[player_index]:
			allSettings.bindings.joystick_bindings[player_index];
		auto find = bindings.find(binding);
		if (find != bindings.end()) {
		    button->setText(find->second.c_str());
		    auto glyph = Input::getGlyphPathForInput(button->getText());
		    button->setIcon(glyph.c_str());
		} else {
			button->setText(emptyBinding);
		}
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		button->setBackground("images/ui/Main Menus/Settings/GenericWindow/Settings_Button_Binding00.png");
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		button->setGlyphPosition(Button::glyph_position_t::CENTERED_BOTTOM);
		return result;
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
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
		button->setWidgetSearchParent(frame.getParent()->getName());
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
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetLeft((fullname + "_button").c_str());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		auto boolean = frame.findButton((fullname + "_button").c_str()); assert(boolean);
		boolean->setWidgetSearchParent(frame.getParent()->getName());
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
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
		button->setWidgetSearchParent(frame.getParent()->getName());
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
		const char* selected,
		void (*callback)(Button&),
		const std::set<int>& grayed_items = {})
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
		auto button = frame.addButton((fullname + "_dropdown_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y,
			174,
			52});
		button->setFont(bigfont_outline);
		button->setText(selected);
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		button->setBackground("images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02.png");
		button->setBackgroundHighlighted("images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02_Highlighted.png");
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		for (int i = 0; i < items.size(); ++i) {
			if (grayed_items.find(i) == grayed_items.end()) {
				button->addWidgetAction((std::string("__") + std::to_string(i)).c_str(), items[i]);
			} else {
				button->addWidgetAction((std::string("~__") + std::to_string(i)).c_str(), items[i]);
			}
		}
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
		void (*callback)(Slider&),
		bool _short = false)
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip, _short);
		auto box = frame.addImage(
			SDL_Rect{_short ? 298 : 402, y + 4, 132, 44},
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
		slider->setOrientation(Slider::orientation_t::SLIDER_HORIZONTAL);
		slider->setMinValue(minValue);
		slider->setMaxValue(maxValue);
		slider->setBorder(16);
		slider->setValue(value);
		slider->setRailSize(SDL_Rect{field->getSize().x + field->getSize().w + 32, y + 14, _short ? 282 : 450, 24});
		slider->setHandleSize(SDL_Rect{0, 0, 52, 42});
		slider->setCallback(callback);
		slider->setColor(makeColor(255,255,255,255));
		slider->setHighlightColor(makeColor(255,255,255,255));
		slider->setHandleImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Slide00.png");
		if (_short) {
			slider->setRailImage("images/ui/Main Menus/Settings/GenericWindow/Settings_ValueSlider_Backing_Short00.png");
		} else {
			slider->setRailImage("images/ui/Main Menus/Settings/Settings_ValueSlider_Backing00.png");
		}
		slider->setWidgetSearchParent(frame.getParent()->getName());
		slider->setWidgetBack("discard_and_exit");
		slider->setWidgetPageLeft("tab_left");
		slider->setWidgetPageRight("tab_right");
		slider->addWidgetAction("MenuAlt1", "restore_defaults");
		slider->addWidgetAction("MenuStart", "confirm_and_exit");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
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
			updateSettingSelection(*frame);
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
		slider->setBorder(24);
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{1040, 8, 30, 440});
		slider->setRailImage("images/ui/Main Menus/Settings/Settings_Slider_Backing00.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = 8 + actualSize.y;
			slider.setRailSize(railSize);
			slider.updateHandlePosition();
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = 8 + actualSize.y;
			slider->setRailSize(railSize);
			slider->updateHandlePosition();
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
		case Setting::Type::Dropdown:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_dropdown_button"),
				std::string(""));
		case Setting::Type::Binding:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_binding_button"),
				std::string(""));
		default:
			return std::make_pair(std::string(""), std::string(""));
		}
	}

	static void settingsSelect(Frame& frame, const Setting& setting) {
		auto names = getFullSettingNames(setting);
		auto widget = frame.findWidget(names.first.c_str(), false); assert(widget);
		widget->select();
	}

	static void settingsSubwindowFinalize(Frame& frame, int y) {
		auto size = frame.getActualSize();
		const int height = std::max(size.h, y);
		frame.setActualSize(SDL_Rect{0, 0, size.w, height});
		auto rock_background = frame.findImage("background"); assert(rock_background);
		rock_background->pos = frame.getActualSize();
		auto slider = frame.findSlider("scroll_slider"); assert(slider);
		slider->setValue(0.f);
		slider->setMinValue(0.f);
		slider->setMaxValue(height - size.h);
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

	static Frame* settingsGenericWindow(
		const char* name,
		const char* title,
		void (*defaults_callback)(Button&),
		void (*discard_callback)(Button&),
		void (*confirm_callback)(Button&))
	{
		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto window = dimmer->addFrame(name);
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 826) / 2,
			(Frame::virtualScreenY - 718) / 2,
			826,
			718});
		window->setActualSize(SDL_Rect{0, 0, 826, 718});
		window->setBorder(0);
		window->setColor(0);

		auto tooltip = window->addField("tooltip", 256);
		tooltip->setSize(SDL_Rect{30, 566, 766, 54});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setJustify(Field::justify_t::CENTER);
		tooltip->setText("");

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Window00.png",
			"background"
		);

		auto timber = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Window01.png",
			"timber"
		);
		timber->ontop = true;

		auto banner = window->addField("title", 64);
		banner->setSize(SDL_Rect{246, 22, 338, 24});
		banner->setFont(banner_font);
		banner->setText(title);
		banner->setJustify(Field::justify_t::CENTER);

		auto subwindow = window->addFrame("subwindow");
		subwindow->setSize(SDL_Rect{30, 64, 766, 502});
		subwindow->setActualSize(SDL_Rect{0, 0, 766, 502});
		subwindow->setScrollBarsEnabled(false);
		subwindow->setBorder(0);
		subwindow->setColor(0);
		subwindow->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			updateSettingSelection(*frame);
			updateSliderArrows(*frame);
			});

		auto rocks = subwindow->addImage(
			subwindow->getActualSize(),
			makeColor(127, 127, 127, 251),
			"images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Rocks00.png",
			"background"
		);
		rocks->tiled = true;

		auto slider = subwindow->addSlider("scroll_slider");
		slider->setBorder(24);
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{724, 8, 30, 486});
		slider->setRailImage("images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ScrollBar00.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ScrollBoulder00.png");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = 8 + actualSize.y;
			slider.setRailSize(railSize);
			slider.updateHandlePosition();
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = 8 + actualSize.y;
			slider->setRailSize(railSize);
			slider->updateHandlePosition();
			});

		auto sliderLeft = subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;

		auto sliderRight = subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;

		auto defaults = window->addButton("restore_defaults");
		defaults->setBackground("images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		defaults->setColor(makeColor(255, 255, 255, 255));
		defaults->setHighlightColor(makeColor(255, 255, 255, 255));
		defaults->setTextColor(makeColor(255, 255, 255, 255));
		defaults->setTextHighlightColor(makeColor(255, 255, 255, 255));
		defaults->setSize(SDL_Rect{156, 630, 164, 62});
		defaults->setText("Restore\nDefaults");
		defaults->setFont(smallfont_outline);
		defaults->setWidgetSearchParent(name);
		defaults->setWidgetBack("discard_and_exit");
		defaults->addWidgetAction("MenuStart", "confirm_and_exit");
		defaults->addWidgetAction("MenuAlt1", "restore_defaults");
		defaults->setWidgetRight("discard_and_exit");
		defaults->setCallback(defaults_callback);
		defaults->setHideKeyboardGlyphs(false);

		auto discard = window->addButton("discard_and_exit");
		discard->setBackground("images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		discard->setColor(makeColor(255, 255, 255, 255));
		discard->setHighlightColor(makeColor(255, 255, 255, 255));
		discard->setTextColor(makeColor(255, 255, 255, 255));
		discard->setTextHighlightColor(makeColor(255, 255, 255, 255));
		discard->setText("Discard\n& Exit");
		discard->setFont(smallfont_outline);
		discard->setSize(SDL_Rect{
			(window->getActualSize().w - 164) / 2,
			630,
			164,
			62}
		);
		discard->setCallback(discard_callback);
		discard->setWidgetSearchParent(name);
		discard->setWidgetBack("discard_and_exit");
		discard->addWidgetAction("MenuStart", "confirm_and_exit");
		discard->addWidgetAction("MenuAlt1", "restore_defaults");
		discard->setWidgetLeft("restore_defaults");
		discard->setWidgetRight("confirm_and_exit");
		discard->setHideKeyboardGlyphs(false);

		auto confirm = window->addButton("confirm_and_exit");
		confirm->setBackground("images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setTextColor(makeColor(255, 255, 255, 255));
		confirm->setTextHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setText("Confirm\n& Exit");
		confirm->setFont(smallfont_outline);
		confirm->setSize(SDL_Rect{504, 630, 164, 62});
		confirm->setCallback(confirm_callback);
		confirm->setWidgetSearchParent(name);
		confirm->setWidgetBack("discard_and_exit");
		confirm->addWidgetAction("MenuStart", "confirm_and_exit");
		confirm->addWidgetAction("MenuAlt1", "restore_defaults");
		confirm->setWidgetLeft("discard_and_exit");
		confirm->setHideKeyboardGlyphs(false);
		confirm->select();

		return window;
	}

	static void settingsMinimap(Button& button) {
		soundActivate();
		auto window = settingsGenericWindow("minimap", "MINIMAP",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap = Minimap::reset();
				settingsMinimap(button);
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_minimap_settings_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_minimap_settings_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);
		int y = 0;

		y += settingsAddSubHeader(*subwindow, y, "scale_header", "Scale", true);

		y += settingsAddSlider(*subwindow, y, "map_scale", "Map scale",
			"Scale the map to be larger or smaller.",
			allSettings.minimap.map_scale, 25, 100, true,
			[](Slider& slider){ allSettings.minimap.map_scale = slider.getValue(); }, true);

		y += settingsAddSlider(*subwindow, y, "icon_scale", "Icon scale",
			"Scale the size of icons on the map (such as players and allies)",
			allSettings.minimap.icon_scale, 25, 200, true,
			[](Slider& slider){ allSettings.minimap.icon_scale = slider.getValue(); }, true);

		y += settingsAddSubHeader(*subwindow, y, "transparency_header", "Transparency", true);

		y += settingsAddSlider(*subwindow, y, "foreground_opacity", "Foreground opacity",
			"Set the opacity of the minimap's foreground.",
			allSettings.minimap.foreground_opacity, 0, 100, true,
			[](Slider& slider){ allSettings.minimap.foreground_opacity = slider.getValue(); }, true);

		y += settingsAddSlider(*subwindow, y, "background_opacity", "Background opacity",
			"Set the opacity of the minimap's background.",
			allSettings.minimap.background_opacity, 0, 100, true,
			[](Slider& slider){ allSettings.minimap.background_opacity = slider.getValue(); }, true);

		hookSettings(*subwindow,
			{{Setting::Type::Slider, "map_scale"},
			{Setting::Type::Slider, "icon_scale"},
			{Setting::Type::Slider, "foreground_opacity"},
			{Setting::Type::Slider, "background_opacity"},
			});
		settingsSubwindowFinalize(*subwindow, y);
		settingsSelect(*subwindow, {Setting::Type::Slider, "map_scale"});
	}

	static void settingsMessages(Button& button) {
		soundActivate();
		auto window = settingsGenericWindow("messages", "MESSAGES",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages = Messages::reset();
				settingsMessages(button);
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_show_messages_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_show_messages_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);
		int y = 0;

		y += settingsAddSubHeader(*subwindow, y, "categories_header", "Categories", true);

		y += settingsAddBooleanOption(*subwindow, y, "messages_combat", "Combat messages",
			"Enable report of damage received or given in combat.",
			allSettings.show_messages.combat, [](Button& button){allSettings.show_messages.combat = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_status", "Status messages",
			"Enable report of player character status changes and other passive effects.",
			allSettings.show_messages.status, [](Button& button){allSettings.show_messages.status = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_inventory", "Inventory messages",
			"Enable report of inventory and item appraisal messages.",
			allSettings.show_messages.inventory, [](Button& button){allSettings.show_messages.inventory = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_equipment", "Equipment messages",
			"Enable report of player equipment changes.",
			allSettings.show_messages.equipment, [](Button& button){allSettings.show_messages.equipment = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_world", "World messages",
			"Enable report of diegetic messages, such as speech and text.",
			allSettings.show_messages.world, [](Button& button){allSettings.show_messages.world = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_chat", "Player chat",
			"Enable multiplayer chat.",
			allSettings.show_messages.chat, [](Button& button){allSettings.show_messages.chat = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_progression", "Progression messages",
			"Enable report of player character progression messages (ie level-ups).",
			allSettings.show_messages.progression, [](Button& button){allSettings.show_messages.progression = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_interaction", "Interaction messages",
			"Enable report of player interactions with the world.",
			allSettings.show_messages.interaction, [](Button& button){allSettings.show_messages.interaction = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_inspection", "Inspection messages",
			"Enable player inspections of world objects.",
			allSettings.show_messages.inspection, [](Button& button){allSettings.show_messages.inspection = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_hint", "Hint messages",
			"Enable cryptic hints for certain items, world events, etc.",
			allSettings.show_messages.hint, [](Button& button){allSettings.show_messages.hint = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_obituary", "Obituary messages",
			"Enable obituary messages for player deaths.",
			allSettings.show_messages.obituary, [](Button& button){allSettings.show_messages.obituary = button.isPressed();});

		hookSettings(*subwindow,
			{{Setting::Type::Boolean, "messages_combat"},
			{Setting::Type::Boolean, "messages_status"},
			{Setting::Type::Boolean, "messages_inventory"},
			{Setting::Type::Boolean, "messages_equipment"},
			{Setting::Type::Boolean, "messages_world"},
			{Setting::Type::Boolean, "messages_chat"},
			{Setting::Type::Boolean, "messages_progression"},
			{Setting::Type::Boolean, "messages_interaction"},
			{Setting::Type::Boolean, "messages_inspection"},
			{Setting::Type::Boolean, "messages_hint"},
			{Setting::Type::Boolean, "messages_obituary"},
			});
		settingsSubwindowFinalize(*subwindow, y);
		settingsSelect(*subwindow, {Setting::Type::Boolean, "messages_combat"});
	}

	static int getDeviceIndexForName(const char* name) {
		if (strcmp(name, "KB & Mouse") == 0) { return 0; }
		if (strcmp(name, "Gamepad") == 0) { return 1; }
		if (strcmp(name, "Joystick") == 0) { return 2; }
		else { return -1; }
	}

	static bool settingsBind(int player_index, int device_index, const char* binding, const char* input) {
		assert(binding);
		auto& bindings =
			device_index == 0 ? allSettings.bindings.kb_mouse_bindings[player_index]:
			device_index >= 1 && device_index <= 4 ? allSettings.bindings.gamepad_bindings[player_index]:
			allSettings.bindings.joystick_bindings[player_index];
		if (input == nullptr) {
			bindings.erase(binding);
			return true;
		} else {
			std::string input_to_store;
			if (device_index >= 1 && device_index <= 4 && strncmp(input, "Pad", 3) == 0) {
				input_to_store = input + 4;
			}
			else if (device_index >= 5 && device_index <= 8 && strncmp(input, "Joy", 3) == 0) {
				input_to_store = input + 4;
			}
			else if (device_index == 0 && strncmp(input, "Pad", 3) && strncmp(input, "Joy", 3)) {
				input_to_store = input;
			}
			if (input_to_store.empty()) {
				return false;
			} else {
				auto find = bindings.find(binding);
				if (find == bindings.end()) {
					bindings.insert(std::make_pair(binding, input_to_store.c_str()));
				} else {
					find->second = input_to_store.c_str();
				}
				return true;
			}
		}
	}

	static void settingsBindings(int player_index, int device_index, Setting setting_to_select) {
		soundActivate();

		static bool bind_mode;
		static Button* bound_button;
		static std::string bound_binding;
		static std::string bound_input;
		static int bound_player;
		static int bound_device;

		bind_mode = false;
		bound_button = nullptr;
		bound_binding = "";
		bound_input = "";
		bound_player = player_index;
		bound_device = device_index;

		auto window = settingsGenericWindow("bindings", "BINDINGS",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings = Bindings::reset();
				settingsBindings(0, 0, {Setting::Type::Dropdown, "player_dropdown_button"});
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_bindings_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_bindings_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);

		std::vector<Setting> bindings;
		bindings.reserve(numBindings);
		for (int c = 0; c < numBindings; ++c) {
		    if (strcmp(defaultBindings[c][device_index + 1], hiddenBinding)) {
		        bindings.push_back({Setting::Type::Binding, defaultBindings[c][0]});
		    }
		}

		int y = 0;
		y += settingsAddSubHeader(*subwindow, y, "bindings_header", "Profiles", true);

		std::string player_str = "Player " + std::to_string(player_index + 1);
		y += settingsAddDropdown(*subwindow, y, "player_dropdown_button", "Player",
			"Select the player whose controls you wish to customize.",
			{"Player 1", "Player 2", "Player 3", "Player 4"}, player_str.c_str(),
			[](Button& button){
				soundActivate();
				settingsOpenDropdown(button, "player_dropdown", true,
					[](Frame::entry_t& entry){
						soundActivate();
						auto parent = main_menu_frame->findFrame("bindings");
						auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
						parent_background->removeSelf();
						int player_index = (int)(entry.name.back() - '1');
						settingsBindings(player_index, bound_device, {Setting::Type::Dropdown, "player_dropdown_button"});
					});
			});

		std::vector<const char*> devices = {
		    "KB & Mouse",
		    "Gamepad",
		    //"Joystick", // Maybe for the future.
		};

		y += settingsAddDropdown(*subwindow, y, "device_dropdown_button", "Device",
			"Select a controller for the given player.", devices, devices[device_index],
			[](Button& button){
				soundActivate();
				settingsOpenDropdown(button, "device_dropdown", true,
					[](Frame::entry_t& entry){
						soundActivate();
						auto parent = main_menu_frame->findFrame("bindings");
						auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
						parent_background->removeSelf();
						int device_index = getDeviceIndexForName(entry.text.c_str());
						settingsBindings(bound_player, device_index, {Setting::Type::Dropdown, "device_dropdown_button"});
					});
			});

		y += settingsAddSubHeader(*subwindow, y, "bindings_header", "Bindings", true);

		for (auto& binding : bindings) {
			char tip[256];
			snprintf(tip, sizeof(tip), "Bind an input device to %s", binding.name);
			y += settingsAddBinding(*subwindow, y, player_index, device_index, binding.name, tip,
				[](Button& button){
					soundToggle();
					auto name = std::string(button.getName());
					bind_mode = true;
					bound_button = &button;
					bound_input = button.getText();
					bound_binding = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_binding_button") - 1) - (sizeof("setting_") - 1));
					button.setText(". . .");
					auto subwindow = static_cast<Frame*>(button.getParent()); assert(subwindow);
					auto settings = static_cast<Frame*>(subwindow->getParent()); assert(settings);
					auto tooltip = settings->findField("tooltip"); assert(tooltip);
					char buf[256];
					snprintf(buf, sizeof(buf),
						"Binding \"%s\". Press ESC to cancel or DEL to delete the binding.\n"
						"The next input you activate will be bound to this action.",
						bound_binding.c_str());
					tooltip->setText(buf);
					Input::inputs[bound_player].setDisabled(true);
					for (auto button : subwindow->getButtons()) {
						button->setDisabled(true);
					}

					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto confirm = bindings->findButton("confirm_and_exit"); assert(confirm);
					auto discard = bindings->findButton("discard_and_exit"); assert(discard);
					auto defaults = bindings->findButton("restore_defaults"); assert(defaults);
					confirm->setDisabled(true);
					discard->setDisabled(true);
					defaults->setDisabled(true);
				});
		}

		window->setTickCallback([](Widget&){
			if (bind_mode) {
				if (bound_button && !Input::lastInputOfAnyKind.empty()) {
					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto tooltip = bindings->findField("tooltip"); assert(tooltip);
					if (Input::lastInputOfAnyKind == "Escape") {
						bound_button->setText(bound_input.c_str());
		                auto glyph = Input::getGlyphPathForInput(bound_button->getText());
		                bound_button->setIcon(glyph.c_str());
						char buf[256];
						snprintf(buf, sizeof(buf), "Cancelled rebinding \"%s\"", bound_binding.c_str());
						tooltip->setText(buf);
					} else if (Input::lastInputOfAnyKind == "Delete") {
						(void)settingsBind(bound_player, bound_device, bound_binding.c_str(), nullptr);
						bound_button->setText(emptyBinding);
		                bound_button->setIcon("");
						char buf[256];
						snprintf(buf, sizeof(buf), "Deleted \"%s\" binding.", bound_binding.c_str());
						tooltip->setText(buf);
					} else {
						bool result = settingsBind(bound_player, bound_device, bound_binding.c_str(), Input::lastInputOfAnyKind.c_str());
						if (!result) {
							goto bind_failed;
						}
						auto begin = Input::lastInputOfAnyKind.substr(0, 3);
						std:: string newinput = begin == "Pad" || begin == "Joy" ?
								Input::lastInputOfAnyKind.substr(4) : Input::lastInputOfAnyKind;
						bound_button->setText(newinput.c_str());
		                auto glyph = Input::getGlyphPathForInput(bound_button->getText());
		                bound_button->setIcon(glyph.c_str());
						char buf[256];
						snprintf(buf, sizeof(buf), "Bound \"%s\" to \"%s\"", bound_binding.c_str(), newinput.c_str());
						tooltip->setText(buf);
					}
					bound_button = nullptr;
bind_failed:
                    // fix a bug where this wasn't always cleared...
                    mousestatus[SDL_BUTTON_LEFT] = 0;
				}
				else if (!bound_button) {
					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto confirm = bindings->findButton("confirm_and_exit"); assert(confirm);
					auto discard = bindings->findButton("discard_and_exit"); assert(discard);
					auto defaults = bindings->findButton("restore_defaults"); assert(defaults);
					confirm->setDisabled(false);
					discard->setDisabled(false);
					defaults->setDisabled(false);

					auto subwindow = bindings->findFrame("subwindow"); assert(subwindow);
					for (auto button : subwindow->getButtons()) {
						button->setDisabled(false);
					}

					Input::inputs[bound_player].setDisabled(false);
					bound_binding = "";
					bind_mode = false;
				}
			}
			});

		hookSettings(*subwindow,
			{{Setting::Type::Dropdown, "player_dropdown_button"},
			{Setting::Type::Dropdown, "device_dropdown_button"},
			bindings[0],
			});
		hookSettings(*subwindow, bindings);
		settingsSubwindowFinalize(*subwindow, y);
		settingsSelect(*subwindow, setting_to_select);
	}

	static void settingsUI(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "add_items_to_hotbar"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "inventory", "Inventory Options");
		y += settingsAddBooleanOption(*settings_subwindow, y, "add_items_to_hotbar", "Add Items to Hotbar",
			"Automatically fill the hotbar with recently collected items.",
			allSettings.add_items_to_hotbar_enabled, [](Button& button){soundToggle(); allSettings.add_items_to_hotbar_enabled = button.isPressed();});
		y += settingsAddCustomize(*settings_subwindow, y, "inventory_sorting", "Inventory Sorting",
			"Customize the way items are automatically sorted in your inventory.",
			[](Button& button){allSettings.inventory_sorting = InventorySorting::load(); settingsCustomizeInventorySorting(button);});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "use_on_release", "Use on Release",
			"Activate an item as soon as the Use key is released in the inventory window.",
			allSettings.use_on_release_enabled, [](Button& button){soundToggle(); allSettings.use_on_release_enabled = button.isPressed();});
#endif

		y += settingsAddSubHeader(*settings_subwindow, y, "hud", "HUD Options");
		y += settingsAddCustomize(*settings_subwindow, y, "minimap_settings", "Minimap Settings",
			"Customize the appearance of the in-game minimap.",
			[](Button& button){allSettings.minimap = Minimap::load(); settingsMinimap(button);});
		y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "show_messages", "Show Messages",
			"Customize which messages will be logged to the player, if any.",
			allSettings.show_messages_enabled, [](Button& button){soundToggle(); allSettings.show_messages_enabled = button.isPressed();},
			[](Button& button){allSettings.show_messages = Messages::load(); settingsMessages(button);});
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_player_nametags", "Show Player Nametags",
			"Display the name of each player character above their avatar.",
			allSettings.show_player_nametags_enabled, [](Button& button){soundToggle(); allSettings.show_player_nametags_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_hud", "Show HUD",
			"Toggle the display of health and other status bars in game when the inventory is closed.",
			allSettings.show_hud_enabled, [](Button& button){soundToggle(); allSettings.show_hud_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_ip_address", "Streamer Mode",
			"If you're a streamer and know what doxxing is, definitely switch this on.",
			allSettings.show_ip_address_enabled, [](Button& button){soundToggle(); allSettings.show_ip_address_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},
			{Setting::Type::Boolean, "use_on_release"},
			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},
			{Setting::Type::Boolean, "show_hud"},
			{Setting::Type::Boolean, "show_ip_address"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},
			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},
			{Setting::Type::Boolean, "show_hud"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y);
		settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "add_items_to_hotbar"});
	}

	static void settingsVideo(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
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
		const char* arachnophobia_desc;
		if (intro) {
		    arachnophobia_desc = "Replace all giant spiders in the game with hostile crustaceans.";
		} else {
		    arachnophobia_desc = "Replace all giant spiders in the game with hostile crustaceans. (Updates at end of current dungeon level)";
		}
		y += settingsAddBooleanOption(*settings_subwindow, y, "arachnophobia_filter", "Arachnophobia Filter",
			arachnophobia_desc, allSettings.arachnophobia_filter_enabled,
			[](Button& button){soundToggle(); allSettings.arachnophobia_filter_enabled = button.isPressed();});

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

		int selected_res = 0;
		std::list<resolution> resolutions;
		getResolutionList(resolutions);
		std::vector<std::string> resolutions_formatted;
		std::vector<const char*> resolutions_formatted_ptrs;
		resolutions_formatted.reserve(resolutions.size());
		resolutions_formatted_ptrs.reserve(resolutions.size());

		int index;
		std::list<resolution>::iterator it;
		for (index = 0, it = resolutions.begin(); it != resolutions.end(); ++it, ++index) {
			auto& res = *it;
			const int x = std::get<0>(res);
			const int y = std::get<1>(res);
			char buf[32];
			snprintf(buf, sizeof(buf), "%d x %d", x, y);
			resolutions_formatted.push_back(std::string(buf));
			resolutions_formatted_ptrs.push_back(resolutions_formatted.back().c_str());
			if (allSettings.resolution_x == x && allSettings.resolution_y == y) {
				selected_res = index;
			}
		}

		const char* selected_mode = fullscreen ? (borderless ? "Borderless" : "Fullscreen") : "Windowed";

		y += settingsAddSubHeader(*settings_subwindow, y, "display", "Display");
#ifndef NINTENDO
		y += settingsAddDropdown(*settings_subwindow, y, "resolution", "Resolution", "Change the current window resolution.",
			resolutions_formatted_ptrs, resolutions_formatted_ptrs[selected_res],
			settingsResolution);
		y += settingsAddDropdown(*settings_subwindow, y, "window_mode", "Window Mode", "Change the current display mode.",
			{"Windowed", "Fullscreen", "Borderless"}, selected_mode,
			settingsWindowMode);
		y += settingsAddBooleanOption(*settings_subwindow, y, "vsync", "Vertical Sync",
			"Prevent screen-tearing by locking the game's refresh rate to the current display.",
			allSettings.vsync_enabled, [](Button& button){soundToggle(); allSettings.vsync_enabled = button.isPressed();});
#endif
		y += settingsAddBooleanOption(*settings_subwindow, y, "vertical_split", "Vertical Splitscreen",
			"For splitscreen with two-players: divide the screen along a vertical line rather than a horizontal one.",
			allSettings.vertical_split_enabled, [](Button& button){soundToggle(); allSettings.vertical_split_enabled = button.isPressed();});
		y += settingsAddSlider(*settings_subwindow, y, "gamma", "Gamma",
			"Adjust the brightness of the visuals in-game.",
			allSettings.gamma, 50, 200, true, [](Slider& slider){soundSlider(true); allSettings.gamma = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "fov", "Field of View",
			"Adjust the vertical field-of-view of the in-game camera.",
			allSettings.fov, 40, 100, false, [](Slider& slider){soundSlider(true); allSettings.fov = slider.getValue();});
#ifndef NINTENDO
		y += settingsAddSlider(*settings_subwindow, y, "fps", "FPS limit",
			"Control the frame-rate limit of the game window.",
			allSettings.fps, 30, 300, false, [](Slider& slider){soundSlider(true); allSettings.fps = slider.getValue();});
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

	static void settingsAudio(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Slider, "master_volume"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "volume", "Volume");
		y += settingsAddSlider(*settings_subwindow, y, "master_volume", "Master Volume",
			"Adjust the volume of all sound sources equally.",
			allSettings.master_volume, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.master_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "gameplay_volume", "Gameplay Volume",
			"Adjust the volume of most game sound effects.",
			allSettings.gameplay_volume, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.gameplay_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "ambient_volume", "Ambient Volume",
			"Adjust the volume of ominous subterranean sound-cues.",
			allSettings.ambient_volume, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.ambient_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "environment_volume", "Environment Volume",
			"Adjust the volume of flowing water and lava.",
			allSettings.environment_volume, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.environment_volume = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "music_volume", "Music Volume",
			"Adjust the volume of the game's soundtrack.",
			allSettings.music_volume, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.music_volume = slider.getValue();});

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

	static void settingsControls(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Customize, "bindings"});
			return;
		}
		int y = 0;

#ifndef NINTENDO
		y += settingsAddSubHeader(*settings_subwindow, y, "general", "General Settings");
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Modify controls for mouse, keyboard, gamepads, and other peripherals.",
			[](Button&){allSettings.bindings = Bindings::load(); settingsBindings(0, 0, {Setting::Type::Dropdown, "player_dropdown_button"});});

		y += settingsAddSubHeader(*settings_subwindow, y, "mouse_and_keyboard", "Mouse & Keyboard");
		y += settingsAddBooleanOption(*settings_subwindow, y, "numkeys_in_inventory", "Number Keys in Inventory",
			"Allow the player to bind inventory items to the hotbar using the number keys on their keyboard.",
			allSettings.numkeys_in_inventory_enabled, [](Button& button){soundToggle(); allSettings.numkeys_in_inventory_enabled = button.isPressed();});
		y += settingsAddSlider(*settings_subwindow, y, "mouse_sensitivity", "Mouse Sensitivity",
			"Control the speed by which mouse movement affects camera movement.",
			allSettings.mouse_sensitivity, 0, 100, false, [](Slider& slider){soundSlider(true); allSettings.mouse_sensitivity = slider.getValue();});
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
		// TODO we need a special window just for Nintendo Switch joycons.
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Modify controller bindings.",
			nullptr);
#else
		y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Gamepad Settings");
#endif
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_x", "Turn Sensitivity X",
			"Affect the horizontal sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_x, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.turn_sensitivity_x = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_y", "Turn Sensitivity Y",
			"Affect the vertical sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_y, 0, 100, true, [](Slider& slider){soundSlider(true); allSettings.turn_sensitivity_y = slider.getValue();});

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

	static void settingsGame(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
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

	static void recordsAdventureArchives(Button& button) {
		soundActivate();
	}

	static void recordsLeaderboards(Button& button) {
		soundActivate();
	}

	static void recordsDungeonCompendium(Button& button) {
		soundActivate();

		destroyMainMenu();
		createDummyMainMenu();

		beginFade(MainMenu::FadeDestination::HerxMidpointHuman);
	}

	static void recordsStoryIntroduction(Button& button) {
		soundActivate();

		destroyMainMenu();
		createDummyMainMenu();

		beginFade(MainMenu::FadeDestination::IntroStoryScreen);
	}

	static void recordsCredits(Button& button) {
		soundActivate();

		destroyMainMenu();
		createDummyMainMenu();

		auto back_button = main_menu_frame->addButton("back");
		back_button->setHideSelectors(true);
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
		back_button->setGlyphPosition(Widget::glyph_position_t::BOTTOM_RIGHT);
		back_button->setCallback([](Button& b){
			destroyMainMenu();
			createMainMenu(false);
			mainHallOfRecords(b);
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto credits = buttons->findButton("CREDITS"); assert(credits);
			credits->select();
			});
		back_button->setWidgetBack("back");
		back_button->select();

		auto font = Font::get(bigfont_outline); assert(font);

		static float credits_scroll = 0.f;

		auto credits = main_menu_frame->addFrame("credits");
		credits->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		credits->setActualSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY + font->height() * 81});
		credits->setScrollBarsEnabled(false);
		credits->setAllowScrollBinds(false);
		credits->setHollow(true);
		credits->setBorder(0);
		credits->setTickCallback([](Widget& widget){
			const float inc = 1.f * ((float)TICKS_PER_SECOND / (float)fpsLimit);
			int old_credits_scroll = (int)credits_scroll;
			credits_scroll += inc;
			if (old_credits_scroll != (int)credits_scroll) {
				auto credits = static_cast<Frame*>(&widget);
				auto size = credits->getActualSize();
				size.y += 1;
				if (size.y >= size.h) {
					size.y = 0;
				}
				credits->setActualSize(size);
			}
			});

		// titles
		auto text1 = credits->addField("text1", 1024);
		text1->setFont(bigfont_outline);
		text1->setColor(makeColor(255, 191, 32, 255));
		text1->setHJustify(Field::justify_t::CENTER);
		text1->setVJustify(Field::justify_t::TOP);
		text1->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 90});
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
#ifdef USE_FMOD
			u8" \n"
#endif
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
		text2->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * 90});
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
#ifdef USE_FMOD
			u8"Made with FMOD Core by Firelight Technologies Pty Ltd.\n"
#endif
			u8"http://www.baronygame.com/\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8"Thank you!\n"
		);
	}

	static void recordsBackToMainMenu(Button& button) {
		soundCancel();
        destroyMainMenu();
        createMainMenu(false);
        assert(main_menu_frame);
		auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
		auto records = buttons->findButton("HALL OF RECORDS");
		if (records) {
			records->select();
		}
	}

/******************************************************************************/

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
			if (isCharacterValidFromDLC(*stats[index], c) == VALID_OK_CHARACTER ) {
				result.emplace_back(classes_in_order[c + 1]);
			}
		}
		return result;
	}

	static auto male_button_fn = [](Button& button, int index) {
		button.setColor(makeColor(255, 255, 255, 255));
		button.setHighlightColor(makeColor(255, 255, 255, 255));
		auto card = static_cast<Frame*>(button.getParent());
		auto female = card->findButton("female");
		if (female) {
			female->setColor(makeColor(127, 127, 127, 255));
			female->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		stats[index]->clearStats();
		initClass(index);
	};

	static auto female_button_fn = [](Button& button, int index) {
		button.setColor(makeColor(255, 255, 255, 255));
		button.setHighlightColor(makeColor(255, 255, 255, 255));
		auto card = static_cast<Frame*>(button.getParent());
		auto male = card->findButton("male");
		if (male) {
			male->setColor(makeColor(127, 127, 127, 255));
			male->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		stats[index]->clearStats();
		initClass(index);
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
		card->setClickable(true);
		card->setHideSelectors(true);
		card->setHideGlyphs(true);

		return card;
	}

	static void characterCardGameSettingsMenu(int index) {
		bool local = currentLobbyType == LobbyType::LobbyLocal;

		auto card = initCharacterCard(index, 664);

		static void (*back_fn)(int) = [](int index){
			characterCardLobbySettingsMenu(index);
			svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
			svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
			svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
			svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
			svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
			svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
			svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
			svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
			svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
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

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDiff_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 606, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		confirm->setWidgetUp((std::string("setting") + std::to_string(num_settings - 1)).c_str());
		switch (index) {
		case 0: confirm->setCallback([](Button&){soundActivate(); back_fn(0);}); break;
		case 1: confirm->setCallback([](Button&){soundActivate(); back_fn(1);}); break;
		case 2: confirm->setCallback([](Button&){soundActivate(); back_fn(2);}); break;
		case 3: confirm->setCallback([](Button&){soundActivate(); back_fn(3);}); break;
		}*/
	}

	static void characterCardLobbySettingsMenu(int index) {
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
				auto other_button = frame->findButton(mode);
				if (other_button) {
				    other_button->setPressed(false);
				}
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

		// TODO on non-english languages, normal text will need to be used.
        // here's the code for that, but how to gate it?
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
		custom_difficulty->setColor(makeColor(255, 255, 255, 255));
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

		auto multiplayer_header = card->addField("multiplayer_header", 64);
		multiplayer_header->setSize(SDL_Rect{70, 328, 182, 34});
		multiplayer_header->setFont(smallfont_outline);
		multiplayer_header->setText("LOBBY SETTINGS");
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

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
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
		}*/
	}

	static void characterCardRaceMenu(int index) {
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
						stats[index]->appearance = 0;
						stats[index]->sex = FEMALE;
					}
					else if (stats[index]->playerRace == RACE_INCUBUS) {
						stats[index]->appearance = 0;
						stats[index]->sex = MALE;
					}
					else if (stats[index]->playerRace == RACE_HUMAN) {
						stats[index]->appearance = rand() % NUMAPPEARANCES;
					}
					else {
						stats[index]->appearance = 0;
					}
				} else {
					auto other_button = frame->findButton(race);
					if (other_button) {
					    other_button->setPressed(false);
					}
				}
			}
			auto disable_abilities = frame->findButton("disable_abilities");
			if (disable_abilities) {
				disable_abilities->setPressed(false);
			}
			stats[index]->clearStats();
			initClass(index);
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
		appearances->setClickable(true);
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
			auto appearance_downarrow = card->findButton("appearance_downarrow");
			if (frame->isActivated()) {
				appearance_uparrow->setDisabled(false);
				appearance_uparrow->setInvisible(false);
				appearance_downarrow->setDisabled(false);
				appearance_downarrow->setInvisible(false);
			} else if (!frame->isSelected() &&
				!appearance_uparrow->isSelected() &&
				!appearance_downarrow->isSelected()) {
				appearance_uparrow->setDisabled(true);
				appearance_uparrow->setInvisible(true);
				appearance_downarrow->setDisabled(true);
				appearance_downarrow->setInvisible(true);
			}
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

		auto appearance_uparrow = card->addButton("appearance_uparrow");
		appearance_uparrow->setSize(SDL_Rect{198, 58, 32, 20});
		appearance_uparrow->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonUp_00.png");
		appearance_uparrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_uparrow->setColor(makeColor(255, 255, 255, 255));
		appearance_uparrow->setDisabled(true);
		appearance_uparrow->setInvisible(true);
		appearance_uparrow->setCallback([](Button& button){
			auto card = static_cast<Frame*>(button.getParent());
			auto appearances = card->findFrame("appearances"); assert(appearances);
			int selection = std::max(appearances->getSelection() - 1, 0);
			appearances->setSelection(selection);
			appearances->scrollToSelection();
			appearances->activateSelection();
			button.select();
			});

		auto appearance_downarrow = card->addButton("appearance_downarrow");
		appearance_downarrow->setSize(SDL_Rect{198, 114, 32, 20});
		appearance_downarrow->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonDown_00.png");
		appearance_downarrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_downarrow->setColor(makeColor(255, 255, 255, 255));
		appearance_downarrow->setDisabled(true);
		appearance_downarrow->setInvisible(true);
		appearance_downarrow->setCallback([](Button& button){
			auto card = static_cast<Frame*>(button.getParent());
			auto appearances = card->findFrame("appearances"); assert(appearances);
			int selection = std::min(appearances->getSelection() + 1,
				(int)appearances->getEntries().size() - 1);
			appearances->setSelection(selection);
			appearances->scrollToSelection();
			appearances->activateSelection();
			button.select();
			});

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
			case 0: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 0); entry.parent.activate();}; break;
			case 1: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 1); entry.parent.activate();}; break;
			case 2: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 2); entry.parent.activate();}; break;
			case 3: entry->click = [](Frame::entry_t& entry){soundActivate(); appearance_fn(entry, 3); entry.parent.activate();}; break;
			}
			if (stats[index]->appearance == c) {
				appearances->setSelection(c);
				appearances->scrollToSelection();
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
		disable_abilities->setSize(SDL_Rect{194, 284, 44, 44});
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
		if (stats[index]->sex == MALE) {
			male_button->setColor(makeColor(255, 255, 255, 255));
			male_button->setHighlightColor(makeColor(255, 255, 255, 255));
		} else {
			male_button->setColor(makeColor(127, 127, 127, 255));
			male_button->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		if (stats[index]->sex == FEMALE) {
			female_button->setColor(makeColor(255, 255, 255, 255));
			female_button->setHighlightColor(makeColor(255, 255, 255, 255));
		} else {
			female_button->setColor(makeColor(127, 127, 127, 255));
			female_button->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		show_race_info->setColor(makeColor(255, 255, 255, 255));
		show_race_info->setHighlightColor(makeColor(255, 255, 255, 255));
		show_race_info->setBackground("images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonShowDetails_00.png");
		show_race_info->setSize(SDL_Rect{168, 344, 110, 52});
		show_race_info->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		show_race_info->addWidgetAction("MenuStart", "confirm");
		show_race_info->setWidgetBack("back_button");
		show_race_info->setWidgetUp("disable_abilities");
		show_race_info->setWidgetDown("confirm");
		show_race_info->setWidgetLeft("female");

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
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
		}*/
	}

	static void characterCardClassMenu(int index) {
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
			int i = std::min(std::max(0, client_classes[index] + 1), num_classes - 1);
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
			slider->setBorder(24);
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
		class_info->setColor(makeColor(255, 255, 255, 255));
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
			button->setColor(makeColor(255, 255, 255, 255));
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
			case 1: button->setCallback([](Button& button){button_fn(button, 1);}); break;
			case 2: button->setCallback([](Button& button){button_fn(button, 2);}); break;
			case 3: button->setCallback([](Button& button){button_fn(button, 3);}); break;
			}
		}

		auto first_button = subframe->findButton(reduced_class_list[0]); assert(first_button);
		first_button->select();

		/*auto confirm = card->addButton("confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
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
		}*/
	}

	static void createCharacterCard(int index) {
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
		} else if (currentLobbyType == LobbyType::LobbyLAN) {
			switch (index) {
			case 0: (void)createBackWidget(card,[](Button& button){soundCancel(); createWaitingStone(0);}); break;
			case 1: (void)createBackWidget(card,[](Button& button){soundCancel(); createWaitingStone(1);}); break;
			case 2: (void)createBackWidget(card,[](Button& button){soundCancel(); createWaitingStone(2);}); break;
			case 3: (void)createBackWidget(card,[](Button& button){soundCancel(); createWaitingStone(3);}); break;
			}
		} else if (currentLobbyType == LobbyType::LobbyOnline) {
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
		name_field->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		name_field->setScroll(true);
		name_field->setGuide((std::string("Enter a name for Player ") + std::to_string(index + 1)).c_str());
		name_field->setFont(smallfont_outline);
		name_field->setText(stats[index]->name);
		name_field->setSize(SDL_Rect{90, 34, 146, 28});
		name_field->setColor(makeColor(166, 123, 81, 255));
		name_field->setBackgroundSelectAllColor(makeColor(52, 30, 22, 255));
		name_field->setBackgroundActivatedColor(makeColor(52, 30, 22, 255));
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
		case 0:
			name_field->setCallback([](Field& field){name_field_fn(field.getText(), 0);});
			name_field->setTickCallback([](Widget& widget){
				Field* field = static_cast<Field*>(&widget);
				name_field_fn(field->getText(), 0);
				});
			break;
		case 1:
			name_field->setCallback([](Field& field){name_field_fn(field.getText(), 1);});
			name_field->setTickCallback([](Widget& widget){
				Field* field = static_cast<Field*>(&widget);
				name_field_fn(field->getText(), 1);
				});
			break;
		case 2:
			name_field->setCallback([](Field& field){name_field_fn(field.getText(), 2);});
			name_field->setTickCallback([](Widget& widget){
				Field* field = static_cast<Field*>(&widget);
				name_field_fn(field->getText(), 2);
				});
			break;
		case 3:
			name_field->setCallback([](Field& field){name_field_fn(field.getText(), 3);});
			name_field->setTickCallback([](Widget& widget){
				Field* field = static_cast<Field*>(&widget);
				name_field_fn(field->getText(), 3);
				});
			break;
		}
		name_field->select();

		auto randomize_name = card->addButton("randomize_name");
		randomize_name->setColor(makeColor(255, 255, 255, 255));
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
		game_settings->setColor(makeColor(255, 255, 255, 255));
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
		if (stats[index]->sex == MALE) {
			male_button->setColor(makeColor(255, 255, 255, 255));
			male_button->setHighlightColor(makeColor(255, 255, 255, 255));
		} else {
			male_button->setColor(makeColor(127, 127, 127, 255));
			male_button->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		if (stats[index]->sex == FEMALE) {
			female_button->setColor(makeColor(255, 255, 255, 255));
			female_button->setHighlightColor(makeColor(255, 255, 255, 255));
		} else {
			female_button->setColor(makeColor(127, 127, 127, 255));
			female_button->setHighlightColor(makeColor(127, 127, 127, 255));
		}
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
		race_button->setColor(makeColor(255, 255, 255, 255));
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

			auto card = static_cast<Frame*>(button.getParent());

			// select a random sex
			stats[index]->sex = (sex_t)(rand() % 2);

			// select a random race
			// there are 9 legal races that the player can select from the start.
			if (enabledDLCPack1 && enabledDLCPack2) {
			    stats[index]->playerRace = rand() % NUMPLAYABLERACES;
			} else if (enabledDLCPack1) {
			    stats[index]->playerRace = rand() % 5;
			} else if (enabledDLCPack2) {
			    stats[index]->playerRace = rand() % 5;
			    if (stats[index]->playerRace > 0) {
			        stats[index]->playerRace += 4;
			    }
			} else {
			}
			auto race_button = card->findButton("race");
			if (race_button) {
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
			}

			// choose a random appearance
			if (stats[index]->playerRace == RACE_HUMAN) {
				stats[index]->appearance = rand() % NUMAPPEARANCES;
			} else {
				stats[index]->appearance = 0; rand();
			}

			// update sex buttons after race selection:
			// we might have chosen a succubus or incubus
			auto male_button = card->findButton("male");
			auto female_button = card->findButton("female");
			if (male_button && female_button) {
				if (stats[index]->sex == MALE) {
					male_button->setColor(makeColor(255, 255, 255, 255));
					male_button->setHighlightColor(makeColor(255, 255, 255, 255));
					female_button->setColor(makeColor(127, 127, 127, 255));
					female_button->setHighlightColor(makeColor(127, 127, 127, 255));
				} else {
					female_button->setColor(makeColor(255, 255, 255, 255));
					female_button->setHighlightColor(makeColor(255, 255, 255, 255));
					male_button->setColor(makeColor(127, 127, 127, 255));
					male_button->setHighlightColor(makeColor(127, 127, 127, 255));
				}
			}

			// select a random class
			auto reduced_class_list = reducedClassList(index);
			auto random_class = reduced_class_list[(rand() % (reduced_class_list.size() - 1)) + 1];
			for (int c = 1; c < num_classes; ++c) {
				if (strcmp(random_class, classes_in_order[c]) == 0) {
					client_classes[index] = c - 1;
					break;
				}
			}

			stats[index]->clearStats();
			initClass(index);
		};

		auto randomize_class = card->addButton("randomize_class");
		randomize_class->setColor(makeColor(255, 255, 255, 255));
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
			int i = std::min(std::max(0, client_classes[index] + 1), num_classes - 1);
			auto find = classes.find(classes_in_order[i]);
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
			int i = std::min(std::max(0, client_classes[index] + 1), num_classes - 1);
			auto find = classes.find(classes_in_order[i]);
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
		class_button->setColor(makeColor(255, 255, 255, 255));
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
		ready_button->setColor(makeColor(255, 255, 255, 255));
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
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			for (int c = 0; c < 4; ++c) {
				auto card = lobby->findFrame((std::string("card") + std::to_string(c)).c_str()); assert(card);
				auto backdrop = card->findImage("backdrop"); assert(backdrop);
				if (backdrop->path != "images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png") {
					playersInLobby[c] = true;
				} else {
					playersInLobby[c] = false;
				}
			}
			destroyMainMenu();
			createDummyMainMenu();
			beginFade(MainMenu::FadeDestination::GameStart);
			});
	}

	static void createStartButton(int index) {
		auto lobby = main_menu_frame->findFrame("lobby");
		assert(lobby);

		// release any controller assigned to this player
#ifndef NINTENDO
        if (inputs.hasController(index) && index != 0) {
            inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
            Input::inputs[index].refresh();
        }
#endif

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

		auto banner = card->addField("invite_banner", 64);
		banner->setText((std::string("PLAYER ") + std::to_string(index + 1)).c_str());
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);

		auto invite = card->addButton("invite_button");
		invite->setText("Press Start");
		invite->setHideSelectors(true);
		invite->setFont(smallfont_outline);
		invite->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		invite->setVJustify(Button::justify_t::TOP);
		invite->setHJustify(Button::justify_t::CENTER);
		invite->setBorder(0);
		invite->setColor(0);
		invite->setBorderColor(0);
		invite->setHighlightColor(0);
		invite->setWidgetBack("back_button");
		invite->addWidgetAction("MenuStart", "invite_button");
		switch (index) {
		case 0: invite->setCallback([](Button&){createControllerPrompt(0, true, [](){createCharacterCard(0);});}); break;
		case 1: invite->setCallback([](Button&){createControllerPrompt(1, true, [](){createCharacterCard(1);});}); break;
		case 2: invite->setCallback([](Button&){createControllerPrompt(2, true, [](){createCharacterCard(2);});}); break;
		case 3: invite->setCallback([](Button&){createControllerPrompt(3, true, [](){createCharacterCard(3);});}); break;
		}
		invite->select();
	}

	static void createInviteButton(int index) {
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
		invite->setVJustify(Button::justify_t::TOP);
		invite->setHJustify(Button::justify_t::CENTER);
		invite->setBorder(0);
		invite->setColor(0);
		invite->setBorderColor(0);
		invite->setHighlightColor(0);
		invite->setCallback([](Button&){buttonInviteFriends(NULL);});
		invite->select();
	}

	static void createWaitingStone(int index) {
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

		auto banner = card->addField("banner", 64);
		banner->setText("OPEN");
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);

		auto text = card->addField("text", 128);
		text->setText("Waiting for\nplayer to join");
		text->setFont(smallfont_outline);
		text->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		text->setVJustify(Field::justify_t::TOP);
		text->setHJustify(Field::justify_t::CENTER);
	}

	static void createLobby(LobbyType type) {
		destroyMainMenu();
		createDummyMainMenu();

		// reset ALL player stats
		for (int c = 0; c < 4; ++c) {
			stats[c]->playerRace = 0;
			stats[c]->sex = static_cast<sex_t>(rand() % 2);
			stats[c]->appearance = rand() % NUMAPPEARANCES;
			stats[c]->clearStats();
			client_classes[c] = 0;
			initClass(c);

			// random name
			auto& names = stats[c]->sex == sex_t::MALE ?
				randomPlayerNamesMale : randomPlayerNamesFemale;
			auto name = names[rand() % names.size()].c_str();
			size_t len = strlen(name);
			len = std::min(sizeof(Stat::name) - 1, len);
			memcpy(stats[c]->name, name, len);
			stats[c]->name[len] = '\0';
		}

		currentLobbyType = type;

		auto lobby = main_menu_frame->addFrame("lobby");
		lobby->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		lobby->setActualSize(SDL_Rect{0, 0, lobby->getSize().w, lobby->getSize().h});
		lobby->setHollow(true);
		lobby->setBorder(0);

		auto dimmer = lobby->addImage(
			lobby->getActualSize(),
			makeColor(0, 0, 0, 127),
			"images/system/white.png",
			"dimmer");

		for (int c = 0; c < 4; ++c) {
			auto name = std::string("paperdoll") + std::to_string(c);
			auto paperdoll = lobby->addFrame(name.c_str());
			paperdoll->setOwner(c);
			//paperdoll->setColor(makeColor(33, 26, 24, 255));
			//paperdoll->setBorderColor(makeColor(116, 55, 0, 255));
			//paperdoll->setBorder(2);
			paperdoll->setColor(0);
			paperdoll->setBorderColor(0);
			paperdoll->setBorder(0);
			paperdoll->setInvisible(true);
			paperdoll->setTickCallback([](Widget& widget){
				widget.setInvisible(true);
				int index = widget.getOwner();
				auto paperdoll = static_cast<Frame*>(&widget);
				auto lobby = static_cast<Frame*>(widget.getParent());
				auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
				if (card) {
					auto backdrop = card->findImage("backdrop");
					paperdoll->setSize(SDL_Rect{
						index * Frame::virtualScreenX / 4,
						0,
						Frame::virtualScreenX / 4,
						Frame::virtualScreenY - card->getSize().h / 2
						});
					if (backdrop && backdrop->path != "images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png") {
						widget.setInvisible(false);
					}
				}
				});
			paperdoll->setDrawCallback([](const Widget& widget, SDL_Rect pos){
				auto angle = (330.0 + 20.0 * widget.getOwner()) * PI / 180.0;
				drawCharacterPreview(widget.getOwner(), pos, 80, angle);
				});
		}

		auto back_button = createBackWidget(lobby, [](Button&){
			soundCancel();
			destroyMainMenu();
			currentLobbyType = LobbyType::None;
			createMainMenu(false);
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

		if (type == LobbyType::LobbyLocal) {
			createStartButton(0);
			createStartButton(1);
			createStartButton(2);
			createStartButton(3);
		} else if (type == LobbyType::LobbyLAN) {
			createWaitingStone(0);
			createWaitingStone(1);
			createWaitingStone(2);
			createWaitingStone(3);
		} else if (type == LobbyType::LobbyOnline) {
			createInviteButton(0);
			createInviteButton(1);
			createInviteButton(2);
			createInviteButton(3);
		}
	}

	static void createControllerPrompt(int index, bool show_player_text, void (*after_func)()) {
		auto dimmer = main_menu_frame->addFrame("controller_dimmer");
		dimmer->setOwner(index);
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(0);
		dimmer->setBorder(0);

        static void (*end_func)();
		static bool clicked;
		clicked = false;
		end_func = after_func;

		static auto button_func = [](Button& button) {
		    int index = button.getOwner();
	        if (Input::waitingToBindControllerForPlayer >= 0) {
	            // this only happens if the mouse was used to click this button
	            Input::waitingToBindControllerForPlayer = -1;
	            inputs.setPlayerIDAllowedKeyboard(index);
	            inputs.getVirtualMouse(index)->draw_cursor = true;
	            inputs.getVirtualMouse(index)->lastMovementFromController = false;
	            if (inputs.hasController(index)) {
	                inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
	            }
	        } else {
	            // this happens if a controller was bound to the player
				inputs.getVirtualMouse(index)->draw_cursor = false;
				inputs.getVirtualMouse(index)->lastMovementFromController = true;
	            if (inputs.bPlayerUsingKeyboardControl(index)) {
	                inputs.setPlayerIDAllowedKeyboard(0);
	            }
	        }
		    button.deselect();
		    clicked = true;
		    };

		static auto button_tick_func = [](Widget& widget) {
		    auto button = static_cast<Button*>(&widget);
		    int index = button->getOwner();
		    auto& input = Input::inputs[index];
		    if (clicked && !input.binary("MenuConfirm")) {
                input.refresh(); // this has to be deferred because it knocks out consumed statuses.
		        auto parent = static_cast<Frame*>(button->getParent());
	            parent->removeSelf();
	            parent->setDisabled(true);
	            soundActivate();
	            end_func();
		    }
		    };

		char text[1024];
        if (show_player_text) {
		    snprintf(text, sizeof(text), "Press A on a controller to assign it to Player %d,\n"
		        "or click here to assign only the mouse and keyboard", index + 1);
        } else {
		    snprintf(text, sizeof(text), "Press A on a controller to activate it now,\n"
		        "or click here to use only the mouse and keyboard");
        }

		auto button = dimmer->addButton("button");
		button->setHideSelectors(true);
		button->setBorder(0);
		button->setColor(makeColor(0, 0, 0, 127));
		button->setHighlightColor(makeColor(0, 0, 0, 127));
		button->setTextColor(makeColor(255, 255, 255, 255));
		button->setTextHighlightColor(makeColor(255, 255, 255, 255));
		button->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		button->setJustify(Field::justify_t::CENTER);
		button->setFont(bigfont_outline);
		button->setText(text);
		button->setCallback(button_func);
		button->setTickCallback(button_tick_func);
		button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		button->setButtonsOffset(SDL_Rect{0, 48, 0, 0,});
		button->select();

		Input::waitingToBindControllerForPlayer = index;
	}

/******************************************************************************/

	static void createPlayWindow() {
		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto window = dimmer->addFrame("play_game_window");
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

		bool continueAvailable = anySaveFileExists();

		auto hall_of_trials_button = window->addButton("hall_of_trials");
		hall_of_trials_button->setSize(SDL_Rect{134, 176, 168, 52});
		hall_of_trials_button->setBackground("images/ui/Main Menus/Play/UI_PlayMenu_Button_HallofTrials00.png");
		hall_of_trials_button->setHighlightColor(makeColor(255, 255, 255, 255));
		hall_of_trials_button->setColor(makeColor(255, 255, 255, 255));
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
			beginFade(MainMenu::FadeDestination::HallOfTrials);
			});

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
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
			continue_button->setCallback([](Button& button){soundActivate(); playContinue(button);});
		} else {
			continue_button->setCallback([](Button&){soundError();});
		}
		continue_button->setWidgetSearchParent(window->getName());
		continue_button->setWidgetRight("new");
		continue_button->setWidgetDown("hall_of_trials");
		continue_button->setWidgetBack("back_button");
		continue_button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		continue_button->setButtonsOffset(SDL_Rect{0, 29, 0, 0,});

		auto new_button = window->addButton("new");
		new_button->setSize(SDL_Rect{114 * 2, 36 * 2, 68 * 2, 56 * 2});
		new_button->setBackground("images/ui/Main Menus/Play/UI_PlayMenu_NewB00.png");
		new_button->setBackgroundHighlighted("images/ui/Main Menus/Play/UI_PlayMenu_NewA00.png");
		new_button->setTextColor(makeColor(180, 180, 180, 255));
		new_button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		new_button->setText(" \nNEW");
		new_button->setFont(smallfont_outline);
		new_button->setCallback([](Button& button){soundActivate(); playNew(button);});
		new_button->setWidgetSearchParent(window->getName());
		new_button->setWidgetLeft("continue");
		new_button->setWidgetDown("hall_of_trials");
		new_button->setWidgetBack("back_button");
		new_button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		new_button->setButtonsOffset(SDL_Rect{0, 29, 0, 0,});

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

	static void createLobbyBrowser(Button& button) {
		soundActivate();

		enum class BrowserMode {
			Online,
			Wireless,
		};
		static BrowserMode mode;
		mode = BrowserMode::Online;

		// remove "Local or Network" window
		auto frame = static_cast<Frame*>(button.getParent());
		frame = static_cast<Frame*>(frame->getParent());
		frame->removeSelf();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create lobby browser window
		auto window = dimmer->addFrame("lobby_browser_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 524) / 2,
			(Frame::virtualScreenY - 542) / 2,
			524,
			542});
		window->setActualSize(SDL_Rect{0, 0, 524, 542});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/LobbyBrowser/Lobby_Window00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 64);
		banner_title->setSize(SDL_Rect{160, 24, 204, 18});
		banner_title->setText("ONLINE LOBBY BROWSER");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		auto interior = window->addImage(
			SDL_Rect{82, 70, 358, 364},
			0xffffffff,
			"images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online00.png",
			"interior"
		);

		auto online_tab = window->addButton("online_tab");
		online_tab->setSize(SDL_Rect{144, 70, 106, 36});
		online_tab->setHighlightColor(0);
		online_tab->setBorder(0);
		online_tab->setColor(0);
		online_tab->setText("ONLINE");
		online_tab->setFont(smallfont_outline);
		online_tab->setWidgetSearchParent(window->getName());
		online_tab->addWidgetAction("MenuPageLeft", "online_tab");
		online_tab->addWidgetAction("MenuPageRight", "wireless_tab");
		online_tab->addWidgetAction("MenuStart", "enter_code");
		online_tab->setWidgetBack("cancel");
		online_tab->setCallback([](Button& button){
			auto frame = static_cast<Frame*>(button.getParent());
			auto interior = frame->findImage("interior");
			interior->path = "images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online00.png";
			});

		auto wireless_tab = window->addButton("wireless_tab");
		wireless_tab->setSize(SDL_Rect{254, 70, 128, 36});
		wireless_tab->setHighlightColor(0);
		wireless_tab->setBorder(0);
		wireless_tab->setColor(0);
		wireless_tab->setText("WIRELESS");
		wireless_tab->setFont(smallfont_outline);
		wireless_tab->setWidgetSearchParent(window->getName());
		wireless_tab->addWidgetAction("MenuPageLeft", "online_tab");
		wireless_tab->addWidgetAction("MenuPageRight", "wireless_tab");
		wireless_tab->addWidgetAction("MenuStart", "enter_code");
		wireless_tab->setWidgetBack("cancel");
		wireless_tab->setCallback([](Button& button){
			auto frame = static_cast<Frame*>(button.getParent());
			auto interior = frame->findImage("interior");
			interior->path = "images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless00.png";
			});

		auto friends_only = window->addButton("friends_only");
		friends_only->setSize(SDL_Rect{132, 372, 44, 44});
		friends_only->setIcon("images/ui/Main Menus/Play/LobbyBrowser/Fill_Checked_00.png");
		friends_only->setStyle(Button::style_t::STYLE_CHECKBOX);
		friends_only->setHighlightColor(0);
		friends_only->setBorderColor(0);
		friends_only->setBorder(0);
		friends_only->setColor(0);

		auto friends_only_label = window->addField("friends_only_label", 64);
		friends_only_label->setSize(SDL_Rect{166, 372, 98, 48});
		friends_only_label->setText("Friends\nOnly");
		friends_only_label->setFont(smallfont_no_outline);
		friends_only_label->setJustify(Field::justify_t::CENTER);
		friends_only_label->setColor(makeColor(166, 123, 81, 255));

		auto show_full = window->addButton("show_full");
		show_full->setSize(SDL_Rect{258, 372, 44, 44});
		show_full->setIcon("images/ui/Main Menus/Play/LobbyBrowser/Fill_Checked_00.png");
		show_full->setStyle(Button::style_t::STYLE_CHECKBOX);
		show_full->setHighlightColor(0);
		show_full->setBorderColor(0);
		show_full->setBorder(0);
		show_full->setColor(0);

		auto show_full_label = window->addField("show_full_label", 64);
		show_full_label->setSize(SDL_Rect{294, 372, 98, 48});
		show_full_label->setText("Show\nFull");
		show_full_label->setFont(smallfont_no_outline);
		show_full_label->setJustify(Field::justify_t::CENTER);
		show_full_label->setColor(makeColor(166, 123, 81, 255));

		auto cancel = window->addButton("cancel");
		cancel->setSize(SDL_Rect{94, 440, 164, 62});
		cancel->setBackground("images/ui/Main Menus/Play/LobbyBrowser/UI_Button_Basic00.png");
		cancel->setHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setColor(makeColor(255, 255, 255, 255));
		cancel->setText("Cancel");
		cancel->setFont(smallfont_outline);
		cancel->setWidgetSearchParent(window->getName());
		cancel->addWidgetAction("MenuPageLeft", "online_tab");
		cancel->addWidgetAction("MenuPageRight", "wireless_tab");
		cancel->addWidgetAction("MenuStart", "enter_code");
		cancel->setWidgetBack("cancel");
		cancel->setWidgetRight("enter_code");
		cancel->setCallback([](Button& button){soundCancel(); playNew(button);});

		auto enter_code = window->addButton("enter_code");
		enter_code->setSize(SDL_Rect{266, 440, 164, 62});
		enter_code->setBackground("images/ui/Main Menus/Play/LobbyBrowser/UI_Button_Basic00.png");
		enter_code->setHighlightColor(makeColor(255, 255, 255, 255));
		enter_code->setColor(makeColor(255, 255, 255, 255));
		enter_code->setText("Enter Lobby\nCode");
		enter_code->setFont(smallfont_outline);
		enter_code->setWidgetSearchParent(window->getName());
		enter_code->addWidgetAction("MenuPageLeft", "online_tab");
		enter_code->addWidgetAction("MenuPageRight", "wireless_tab");
		enter_code->addWidgetAction("MenuStart", "enter_code");
		enter_code->setWidgetBack("cancel");
		enter_code->setWidgetLeft("cancel");
	}

	static void playNew(Button& button) {
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
		frame = static_cast<Frame*>(frame->getParent());
		frame->removeSelf();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create "Local or Network" window
		auto window = dimmer->addFrame("local_or_network_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 436) / 2 - 38,
			(Frame::virtualScreenY - 494) / 2,
			496,
			494});
		window->setActualSize(SDL_Rect{0, 0, 496, 494});
		window->setColor(0);
		window->setBorder(0);
		window->setTickCallback([](Widget& widget){
		    auto window = static_cast<Frame*>(&widget); assert(window);
		    auto tooltip = window->findField("tooltip"); assert(tooltip);
		    auto local_button = window->findButton("local"); assert(local_button);
		    auto local_image = window->findImage("local_image"); assert(local_image);
		    if (local_button->isSelected()) {
		        tooltip->setText("Play singleplayer or with 2-4\nplayers in splitscreen multiplayer");
                local_image->path =
#ifdef NINTENDO
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00.png";
#else
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00_NoNX.png";
#endif
		    } else {
                local_image->path =
#ifdef NINTENDO
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected.png";
#else
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected_NoNX.png";
#endif
		    }
		    auto host_lan_button = window->findButton("host_lan"); assert(host_lan_button);
		    auto host_lan_image = window->findImage("host_lan_image"); assert(host_lan_image);
		    if (host_lan_button->isSelected()) {
		        tooltip->setText("Host a game with 2-4 players\nover a local area network (LAN)");
                host_lan_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00.png";
		    } else {
                host_lan_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00B_Unselected.png";
		    }
		    auto host_online_button = window->findButton("host_online"); assert(host_online_button);
		    auto host_online_image = window->findImage("host_online_image"); assert(host_online_image);
		    if (host_online_button->isSelected()) {
		        tooltip->setText("Host a game with 2-4 players\nover the internet");
                host_online_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00.png";
		    } else {
                host_online_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00B_Unselected.png";
		    }
		    auto join_button = window->findButton("join"); assert(join_button);
		    auto join_image = window->findImage("join_image"); assert(join_image);
		    if (join_button->isSelected()) {
		        tooltip->setText("Join a multiplayer game");
                join_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00.png";
		    } else {
                join_image->path =
		            "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00B_Unselected.png";
		    }
		    });

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Window_00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{180, 24, 152, 18});
		banner_title->setText("NEW ADVENTURER");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			createPlayWindow();
			}, SDL_Rect{42, 4, 0, 0});

		auto local_button = window->addButton("local");
		local_button->setSize(SDL_Rect{96, 72, 164, 62});
		local_button->setBackground("images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		local_button->setHighlightColor(makeColor(255, 255, 255, 255));
		local_button->setColor(makeColor(255, 255, 255, 255));
		local_button->setText("Local Adventure");
		local_button->setFont(smallfont_outline);
		local_button->setWidgetSearchParent(window->getName());
		local_button->setWidgetBack("back_button");
		local_button->setWidgetDown("host_lan");
		local_button->setCallback([](Button&){soundActivate(); createLobby(LobbyType::LobbyLocal);});

		local_button->select();

		(void)window->addImage(
		    SDL_Rect{278, 76, 104, 52},
		    0xffffffff,
#ifdef NINTENDO
		    "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected.png",
#else
		    "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected_NoNX.png",
#endif
		    "local_image"
		);

		auto host_lan_button = window->addButton("host_lan");
		host_lan_button->setSize(SDL_Rect{96, 166, 164, 62});
		host_lan_button->setBackground("images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		host_lan_button->setHighlightColor(makeColor(255, 255, 255, 255));
		host_lan_button->setColor(makeColor(255, 255, 255, 255));
		host_lan_button->setText("Host LAN Party");
		host_lan_button->setFont(smallfont_outline);
		host_lan_button->setWidgetSearchParent(window->getName());
		host_lan_button->setWidgetBack("back_button");
		host_lan_button->setWidgetUp("local");
		host_lan_button->setWidgetDown("host_online");
		host_lan_button->setCallback([](Button&){soundActivate(); createLobby(LobbyType::LobbyLAN);});

		(void)window->addImage(
		    SDL_Rect{270, 170, 126, 50},
		    0xffffffff,
		    "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00B_Unselected.png",
		    "host_lan_image"
		);

		auto host_online_button = window->addButton("host_online");
		host_online_button->setSize(SDL_Rect{96, 232, 164, 62});
		host_online_button->setBackground("images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		host_online_button->setHighlightColor(makeColor(255, 255, 255, 255));
		host_online_button->setColor(makeColor(255, 255, 255, 255));
		host_online_button->setText("Host Online Party");
		host_online_button->setFont(smallfont_outline);
		host_online_button->setWidgetSearchParent(window->getName());
		host_online_button->setWidgetBack("back_button");
		host_online_button->setWidgetUp("host_lan");
		host_online_button->setWidgetDown("join");
		host_online_button->setCallback([](Button&){soundActivate(); createLobby(LobbyType::LobbyOnline);});

		(void)window->addImage(
		    SDL_Rect{270, 234, 126, 50},
		    0xffffffff,
		    "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00B_Unselected.png",
		    "host_online_image"
		);

		auto join_button = window->addButton("join");
		join_button->setSize(SDL_Rect{96, 326, 164, 62});
		join_button->setBackground("images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		join_button->setHighlightColor(makeColor(255, 255, 255, 255));
		join_button->setColor(makeColor(255, 255, 255, 255));
		join_button->setText("Lobby Browser");
		join_button->setFont(smallfont_outline);
		join_button->setWidgetSearchParent(window->getName());
		join_button->setWidgetBack("back_button");
		join_button->setWidgetUp("host_online");
		join_button->setCallback(createLobbyBrowser);

		(void)window->addImage(
		    SDL_Rect{270, 324, 120, 68},
		    0xffffffff,
		    "images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00B_Unselected.png",
		    "join_image"
		);

		auto tooltip = window->addField("tooltip", 1024);
		tooltip->setSize(SDL_Rect{106, 398, 300, 48});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setColor(makeColor(91, 76, 50, 255));
		tooltip->setJustify(Field::justify_t::CENTER);
		tooltip->setText("Help text goes here.");
	}

	static Button* savegame_selected = nullptr;
	static bool continueSingleplayer = false;
	static Button* populateContinueSubwindow(Frame& subwindow, bool singleplayer);

	static void deleteSavePrompt(bool singleplayer, int save_index) {
	    static bool delete_singleplayer;
	    static int delete_save_index;
	    delete_singleplayer = singleplayer;
	    delete_save_index = save_index;

        // extract savegame info
        auto saveGameInfo = getSaveGameInfo(singleplayer, save_index);
        const std::string& player_name = saveGameInfo.player_name;

        // create shortened player name
        char shortened_name[20] = { '\0' };
        int len = (int)player_name.size();
        strncpy(shortened_name, player_name.c_str(), std::min(len, 16));
        if (len > 16) {
            strcat(shortened_name, "...");
        }

	    // window text
	    char window_text[1024];
	    snprintf(window_text, sizeof(window_text),
	        "Are you sure you want to delete\nthe save game \"%s\"?", shortened_name);

	    binaryPrompt(
	        window_text, "Yes", "No",
	        [](Button& button) { // Yes button
			    soundActivate();
			    soundDeleteSave();

                // delete save game
                (void)deleteSaveGame(delete_singleplayer ? SINGLE : SERVER, delete_save_index);

                // find frame elements
			    assert(main_menu_frame);
		        auto window = main_menu_frame->findFrame("continue_window"); assert(window);
		        auto subwindow = window->findFrame("subwindow"); assert(subwindow);

                // repopulate save game window & select a new button
			    savegame_selected = nullptr;
                subwindow->removeSelf();
                subwindow = window->addFrame("subwindow");
	            Button* first_savegame = populateContinueSubwindow(*subwindow, delete_singleplayer);
	            if (first_savegame) {
	                first_savegame->select();
		            savegame_selected = first_savegame;
	            } else {
	                if (delete_singleplayer) {
                        auto singleplayer = window->findButton("singleplayer");
                        singleplayer->select();
	                } else {
                        auto multiplayer = window->findButton("multiplayer");
                        multiplayer->select();
	                }
	            }

                // remove prompt
			    auto prompt = main_menu_frame->findFrame("binary_prompt");
			    if (prompt) {
				    auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
				    dimmer->removeSelf();
			    }
	        },
	        [](Button& button) { // No button
			    soundCancel();
			    if (savegame_selected) {
			        savegame_selected->select();
			    } else {
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                if (delete_singleplayer) {
                        auto singleplayer = window->findButton("singleplayer");
                        singleplayer->select();
	                } else {
                        auto multiplayer = window->findButton("multiplayer");
                        multiplayer->select();
	                }
			    }
			    assert(main_menu_frame);
			    auto prompt = main_menu_frame->findFrame("binary_prompt");
			    if (prompt) {
				    auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
				    dimmer->removeSelf();
			    }
	        }
	    );
	}

	static void loadSavePrompt(bool singleplayer, int save_index) {
	    static bool load_singleplayer;
	    static int load_save_index;
	    load_singleplayer = singleplayer;
	    load_save_index = save_index;

        // extract savegame info
        auto saveGameInfo = getSaveGameInfo(singleplayer, save_index);
        const std::string& player_name = saveGameInfo.player_name;

        // create shortened player name
        char shortened_name[20] = { '\0' };
        int len = (int)player_name.size();
        strncpy(shortened_name, player_name.c_str(), std::min(len, 16));
        if (len > 16) {
            strcat(shortened_name, "...");
        }

	    // window text
	    char window_text[1024];
	    snprintf(window_text, sizeof(window_text),
	        "Are you sure you want to load\nthe save game \"%s\"?", shortened_name);

	    binaryPrompt(
	        window_text, "Yes", "No",
	        [](Button& button) { // Yes button
                soundActivate();
                destroyMainMenu();
                savegameCurrentFileIndex = load_save_index;
                loadingsavegame = getSaveGameUniqueGameKey(load_singleplayer);
                beginFade(MainMenu::FadeDestination::GameStart);
	        },
	        [](Button& button) { // No button
			    soundCancel();
			    if (savegame_selected) {
			        savegame_selected->select();
			    } else {
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                if (load_singleplayer) {
                        auto singleplayer = window->findButton("singleplayer");
                        singleplayer->select();
	                } else {
                        auto multiplayer = window->findButton("multiplayer");
                        multiplayer->select();
	                }
			    }
			    assert(main_menu_frame);
			    auto prompt = main_menu_frame->findFrame("binary_prompt");
			    if (prompt) {
				    auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
				    dimmer->removeSelf();
			    }
	        }
	    );
	}

	static Button* populateContinueSubwindow(Frame& subwindow, bool singleplayer) {
		subwindow.setActualSize(SDL_Rect{0, 0, 898, 294});
		subwindow.setSize(SDL_Rect{90, 82, 898, 294});
		subwindow.setColor(0);
		subwindow.setBorder(0);
	    Button* first_savegame = nullptr;
        if (!anySaveFileExists(singleplayer)) {
            auto none_exists = subwindow.addField("none_exists", 256);
            none_exists->setSize(subwindow.getActualSize());
            none_exists->setFont(bigfont_outline);
            none_exists->setText("No compatible save files found.");
            none_exists->setJustify(Field::justify_t::CENTER);
        } else {
            int saveGameCount = 0;
		    for (int i = 0; i < SAVE_GAMES_MAX; ++i) {
                if (saveGameExists(singleplayer, i)) {
                    auto str = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(i);
                    auto savegame_book = subwindow.addButton(str.c_str());
                    savegame_book->setSize(SDL_Rect{saveGameCount * 256 + (898 - 220) / 2, 0, 220, 280});
                    savegame_book->setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Book_00.png");
		            savegame_book->setColor(makeColor(255, 255, 255, 255));
		            savegame_book->setHighlightColor(makeColor(255, 255, 255, 255));
		            savegame_book->setFont(smallfont_outline);
		            savegame_book->setTextColor(makeColor(255, 182, 73, 255));
		            savegame_book->setTextHighlightColor(makeColor(255, 182, 73, 255));
		            savegame_book->setTickCallback([](Widget& widget){
	                    auto button = static_cast<Button*>(&widget);
	                    auto frame = static_cast<Frame*>(widget.getParent());

		                Input& input = Input::inputs[widget.getOwner()];
		                bool scrolled = false;
		                scrolled |= input.binary("MenuScrollDown");
		                scrolled |= input.binary("MenuScrollUp");
		                scrolled |= input.binary("MenuScrollLeft");
		                scrolled |= input.binary("MenuScrollRight");
		                scrolled |= input.binary("MenuMouseWheelUp");
		                scrolled |= input.binary("MenuMouseWheelDown");
		                if (scrolled) {
		                    savegame_selected = nullptr;
		                } else if (widget.isSelected() && !inputs.getVirtualMouse(clientnum)->draw_cursor) {
		                    savegame_selected = button;
		                }

		                if (savegame_selected == &widget) {
		                    auto frame_pos = frame->getActualSize();
		                    auto button_size = button->getSize();
		                    int diff = ((button_size.x - (898 - 220) / 2) - frame_pos.x);
		                    if (diff > 0) {
		                        frame_pos.x += std::max(1, diff / 8);
		                    } else if (diff < 0) {
		                        frame_pos.x += std::min(-1, diff / 8);
		                    }
		                    frame->setActualSize(frame_pos);
		                }
		                });
		            savegame_book->setCallback([](Button& button){
		                if (savegame_selected != &button) {
		                    soundCheckmark();
		                    savegame_selected = &button;
		                    return;
		                } else {
		                    soundActivate();
		                    int save_index = -1;
	                        const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	                        size_t name_len = strlen(name);
                            save_index = (int)strtol(button.getName() + name_len, nullptr, 10);
                            if (cursor_delete_mode) {
                                deleteSavePrompt(continueSingleplayer, save_index);
                            } else {
                                loadSavePrompt(continueSingleplayer, save_index);
                            }
		                }
		                });
		            if (i > 0) {
		                int prev;
		                for (prev = i - 1; prev >= 0 && !saveGameExists(singleplayer, prev); --prev);
                        auto str = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(prev);
                        savegame_book->setWidgetLeft(str.c_str());
		            }
		            if (i < SAVE_GAMES_MAX - 1) {
		                int next;
		                for (next = i + 1; next < SAVE_GAMES_MAX && !saveGameExists(singleplayer, next); ++next);
                        auto str = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(next);
                        savegame_book->setWidgetRight(str.c_str());
		            }
		            savegame_book->setWidgetBack("back");

		            first_savegame = first_savegame ? first_savegame : savegame_book;

		            auto saveGameInfo = getSaveGameInfo(singleplayer, i);

                    // extract savegame info
                    const std::string& player_name = saveGameInfo.player_name;
                    const std::string& class_name = saveGameInfo.player_class;
                    char* class_name_c = const_cast<char*>(class_name.c_str());
                    class_name_c[0] = (char)toupper((int)class_name[0]);
                    int dungeon_lvl = saveGameInfo.dungeon_lvl;
                    int player_lvl = saveGameInfo.player_lvl;

                    // create shortened player name
                    char shortened_name[20] = { '\0' };
                    int len = (int)player_name.size();
                    strncpy(shortened_name, player_name.c_str(), std::min(len, 16));
                    if (len > 16) {
                        strcat(shortened_name, "...");
                    }

                    // format book label string
		            char text[1024];
		            snprintf(text, sizeof(text), "%s\n%s LVL %d\nDungeon LVL %d",
		                shortened_name, class_name.c_str(), player_lvl, dungeon_lvl);
		            savegame_book->setText(text);

                    // offset text
                    SDL_Rect offset;
                    offset.x = 34;
                    offset.y = 184;
		            savegame_book->setTextOffset(offset);
		            savegame_book->setJustify(Button::justify_t::LEFT);

		            // add savegame screenshot
		            auto screenshot_path = setSaveGameFileName(singleplayer, SaveFileType::SCREENSHOT, i);
                    if (dataPathExists(screenshot_path.c_str(), false)) {
		                auto screenshot = subwindow.addImage(
		                    SDL_Rect{saveGameCount * 256 + (898 - 220) / 2 + 32, 16, 160, 162},
		                    0xffffffff,
		                    screenshot_path.c_str(),
		                    (str + "_screenshot").c_str()
		                );
		                screenshot->ontop = true;
		                Image* image = Image::get(screenshot_path.c_str()); assert(image);
		                screenshot->section.x = (image->getWidth() - image->getHeight()) / 2;
		                screenshot->section.w = image->getHeight();
		            }

		            // add book overlay
		            auto overlay = subwindow.addImage(
		                SDL_Rect{saveGameCount * 256 + (898 - 220) / 2 + 32, 16, 160, 162},
		                0xffffffff,
		                "images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Book_Corners_00.png",
		                (str + "_overlay").c_str()
		            );
		            overlay->ontop = true;

		            ++saveGameCount;
                }
		    }
		    subwindow.setActualSize(SDL_Rect{0, 0, 898 + 256 * (saveGameCount - 1), 294});
        }
        return first_savegame;
	}

	static void playContinue(Button& button) {
        continueSingleplayer = ~(!anySaveFileExists(true) && anySaveFileExists(false));

        savegame_selected = nullptr;

		// remove "Play Game" window
		auto frame = static_cast<Frame*>(button.getParent());
		frame = static_cast<Frame*>(frame->getParent());
		frame->removeSelf();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create Continue window
		auto window = dimmer->addFrame("continue_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 1080) / 2,
			(Frame::virtualScreenY - 474) / 2,
			1080,
			474});
		window->setActualSize(SDL_Rect{0, 0, 1080, 474});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"images/ui/Main Menus/ContinueGame/UI_Cont_Window_00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{
		    (window->getActualSize().w - 256) / 2,
		    8,
		    256,
		    48});
		banner_title->setText("CONTINUE ADVENTURE");
		banner_title->setFont(bigfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		auto subwindow = window->addFrame("subwindow");
		auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		if (first_savegame) {
		    first_savegame->select();
		    savegame_selected = first_savegame;
		}

		auto gradient = window->addImage(
		    subwindow->getSize(),
		    0xffffffff,
		    "images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Grad_01.png",
		    "gradient"
		);
		gradient->ontop = true;

		auto singleplayer = window->addButton("singleplayer");
		singleplayer->setText("Local Games");
		singleplayer->setFont(smallfont_outline);
		singleplayer->setSize(SDL_Rect{226, 38, 156, 36});
		singleplayer->setColor(makeColor(255, 255, 255, 255));
		singleplayer->setHighlightColor(makeColor(255, 255, 255, 255));
		singleplayer->setTextColor(
		    continueSingleplayer ?
		    makeColor(255, 255, 255, 255) :
		    makeColor(127, 127, 127, 255));
		singleplayer->setBackground(
		    continueSingleplayer ?
		    "images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_ON_00.png" :
		    "images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_OFF_00.png");
		singleplayer->setGlyphPosition(Button::glyph_position_t::CENTERED_LEFT);
		singleplayer->setWidgetRight("multiplayer");
		singleplayer->setWidgetBack("back");
		singleplayer->addWidgetAction("MenuAlt2", "delete");
		singleplayer->addWidgetAction("MenuConfirm", "enter");
		singleplayer->addWidgetAction("MenuPageLeft", "singleplayer");
		singleplayer->addWidgetAction("MenuPageRight", "multiplayer");
		singleplayer->setCallback([](Button& button){
		    continueSingleplayer = true;
            Frame* window = static_cast<Frame*>(button.getParent());
            button.setTextColor(makeColor(255, 255, 255, 255));
            button.setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_ON_00.png");
            auto multiplayer = window->findButton("multiplayer");
		    multiplayer->setTextColor(makeColor(127, 127, 127, 255));
		    multiplayer->setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_OFF_00.png");
            Frame* subwindow = window->findFrame("subwindow");
            subwindow->removeSelf();
            subwindow = window->addFrame("subwindow");
		    auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		    if (first_savegame) {
		        first_savegame->select();
		        savegame_selected = first_savegame;
		    }
		    auto slider = window->findSlider("slider");
		    if (slider) {
		        const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;
		        slider->setMinValue(0.f);
		        slider->setValue(0.f);
		        slider->setMaxValue(sliderMaxSize);
	        }
		    cursor_delete_mode = false;
			savegame_selected = nullptr;
		    });

		auto multiplayer = window->addButton("multiplayer");
		multiplayer->setText("Online + LAN");
		multiplayer->setFont(smallfont_outline);
		multiplayer->setSize(SDL_Rect{702, 38, 144, 36});
		multiplayer->setColor(makeColor(255, 255, 255, 255));
		multiplayer->setHighlightColor(makeColor(255, 255, 255, 255));
		multiplayer->setTextColor(
		    continueSingleplayer ?
		    makeColor(127, 127, 127, 255) :
		    makeColor(255, 255, 255, 255));
		multiplayer->setBackground(
		    continueSingleplayer ?
		    "images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_OFF_00.png" :
		    "images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_ON_00.png");
		multiplayer->setGlyphPosition(Button::glyph_position_t::CENTERED_RIGHT);
		multiplayer->setWidgetLeft("singleplayer");
		multiplayer->setWidgetBack("back");
		multiplayer->addWidgetAction("MenuAlt2", "delete");
		multiplayer->addWidgetAction("MenuConfirm", "enter");
		multiplayer->addWidgetAction("MenuPageLeft", "singleplayer");
		multiplayer->addWidgetAction("MenuPageRight", "multiplayer");
		multiplayer->setCallback([](Button& button){
		    continueSingleplayer = false;
            Frame* window = static_cast<Frame*>(button.getParent());
            button.setTextColor(makeColor(255, 255, 255, 255));
            button.setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_ON_00.png");
            auto singleplayer = window->findButton("singleplayer");
		    singleplayer->setTextColor(makeColor(127, 127, 127, 255));
		    singleplayer->setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_OFF_00.png");
            Frame* subwindow = window->findFrame("subwindow");
            subwindow->removeSelf();
            subwindow = window->addFrame("subwindow");
		    auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		    if (first_savegame) {
		        first_savegame->select();
		        savegame_selected = first_savegame;
		    }
		    auto slider = window->findSlider("slider");
		    if (slider) {
		        const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;
		        slider->setMinValue(0.f);
		        slider->setValue(0.f);
		        slider->setMaxValue(sliderMaxSize);
		    }
		    cursor_delete_mode = false;
			savegame_selected = nullptr;
		    });

		auto delete_button = window->addButton("delete");
		delete_button->setText("Delete Save");
		delete_button->setFont(smallfont_outline);
		delete_button->setSize(SDL_Rect{278, 390, 164, 62});
		delete_button->setColor(makeColor(255, 255, 255, 255));
		delete_button->setHighlightColor(makeColor(255, 255, 255, 255));
		delete_button->setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Button_Delete_00.png");
		delete_button->setWidgetBack("back");
		delete_button->addWidgetAction("MenuAlt2", "delete");
		delete_button->addWidgetAction("MenuConfirm", "enter");
		delete_button->addWidgetAction("MenuPageLeft", "singleplayer");
		delete_button->addWidgetAction("MenuPageRight", "multiplayer");
		delete_button->setCallback([](Button& button){
	        int save_index = -1;
	        if (savegame_selected) {
	            const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	            size_t name_len = strlen(name);
	            if (strncmp(savegame_selected->getName(), name, name_len) == 0) {
                    save_index = (int)strtol(savegame_selected->getName() + name_len, nullptr, 10);
	            }
	        }
	        if (save_index >= 0) {
	            deleteSavePrompt(continueSingleplayer, save_index);
	        } else {
	            monoPrompt(
	                "Select a savegame to delete first.",
	                "Okay",
	                [](Button& button){
			            soundCancel();
			            if (savegame_selected) {
			                savegame_selected->select();
			            } else {
			                assert(main_menu_frame);
		                    auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                        if (continueSingleplayer) {
                                auto singleplayer = window->findButton("singleplayer");
                                singleplayer->select();
	                        } else {
                                auto multiplayer = window->findButton("multiplayer");
                                multiplayer->select();
	                        }
			            }
			            assert(main_menu_frame);
			            auto prompt = main_menu_frame->findFrame("mono_prompt");
			            if (prompt) {
				            auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
				            dimmer->removeSelf();
			            }
	                }
	            );
	        }
		    });

		auto enter_button = window->addButton("enter");
		enter_button->setText("Enter Dungeon");
		enter_button->setFont(smallfont_outline);
		enter_button->setSize(SDL_Rect{642, 390, 164, 62});
		enter_button->setColor(makeColor(255, 255, 255, 255));
		enter_button->setHighlightColor(makeColor(255, 255, 255, 255));
		enter_button->setBackground("images/ui/Main Menus/ContinueGame/UI_Cont_Button_00.png");
		enter_button->setWidgetBack("back");
		enter_button->addWidgetAction("MenuAlt2", "delete");
		enter_button->addWidgetAction("MenuConfirm", "enter");
		enter_button->addWidgetAction("MenuPageLeft", "singleplayer");
		enter_button->addWidgetAction("MenuPageRight", "multiplayer");
		enter_button->setCallback([](Button& button){
	        int save_index = -1;
	        if (savegame_selected) {
	            const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	            size_t name_len = strlen(name);
	            if (strncmp(savegame_selected->getName(), name, name_len) == 0) {
                    save_index = (int)strtol(savegame_selected->getName() + name_len, nullptr, 10);
	            }
	        }
	        if (save_index >= 0) {
	            loadSavePrompt(continueSingleplayer, save_index);
	        } else {
                monoPrompt(
                    "Select a savegame to load first.",
                    "Okay",
                    [](Button& button){
		                soundCancel();
		                if (savegame_selected) {
		                    savegame_selected->select();
		                } else {
			                assert(main_menu_frame);
		                    auto window = main_menu_frame->findFrame("continue_window"); assert(window);
                            if (continueSingleplayer) {
                                auto singleplayer = window->findButton("singleplayer");
                                singleplayer->select();
                            } else {
                                auto multiplayer = window->findButton("multiplayer");
                                multiplayer->select();
                            }
		                }
		                assert(main_menu_frame);
		                auto prompt = main_menu_frame->findFrame("mono_prompt");
		                if (prompt) {
			                auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
			                dimmer->removeSelf();
		                }
                    }
                );
	        }
		    });

		const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;

        Slider* slider = nullptr;
		//slider = window->addSlider("slider");
		if (slider) {
		    slider->setHandleSize(SDL_Rect{0, 0, 98, 16});
		    slider->setHandleImage("images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderBar_00.png");
		    slider->setRailSize(SDL_Rect{129, 374, 820, 16});
		    slider->setRailImage("__empty");
		    slider->setMinValue(0.f);
		    slider->setValue(0.f);
		    slider->setMaxValue(sliderMaxSize);
		    slider->setTickCallback([](Widget& widget){
		        auto slider = static_cast<Slider*>(&widget);
		        auto frame = static_cast<Frame*>(slider->getParent());
		        auto subwindow = frame->findFrame("subwindow");
		        auto size = subwindow->getActualSize();
		        slider->setValue(size.x);

		        /*auto slider_left = frame->findImage("slider_left");
		        auto slider_right = frame->findImage("slider_right");
	            slider_left->pos.x = slider->getHandleSize().x - 20;
	            slider_right->pos.x = slider->getHandleSize().x + slider->getHandleSize().w;
		        if (slider->isActivated()) {
		            slider_left->disabled = false;
		            slider_right->disabled = false;
		        } else {
		            slider_left->disabled = true;
		            slider_right->disabled = true;
		        }*/
		    });
		    slider->setCallback([](Slider& slider){
		        auto frame = static_cast<Frame*>(slider.getParent());
		        auto subwindow = frame->findFrame("subwindow");
		        auto size = subwindow->getActualSize();
		        size.x = slider.getValue();
		        subwindow->setActualSize(size);
		        savegame_selected = nullptr;
		        });

		    auto slider_left = window->addImage(
		        SDL_Rect{0, 354, 20, 30},
		        0xffffffff,
		        "images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderL_00.png",
		        "slider_left"
		    );
		    slider_left->ontop = true;
		    slider_left->disabled = true;

		    auto slider_right = window->addImage(
		        SDL_Rect{0, 354, 20, 30},
		        0xffffffff,
		        "images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderR_00.png",
		        "slider_right"
		    );
		    slider_right->ontop = true;
		    slider_right->disabled = true;
	    }

		(void)createBackWidget(window, [](Button& button){
			cursor_delete_mode = false;
			savegame_selected = nullptr;
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			createPlayWindow();
			}, SDL_Rect{16, 0, 0, 0});
	}

/******************************************************************************/

	static void mainPlayGame(Button& button) {
		soundActivate();
		createPlayWindow();
	}

	static void mainPlayModdedGame(Button& button) {
		// TODO add a mod selection menu or something here
		soundActivate();
		createPlayWindow();
	}

	static void mainHallOfRecords(Button& button) {
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
			button->setBackground("#images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setTextColor(makeColor(180, 180, 180, 255));
			button->setTextHighlightColor(makeColor(180, 133, 13, 255));
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
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

	static void mainAssignControllers(Button& button) {
	    soundActivate();
        button.deselect();
	    static auto return_to_main_menu = [](){
            assert(main_menu_frame);
	        auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
	        auto button = buttons->findButton("ASSIGN CONTROLLERS"); assert(button);
	        button->select();
	    };
	    if (splitscreen) {
	        static std::vector<int> players;
	        players.clear();
	        players.reserve(4);
	        for (int c = 0; c < 4; ++c) {
	            if (!client_disconnected[c]) {
	                players.push_back(c);
	            }
	        }
	        if (!players.empty()) {
	            createControllerPrompt(players[0], true,
	                [](){if (players.size() >= 2) createControllerPrompt(players[1], true,
                    [](){if (players.size() >= 3) createControllerPrompt(players[2], true,
                    [](){if (players.size() >= 4) createControllerPrompt(players[3], true,
                    return_to_main_menu
                    ); else return_to_main_menu();}
                    ); else return_to_main_menu();}
                    ); else return_to_main_menu();}
                    );
	        }
	    } else {
	        createControllerPrompt(clientnum, false, return_to_main_menu);
	    }
	}

	static void mainSettings(Button& button) {
		//soundActivate(); // not needed, activated tab will do this

		settings_tab_name = "";

		settingsMount();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto settings = dimmer->addFrame("settings");
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
			std::vector<const char*> tabs = {
				"UI",
				"Video",
				"Audio",
				"Controls",
			};
			if (!intro) {
				tabs.push_back("Game");
			}
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

		auto window_title = settings->addField("window_title", 64);
		window_title->setFont(banner_font);
		window_title->setSize(SDL_Rect{394, 26, 338, 24});
		window_title->setJustify(Field::justify_t::CENTER);
		window_title->setText("SETTINGS");

		struct Option {
			const char* name;
			void (*callback)(Button&);
		};
		std::vector<Option> tabs = {
			{"UI", settingsUI},
			{"Video", settingsVideo},
			{"Audio", settingsAudio},
			{"Controls", settingsControls},
		};
		if (!intro) {
			tabs.push_back({"Game", settingsGame});
		}
		const int num_tabs = tabs.size();
		for (int c = 0; c < num_tabs; ++c) {
			const int x = settings->getSize().w / (num_tabs + 1);
			auto button = settings->addButton(tabs[c].name);
			button->setCallback(tabs[c].callback);
			button->setText(tabs[c].name);
			button->setFont(banner_font);
			button->setBackground("images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
			button->setBackgroundActivated("images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
			button->setSize(SDL_Rect{x + (x * c) - 184 / 2, 64, 184, 64});
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setWidgetSearchParent("settings");
			button->setWidgetPageLeft("tab_left");
			button->setWidgetPageRight("tab_right");
			button->addWidgetAction("MenuAlt1", "restore_defaults");
			button->addWidgetAction("MenuStart", "confirm_and_exit");
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
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
		tab_left->setColor(makeColor(255, 255, 255, 255));
		tab_left->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_left->setWidgetSearchParent("settings");
		tab_left->setWidgetBack("discard_and_exit");
		tab_left->setWidgetPageLeft("tab_left");
		tab_left->setWidgetPageRight("tab_right");
		tab_left->setWidgetRight("UI");
		tab_left->setWidgetDown("restore_defaults");
		tab_left->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_left->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_left->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			std::vector<const char*> tabs = {
				"UI",
				"Video",
				"Audio",
				"Controls",
			};
			if (!intro) {
				tabs.push_back("Game");
			}
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
		tab_left->setGlyphPosition(Button::glyph_position_t::CENTERED);

		auto tab_right = settings->addButton("tab_right");
		tab_right->setBackground("images/ui/Main Menus/Settings/Settings_Button_R00.png");
		tab_right->setSize(SDL_Rect{1056, 68, 38, 58});
		tab_right->setColor(makeColor(255, 255, 255, 255));
		tab_right->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_right->setWidgetSearchParent("settings");
		tab_right->setWidgetBack("discard_and_exit");
		tab_right->setWidgetPageLeft("tab_left");
		tab_right->setWidgetPageRight("tab_right");
		tab_right->setWidgetLeft("Controls");
		tab_right->setWidgetDown("confirm_and_exit");
		tab_right->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_right->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_right->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			std::vector<const char*> tabs = {
				"Controls",
				"Audio",
				"Video",
				"UI",
			};
			if (!intro) {
				tabs.insert(tabs.begin(), "Game");
			}
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
		tab_right->setGlyphPosition(Button::glyph_position_t::CENTERED);

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
		restore_defaults->setColor(makeColor(255, 255, 255, 255));
		restore_defaults->setHighlightColor(makeColor(255, 255, 255, 255));
		restore_defaults->setWidgetSearchParent("settings");
		restore_defaults->setWidgetBack("discard_and_exit");
		restore_defaults->setWidgetPageLeft("tab_left");
		restore_defaults->setWidgetPageRight("tab_right");
		restore_defaults->setWidgetUp("UI");
		restore_defaults->setWidgetRight("discard_and_exit");
		restore_defaults->addWidgetAction("MenuAlt1", "restore_defaults");
		restore_defaults->addWidgetAction("MenuStart", "confirm_and_exit");
		restore_defaults->setHideKeyboardGlyphs(false);
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
		discard_and_exit->setColor(makeColor(255, 255, 255, 255));
		discard_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		discard_and_exit->setCallback([](Button& button){
			soundCancel();
			if (main_menu_frame) {
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto settings_button = buttons->findButton("SETTINGS"); assert(settings_button);
				settings_button->select();
			}
			auto settings = static_cast<Frame*>(button.getParent());
			if (settings) {
				auto dimmer = static_cast<Frame*>(settings->getParent());
				dimmer->removeSelf();
			}
			});
		discard_and_exit->setWidgetSearchParent("settings");
		discard_and_exit->setWidgetBack("discard_and_exit");
		discard_and_exit->setWidgetPageLeft("tab_left");
		discard_and_exit->setWidgetPageRight("tab_right");
		discard_and_exit->setWidgetUp("Controls");
		discard_and_exit->setWidgetLeft("restore_defaults");
		discard_and_exit->setWidgetRight("confirm_and_exit");
		discard_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		discard_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");
		discard_and_exit->setHideKeyboardGlyphs(false);

		auto confirm_and_exit = settings->addButton("confirm_and_exit");
		confirm_and_exit->setBackground("images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		confirm_and_exit->setSize(SDL_Rect{880, 630, 164, 62});
		confirm_and_exit->setText("Confirm\n& Exit");
		confirm_and_exit->setJustify(Button::justify_t::CENTER);
		confirm_and_exit->setFont(smallfont_outline);
		confirm_and_exit->setColor(makeColor(255, 255, 255, 255));
		confirm_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm_and_exit->setCallback([](Button& button){
			soundActivate();
			settingsApply();
			(void)settingsSave();
			if (main_menu_frame) {
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto settings_button = buttons->findButton("SETTINGS"); assert(settings_button);
				settings_button->select();
			}
			auto settings = static_cast<Frame*>(button.getParent());
			if (settings) {
				auto dimmer = static_cast<Frame*>(settings->getParent());
				dimmer->removeSelf();
			}
			});
		confirm_and_exit->setWidgetSearchParent("settings");
		confirm_and_exit->setWidgetBack("discard_and_exit");
		confirm_and_exit->setWidgetPageLeft("tab_left");
		confirm_and_exit->setWidgetPageRight("tab_right");
		confirm_and_exit->setWidgetUp("tab_right");
		confirm_and_exit->setWidgetLeft("discard_and_exit");
		confirm_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		confirm_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");
		confirm_and_exit->setHideKeyboardGlyphs(false);
	}

	static void mainEditor(Button& button) {
#if defined(WINDOWS)
	    char path[PATH_MAX];
	    completePath(path, "editor.exe");
	    stopMusic();
	    system(path);
#elif defined(LINUX)
	    char path[PATH_MAX];
	    completePath(path, "editor");
	    stopMusic();
	    system(path);
#else
        return;
#endif
	}

	static void mainEndLife(Button& button) {
		binaryPrompt(
			"Are you sure you want to die?\nThere is no return from this.", // window text
			"End Life", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				closeMainMenu();
				stats[clientnum]->HP = 0;
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("END LIFE"); assert(quit_button);
				quit_button->select();
				auto quit_confirm = main_menu_frame->findFrame("binary_prompt");
				if (quit_confirm) {
					auto dimmer = static_cast<Frame*>(quit_confirm->getParent()); assert(dimmer);
					dimmer->removeSelf();
				}
			});
	}

	static void mainRestartGame(Button& button) {
		binaryPrompt(
			"Are you sure you want to restart?\nThis adventure will be lost forever.", // window text
			"Restart", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
				if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
					beginFade(MainMenu::FadeDestination::GameStart);
				} else {
					beginFade(MainMenu::FadeDestination::HallOfTrials);
				}
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("RESTART GAME"); assert(quit_button);
				quit_button->select();
				auto quit_confirm = main_menu_frame->findFrame("binary_prompt");
				if (quit_confirm) {
					auto dimmer = static_cast<Frame*>(quit_confirm->getParent()); assert(dimmer);
					dimmer->removeSelf();
				}
			});
	}

	static void mainQuitToMainMenu(Button& button) {
		binaryPrompt(
			"All progress before the current\ndungeon level will be saved.", // window text
			"Quit to Menu", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
				savethisgame = true;
				beginFade(MainMenu::FadeDestination::RootMainMenu);
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("QUIT TO MAIN MENU"); assert(quit_button);
				quit_button->select();
				auto quit_confirm = main_menu_frame->findFrame("binary_prompt");
				if (quit_confirm) {
					auto dimmer = static_cast<Frame*>(quit_confirm->getParent()); assert(dimmer);
					dimmer->removeSelf();
				}
			});
	}

	static void mainQuitToDesktop(Button& button) {
		static const char* quit_messages[][3] {
			{"You want to leave, eh?\nThen get out and don't come back!", "Fine geez", "Never!"},
			{"Just cancel your plans.\nI'll wait.", "Good luck", "Sure"},
			{"You couldn't kill the lich anyway.", "You're right", "Oh yeah?"},
			{"The gnomes are laughing at you!\nAre you really gonna take that?", "Yeah :(", "No way!"},
			{"Don't go now! There's a\nboulder trap around the corner!", "Kill me", "Oh thanks"},
			{"I'll tell your parents\nyou said a bad word.", "Poop", "Please no"},
			{"Please don't leave!\nThere's more treasure to loot!", "Don't care", "More loot!"},
			{"Just be glad I can't summon\nthe minotaur in real life.", "Too bad", "Point taken"},
			{"I'd leave too.\nThis game looks just like Minecraft.", "lol", "Ouch"}
		};
		constexpr int num_quit_messages = sizeof(quit_messages) / (sizeof(const char*) * 3);

		static int quit_motd = -2;
		++quit_motd;
		if (quit_motd >= num_quit_messages) {
			quit_motd = 0;
		}
		if (quit_motd < 0) {
			quit_motd = rand() % num_quit_messages;
		}

		binaryPrompt(
			quit_messages[quit_motd][0], // window text
			quit_messages[quit_motd][1], // okay text
			quit_messages[quit_motd][2], // cancel text
			[](Button&){ // okay
				soundActivate();
				mainloop = 0;
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("QUIT");
				if (!quit_button) {
					quit_button = buttons->findButton("QUIT TO DESKTOP");
				}
				assert(quit_button);
				quit_button->select();
				auto quit_confirm = main_menu_frame->findFrame("binary_prompt");
				if (quit_confirm) {
					auto dimmer = static_cast<Frame*>(quit_confirm->getParent()); assert(dimmer);
					dimmer->removeSelf();
				}
			});
	}

	static void mainClose(Button& button) {
		soundActivate();
		closeMainMenu();
	}

/******************************************************************************/


    static void handleFadeFinished(bool ingame) {
		if (main_menu_fade_destination == FadeDestination::None) {
			// generally speaking, this shouldn't ever happen. if it did: fix your shit!
			// if for some reason this happens in release mode, just boot the player to the main menu.
			assert(0 &&
				"Set a FadeDestination so the new menu manager knows where to kick the player to."
				"Don't know where? Try MainMenu::FadeDestination::RootMainMenu");
			main_menu_fade_destination = FadeDestination::RootMainMenu;
		} else {
			if (main_menu_fade_destination == FadeDestination::RootMainMenu) {
				destroyMainMenu();
				if (ingame) {
				    victory = 0;
				    doEndgame();
				}
	            playMusic(intromusic[rand() % (NUMINTROMUSIC - 1)], true, false, false);
				createMainMenu(false);
			}
			if (main_menu_fade_destination == FadeDestination::IntroStoryScreen
			    || main_menu_fade_destination == FadeDestination::IntroStoryScreenNoMusicFade) {
				destroyMainMenu();
				createDummyMainMenu();
				createStoryScreen("data/story/intro.json", [](){beginFade(FadeDestination::RootMainMenu);});
				bool fadeMusic = main_menu_fade_destination == FadeDestination::IntroStoryScreen;
				playMusic(sounds[501], false, fadeMusic, false);
			}
			if (main_menu_fade_destination == FadeDestination::HerxMidpointHuman) {
				destroyMainMenu();
				createDummyMainMenu();
				createStoryScreen("data/story/HerxMidpointHuman.json", [](){beginFade(FadeDestination::RootMainMenu);});
				playMusic(intermissionmusic, false, false, false);
			}
			if (main_menu_fade_destination == FadeDestination::HallOfTrials) {
				destroyMainMenu();
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
				setupSplitscreen();
				if (!loadingsavegame) {
	                for (int i = 0; i < SAVE_GAMES_MAX; ++i) {
                        if (!saveGameExists(multiplayer == SINGLE, i)) {
                            savegameCurrentFileIndex = i;
                            break;
                        }
                    }
				}
				doNewGame(false);
				destroyMainMenu();
			}
			fadeout = false;
			main_menu_fade_destination = FadeDestination::None;
		}
	}

	void doMainMenu(bool ingame) {
		if (!main_menu_frame) {
			createMainMenu(ingame);
			assert(main_menu_frame);
		}

        // just always enable DLC in debug. saves headaches
#ifndef NDEBUG
		enabledDLCPack1 = true;
		enabledDLCPack2 = true;
#endif

#ifdef STEAMWORKS
		if ( SteamApps()->BIsDlcInstalled(1010820) )
		{
			enabledDLCPack1 = true;
		}
		if ( SteamApps()->BIsDlcInstalled(1010821) )
		{
			enabledDLCPack2 = true;
		}
#else
#endif // STEAMWORKS

		if (fadeout && fadealpha >= 255) {
            handleFadeFinished(ingame);
        }

        // if no controller is connected, you can always connect one just for the main menu.
        if (!ingame && currentLobbyType == LobbyType::None) {
            if (!inputs.hasController(clientnum)) {
                Input::waitingToBindControllerForPlayer = clientnum;
            }
        }
	}

	void createMainMenu(bool ingame) {
		main_menu_frame = gui->addFrame("main_menu");

        main_menu_frame->setOwner(ingame ? pause_menu_owner : 0);
		main_menu_frame->setBorder(0);
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setColor(ingame ? makeColor(0, 0, 0, 63) : 0);
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

		if (!ingame) {
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
		}

		struct Option {
			const char* name;
			void (*callback)(Button&);
		};
		std::vector<Option> options;
		if (ingame) {
			options.insert(options.begin(), {
				{"BACK TO GAME", mainClose},
				{"ASSIGN CONTROLLERS", mainAssignControllers},
				{"DUNGEON COMPENDIUM", recordsDungeonCompendium},
				{"SETTINGS", mainSettings},
				{"END LIFE", mainEndLife},
				{"RESTART GAME", mainRestartGame},
				{"QUIT TO MAIN MENU", mainQuitToMainMenu},
				{"QUIT TO DESKTOP", mainQuitToDesktop},
				});
		} else {
#ifdef NINTENDO
			options.insert(options.begin(), {
				{"PLAY GAME", mainPlayGame},
				{"HALL OF RECORDS", mainHallOfRecords},
				{"SETTINGS", mainSettings},
				});
#else
			options.insert(options.begin(), {
				{"PLAY GAME", mainPlayGame},
				{"PLAY MODDED GAME", mainPlayModdedGame},
				{"HALL OF RECORDS", mainHallOfRecords},
				{"SETTINGS", mainSettings},
#ifndef NDEBUG
			    {"EDITOR", mainEditor},
#endif
				{"QUIT", mainQuitToDesktop},
				});
#endif
		}

		const int num_options = options.size();

		if (ingame) {
			y = (Frame::virtualScreenY - num_options * 32) / 2;
		}
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
			button->setBackground("#images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setTextColor(makeColor(180, 180, 180, 255));
			button->setTextHighlightColor(makeColor(180, 133, 13, 255));
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
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

		auto button = buttons->findButton(ingame ? "BACK TO GAME" : "PLAY GAME");
		if (button) {
			button->select();
			if (main_menu_cursor_x == 0 && main_menu_cursor_y == 0) {
				main_menu_cursor_x = button->getSize().x - 80;
				main_menu_cursor_y = button->getSize().y - 9 + buttons->getSize().y;
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

		if (!ingame) {
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
	}

	void destroyMainMenu() {
		if (main_menu_frame) {
			main_menu_frame->removeSelf();
			main_menu_frame = nullptr;
		}
		cursor_delete_mode = false;
	}

	void createDummyMainMenu() {
		main_menu_frame = gui->addFrame("main_menu");
        main_menu_frame->setOwner(clientnum);
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setHollow(true);
		main_menu_frame->setBorder(0);
		main_menu_frame->setTickCallback(tickMainMenu);
	}

	void closeMainMenu() {
		destroyMainMenu();
		gamePaused = false;
	}

	void disconnectedFromServer() {
	    assert(0 && "Disconnected from server. Need a window here!");
	}

	void receiveInvite() {
	    assert(0 && "Received an invite. Behavior goes here!");
	}
}
