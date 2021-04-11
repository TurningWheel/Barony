#include "LoadingScreen.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../draw.hpp"

#include <mutex>
#include <thread>

static std::mutex loading_mutex;

void createLoadingScreen(real_t progress) {
	std::lock_guard<std::mutex> lock(loading_mutex);
	auto loading_frame = gui->addFrame("loading_frame");
	loading_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
	loading_frame->setActualSize(SDL_Rect{0, 0, loading_frame->getSize().w, loading_frame->getSize().h});
	loading_frame->setHollow(true);
	loading_frame->setBorder(0);
	loading_frame->addImage(
		SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY},
		makeColor(255, 255, 255, 255),
		"images/system/backdrop_loading.png",
		"backdrop"
	);
	loading_frame->addImage(
		SDL_Rect{10, Frame::virtualScreenY - 60, Frame::virtualScreenX - 20, 50},
		makeColor(255, 0, 0, 127),
		"images/system/white.png",
		"progress_background"
	);
	loading_frame->addImage(
		SDL_Rect{10, Frame::virtualScreenY - 60, (int)((Frame::virtualScreenX - 20) * progress / (real_t)100), 50},
		makeColor(255, 0, 0, 255),
		"images/system/white.png",
		"progress_filled"
	);
	loading_frame->addImage(
		SDL_Rect{10, 10, 100, 100},
		makeColor(255, 255, 255, 255),
		"images/ui/boulder/boulder0.png",
		"spinning_widget"
	);
}

void updateLoadingScreen(real_t progress) {
	std::lock_guard<std::mutex> lock(loading_mutex);
	auto loading_frame = gui->findFrame("loading_frame");
	if (!loading_frame)
	{
		return;
	}
	auto progress_filled = loading_frame->findImage("progress_filled");
	if (!progress_filled)
	{
		return;
	}
	progress_filled->pos = SDL_Rect{10, Frame::virtualScreenY - 60, (int)((Frame::virtualScreenX - 20) * progress / (real_t)100), 50};
}

void doLoadingScreen() {
	Uint32 oldTicks = ticks;
	handleEvents();
	if (oldTicks != ticks) {
		std::lock_guard<std::mutex> lock(loading_mutex);
		auto loading_frame = gui->findFrame("loading_frame");
		assert(loading_frame);
		auto spinning_widget = loading_frame->findImage("spinning_widget");
		int i = (int)strtol(spinning_widget->path.substr(25).c_str(), nullptr, 10);
		i = (i + 1) % 30;
		spinning_widget->path = std::string("images/ui/boulder/boulder") + std::to_string(i) + ".png";
		drawClearBuffers();
		gui->draw();
		GO_SwapBuffers(screen);
	}
}

void destroyLoadingScreen() {
	std::lock_guard<std::mutex> lock(loading_mutex);
	auto loading_frame = gui->findFrame("loading_frame");
	assert(loading_frame);
	loading_frame->removeSelf();
}