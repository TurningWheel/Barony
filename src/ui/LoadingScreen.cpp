#include "LoadingScreen.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../draw.hpp"
#include "../prng.hpp"
#include "../mod_tools.hpp"

#include <mutex>
#include <thread>
#include <cassert>

static std::mutex loading_mutex;
static framebuffer loading_fb;
constexpr Uint32 loading_bar_color_empty  = makeColor(255, 76, 49, 127);
constexpr Uint32 loading_bar_color_filled = makeColor(255, 76, 49, 255);
Uint32 loadingticks = 0;

static void baseCreateLoadingScreen(real_t progress, const char* background_image) {
	std::lock_guard<std::mutex> lock(loading_mutex);

	if (!background_image) {
	    // clears any existing player UI...
	    Frame::guiDestroy();
	    Frame::guiInit();
	}

	const SDL_Rect fullscreen{0, 0,
	    Frame::virtualScreenX, Frame::virtualScreenY};

	auto loading_frame = gui->addFrame("loading_frame");
	loading_frame->setSize(fullscreen);
	loading_frame->setActualSize(fullscreen);
	loading_frame->setBorder(0);
	loading_frame->setColor(0);

	// background image
	if (background_image) {
	    loading_frame->addImage(
		    fullscreen,
		    makeColor(255, 255, 255, 255),
		    background_image,
		    "backdrop"
	    );

		// loading bar
		auto loading_bar = loading_frame->addFrame("loading_bar");
		loading_bar->setSize(SDL_Rect{(Frame::virtualScreenX - 1168) / 2, Frame::virtualScreenY - (720 - 474), 1168, 228});
		{
			// background
			auto background = loading_bar->addImage(
				SDL_Rect{0, 0, 1132, 190},
				0xffffffff,
				"images/ui/LoadingScreen/LoadingBar/UI_Loading_Bar_BodyUnder_00.png",
				"background"
			);

			// gas
			auto gas = loading_bar->addImage(
				SDL_Rect{52, 114, 0, 36},
				0xffffffff,
				"images/ui/LoadingScreen/LoadingBar/Gas/000.png",
				"gas"
			);
			gas->tiled = true;

			// bubbles
			auto bubbles = loading_bar->addImage(
				SDL_Rect{52, 102, 0, 60},
				0xffffffff,
				"images/ui/LoadingScreen/LoadingBar/Bubbles/000.png",
				"bubbles"
			);
			bubbles->tiled = true;

			// foreground
			auto foreground = loading_bar->addImage(
				SDL_Rect{28, 90, 1140, 138},
				0xffffffff,
				"images/ui/LoadingScreen/LoadingBar/UI_Loading_Bar_FlourishOver_00.png",
				"foreground"
			);

			// gungnir tip
			auto gungnir = loading_bar->addImage(
				SDL_Rect{-16, 98, 112, 66},
				0xffffffff,
				"images/ui/LoadingScreen/LoadingBar/UI_Loading_GungnirMiddle_00.png",
				"gungnir"
			);
		}
	} else {
        // dimmer
        loading_frame->addImage(
	        fullscreen,
	        makeColor(0, 0, 0, 63),
	        "images/system/white.png",
	        "dimmer"
        );

#ifndef EDITOR
	    // create framebuffer for background
	    loading_fb.init(xres, yres, GL_LINEAR, GL_LINEAR);
	    loading_fb.bindForWriting();
        GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	    drawAllPlayerCameras();
        loading_fb.unbindForWriting();
#endif

		// spinning widget
		loading_frame->addImage(
			SDL_Rect{10, 10, 100, 100},
			makeColor(255, 255, 255, 255),
			"images/ui/LoadingScreen/boulder0.png",
			"spinning_widget"
		);
	}

	// Loading... text
	auto label = loading_frame->addField("loading_label", 128);
    label->setSize(fullscreen);
    label->setJustify(Field::justify_t::CENTER);
	label->setFont("fonts/pixel_maz.ttf#64#2");
	label->setText(Language::get(709));
}

void createLoadingScreen(real_t progress) {
    const char* image;
    switch (local_rng.uniform(0, 3)) {
    default:
    case 0: image = "#images/ui/LoadingScreen/backdrop0.png"; break;
    case 1: image = "#images/ui/LoadingScreen/backdrop1.png"; break;
    case 2: image = "#images/ui/LoadingScreen/backdrop2.png"; break;
    case 3: image = "#images/ui/LoadingScreen/backdrop4.png"; break;
    //case 4: image = "#images/ui/LoadingScreen/backdrop3.png"; break;
    }
    baseCreateLoadingScreen(progress, image);
}

void createLevelLoadScreen(real_t progress) {
    baseCreateLoadingScreen(progress, nullptr); // create loading screen with no background
}

void doLoadingScreen() {
	std::lock_guard<std::mutex> lock(loading_mutex);
	auto loading_frame = gui->findFrame("loading_frame"); assert(loading_frame);
	if (!loading_frame)
	{
		return;
	}

	const Uint32 oldTicks = loadingticks;
	(void)handleEvents();
	if (oldTicks != loadingticks) {

		if ( Mods::isLoading )
		{
			++Mods::loadingTicks;
		}

		// spinning widget
		auto spinning_widget = loading_frame->findImage("spinning_widget");
		if (spinning_widget) {
			// build new image path
			const char path[] = "images/ui/LoadingScreen/boulder";
			auto image_num = spinning_widget->path.substr(sizeof(path) - 1);
			int i = (int)strtol(image_num.c_str(), nullptr, 10);
			i = (i + 1) % 30;

			// assign image path
			spinning_widget->path = std::string(path) + std::to_string(i) + ".png";
		}

		// loading bar
		auto loading_bar = loading_frame->findFrame("loading_bar");
		if (loading_bar) {
			// gas
			if (loadingticks % 4 == 0) {
				auto gas = loading_bar->findImage("gas");
				if (gas) {
					constexpr int num_frames = 6;

					// build new image path
					const char path[] = "images/ui/LoadingScreen/LoadingBar/Gas/";
					auto image_num = gas->path.substr(sizeof(path) - 1);
					int i = (int)strtol(image_num.c_str(), nullptr, 10);
					i = (i + 1) % num_frames;

					// assign image path
					char buf[256];
					snprintf(buf, sizeof(buf), "%s%0.3d.png", path, i);
					gas->path = buf;
				}
			}

			// bubbles
			if (loadingticks % 4 == 2) {
				auto bubbles = loading_bar->findImage("bubbles");
				if (bubbles) {
					constexpr int num_frames = 12;

					// build new image path
					const char path[] = "images/ui/LoadingScreen/LoadingBar/Bubbles/";
					auto image_num = bubbles->path.substr(sizeof(path) - 1);
					int i = (int)strtol(image_num.c_str(), nullptr, 10);
					i = (i + 1) % num_frames;

					// assign image path
					char buf[256];
					snprintf(buf, sizeof(buf), "%s%0.3d.png", path, i);
					bubbles->path = buf;
				}
			}
		}

		// render
		drawClearBuffers();
		if (loading_fb.fbo) {
		    loading_fb.bindForReading();
            loading_fb.draw();
		}
		if (fadealpha > 0) {
			drawRect(NULL, makeColor(0, 0, 0, 255), fadealpha);
		}
		gui->process();
		gui->predraw();
		gui->draw();
		gui->postdraw();
		GO_SwapBuffers(screen);
	}
}

void updateLoadingScreen(real_t progress) {
	std::lock_guard<std::mutex> lock(loading_mutex);
	auto loading_frame = gui->findFrame("loading_frame");
	if (!loading_frame)
	{
		return;
	}

#if 0
	// update red progress bar (deprecated)
	auto progress_filled = loading_frame->findImage("progress_filled");
	if (progress_filled)
	{
		progress_filled->pos = SDL_Rect{0,
		    Frame::virtualScreenY - 64,
		    (int)((Frame::virtualScreenX * progress) / (real_t)100),
		    64};
	}

	// update percentage text (deprecated)
	auto text = loading_frame->findField("text");
	if (text) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d%%", (int)progress);
		text->setText(buf);
	}
#endif

	// update loading bar
	auto loading_bar = loading_frame->findFrame("loading_bar");
	if (loading_bar) {
		const int size = ((int)progress * 1062) / 100;

		// gas
		auto gas = loading_bar->findImage("gas");
		if (gas) {
			gas->pos.w = size;
		}

		// bubbles
		auto bubbles = loading_bar->findImage("bubbles");
		if (bubbles) {
			bubbles->pos.w = size;
		}

		// gungnir
		auto gungnir = loading_bar->findImage("gungnir");
		if (gungnir) {
			gungnir->pos.x = size - 16;
		}
	}
}

void destroyLoadingScreen() {
	std::lock_guard<std::mutex> lock(loading_mutex);
	gui->remove("loading_frame");
	loading_fb.destroy();
}
