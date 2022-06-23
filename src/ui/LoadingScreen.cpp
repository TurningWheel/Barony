#include "LoadingScreen.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../draw.hpp"
#include "../prng.hpp"

#include <mutex>
#include <thread>
#include <cassert>

static std::mutex loading_mutex;
static framebuffer loading_fb;
constexpr Uint32 loading_bar_color_empty  = makeColor(255, 76, 49, 127);
constexpr Uint32 loading_bar_color_filled = makeColor(255, 76, 49, 255);

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
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    drawAllPlayerCameras();
	    framebuffer::unbind();
#endif
	}

    // Loading... text
    auto label = loading_frame->addField("loading_label", 128);
    label->setSize(fullscreen);
    label->setJustify(Field::justify_t::CENTER);
    label->setFont("fonts/pixel_maz.ttf#64#2");
    label->setText(language[709]);

	// red bar
	auto progress_background = loading_frame->addImage(
		SDL_Rect{0,
		    Frame::virtualScreenY - 64,
		    Frame::virtualScreenX,
		    64},
		background_image ? loading_bar_color_empty : 0,
		"images/system/white.png",
		"progress_background"
	);

	// red bar (filled)
	loading_frame->addImage(
		SDL_Rect{0,
		    Frame::virtualScreenY - 64,
		    (int)((Frame::virtualScreenX * progress) / (real_t)100),
		    64},
		background_image ? loading_bar_color_filled : 0,
		"images/system/white.png",
		"progress_filled"
	);

	// percentage text
	auto text = loading_frame->addField("text", 8);
	text->setSize(progress_background->pos);
	text->setJustify(Field::justify_t::CENTER);
	text->setFont("fonts/pixelmix.ttf#32#2");
	char buf[8];
	snprintf(buf, sizeof(buf), "%d%%", (int)progress);
	text->setText(buf);
	if (!background_image) {
	    text->setInvisible(true);
	}

	// spinning widget
	loading_frame->addImage(
		SDL_Rect{10, 10, 100, 100},
		makeColor(255, 255, 255, 255),
		"images/ui/LoadingScreen/boulder0.png",
		"spinning_widget"
	);
}

void createLoadingScreen(real_t progress) {
    const char* image;
    switch (local_rng.getU32()%3) {
    default:
    case 0: image = "#images/ui/LoadingScreen/backdrop0.png"; break;
    case 1: image = "#images/ui/LoadingScreen/backdrop1.png"; break;
    case 2: image = "#images/ui/LoadingScreen/backdrop4.png"; break;
    //case 3: image = "#images/ui/LoadingScreen/backdrop2.png"; break;
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

	Uint32 oldTicks = ticks;
	handleEvents();
	if (oldTicks != ticks) {
		// find spinning widget
		auto spinning_widget = loading_frame->findImage("spinning_widget"); assert(spinning_widget);

		// build new image path
		const char path[] = "images/ui/LoadingScreen/boulder";
		auto image_num = spinning_widget->path.substr(sizeof(path) - 1);
		int i = (int)strtol(image_num.c_str(), nullptr, 10);
		i = (i + 1) % 30;

		// assign image path
		spinning_widget->path = std::string(path) + std::to_string(i) + ".png";

		// render
		drawClearBuffers();
		if (loading_fb.fbo) {
		    loading_fb.bindForReading();
		    framebuffer::blit();
		    framebuffer::unbind();
		}
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

	auto progress_filled = loading_frame->findImage("progress_filled");
	if (progress_filled)
	{
		progress_filled->pos = SDL_Rect{0,
		    Frame::virtualScreenY - 64,
		    (int)((Frame::virtualScreenX * progress) / (real_t)100),
		    64};
	}

	auto text = loading_frame->findField("text");
	if (text) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d%%", (int)progress);
		text->setText(buf);
	}
}

void destroyLoadingScreen() {
	std::lock_guard<std::mutex> lock(loading_mutex);
	gui->remove("loading_frame");
	loading_fb.destroy();
}
