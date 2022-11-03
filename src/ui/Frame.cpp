// Frame.cpp

#include <assert.h>

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Button.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Slider.hpp"
#include "Text.hpp"
#include "../interface/consolecommand.hpp"
#include <queue>
#include "GameUI.hpp"
#ifndef EDITOR
#include "MainMenu.hpp"
#endif

const Sint32 Frame::sliderSize = 16;

static const int _virtualScreenDefaultWidth = 1280;
static const int _virtualScreenMinHeight = 720;
int Frame::_virtualScreenX = 0;
int Frame::_virtualScreenY = 0;

static int getMouseOwnerPauseMenu() {
#ifndef EDITOR
	if (gamePaused) {
		for (int i = 0; i < MAXPLAYERS; ++i) {
			if (inputs.bPlayerUsingKeyboardControl(i)) {
				return i;
			}
		}
	}
#endif
    return clientnum;
}

#ifndef EDITOR
#include "../net.hpp"
ConsoleCommand myCmd("/resizegui", "change gui size",
    [](int argc, const char** argv){
    if (argc < 3) {
        messagePlayer(clientnum, MESSAGE_MISC, "Needs 2 args eg: /resizegui 1280 720");
        return;
    }
    const int x = (int)strtol(argv[1], nullptr, 10);
    const int y = (int)strtol(argv[2], nullptr, 10);
    Frame::guiResize(x, y);
    });
#endif

static const Uint32 tooltip_background = makeColor(0, 0, 0, 191);
static const Uint32 tooltip_border_color = makeColor(51, 33, 26, 255);
static const int tooltip_border_width = 2;
static const Uint32 tooltip_text_color = makeColor(255, 255, 255, 255);
static const char* tooltip_text_font = "fonts/pixel_maz_multiline.ttf#16#2";

static framebuffer gui_fb, gui_fb_upscaled, gui_fb_downscaled;

// root of all widgets
Frame* gui = nullptr;

Frame::FrameSearchType Frame::findFrameDefaultSearchType = Frame::FRAME_SEARCH_BREADTH_FIRST;

void Frame::listener_t::onDeleted() {
	if (!entry) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	entryCast->suicide = true;
}

void Frame::listener_t::onChangeColor(bool selected, bool highlighted) {
	if (!entry) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	if (selected) {
		entryCast->color = makeColor( 255, 0, 0, 255);
	} else if (highlighted) {
		entryCast->color = makeColor( 255, 255, 0, 255);
	} else {
		entryCast->color = 0xffffffff;
	}
}

void Frame::listener_t::onChangeName(const char* name) {
	if (!entry) {
		return;
	}
	Frame::entry_t* entryCast = (Frame::entry_t *)entry;
	entryCast->text = name;
}

#ifndef EDITOR
static ConsoleVariable<bool> ui_filter("/ui_filter", false);
static ConsoleCommand ui_filter_refresh("/ui_filter_refresh", "refresh ui filter state",
    [](int argc, const char** argv){
    Frame::fboDestroy();
    Frame::fboInit();
    });
#endif

void Frame::fboInit() {
#ifdef EDITOR
    gui_fb.init(Frame::virtualScreenX, Frame::virtualScreenY, GL_NEAREST, GL_NEAREST);
#else
    if (*ui_filter) {
        gui_fb.init(Frame::virtualScreenX, Frame::virtualScreenY, GL_LINEAR, GL_LINEAR);
    } else {
        gui_fb.init(Frame::virtualScreenX, Frame::virtualScreenY, GL_NEAREST, GL_NEAREST);
    }
#endif
#ifndef NINTENDO
    gui_fb_upscaled.init(Frame::virtualScreenX * 3, Frame::virtualScreenY * 3, GL_LINEAR, GL_NEAREST); // 4k resolution
    gui_fb_downscaled.init(Frame::virtualScreenX / 2, Frame::virtualScreenY / 2, GL_LINEAR, GL_NEAREST); // 360p resolution
#endif
}

void Frame::fboDestroy() {
	gui_fb.destroy();
#ifndef NINTENDO
	gui_fb_upscaled.destroy();
	gui_fb_downscaled.destroy();
#endif
}

void Frame::guiInit() {
	if ( _virtualScreenX == 0 && _virtualScreenY == 0 ) {
		const int defaultWidth = _virtualScreenDefaultWidth;
		const int vsize = (yres * defaultWidth) / xres;
		_virtualScreenX = defaultWidth;
		_virtualScreenY = std::max(vsize, _virtualScreenMinHeight);
	}
	fboInit();

	assert(!gui && "gui already exists!");
	gui = new Frame("root");
	SDL_Rect guiRect;
	guiRect.x = 0;
	guiRect.y = 0;
	guiRect.w = Frame::virtualScreenX;
	guiRect.h = Frame::virtualScreenY;
	gui->setSize(guiRect);
	gui->setActualSize(guiRect);
	gui->setHollow(true);

#ifndef EDITOR
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		char name[32] = "";
		snprintf(name, sizeof(name), "game_ui_%d", i);
		gameUIFrame[i] = gui->addFrame(name);
		gameUIFrame[i]->setSize(guiRect);
		gameUIFrame[i]->setActualSize(guiRect);
		gameUIFrame[i]->setHollow(true);
		gameUIFrame[i]->setOwner(i);
		gameUIFrame[i]->setDisabled(true);
	}
#endif
}

void Frame::guiDestroy() {
#ifndef EDITOR
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( gameUIFrame[i] )
		{
			gameUIFrame[i] = nullptr;
		}
		if ( players[i] )
		{
			players[i]->clearGUIPointers();
		}
		MainMenu::destroyMainMenu();
	}
	minimapFrame = nullptr; // shared minimap
#endif

	if (gui) {
		delete gui;
		gui = nullptr;
	}

	fboDestroy();
}

void Frame::guiResize(int x, int y) {
    _virtualScreenX = x;
    _virtualScreenY = y;
    guiDestroy();
    guiInit();
}

Frame::Frame(const char* _name) {
	size.x = 0;
	size.y = 0;
	size.w = 0;
	size.h = 0;

	actualSize.x = 0;
	actualSize.y = 0;
	actualSize.w = 0;
	actualSize.h = 0;

	color = 0;
	borderColor = 0;

	name = _name;
}

Frame::Frame(Frame& _parent, const char* _name) : Frame(_name) {
	parent = &_parent;
	_parent.getFrames().push_back(this);
	_parent.adoptWidget(*this);
}

Frame::~Frame() {
	clear();
}

#ifndef EDITOR
static ConsoleVariable<bool> ui_scale_native("/ui_scale_native", false);    // if true, causes the UI to blit from a backbuffer even if it's already native res
static ConsoleVariable<bool> ui_upscale("/ui_upscale", false);              // upscale UI layer to 4k before downscaling to native res
static ConsoleVariable<bool> ui_downscale("/ui_downscale", false);          // downscale UI layer to 360p before upscaling to native res
static ConsoleVariable<bool> ui_scale("/ui_scale", true);                   // scale the UI layer to native res (should always be on)
#endif

#if !defined(EDITOR)
void Frame::predraw() {
    if (!*ui_scale_native) {
        if (xres == Frame::virtualScreenX && yres == Frame::virtualScreenY) {
            return;
        }
    }
    if (!*ui_scale) {
        return;
    }
    gui_fb.bindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
	SDL_glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Frame::postdraw() {
    if (!*ui_scale_native) {
        if (xres == Frame::virtualScreenX && yres == Frame::virtualScreenY) {
            return;
        }
    }
    if (!*ui_scale) {
        return;
    }
    SDL_glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (*ui_downscale) {
	    gui_fb.bindForReading();
        gui_fb_downscaled.bindForWriting();
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    framebuffer::blit();

        main_framebuffer.bindForWriting();
        gui_fb_downscaled.bindForReading();
        framebuffer::blit();
        framebuffer::unbindForReading();
    }
    else if (*ui_upscale) {
	    gui_fb.bindForReading();
        gui_fb_upscaled.bindForWriting();
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    framebuffer::blit();

        main_framebuffer.bindForWriting();
        gui_fb_upscaled.bindForReading();
        framebuffer::blit();
        framebuffer::unbindForReading();
    }
    else {
        main_framebuffer.bindForWriting();
	    gui_fb.bindForReading();
	    framebuffer::blit();
        framebuffer::unbindForReading();
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
#else
// EDITOR ONLY DEFINITIONS:
void Frame::predraw() {
    if (xres == Frame::virtualScreenX && yres == Frame::virtualScreenY) {
        return;
    }
    gui_fb.bindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//SDL_glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}

void Frame::postdraw() {
    if (xres == Frame::virtualScreenX && yres == Frame::virtualScreenY) {
        return;
    }
    SDL_glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    main_framebuffer.bindForWriting();
    gui_fb.bindForReading();
    framebuffer::blit();
    framebuffer::unbindForReading();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
#endif

void Frame::draw() const {
	auto _actualSize = allowScrolling ? actualSize : SDL_Rect{0, 0, size.w, size.h};
	std::vector<const Widget*> selectedWidgets;
	std::vector<const Widget*> searchParents;
	findSelectedWidgets(selectedWidgets);
	for (auto widget : selectedWidgets) {
	    searchParents.push_back(widget->findSearchRoot());
	}
	Frame::draw(size, _actualSize, selectedWidgets);
	Frame::drawPost(size, _actualSize, selectedWidgets, searchParents);
}

void Frame::drawPost(SDL_Rect _size, SDL_Rect _actualSize,
    const std::vector<const Widget*>& selectedWidgets,
    const std::vector<const Widget*>& searchParents) const {
	if (disabled || invisible)
		return;

	// warning: overloading member variable!
	SDL_Rect actualSize = allowScrolling ? this->actualSize : SDL_Rect{0, 0, size.w, size.h};

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	if (scrollbars && size.h < actualSize.h) {
		_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	} else {
		_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	}
	if (scrollbars && size.w < actualSize.w) {
		_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	} else {
		_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	}
	if (_size.w <= 0 || _size.h <= 0)
		return;

	SDL_Rect scroll = actualSize;
	if (size.x - _actualSize.x < 0) {
		scroll.x -= size.x - _actualSize.x;
	}
	if (size.y - _actualSize.y < 0) {
		scroll.y -= size.y - _actualSize.y;
	}

	for (auto field : fields) {
		field->drawPost(_size, scroll, selectedWidgets, searchParents);
	}
	for (auto button : buttons) {
		button->drawPost(_size, scroll, selectedWidgets, searchParents);
	}
	for (auto slider : sliders) {
		slider->drawPost(_size, scroll, selectedWidgets, searchParents);
	}
	for ( auto frame : frames ) {
		frame->drawPost(_size, scroll, selectedWidgets, searchParents);
	}

	Widget::drawPost(_size, selectedWidgets, searchParents);
}

static bool isMouseActive(int owner) {
#if defined(EDITOR)
	return true;
#elif defined(NINTENDO)
	return fingerdown;
#else
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	return inputs.getVirtualMouse(mouseowner)->draw_cursor || mousexrel || mouseyrel;
#endif
}

void Frame::draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const {
	if (disabled || invisible)
		return;

	const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};

	// warning: overloading member variable!
	SDL_Rect actualSize = allowScrolling ? this->actualSize : SDL_Rect{0, 0, size.w, size.h};

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	if (scrollbars && size.h < actualSize.h) {
		_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	} else {
		_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	}
	if (scrollbars && size.w < actualSize.w) {
		_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	} else {
		_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	}
	if (_size.w <= 0 || _size.h <= 0)
		return;

    int entrySize = this->entrySize;
    if (entrySize <= 0) {
	    Font* _font = Font::get(font.c_str());
	    if (_font == nullptr) {
	        entrySize = 20;
        } else {
		    entrySize = _font->height();
		    entrySize += entrySize / 2;
	    }
	}

	SDL_Rect scaledSize;
	scaledSize.x = _size.x;
	scaledSize.y = _size.y;
	scaledSize.w = _size.w;
	scaledSize.h = _size.h;

	auto white = Image::get("images/system/white.png");

	// draw frame background
	if (!hollow) {
	    if (border) {
		    SDL_Rect inner;
		    inner.x = (_size.x + border);
		    inner.y = (_size.y + border);
		    inner.w = (_size.w - border*2);
		    inner.h = (_size.h - border*2);
		    if (borderStyle == BORDER_BEVEL_LOW) {
			    white->drawColor(nullptr, inner, viewport, borderColor);
		    } else {
			    white->drawColor(nullptr, inner, viewport, color);
		    }
		} else {
			white->drawColor(nullptr, _size, viewport, color);
		}
	}

#if defined(NINTENDO)
	Sint32 mousex = (::fingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::fingery / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::ofingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::ofingery / (float)yres) * (float)Frame::virtualScreenY;
#elif defined(EDITOR)
	Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 mousexrel = (::mousexrel / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mouseyrel = (::mouseyrel / (float)yres) * (float)Frame::virtualScreenY;
#else
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	Sint32 mousex = (inputs.getMouse(mouseowner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (inputs.getMouse(mouseowner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (inputs.getMouse(mouseowner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (inputs.getMouse(mouseowner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 mousexrel = (inputs.getMouse(mouseowner, Inputs::XREL) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mouseyrel = (inputs.getMouse(mouseowner, Inputs::YREL) / (float)yres) * (float)Frame::virtualScreenY;
#endif

	// horizontal slider
	if (actualSize.w > size.w && scrollbars) {

		// slider rail
		SDL_Rect barRect;
		barRect.x = scaledSize.x;
		barRect.y = scaledSize.y + scaledSize.h;
		barRect.w = scaledSize.w;
		barRect.h = sliderSize;
		white->drawColor(nullptr, barRect, viewport, borderColor);

		// handle
		float winFactor = ((float)_size.w / (float)actualSize.w);
		int handleSize = actualSize.h > size.h ?
		    std::max((int)((size.w - sliderSize) * winFactor), sliderSize):
		    std::max((int)(size.w * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.x;

		SDL_Rect handleRect;
		handleRect.x = scaledSize.x + sliderPos;
		handleRect.y = scaledSize.y + scaledSize.h;
		handleRect.w = handleSize;
		handleRect.h = sliderSize;

		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			white->drawColor(nullptr, handleRect, viewport, sliderColor);
		} else {
			white->drawColor(nullptr, handleRect, viewport, sliderColor);
		}
	}

	// vertical slider
	if (actualSize.h > size.h && _size.y && scrollbars) {
		SDL_Rect barRect;
		barRect.x = scaledSize.x + scaledSize.w;
		barRect.y = scaledSize.y;
		barRect.w = sliderSize;
		barRect.h = scaledSize.h;
		white->drawColor(nullptr, barRect, viewport, borderColor);

		// handle
		float winFactor = ((float)_size.h / (float)actualSize.h);
		int handleSize = actualSize.w > size.w ?
		    std::max((int)((size.h - sliderSize) * winFactor), sliderSize):
		    std::max((int)(size.h * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.y;

		SDL_Rect handleRect;
		handleRect.x = scaledSize.x + scaledSize.w;
		handleRect.y = scaledSize.y + sliderPos;
		handleRect.w = sliderSize;
		handleRect.h = handleSize;

		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			white->drawColor(nullptr, handleRect, viewport, sliderColor);
		} else {
			white->drawColor(nullptr, handleRect, viewport, sliderColor);
		}
	}

	// slider filler (at the corner between sliders)
	if (actualSize.w > size.w && actualSize.h > size.h && scrollbars) {
		SDL_Rect barRect;
		barRect.x = scaledSize.x + scaledSize.w;
		barRect.y = scaledSize.y + scaledSize.h;
		barRect.w = sliderSize;
		barRect.h = sliderSize;
		white->drawColor(nullptr, barRect, viewport, borderColor);
	}

	SDL_Rect scroll = actualSize;
	if (size.x - _actualSize.x < 0) {
		scroll.x -= size.x - _actualSize.x;
	}
	if (size.y - _actualSize.y < 0) {
		scroll.y -= size.y - _actualSize.y;
	}

	// draw images
	for (auto image : images) {
		if (image->disabled) {
			continue;
		}
		if (image->ontop) {
			continue;
		}
		drawImage(image, _size, scroll);
	}

	const bool mouseActive = isMouseActive(owner);

	// draw list entries
	if (list.size()) {
		int listStart = std::min(std::max(0, scroll.y / entrySize), (int)list.size() - 1);
		int i = listStart;
		for (int i = listStart; i < list.size(); ++i) {
			entry_t& entry = *list[i];

			// draw highlighted background
		    if (activated || mouseActive) {
		        SDL_Rect pos;
		        pos.x = _size.x + border - scroll.x;
		        pos.y = _size.y + border + i * entrySize - scroll.y;
		        pos.w = _size.w;
		        pos.h = entrySize;

		        SDL_Rect dest;
		        dest.x = std::max(_size.x, pos.x);
		        dest.y = std::max(_size.y, pos.y);
		        dest.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
		        dest.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));

                if (activation == &entry && activatedEntryColor) {
                    white->drawColor(nullptr, dest, viewport, activatedEntryColor);
                }
		        else if (selection == i && selectedEntryColor) {
                    white->drawColor(nullptr, dest, viewport, selectedEntryColor);
                }
            }

			// draw an image if applicable
			if (entry.text.empty()) {
			    if (!entry.image.empty()) {
			        auto image = Image::get(entry.image.c_str());
			        if (!image) {
			            continue;
			        }

			        int imageW = image->getWidth();
			        int imageH = image->getHeight();

			        SDL_Rect pos;
			        switch (justify) {
			        case justify_t::LEFT: pos.x = border + listOffset.x; break;
			        case justify_t::CENTER: pos.x = (_size.w - imageW) / 2 + listOffset.x; break;
			        case justify_t::RIGHT: pos.x = _size.w - imageW - border + listOffset.x; break;
			        default: break;
			        }
			        pos.y = border + listOffset.y + i * entrySize;
			        pos.w = imageW;
			        pos.h = imageH;

			        image_t _image;
			        _image.pos = pos;
			        _image.path = entry.image;
			        _image.color = entry.color;
		            drawImage(&_image, _size, scroll);
			    }
				continue;
			}

			// get rendered text
			Text* text = Text::get(entry.text.c_str(), font.c_str(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
			if (text == nullptr) {
				continue;
			}

			// get the size of the rendered text
			int textSizeW = text->getWidth();
			int textSizeH = text->getHeight();

			SDL_Rect pos;
			switch (justify) {
			case justify_t::LEFT: pos.x = _size.x + border + listOffset.x - scroll.x; break;
			case justify_t::CENTER: pos.x = _size.x + (_size.w - textSizeW) / 2 + listOffset.x - scroll.x; break;
			case justify_t::RIGHT: pos.x = _size.x + _size.w - textSizeW - border + listOffset.x - scroll.x; break;
			default: break;
			}
			pos.y = _size.y + border + listOffset.y + i * entrySize - scroll.y;
			pos.w = textSizeW;
			pos.h = textSizeH;

			SDL_Rect dest;
			dest.x = std::max(_size.x, pos.x);
			dest.y = std::max(_size.y, pos.y);
			dest.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
			dest.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));

			SDL_Rect src;
			src.x = std::max(0, _size.x - pos.x);
			src.y = std::max(0, _size.y - pos.y);
			src.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
			src.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));

			if (src.w <= 0 || src.h <= 0 || dest.w <= 0 || dest.h <= 0)
				continue;

			// TODO entry highlighting
			SDL_Rect entryback = dest;
			entryback.w = _size.w - border * 2;
		
			entryback.x = entryback.x;
			entryback.y = entryback.y;
			entryback.w = entryback.w;
			entryback.h = entryback.h;
			if (entry.pressed) {
				white->drawColor(nullptr, entryback, viewport, color);
			} else if (entry.highlighted) {
				white->drawColor(nullptr, entryback, viewport, color);
			} else if (!mouseActive && selection >= 0 && selection == i) {
				white->drawColor(nullptr, entryback, viewport, color);
			}

			text->drawColor(src, dest, viewport, entry.color);
		}
	}

	// draw sliders
	for (auto slider : sliders) {
		if (!slider->isOntop()) {
			slider->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw fields
	for (auto field : fields) {
		if ( !field->isOntop() ) {
		    field->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw buttons
	for (auto button : buttons) {
		if ( !button->isOntop() ) {
		    button->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw subframes
	for ( auto frame : frames ) {
		frame->draw(_size, scroll, selectedWidgets);
	}

	// draw "on top" buttons
	for (auto button : buttons) {
		if ( button->isOntop() ) {
		    button->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw "on top" fields
	for ( auto field : fields ) {
		if ( field->isOntop() ) {
		    field->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw "on top" sliders
	for (auto slider : sliders) {
		if (slider->isOntop()) {
			slider->draw(_size, scroll, selectedWidgets);
		}
	}

	// draw "on top" images
	for (auto image : images) {
		if (image->disabled) {
			continue;
		}
		if (!image->ontop) {
			continue;
		}
		drawImage(image, _size, scroll);
	}

	// draw user stuff
	if (drawCallback) {
		drawCallback(*this, _size);
	}

	// draw frame border
	if (!hollow) {
	    if (border) {
	        auto white = Image::get("images/system/white.png");
	        const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
		    if (borderStyle == BORDER_BEVEL_LOW) {
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y, border, _size.h}, viewport, color);
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y, _size.w, border}, viewport, color);
			    white->drawColor(nullptr, SDL_Rect{_size.x + _size.w - border, _size.y, border, _size.h}, viewport, color);
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y + _size.h - border, _size.w, border}, viewport, color);
		    } else {
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y, border, _size.h}, viewport, borderColor);
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y, _size.w, border}, viewport, borderColor);
			    white->drawColor(nullptr, SDL_Rect{_size.x + _size.w - border, _size.y, border, _size.h}, viewport, borderColor);
			    white->drawColor(nullptr, SDL_Rect{_size.x, _size.y + _size.h - border, _size.w, border}, viewport, borderColor);
		    }
		}
	}

	// root frame draws tooltip
	// TODO on Nintendo, display this next to the currently selected widget
	if (!parent) {
		if (tooltip && tooltip[0] != '\0') {
			Font* font = Font::get(tooltip_text_font);
			if (font) {
				const int border = tooltip_border_width;
				Text* text = Text::get(tooltip, font->getName(),
					makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));

				SDL_Rect src;
				src.w = text->getWidth() + border * 2;
				src.h = text->getHeight() + border * 2;
				src.x = mousex + 24;
				src.y = mousey + 24;

				white->drawColor(nullptr, SDL_Rect{src.x, src.y, border, src.h}, viewport, tooltip_border_color);
				white->drawColor(nullptr, SDL_Rect{src.x, src.y, src.w, border}, viewport, tooltip_border_color);
				white->drawColor(nullptr, SDL_Rect{src.x + src.w - border, src.y, border, src.h}, viewport, tooltip_border_color);
				white->drawColor(nullptr, SDL_Rect{src.x, src.y + src.h - border, src.w, border}, viewport, tooltip_border_color);

				SDL_Rect src2{src.x + border, src.y + border, src.w - border * 2, src.h - border * 2};
				src2.x = src2.x;
				src2.y = src2.y;
				src2.w = src2.w;
				src2.h = src2.h;
				white->drawColor(nullptr, src2, viewport, tooltip_background);

				text->drawColor(SDL_Rect{0,0,0,0}, src2, viewport, tooltip_text_color);
			}
		}
	}
}

Frame::result_t Frame::process() {
	std::vector<Widget*> selectedWidgets;
	findSelectedWidgets(selectedWidgets);
	result_t result = process(size, allowScrolling ? actualSize : SDL_Rect{0, 0, size.w, size.h}, selectedWidgets, true);

	tooltip = nullptr;
	if (result.tooltip && result.tooltip[0] != '\0') {
		if (SDL_GetTicks() - result.highlightTime >= tooltipTime) {
			tooltip = result.tooltip;
		}
	}
	postprocess();

	return result;
}

Frame::result_t Frame::process(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<Widget*>& selectedWidgets, bool usable) {
	result_t result;
	result.removed = toBeDeleted;
	result.usable = usable;
	result.highlightTime = SDL_GetTicks();
	result.tooltip = nullptr;

	if (disabled) {
		return result;
	}

	if ( parent && inheritParentFrameOpacity ) {
		setOpacity(static_cast<Frame*>(parent)->getOpacity());
	}

	// warning: overloading member variable!
	SDL_Rect actualSize = allowScrolling ? this->actualSize : SDL_Rect{0, 0, size.w, size.h};

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	if (scrollbars && size.h < actualSize.h) {
		_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	} else {
		_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	}
	if (scrollbars && size.w < actualSize.w) {
		_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	} else {
		_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	}
	if (_size.w <= 0 || _size.h <= 0) {
		return result;
	}

    int entrySize = this->entrySize;
    if (entrySize <= 0) {
	    Font* _font = Font::get(font.c_str());
	    if (_font == nullptr) {
	        entrySize = 20;
        } else {
		    entrySize = _font->height();
		    entrySize += entrySize / 2;
	    }
	}

	SDL_Rect fullSize = _size;
	fullSize.h += (actualSize.w > size.w) ? sliderSize : 0;
	fullSize.w += (actualSize.h > size.h) ? sliderSize : 0;

#if defined(NINTENDO)
	Sint32 mousex = (::fingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::fingery / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::ofingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::ofingery / (float)yres) * (float)Frame::virtualScreenY;
#elif defined(EDITOR)
	Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 mousexrel = (::mousexrel / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mouseyrel = (::mouseyrel / (float)yres) * (float)Frame::virtualScreenY;
#else
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	Sint32 mousex = (inputs.getMouse(mouseowner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (inputs.getMouse(mouseowner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (inputs.getMouse(mouseowner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (inputs.getMouse(mouseowner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 mousexrel = (inputs.getMouse(mouseowner, Inputs::XREL) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mouseyrel = (inputs.getMouse(mouseowner, Inputs::YREL) / (float)yres) * (float)Frame::virtualScreenY;
#endif

	Input& input = Input::inputs[owner];

	// widget to move to after processing inputs
	Widget* destWidget = nullptr;

	if (activated) {
		// unselect list
		if (input.consumeBinaryToggle("MenuCancel")) {
			deselect();
			std::string deselectTarget;
			auto find = widgetMovements.find("MenuListCancel");
			if (find != widgetMovements.end()) {
				deselectTarget = find->second;
			}
			if (!deselectTarget.empty()) {
				Frame* root = findSearchRoot(); assert(root);
				Widget* search = root->findWidget(deselectTarget.c_str(), true);
				if (search) {
					search->select();
				}
			}
			if (dropDown) {
				toBeDeleted = true;
			}
			// this special case is necessary for settings menu dropdowns...
			auto fparent = static_cast<Frame*>(parent);
			if (fparent && fparent->dropDown) {
			    fparent->removeSelf();
			}
		}

		// activate selection
		if (input.consumeBinaryToggle("MenuConfirm")) {
			if (selection != -1) {
				activateEntry(*list[selection]);
			}
			std::string deselectTarget;
			auto find = widgetMovements.find("MenuListConfirm");
			if (find != widgetMovements.end()) {
				deselectTarget = find->second;
			}
			if (!deselectTarget.empty()) {
				deselect();
				Frame* root = findSearchRoot(); assert(root);
				Widget* search = root->findWidget(deselectTarget.c_str(), true);
				if (search) {
					search->select();
				}
			}
		}

		// choose a selection
		if (list.size()) {
			if (selection == -1) {
				if (input.consumeBinaryToggle("MenuUp") || 
					input.consumeBinaryToggle("MenuDown") ||
					input.consumeBinaryToggle("MenuRight") ||
					input.consumeBinaryToggle("MenuLeft") ||
					input.consumeBinaryToggle("AltMenuUp") ||
					input.consumeBinaryToggle("AltMenuDown") ||
					input.consumeBinaryToggle("AltMenuRight") ||
					input.consumeBinaryToggle("AltMenuLeft")) {
					selection = 0;
					scrollToSelection();
					auto entry = list[selection];
					if (entry->selected) {
						(*entry->selected)(*entry);
					}
				}
			} else {
				if (input.consumeBinaryToggle("MenuUp") || input.consumeBinaryToggle("AltMenuUp") ||
				    input.consumeBinaryToggle("MenuLeft") || input.consumeBinaryToggle("AltMenuLeft")) {
					selection = std::max(0, selection - 1);
					scrollToSelection();
					auto entry = list[selection];
					if (entry->selected) {
						(*entry->selected)(*entry);
					}
				}
				if (input.consumeBinaryToggle("MenuDown") || input.consumeBinaryToggle("AltMenuDown") ||
				    input.consumeBinaryToggle("MenuRight") || input.consumeBinaryToggle("AltMenuRight")) {
					selection = std::min((int)list.size() - 1, selection + 1);
					scrollToSelection();
					auto entry = list[selection];
					if (entry->selected) {
						(*entry->selected)(*entry);
					}
				}
			}
		}
	} else if (selected) {
		if (!destWidget) {
			destWidget = handleInput();
		}
		if (destWidget) {
			deselect();
		}
	}

	// process "ontop" (widget) sliders
	for (int i = sliders.size() - 1; i >= 0; --i) {
		Slider* slider = sliders[i];
		if (slider->isOntop()) {
			processSlider(_size, *slider, destWidget, result);
		}
	}

	// process "ontop" fields
	for (int i = fields.size() - 1; i >= 0; --i) {
		Field* field = fields[i];
		if (field->isOntop()) {
            processField(_size, *field, destWidget, result);
        }
	}

	// process "ontop" buttons
	for (int i = buttons.size() - 1; i >= 0; --i) {
		Button* button = buttons[i];
		if (button->isOntop()) {
		    processButton(_size, *button, destWidget, result);
	    }
	}

	// process frames
	{
		for (int i = frames.size() - 1; i >= 0; --i) {
			Frame* frame = frames[i];
			result_t frameResult = frame->process(_size, actualSize, selectedWidgets, result.usable);
			result.usable = frameResult.usable;
			if (!frameResult.removed) {
				if (frameResult.tooltip != nullptr) {
					result = frameResult;
				}
			} else {
				delete frame;
				frames.erase(frames.begin() + i);
			}
		}
	}

	const real_t timeFactor = 1.0 / (real_t)fpsLimit;

	// scroll with right stick
	if (result.usable && allowScrolling && allowScrollBinds) {
		Input& input = Input::inputs[owner];

		const float speed = 1000.0 * timeFactor;

		// x scroll
		if (this->actualSize.w > size.w) {
			const float power = input.analog("MenuScrollRight") - input.analog("MenuScrollLeft");
			if (power) {
				scrollX = std::min(std::max(0.0, scrollX + speed * power),
					(real_t)(this->actualSize.w - _size.w));
				this->actualSize.x = scrollX;
				result.usable = false;
		        syncScroll();
			}
		}

		// y scroll
		if (this->actualSize.h > size.h) {
			const float power = input.analog("MenuScrollDown") - input.analog("MenuScrollUp");
			if (power) {
				scrollY = std::min(std::max(0.0, scrollY + speed * power),
					(real_t)(this->actualSize.h - _size.h));
				this->actualSize.y = scrollY;
				result.usable = false;
		        syncScroll();
			}
		}
	}

	const bool mouseActive = isMouseActive(owner);

#ifndef EDITOR
	static ConsoleVariable<float> cvar_scrollFriction("/scroll_friction", 10.0);
	static ConsoleVariable<float> cvar_scrollSpeed("/scroll_speed", 50000.0);
	const real_t scrollFriction = *cvar_scrollFriction * timeFactor;
	const real_t scrollSpeed = *cvar_scrollSpeed * timeFactor;
#else
	const real_t scrollFriction = 10.0 * timeFactor;
	const real_t scrollSpeed = 50000.0 * timeFactor;
#endif

	// scroll with mouse wheel
	if (parent != nullptr && !hollow && mouseActive && rectContainsPoint(fullSize, omousex, omousey) && result.usable) {
		bool mwheeldown = false;
		bool mwheelup = false;
		if (allowScrolling && allowScrollBinds) {
			if (input.binaryToggle("MenuMouseWheelDown")) {
				mwheeldown = true;
			}
			if (input.binaryToggle("MenuMouseWheelUp")) {
				mwheelup = true;
			}
			if (mwheeldown || mwheelup) {
				result.usable = false;

				// x scroll with mouse wheel
				if (this->actualSize.w > size.w) {
					if (this->actualSize.h <= size.h) {
						if (mwheeldown) {
							scrollAccelerationX += scrollSpeed;
						}
						if (mwheelup) {
							scrollAccelerationX -= scrollSpeed;
						}
					}
				}

				// y scroll with mouse wheel
				if (this->actualSize.h > size.h) {
					if (mwheeldown) {
						scrollAccelerationY += scrollSpeed;
					}
					if (mwheelup) {
						scrollAccelerationY -= scrollSpeed;
					}
				}
			}
		}
	}

	scrollVelocityX -= scrollVelocityX * std::min(1.0, scrollFriction);
	scrollVelocityX += scrollAccelerationX * timeFactor;
	scrollAccelerationX = 0.0;
    if (scrollVelocityX) {
	    scrollX += scrollVelocityX;
		const real_t oldScrollX = scrollX;
		scrollX = std::min(std::max(0.0, scrollX),
			std::max(0.0, (real_t)(this->actualSize.w - _size.w)));
		if (oldScrollX != scrollX) {
			scrollVelocityX = 0.0;
		}
		this->actualSize.x = scrollX;
	    syncScroll();
	}

	scrollVelocityY -= scrollVelocityY * std::min(1.0, scrollFriction);
	scrollVelocityY += scrollAccelerationY * timeFactor;
	scrollAccelerationY = 0.0;
	if (scrollVelocityY) {
	    scrollY += scrollVelocityY;
		const real_t oldScrollY = scrollY;
		scrollY = std::min(std::max(0.0, scrollY),
			std::max(0.0, (real_t)(this->actualSize.h - _size.h)));
		if (oldScrollY != scrollY) {
			scrollVelocityY = 0.0;
		}
		this->actualSize.y = scrollY;
	    syncScroll();
	}

	if ((scrollbars || allowScrollBinds) && allowScrolling) {
		scrollX = std::min(std::max(0.0, scrollX),
			std::max(0.0, (real_t)(this->actualSize.w - _size.w)));
		this->actualSize.x = scrollX;
		scrollY = std::min(std::max(0.0, scrollY),
			std::max(0.0, (real_t)(this->actualSize.h - _size.h)));
		this->actualSize.y = scrollY;
	}

	bool clicked = false;
	if (mousestatus[SDL_BUTTON_LEFT]) {
		clicked = true;
	}
	else if (fingerdown) {
		clicked = true;
	}

	// process (frame view) sliders
	if (parent != nullptr && !hollow && usable && scrollbars) {
		// filler in between sliders
		if (actualSize.w > size.w && actualSize.h > size.h) {
			SDL_Rect sliderRect;
			sliderRect.x = _size.x + _size.w; sliderRect.w = sliderSize;
			sliderRect.y = _size.y + _size.h; sliderRect.h = sliderSize;
			if ( mouseActive && rectContainsPoint(sliderRect, omousex, omousey) ) {
				result.usable = false;
			}
		}

		// horizontal slider
		if (actualSize.w > size.w) {
			// rail
			SDL_Rect sliderRect;
			sliderRect.x = _size.x;
			sliderRect.y = _size.y + _size.h;
			sliderRect.w = _size.w;
			sliderRect.h = sliderSize;

			// handle
			float winFactor = ((float)_size.w / (float)actualSize.w);
		    int handleSize = actualSize.h > size.h ?
		        std::max((int)((size.w - sliderSize) * winFactor), sliderSize):
		        std::max((int)(size.w * winFactor), sliderSize);
			int sliderPos = winFactor * actualSize.x;
			SDL_Rect handleRect;
			handleRect.x = _size.x + sliderPos;
			handleRect.y = _size.y + _size.h;
			handleRect.w = handleSize;
			handleRect.h = sliderSize;

			// click & drag
			if (draggingHSlider) {
				if (!clicked) {
					draggingHSlider = false;
				} else {
					float winFactor = ((float)_size.w / (float)this->actualSize.w);
					this->actualSize.x = (mousex - omousex) / winFactor + oldSliderX;
					this->actualSize.x = std::min(std::max(0, this->actualSize.x), std::max(0, this->actualSize.w - _size.w));
					scrollX = this->actualSize.x;
					syncScroll();
				}
				result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if ( mouseActive && rectContainsPoint(handleRect, omousex, omousey) ) {
					if (clicked) {
						draggingHSlider = true;
						oldSliderX = this->actualSize.x;
					}
					result.usable = false;
					ticks = -1; // hack to fix sliders in drop.15 *  downs
				} else if ( mouseActive && rectContainsPoint(sliderRect, omousex, omousey) ) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					if (clicked) {
						this->actualSize.x += omousex < handleRect.x ? -std::min(entrySize, size.w) : std::min(entrySize, size.w);
						this->actualSize.x = std::min(std::max(0, this->actualSize.x), std::max(0, this->actualSize.w - _size.w));
						scrollX = this->actualSize.x;
					    syncScroll();
					}
					result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				}
			}
		}

		// vertical slider
		if (actualSize.h > size.h) {
			// rail
			SDL_Rect sliderRect;
			sliderRect.x = _size.x + _size.w;
			sliderRect.y = _size.y;
			sliderRect.w = sliderSize;
			sliderRect.h = _size.h;

			// handle
			float winFactor = ((float)_size.h / (float)actualSize.h);
		    int handleSize = actualSize.w > size.w ?
		        std::max((int)((size.h - sliderSize) * winFactor), sliderSize):
		        std::max((int)(size.h * winFactor), sliderSize);
			int sliderPos = winFactor * actualSize.y;
			SDL_Rect handleRect;
			handleRect.x = _size.x + _size.w;
			handleRect.y = _size.y + sliderPos;
			handleRect.w = sliderSize;
			handleRect.h = handleSize;

			// click & drag
			if (draggingVSlider) {
				if (!clicked) {
					draggingVSlider = false;
				} else {
					float winFactor = ((float)_size.h / (float)this->actualSize.h);
					this->actualSize.y = (mousey - omousey) / winFactor + oldSliderY;
					this->actualSize.y = std::min(std::max(0, this->actualSize.y), std::max(0, this->actualSize.h - _size.h));
					scrollY = this->actualSize.y;
					syncScroll();
				}
				result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if ( mouseActive && rectContainsPoint(handleRect, omousex, omousey) ) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						draggingVSlider = true;
						oldSliderY = this->actualSize.y;
					}
					result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				} else if ( mouseActive && rectContainsPoint(sliderRect, omousex, omousey) ) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					if (clicked) {
						this->actualSize.y += omousey < handleRect.y ? -std::min(entrySize, size.h) : std::min(entrySize, size.h);
						this->actualSize.y = std::min(std::max(0, this->actualSize.y), std::max(0, this->actualSize.h - _size.h));
						scrollY = this->actualSize.y;
					    syncScroll();
					}
					result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				}
			}
		}
	}

	// process buttons
	for (int i = buttons.size() - 1; i >= 0; --i) {
		Button* button = buttons[i];
		if (!button->isOntop()) {
		    processButton(_size, *button, destWidget, result);
	    }
	}

	// process fields
	for (int i = fields.size() - 1; i >= 0; --i) {
		Field* field = fields[i];
		if (!field->isOntop()) {
            processField(_size, *field, destWidget, result);
        }
	}

	// process (widget) sliders
	for (int i = sliders.size() - 1; i >= 0; --i) {
		Slider* slider = sliders[i];
		if (!slider->isOntop()) {
			processSlider(_size, *slider, destWidget, result);
		}
	}

	// process the frame's list entries
	if (result.usable && list.size() > 0) {
		for (int i = 0; i < list.size(); ++i) {
			entry_t* entry = list[i];
			if (entry->suicide) {
				if (selection == i) {
					--selection;
				}
				delete entry;
				list.erase(list.begin() + i);
				continue;
			}

			SDL_Rect entryRect;
			entryRect.x = _size.x + border - actualSize.x + listOffset.x; entryRect.w = _size.w - border * 2;
			entryRect.y = _size.y + border + i * entrySize - actualSize.y + listOffset.y; entryRect.h = entrySize;

			if (mouseActive && entry->clickable
				&& rectContainsPoint(_size, omousex, omousey) 
				&& rectContainsPoint(entryRect, omousex, omousey)) {
				result.highlightTime = entry->highlightTime;
				result.tooltip = entry->tooltip.c_str();
				if (mouseActive) {
					select();
					selection = i;
				}
				if (clicked) {
					if (!entry->pressed) {
						if (mousestatus[SDL_BUTTON_LEFT]) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
						}
						entry->pressed = true;
						activateEntry(*entry);
						activate();
					}
				} else {
					entry->pressed = false;
					if (entry->highlighting) {
						(*entry->highlighting)(*entry);
					}
					if (!entry->highlighted) {
						entry->highlighted = true;
						if (entry->highlight) {
							(*entry->highlight)(*entry);
						}
					}
				}
				result.usable = false;
			} else {
				entry->highlightTime = SDL_GetTicks();
				entry->highlighted = false;
				entry->pressed = false;
			}
		}
	}

	// scroll with arrows or left stick
	if (result.usable && allowScrolling && allowScrollBinds && scrollWithLeftControls) {
		Input& input = Input::inputs[owner];

		// x scroll
		if (this->actualSize.w > size.w) {
			if (input.binaryToggle("MenuRight") || input.binaryToggle("AltMenuRight")) {
				scrollAccelerationX += scrollSpeed;
				result.usable = false;
			}
			else if (input.binaryToggle("MenuLeft") || input.binaryToggle("AltMenuLeft")) {
				scrollAccelerationX -= scrollSpeed;
				result.usable = false;
			}
		}

		// y scroll
		if (this->actualSize.h > size.h) {
			if (input.binaryToggle("MenuDown") || input.binaryToggle("AltMenuDown")) {
				scrollAccelerationY += scrollSpeed;
				result.usable = false;
			}
			else if (input.binaryToggle("MenuUp") || input.binaryToggle("AltMenuUp")) {
				scrollAccelerationY -= scrollSpeed;
				result.usable = false;
			}
		}
	}

	if ( mouseActive && rectContainsPoint(_size, omousex, omousey) && !hollow ) {
		//messagePlayer(0, "%d: %s", getOwner(), getName());
		if (clickable && result.usable) {
			if (mousestatus[SDL_BUTTON_LEFT]) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
			}
			if (clicked && !activated) {
				activate();
			}
		}
		result.usable = false;
	}

	if (toBeDeleted) {
		result.removed = true;
	} else {
		++this->ticks;
		if (destWidget) {
			destWidget->select();
		}
	}

	return result;
}

void Frame::processField(const SDL_Rect& _size, Field& field, Widget*& destWidget, result_t& result) {
	Input& input = Input::inputs[owner];

	const bool mouseActive = isMouseActive(owner);

	// widget capture input
	if (field.isActivated()) {
#ifndef EDITOR
	    if (inputs.hasController(field.getOwner())) {
	        if (input.consumeBinaryToggle("MenuConfirm") ||
	            input.consumeBinaryToggle("MenuCancel")) {
	            field.deactivate();
            }
        }
#endif
	}
	else if (!destWidget) {
		destWidget = field.handleInput();
	}

	Field::result_t fieldResult = field.process(_size, actualSize, result.usable);
	if (result.usable && fieldResult.highlighted) {
		result.highlightTime = fieldResult.highlightTime;
	    result.tooltip = fieldResult.tooltip;
		if (mouseActive && field.isEditable()) {
			field.select();
		}
		if (field.isSelected()) {
			result.usable = false;
		}
	}

	if (fieldResult.entered) {
		result.usable = false;
		if (field.getCallback()) {
			(*field.getCallback())(field);
		} else {
			printlog("modified field with no callback");
		}
	}

	if (destWidget && field.isSelected()) {
		field.deselect();
	}
}

void Frame::processButton(const SDL_Rect& _size, Button& button, Widget*& destWidget, result_t& result) {
	Input& input = Input::inputs[owner];

	const bool mouseActive = isMouseActive(owner);

	if (!destWidget) {
		destWidget = button.handleInput();
	}

	Button::result_t buttonResult = button.process(_size, actualSize, result.usable);
	if (result.usable && buttonResult.highlighted) {
		result.highlightTime = buttonResult.highlightTime;
		result.tooltip = buttonResult.tooltip;
		if (mouseActive) {
			button.select();
		}
		if (buttonResult.clicked) {
			button.activate();
		}
		result.usable = false;
	}

	if (destWidget && button.isSelected()) {
		button.deselect();
	}
}

void Frame::processSlider(const SDL_Rect& _size, Slider& slider, Widget*& destWidget, result_t& result) {
	Input& input = Input::inputs[owner];

	const bool mouseActive = isMouseActive(owner);

	if (!destWidget && !slider.isActivated()) {
		destWidget = slider.handleInput();
	} else {
		result.usable = slider.control() ? result.usable : false;
	}

	Slider::result_t sliderResult = slider.process(_size, actualSize, result.usable);
	if (result.usable && sliderResult.highlighted) {
		result.highlightTime = sliderResult.highlightTime;
		result.tooltip = sliderResult.tooltip;
		if (mouseActive) {
			slider.select();
		}
		if (sliderResult.clicked) {
			slider.fireCallback();
		}
		result.usable = false;
	}

	if (destWidget && slider.isSelected()) {
		slider.deselect();
	}
}

void Frame::postprocess() {
#if !defined(EDITOR) && !defined(NDEBUG)
    static ConsoleVariable<bool> cvar("/disableframetick", false);
    if (*cvar) {
        return;
    }
#endif

	if (tickCallback) {
		(*tickCallback)(*this);
	}
	if (!dontTickChildren) {
	    for (int c = 0; c < frames.size(); ++c) {
	        auto frame = frames[c];
	        if (!frame->disabled) {
		        frame->postprocess();
	        }
	    }
	}

#ifndef EDITOR
	if (dropDown && inputs.bPlayerUsingKeyboardControl(owner)) {
		if (!dropDownClicked) {
			for (int c = 0; c < 3; ++c) {
				if (mousestatus[c]) {
					dropDownClicked |= 1 << c;
				}
			}
		} else {
			for (int c = 0; c < 3; ++c) {
				if (!mousestatus[c]) {
					dropDownClicked &= ~(1 << c);
				}
			}
			if (!dropDownClicked && ticks > 0) {
				toBeDeleted = true;
			}
		}
	}
#endif
}

Frame* Frame::addFrame(const char* name) {
	return new Frame(*this, name);
}

Button* Frame::addButton(const char* name) {
	Button* button = new Button(*this);
	button->setName(name);
	return button;
}

Field* Frame::addField(const char* name, const int len) {
	Field* field = new Field(*this, len);
	field->setName(name);
	return field;
}

Frame::image_t* Frame::addImage(const SDL_Rect pos, const Uint32 color, const char* image, const char* name) {
	if (!image || !name) {
		return nullptr;
	}
	image_t* imageObj = new image_t();
	imageObj->pos = pos;
	imageObj->color = color;
	imageObj->name = name;
	imageObj->path = image;
	images.push_back(imageObj);
	return imageObj;
}

Slider* Frame::addSlider(const char* name) {
	if (!name) {
		return nullptr;
	}
	Slider* slider = new Slider(*this);
	slider->setName(name);
	sliders.push_back(slider);
	return slider;
}

Frame::entry_t* Frame::addEntry(const char* name, bool resizeFrame) {
	entry_t* entry = new entry_t(*this);
	entry->name = name;
	entry->color = 0xffffffff;
	list.push_back(entry);

	if (resizeFrame) {
		resizeForEntries();
	}

	return entry;
}

void Frame::removeSelf() {
	toBeDeleted = true;
}

void Frame::clear() {
	// delete frames
	while (frames.size()) {
		delete frames.front();
		frames.erase(frames.begin());
	}

	// delete buttons
	while (buttons.size()) {
		delete buttons.front();
		buttons.erase(buttons.begin());
	}

	// delete fields
	while (fields.size()) {
		delete fields.front();
		fields.erase(fields.begin());
	}

	// delete images
	while (images.size()) {
		delete images.front();
		images.erase(images.begin());
	}

	// delete sliders
	while (sliders.size()) {
		delete sliders.front();
		sliders.erase(sliders.begin());
	}

	// delete list
	while (list.size()) {
		delete list.front();
		list.erase(list.begin());
	}
	selection = -1;
}

void Frame::clearEntries() {
	while (list.size()) {
		delete list.front();
		list.erase(list.begin());
	}
	selection = -1;
}

bool Frame::remove(const char* name) {
	for (int i = 0; i < frames.size(); ++i) {
		Frame* frame = frames[i];
		if (strcmp(frame->getName(), name) == 0) {
			delete frame;
			frames.erase(frames.begin() + i);
			return true;
		}
	}
	for (int i = 0; i < buttons.size(); ++i) {
		Button* button = buttons[i];
		if (strcmp(button->getName(), name) == 0) {
			delete button;
			buttons.erase(buttons.begin() + i);
			return true;
		}
	}
	for (int i = 0; i < fields.size(); ++i) {
		Field* field = fields[i];
		if (strcmp(field->getName(), name) == 0) {
			delete field;
			fields.erase(fields.begin() + i);
			return true;
		}
	}
	for (int i = 0; i < images.size(); ++i) {
		image_t* image = images[i];
		if (strcmp(image->name.c_str(), name) == 0) {
			delete image;
			images.erase(images.begin() + i);
			return true;
		}
	}
	for (int i = 0; i < sliders.size(); ++i) {
		Slider* slider = sliders[i];
		if (strcmp(slider->getName(), name) == 0) {
			delete slider;
			sliders.erase(sliders.begin() + i);
			return true;
		}
	}
	return false;
}

bool Frame::removeEntry(const char* name, bool resizeFrame) {
	for (int i = 0; i < list.size(); ++i) {
		entry_t* entry = list[i];
		if (entry->name == name) {
			if (selection == i) {
				--selection;
			}
			delete entry;
			list.erase(list.begin() + i);
			if (resizeFrame) {
				resizeForEntries();
			}
			return true;
		}
	}
	return false;
}

int Frame::numFindFrameCalls = 0;

Frame* Frame::findFrame(const char* name, const FrameSearchType frameSearchType) {

	if ( frameSearchType == FRAME_SEARCH_DEPTH_FIRST )
	{
		++numFindFrameCalls;
		int localNumberOfCalls = 0;
		for (auto frame : frames) {
			if (frame->toBeDeleted) {
				continue;
			}
			if (strcmp(frame->getName(), name) == 0) {
				return frame;
			} else {
				Frame* subFrame = frame->findFrame(name);
				if (subFrame) {
					return subFrame;
				}
			}
		}
	}
	else if ( frameSearchType == FRAME_SEARCH_BREADTH_FIRST )
	{
		int localNumberOfCalls = 0;
		std::queue<Frame*> q;
		for ( auto frame : frames )
		{
			if ( frame->toBeDeleted ) 
			{
				continue;
			}
			q.push(frame);
		}
		q.push(nullptr);

		int currentDepth = 0;

		while ( !q.empty() )
		{
			auto subFrame = q.front();
			q.pop();
			++numFindFrameCalls;
			++localNumberOfCalls;
			if ( subFrame == nullptr )
			{
				++currentDepth;
			}
			else
			{
				if ( strcmp(subFrame->getName(), name) == 0 )
				{
					if ( localNumberOfCalls > 1 )
					{
						//printlog("findFrame(): [%s]: searching for '%s' - misses: %d", getName(), name, localNumberOfCalls);
					}
					return subFrame;
				}
				for ( auto frame : subFrame->frames )
				{
					if ( frame->toBeDeleted )
					{
						continue;
					}
					q.push(frame);
				}
				q.push(nullptr);
			}
		}
	}
	return nullptr;
}

Button* Frame::findButton(const char* name) {
	for (auto button : buttons) {
		if (strcmp(button->getName(), name) == 0) {
			return button;
		}
	}
	return nullptr;
}

Field* Frame::findField(const char* name) {
	for (auto field : fields) {
		if (strcmp(field->getName(), name) == 0) {
			return field;
		}
	}
	return nullptr;
}

Frame::image_t* Frame::findImage(const char* name) {
	for (auto image : images) {
		if (image->name == name) {
			return image;
		}
	}
	return nullptr;
}

Frame::entry_t* Frame::findEntry(const char* name) {
	for (auto entry : list) {
		if (entry->name == name) {
			return entry;
		}
	}
	return nullptr;
}

Slider* Frame::findSlider(const char* name) {
	for (auto slider : sliders) {
		if (strcmp(slider->getName(), name) == 0) {
			return slider;
		}
	}
	return nullptr;
}

void Frame::resizeForEntries() {
    int entrySize = this->entrySize;
    if (entrySize <= 0) {
	    Font* _font = Font::get(font.c_str());
	    if (_font == nullptr) {
	        entrySize = 20;
        } else {
		    entrySize = _font->height();
		    entrySize += entrySize / 2;
	    }
	}
	actualSize.h = (Uint32)list.size() * entrySize;
	actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
}

SDL_Rect Frame::getRelativeMousePositionImpl(SDL_Rect& _size, SDL_Rect& _actualSize, bool realtime) const {
#ifdef EDITOR
    return SDL_Rect{0, 0, 0, 0};
#else
	Sint32 _mousex = (inputs.getMouse(owner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 _mousey = (inputs.getMouse(owner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 _omousex = (inputs.getMouse(owner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 _omousey = (inputs.getMouse(owner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 mousex = realtime ? _mousex : _omousex;
	Sint32 mousey = realtime ? _mousey : _omousey;

	if (parent) {
		auto pframe = static_cast<Frame*>(parent);
		auto presult = pframe->getRelativeMousePositionImpl(_size, _actualSize, realtime);
		if (presult.w > 0 && presult.h > 0) {
			_size.x = _size.x + std::max(0, size.x - _actualSize.x);
			_size.y = _size.y + std::max(0, size.y - _actualSize.y);
			if (size.h < actualSize.h && allowScrolling && scrollbars) {
				_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			} else {
				_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			if (size.w < actualSize.w && allowScrolling && scrollbars) {
				_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			} else {
				_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			_actualSize = actualSize;
			if (_size.w <= 0 || _size.h <= 0) {
		        return SDL_Rect{0, 0, 0, 0};
			} else {
				if (rectContainsPoint(_size, mousex, mousey)) {
					return SDL_Rect{mousex - _size.x, mousey - _size.y, _size.w, _size.h};
				}
				else {
			        return SDL_Rect{0, 0, 0, 0};
				}
			}
		} else {
			return SDL_Rect{0, 0, 0, 0};
		}
	} else {
		return SDL_Rect{mousex, mousey, actualSize.w, actualSize.h};
	}
#endif
}

SDL_Rect Frame::getRelativeMousePosition(bool realtime) const {
	SDL_Rect _size = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	SDL_Rect _actualSize = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	return getRelativeMousePositionImpl(_size, _actualSize, realtime);
}

bool Frame::capturesMouseImpl(SDL_Rect& _size, SDL_Rect& _actualSize, bool realtime) const {
	if (parent) {
		auto pframe = static_cast<Frame*>(parent);
		if (pframe->capturesMouseImpl(_size, _actualSize, realtime)) {
			_size.x = _size.x + std::max(0, size.x - _actualSize.x);
			_size.y = _size.y + std::max(0, size.y - _actualSize.y);
			if (size.h < actualSize.h && allowScrolling && scrollbars) {
				_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			} else {
				_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			if (size.w < actualSize.w && allowScrolling && scrollbars) {
				_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			} else {
				_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			_actualSize = actualSize;
			if (_size.w <= 0 || _size.h <= 0) {
				return false;
			} else {
#ifdef EDITOR
				Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
				Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
#else
				Sint32 mousex = (inputs.getMouse(owner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 mousey = (inputs.getMouse(owner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
				Sint32 omousex = (inputs.getMouse(owner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 omousey = (inputs.getMouse(owner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
#endif
				if (realtime && rectContainsPoint(_size, mousex, mousey)) {
					return true;
				}
				else if (!realtime && rectContainsPoint(_size, omousex, omousey)) {
					return true;
				}
				else {
					return false;
				}
			}
		} else {
			return false;
		}
	} else {
		return true;
	}
}

bool Frame::capturesMouseInRealtimeCoords() const {
	SDL_Rect _size = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	SDL_Rect _actualSize = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	return capturesMouseImpl(_size, _actualSize, true);
}

bool Frame::capturesMouse() const {
	SDL_Rect _size = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	SDL_Rect _actualSize = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	return capturesMouseImpl(_size, _actualSize, false);
}

void Frame::warpMouseToFrame(const int player, Uint32 flags) const
{
#ifndef EDITOR
	SDL_Rect _size = getAbsoluteSize();
	inputs.warpMouse(player,
		(_size.x + _size.w / 2) * ((float)xres / (float)Frame::virtualScreenX),
		(_size.y + _size.h / 2) * ((float)yres / (float)Frame::virtualScreenY),
		flags);
#endif
}

SDL_Rect Frame::getAbsoluteSize() const
{
	SDL_Rect _size{ size.x, size.y, size.w, size.h };
	auto _parent = this->parent;
	while ( _parent ) {
		auto pframe = static_cast<Frame*>(_parent);
		_size.x += pframe->size.x - pframe->actualSize.x;
		_size.y += pframe->size.y - pframe->actualSize.y;
		_parent = pframe->parent;
	}
	return _size;
}

Frame* Frame::getParent() {
	if (parent && parent->getType() == WIDGET_FRAME) {
		return static_cast<Frame*>(parent);
	} else {
		return nullptr;
	}
}

void Frame::deselect() {
	Widget::deselect();
	activated = false;
	for (auto frame : frames) {
		if (frame->getOwner() == owner) {
			frame->deselect();
		}
	}
	for (auto button : buttons) {
		if (button->getOwner() == owner) {
			button->deselect();
		}
	}
	for (auto field : fields) {
		if (field->getOwner() == owner) {
			field->deselect();
		}
	}
	for (auto slider : sliders) {
		if (slider->getOwner() == owner) {
			slider->deselect();
		}
	}
}

void Frame::activate() {
	select();
	if (!list.size()) {
		return;
	}
	activated = true;
	if (selection < 0 || selection >= list.size()) {
		selection = 0;
	}
}

void Frame::activateSelection() {
	if (selection >= 0 && selection < list.size()) {
		activateEntry(*list[selection]);
	}
}

void Frame::setSelection(int index) {
	selection = index;
}

void Frame::enableScroll(bool enabled) {
	allowScrolling = enabled;
}

void Frame::scrollToSelection(bool scroll_to_top) {
	if (selection < 0 || selection >= list.size()) {
		return;
	}
    int entrySize = this->entrySize;
    if (entrySize <= 0) {
	    Font* _font = Font::get(font.c_str());
	    if (_font == nullptr) {
	        entrySize = 20;
        } else {
		    entrySize = _font->height();
		    entrySize += entrySize / 2;
	    }
	}
	const int index = selection;
	if (scroll_to_top || actualSize.y > index * entrySize) {
		actualSize.y = index * entrySize;
		actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
		scrollY = actualSize.y;
	}
	if (actualSize.y + size.h < (index + 1) * entrySize) {
		actualSize.y = (index + 1) * entrySize - size.h;
		actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
		scrollY = actualSize.y;
	}
	syncScroll();
}

void Frame::activateEntry(entry_t& entry) {
	activation = &entry;
	if (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) {
		if (entry.ctrlClick) {
			(*entry.ctrlClick)(entry);
		}
	} else {
		if (entry.click) {
			(*entry.click)(entry);
		}
	}
	if (dropDown) {
		toBeDeleted = true;
	}
}

void createTestUI() {
	Frame* window = gui->addFrame("window");
	window->setSize(SDL_Rect{(Frame::virtualScreenX - 500) / 2, (Frame::virtualScreenY - 400) / 2, 500, 400});
	window->setActualSize(SDL_Rect{0, 0, 1500, 1500});
	window->setColor(makeColor( 128, 128, 160, 255));

	{
		Button* bt = window->addButton("closeButton");
		bt->setBorder(3);
		bt->setSize(SDL_Rect{10, 10, 50, 50});
		bt->setText("x");
		bt->setTooltip("Close window");
		bt->setCallback([](Button& bt){
			Widget* w = bt.getParent();
			Frame* frame = static_cast<Frame*>(w);
			frame->removeSelf();
		});
	}

	int y = 500;

	{
		Button* bt = window->addButton("testButton1");
		bt->setBorder(3);
		bt->setSize(SDL_Rect{510, y, 240, 50});
		bt->setText("Normal button");
		bt->setTooltip("Only pressed when button is held");

		y += 60;
	}

	{
		Button* bt = window->addButton("testButton2");
		bt->setBorder(3);
		bt->setSize(SDL_Rect{510, y, 240, 50});
		bt->setText("Toggle button");
		bt->setTooltip("Toggles on/off state");
		bt->setStyle(Button::STYLE_TOGGLE);

		y += 60;
	}

	{
		Button* bt = window->addButton("testButton3");
		bt->setBorder(3);
		bt->setSize(SDL_Rect{510, y, 240, 50});
		//bt->setText("Checkmark");
		bt->setIcon("images/system/locksidebar.png");
		bt->setTooltip("Checkmark style button");
		bt->setStyle(Button::STYLE_CHECKBOX);

		y += 60;
	}

	{
		Frame* textBox = window->addFrame("testTextBox");
		textBox->setSize(SDL_Rect{510, y, 200, 40});
		textBox->setActualSize(SDL_Rect{0, 0, 200, 40});
		textBox->setColor(makeColor( 96, 96, 128, 255));

		Field* field = textBox->addField("testField", 32);
		field->setSize(SDL_Rect{0, 0, 200, 40});
		field->setText("Editable text");
		field->setEditable(true);

		y += 60;
	}

	{
		Slider* slider = window->addSlider("testSlider");
		slider->setRailSize(SDL_Rect{510, y, 200, 5});
		slider->setHandleSize(SDL_Rect{0, 0, 20, 30});
		slider->setTooltip("Test Slider");
		slider->setMinValue(0.f);
		slider->setMaxValue(10.f);
		slider->setValue(5.f);

		y += 50;
	}

	{
		Frame* frame = window->addFrame("testFrame");
		frame->setSize(SDL_Rect{510, y, 200, 200});
		frame->setActualSize(SDL_Rect{0, 0, 200, 200});
		frame->setColor(makeColor( 96, 96, 128, 255));
		{
			Frame::entry_t* entry = frame->addEntry("entry1", true);
			entry->text = "Entry #1";
			entry->tooltip = "The first entry in the frame";
		}
		{
			Frame::entry_t* entry = frame->addEntry("entry2", true);
			entry->text = "Entry #2";
			entry->tooltip = "Another entry in the frame";
		}

		y += 210;
	}

	{
		Frame::image_t* image = window->addImage(
			SDL_Rect{510, y, 200, 200}, 0xffffffff,
			"images/system/shopkeeper.png", "shopkeeper"
		);

		y += 210;
	}
}

// sample function - would need to cache the blitted images somewhere for real-time use.
void drawImageOutline(Image* actualImage, SDL_Rect src, SDL_Rect scaledDest, const SDL_Rect viewport, const Uint32 baseOutlineColor)
{
	if ( !actualImage )
	{
		return;
	}
	if ( !actualImage->getOutlineSurf() )
	{
		SDL_Surface* scaledImg = SDL_CreateRGBSurface(0, scaledDest.w, scaledDest.h, 32,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		SDL_Surface* srcSurf = const_cast<SDL_Surface*>(actualImage->getSurf());
		SDL_Rect destScaledRect{ 0, 0, scaledDest.w, scaledDest.h };
		SDL_SetSurfaceAlphaMod(srcSurf, 255);
		// blit a scaled version of the image to draw an outline around
		SDL_BlitScaled(srcSurf, nullptr, scaledImg, &destScaledRect);

		// outline has 1px border around original image.
		SDL_Surface* outlineSurface = SDL_CreateRGBSurface(0, scaledDest.w + 2, scaledDest.h + 2, 32,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

		Uint32 outlineColor = makeColor(255, 255, 255, 255);

		SDL_LockSurface(outlineSurface);

		std::map<int, std::pair<int, int>> visitedPixels; // if we blitted an outline onto this pixel
		for ( int loopx = 0; loopx < outlineSurface->w; loopx++ )
		{
			for ( int loopy = 0; loopy < outlineSurface->h; loopy++ )
			{
				// loopx/loopy is looking at the larger outline surface

				int x = loopx - 1; // x is looking at the original image surface, offset by the outline border 1px
				int y = loopy - 1; // y is looking at the original image surface, offset by the outline border 1px

				Uint32 color = outlineColor;
				Uint32 pixLeft = 0, pixRight = 0, pixUp = 0, pixDown = 0;
				Uint32 pixCurrent = 0;
				int neighbours = 0;

				if ( x >= 0 && x < scaledImg->w
					&& y >= 0 && y < scaledImg->h )
				{
					pixCurrent = getPixel(scaledImg, x, y);
				}

				if ( y >= 0 && y < scaledImg->h )
				{
					if ( x - 1 >= 0 && x - 1 < scaledImg->w )
					{
						pixLeft = getPixel(scaledImg, x - 1, y);
						if ( pixLeft != 0 )
						{
							Uint8 r, g, b, a;
							getColor(pixLeft, &r, &g, &b, &a);
							if ( a > 0 )
							{
								++neighbours;
							}
						}
					}
					if ( x + 1 < scaledImg->w )
					{
						pixRight = getPixel(scaledImg, x + 1, y);
						if ( pixRight != 0 )
						{
							Uint8 r, g, b, a;
							getColor(pixRight, &r, &g, &b, &a);
							if ( a > 0 )
							{
								++neighbours;
							}
						}
					}
				}

				if ( x >= 0 && x < scaledImg->w )
				{
					if ( y - 1 >= 0 && y - 1 < scaledImg->h )
					{
						pixUp = getPixel(scaledImg, x, y - 1);
						if ( pixUp != 0 )
						{
							Uint8 r, g, b, a;
							getColor(pixUp, &r, &g, &b, &a);
							if ( a > 0 )
							{
								++neighbours;
							}
						}
					}
					if ( y + 1 < scaledImg->h )
					{
						pixDown = getPixel(scaledImg, x, y + 1);
						if ( pixDown != 0 )
						{
							Uint8 r, g, b, a;
							getColor(pixDown, &r, &g, &b, &a);
							if ( a > 0 )
							{
								++neighbours;
							}
						}
					}
				}
				if ( neighbours > 0 && neighbours < 4 && pixCurrent == 0 )
				{
					// outline is drawn on current pixel if non-empty neighbouring pixel(s) found in source image.
					putPixel(outlineSurface, loopx, loopy, outlineColor);
					int key = loopx + loopy * outlineSurface->h;
					visitedPixels[key] = std::make_pair(loopx, loopy); // mark as visited for thickening the outline later.
				}
				//if ( loopx == 0 || loopy == 0 || loopx == outlineSurface->w - 1 || loopy == outlineSurface->h - 1 )
				//{
				//		draw a border around the outline surface
				//		putPixel(sprite, loopx, loopy, makeColor(0, 255, 255, 255));
				//}
			}
		}

		bool thickenBorder = true;
		Uint32 borderColor = makeColor(255, 255, 255, 192);
		for ( auto& pixel : visitedPixels )
		{
			if ( !thickenBorder ) { break; }

			// now trace the new outline pixels, and add 1px to neighbouring empty pixels
			int x = pixel.second.first;
			int y = pixel.second.second;
			Uint32 color = borderColor;
			Uint32 pixLeft = 0, pixRight = 0, pixUp = 0, pixDown = 0;
			Uint32 pixCurrent = 0;
			int neighbours = 0;

			if ( x >= 0 && x < outlineSurface->w
				&& y >= 0 && y < outlineSurface->h )
			{
				pixCurrent = getPixel(outlineSurface, x, y);
			}

			if ( y >= 0 && y < outlineSurface->h )
			{
				if ( x - 1 >= 0 && x - 1 < outlineSurface->w )
				{
					int key = (x - 1) + y * outlineSurface->h;
					bool visited = visitedPixels.find(key) != visitedPixels.end();
					if ( !visited )
					{
						pixLeft = getPixel(outlineSurface, x - 1, y);
						if ( pixLeft == 0 )
						{
							putPixel(outlineSurface, x - 1, y, color);
						}
					}
				}
				if ( x + 1 < outlineSurface->w )
				{
					int key = (x + 1) + y * outlineSurface->h;
					bool visited = visitedPixels.find(key) != visitedPixels.end();
					if ( !visited )
					{
						pixRight = getPixel(outlineSurface, x + 1, y);
						if ( pixRight == 0 )
						{
							putPixel(outlineSurface, x + 1, y, color);
						}
					}
				}
			}

			if ( x >= 0 && x < outlineSurface->w )
			{
				if ( y - 1 >= 0 && y - 1 < outlineSurface->h )
				{
					int key = x + (y - 1) * outlineSurface->h;
					bool visited = visitedPixels.find(key) != visitedPixels.end();
					if ( !visited )
					{
						pixUp = getPixel(outlineSurface, x, y - 1);
						if ( pixUp == 0 )
						{
							putPixel(outlineSurface, x, y - 1, color);
						}
					}
				}
				if ( y + 1 < outlineSurface->h )
				{
					int key = x + (y + 1) * outlineSurface->h;
					bool visited = visitedPixels.find(key) != visitedPixels.end();
					if ( !visited )
					{
						pixDown = getPixel(outlineSurface, x, y + 1);
						if ( pixDown == 0 )
						{
							putPixel(outlineSurface, x, y + 1, color);
						}
					}
				}
			}
		}
		SDL_UnlockSurface(outlineSurface);
		actualImage->setOutlineSurf(outlineSurface);
		if ( scaledImg )
		{
			SDL_FreeSurface(scaledImg);
			scaledImg = nullptr;
		}
	}
	TempTexture* outlineTexture = new TempTexture();
	outlineTexture->load(const_cast<SDL_Surface*>(actualImage->getOutlineSurf()), true, true);
	outlineTexture->bind();

	SDL_Rect newDest = scaledDest;
	newDest.x -= 1; // offset by 1px due to 2px border addition
	newDest.y -= 1; // offset by 1px due to 2px border addition
	newDest.w = actualImage->getOutlineSurf()->w;
	newDest.h = actualImage->getOutlineSurf()->h;
	SDL_Rect newSrc = src;
	newSrc.w = actualImage->getOutlineSurf()->w;
	newSrc.h = actualImage->getOutlineSurf()->h;

	Image::drawSurface(outlineTexture->texid, const_cast<SDL_Surface*>(actualImage->getOutlineSurf()), &newSrc, newDest, viewport, baseOutlineColor);

	if ( outlineTexture ) {
		delete outlineTexture;
		outlineTexture = nullptr;
	}
}

const Uint32 imageGlowInterval = TICKS_PER_SECOND;

void Frame::drawImage(const image_t* image, const SDL_Rect& _size, const SDL_Rect& scroll) const {
	assert(image);
	const Image* actualImage = Image::get(image->path.c_str());
	if (actualImage) {
		SDL_Rect pos;
		pos.x = _size.x + image->pos.x - scroll.x;
		pos.y = _size.y + image->pos.y - scroll.y;
		pos.w = image->pos.w > 0 ? image->pos.w : actualImage->getWidth();
		pos.h = image->pos.h > 0 ? image->pos.h : actualImage->getHeight();

		SDL_Rect dest;
		dest.x = std::max(_size.x, pos.x);
		dest.y = std::max(_size.y, pos.y);
		dest.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
		dest.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));
		SDL_Rect scaledDest;
		scaledDest.x = dest.x;
		scaledDest.y = dest.y;
		scaledDest.w = dest.w;
		scaledDest.h = dest.h;
		if (scaledDest.w <= 0 || scaledDest.h <= 0) {
			return;
		}

		SDL_Rect src;
		if (image->tiled) {
			src.x = std::max(0, _size.x - pos.x);
			src.y = std::max(0, _size.y - pos.y);
			src.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
			src.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));
		} else {
			const int w = image->section.w ? image->section.w : (actualImage->getWidth() - image->section.x);
			const int h = image->section.h ? image->section.h : (actualImage->getHeight() - image->section.y);
			src.x = std::max((float)image->section.x, image->section.x + (_size.x - pos.x) * (w / (float)image->pos.w));
			src.y = std::max((float)image->section.y, image->section.y + (_size.y - pos.y) * (h / (float)image->pos.h));
			src.w = ((float)dest.w / pos.w) * w;
			src.h = ((float)dest.h / pos.h) * h;
			//src.x += image->section.x - std::min(0, _size.x - pos.x);
			//src.y += image->section.y - std::min(0, _size.y - pos.y);
		}


		if ( getOpacity() < 100.0 )
		{
			Uint8 r, g, b, a;
			getColor(image->color, &r, &g, &b, &a);
			a *= getOpacity() / 100.0;
			if ( a > 0 )
			{
				if ( image->outline )
				{
					real_t outlineGlowEffect = 0.0;
					Uint32 halfInterval = imageGlowInterval / 2;
					if ( ::ticks % imageGlowInterval > halfInterval )
					{
						outlineGlowEffect = (halfInterval - ((::ticks % imageGlowInterval) - halfInterval)) / static_cast<real_t>(imageGlowInterval);
					}
					else
					{
						outlineGlowEffect = ::ticks % imageGlowInterval / static_cast<real_t>(imageGlowInterval);
					}
					outlineGlowEffect = (outlineGlowEffect * .5) + .5;
					Uint8 r2, g2, b2, a2;
					getColor(image->outlineColor, &r2, &g2, &b2, &a2);
					Uint32 alpha = static_cast<Uint8>(255.0 * ((static_cast<real_t>(a) / 255.0) * static_cast<real_t>(a2 / 255.0) * outlineGlowEffect));
					if ( alpha > 0 )
					{
						drawImageOutline(const_cast<Image*>(actualImage), src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY },
							makeColor( r2, g2, b2, alpha));
					}
				}
				actualImage->drawColor(&src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY },
					makeColor( r, g, b, a));
			}
		}
		else
		{
			if ( image->outline )
			{
				real_t outlineGlowEffect = 0.0;
				Uint32 halfInterval = imageGlowInterval / 2;
				if ( ::ticks % imageGlowInterval > halfInterval )
				{
					outlineGlowEffect = (halfInterval - ((::ticks % imageGlowInterval) - halfInterval)) / static_cast<real_t>(imageGlowInterval);
				}
				else
				{
					outlineGlowEffect = ::ticks % imageGlowInterval / static_cast<real_t>(imageGlowInterval);
				}
				outlineGlowEffect = (outlineGlowEffect * .5) + .5;
				Uint8 r2, g2, b2, a2;
				getColor(image->outlineColor, &r2, &g2, &b2, &a2);
				Uint32 alpha = static_cast<Uint8>(static_cast<real_t>(a2) * outlineGlowEffect);
				if ( alpha > 0 )
				{
					drawImageOutline(const_cast<Image*>(actualImage), src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY },
						makeColor( r2, g2, b2, alpha));
				}
			}
			actualImage->drawColor(&src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY }, image->color);
		}
	}
}

void Frame::addSyncScrollTarget(const char* name) {
    syncScrollTargets.push_back(std::move(std::string(name)));
}

void Frame::syncScroll() {
    assert(gui);
    for (auto name : syncScrollTargets) {
        auto frame = gui->findFrame(name.c_str());
        if (frame) {
            auto _size = frame->getActualSize();
            _size.x = actualSize.x;
            _size.y = actualSize.y;
            frame->setActualSize(_size);
        }
    }
}

void Frame::bringToTop() {
    if (!parent) {
        return;
    }
    auto& frames = static_cast<Frame*>(parent)->frames;
    for (auto it = frames.begin(); it != frames.end(); ++it) {
        if (*it == this) {
            frames.erase(it);
            frames.push_back(this);
        }
    }
}