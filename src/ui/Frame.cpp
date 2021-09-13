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

const Sint32 Frame::sliderSize = 15;

static const Uint32 tooltip_background = 0xEE000000;
static const Uint32 tooltip_border_color = 0xFFEE00AA;
static const int tooltip_border_width = 2;
static const Uint32 tooltip_text_color = 0xFFFFFFFF;
static const char* tooltip_text_font = Font::defaultFont;

static unsigned int gui_fbo = 0;
static unsigned int gui_fbo_color = 0;
static unsigned int gui_fbo_depth = 0;

// root of all widgets
Frame* gui = nullptr;

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
		entryCast->color = SDL_MapRGBA(mainsurface->format, 255, 0, 0, 255);
	} else if (highlighted) {
		entryCast->color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
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

void Frame::guiInit() {
	gui = new Frame("root");
	SDL_Rect guiRect;
	guiRect.x = 0;
	guiRect.y = 0;
	guiRect.w = Frame::virtualScreenX;
	guiRect.h = Frame::virtualScreenY;
	gui->setSize(guiRect);
	gui->setActualSize(guiRect);
	gui->setHollow(true);

	SDL_glGenFramebuffers(1, &gui_fbo);
	SDL_glBindFramebuffer(GL_FRAMEBUFFER, gui_fbo);

	glGenTextures(1, &gui_fbo_color);
	glBindTexture(GL_TEXTURE_2D, gui_fbo_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Frame::virtualScreenX, Frame::virtualScreenY, 0, GL_RGBA, GL_FLOAT, nullptr);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gui_fbo_color, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &gui_fbo_depth);
	glBindTexture(GL_TEXTURE_2D, gui_fbo_depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, Frame::virtualScreenX, Frame::virtualScreenY, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gui_fbo_depth, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	static const GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
	SDL_glDrawBuffers(sizeof(attachments) / sizeof(GLenum), attachments);
	glReadBuffer(GL_NONE);

	SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Frame::guiDestroy() {
	if (gui) {
		delete gui;
		gui = nullptr;
	}
	if (gui_fbo) {
		SDL_glDeleteFramebuffers(1, &gui_fbo);
		gui_fbo = 0;
	}
	if (gui_fbo_color) {
		glDeleteTextures(1, &gui_fbo_color);
		gui_fbo_color = 0;
	}
	if (gui_fbo_depth) {
		glDeleteTextures(1, &gui_fbo_depth);
		gui_fbo_depth = 0;
	}
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

void Frame::draw() {
	SDL_glBindFramebuffer(GL_FRAMEBUFFER, gui_fbo);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gui_fbo_color, 0);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gui_fbo_depth, 0);
	glViewport(0, 0, Frame::virtualScreenX, Frame::virtualScreenY);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	SDL_glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	auto _actualSize = allowScrolling ? actualSize : SDL_Rect{0, 0, size.w, size.h};
	std::vector<Widget*> selectedWidgets;
	findSelectedWidgets(selectedWidgets);
	Frame::draw(size, _actualSize, selectedWidgets);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, xres, yres);

	glBindTexture(GL_TEXTURE_2D, gui_fbo_color);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
	glTexCoord2f(1.f, 0.f); glVertex2f( 1.f, -1.f);
	glTexCoord2f(1.f, 1.f); glVertex2f( 1.f,  1.f);
	glTexCoord2f(0.f, 1.f); glVertex2f(-1.f,  1.f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

void Frame::draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<Widget*>& selectedWidgets) {
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

	int entrySize = 20;
	Font* _font = Font::get(font.c_str());
	if (_font != nullptr) {
		entrySize = _font->height();
		entrySize += entrySize / 2;
	}

	SDL_Rect scaledSize;
	scaledSize.x = _size.x;
	scaledSize.y = _size.y;
	scaledSize.w = _size.w;
	scaledSize.h = _size.h;

	auto white = Image::get("images/system/white.png");
	Uint8 r = color >> mainsurface->format->Rshift; r = (r / 3) * 2;
	Uint8 g = color >> mainsurface->format->Gshift; g = (g / 3) * 2;
	Uint8 b = color >> mainsurface->format->Bshift; b = (b / 3) * 2;
	Uint8 a = color >> mainsurface->format->Ashift;
	Uint32 darkColor =
		(Uint32)r << mainsurface->format->Rshift |
		(Uint32)g << mainsurface->format->Gshift |
		(Uint32)b << mainsurface->format->Bshift |
		(Uint32)a << mainsurface->format->Ashift;

	// draw frame background
	if (!hollow) {
		SDL_Rect inner;
		inner.x = (_size.x + border);
		inner.y = (_size.y + border);
		inner.w = (_size.w - border*2);
		inner.h = (_size.h - border*2);
		if (borderStyle == BORDER_BEVEL_HIGH) {
			white->drawColor(nullptr, scaledSize, viewport, darkColor);
			white->drawColor(nullptr, inner, viewport, color);
		} else if (borderStyle == BORDER_BEVEL_LOW) {
			white->drawColor(nullptr, scaledSize, viewport, color);
			white->drawColor(nullptr, inner, viewport, darkColor);
		} else {
			white->drawColor(nullptr, scaledSize, viewport, color);
		}
	}

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

	// horizontal slider
	if (actualSize.w > size.w && scrollbars) {

		// slider rail
		SDL_Rect barRect;
		barRect.x = scaledSize.x;
		barRect.y = scaledSize.y + scaledSize.h;
		barRect.w = scaledSize.w;
		barRect.h = sliderSize * (float)yres / (float)Frame::virtualScreenY;
		white->drawColor(nullptr, barRect, viewport, darkColor);

		// handle
		float winFactor = ((float)_size.w / (float)actualSize.w);
		int handleSize = std::max((int)(size.w * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.x;

		SDL_Rect handleRect;
		handleRect.x = scaledSize.x + sliderPos;
		handleRect.y = scaledSize.y + scaledSize.h;
		handleRect.w = handleSize;
		handleRect.h = sliderSize;

		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			white->drawColor(nullptr, handleRect, viewport, color);
		} else {
			white->drawColor(nullptr, handleRect, viewport, color);
		}
	}

	// vertical slider
	if (actualSize.h > size.h && _size.y && scrollbars) {
		SDL_Rect barRect;
		barRect.x = scaledSize.x + scaledSize.w;
		barRect.y = scaledSize.y;
		barRect.w = sliderSize;
		barRect.h = scaledSize.h;
		white->drawColor(nullptr, barRect, viewport, darkColor);

		// handle
		float winFactor = ((float)_size.h / (float)actualSize.h);
		int handleSize = std::max((int)(size.h * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.y;

		SDL_Rect handleRect;
		handleRect.x = scaledSize.x + scaledSize.w;
		handleRect.y = scaledSize.y + sliderPos;
		handleRect.w = sliderSize;
		handleRect.h = handleSize;

		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			white->drawColor(nullptr, handleRect, viewport, color);
		} else {
			white->drawColor(nullptr, handleRect, viewport, color);
		}
	}

	// slider filler (at the corner between sliders)
	if (actualSize.w > size.w && actualSize.h > size.h && scrollbars) {
		SDL_Rect barRect;
		barRect.x = scaledSize.x + scaledSize.w;
		barRect.y = scaledSize.y + scaledSize.h;
		barRect.w = sliderSize;
		barRect.h = sliderSize;
		white->drawColor(nullptr, barRect, viewport, color);
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

	// render list entries
	if (list.size()) {
		int listStart = std::min(std::max(0, scroll.y / entrySize), (int)list.size() - 1);
		int i = listStart;
		for (int i = listStart; i < list.size(); ++i) {
			entry_t& entry = *list[i];
			if (entry.text.empty()) {
				continue;
			}

			// get rendered text
			Text* text = Text::get(entry.text.c_str(), font.c_str());
			if (text == nullptr) {
				continue;
			}

			// get the size of the rendered text
			int textSizeW = text->getWidth();
			int textSizeH = text->getHeight();

			SDL_Rect pos;
			pos.x = _size.x + border + listOffset.x - scroll.x;
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
			} else if (selection >= 0 && selection == i) {
				white->drawColor(nullptr, entryback, viewport, color);
			}

			SDL_Rect scaledDest;
			scaledDest.x = dest.x;
			scaledDest.y = dest.y;
			scaledDest.w = dest.w;
			scaledDest.h = dest.h;
			if (scaledDest.h <= 0 || scaledDest.w <= 0) {
				continue;
			}
			text->drawColor(src, scaledDest, viewport, entry.color);
		}
	}

	// draw fields
	for (auto field : fields) {
		if ( field->isOntop() )
		{
			continue;
		}
		field->draw(_size, scroll, selectedWidgets);
	}

	// draw buttons
	for (auto button : buttons) {
		button->draw(_size, scroll, selectedWidgets);
	}

	// draw sliders
	for (auto slider : sliders) {
		slider->draw(_size, scroll, selectedWidgets);
	}

	// draw subframes
	for ( auto frame : frames ) {
		frame->draw(_size, scroll, selectedWidgets);
	}

	// draw "on top" fields
	for ( auto field : fields ) {
		if ( !field->isOntop() )
		{
			continue;
		}
		field->draw(_size, scroll, selectedWidgets);
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

	// draw glyphs
	drawGlyphs(_size, selectedWidgets);

	// root frame draws tooltip
	// TODO on Nintendo, display this next to the currently selected widget
	if (!parent) {
		if (tooltip && tooltip[0] != '\0') {
			Font* font = Font::get(tooltip_text_font);
			if (font) {
				int border = tooltip_border_width;

				Text* text = Text::get(tooltip, font->getName());
				SDL_Rect src;
				src.x = mousex + 20;
				src.y = mousey;
				src.w = text->getWidth() + border * 2;
				src.h = text->getHeight() + border * 2;

				SDL_Rect _src = src;
				_src.x = _src.x;
				_src.y = _src.y;
				_src.w = _src.w;
				_src.h = _src.h;
				white->drawColor(nullptr, _src, viewport, tooltip_border_color);

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
	result.removed = false;
	result.usable = usable;
	result.highlightTime = SDL_GetTicks();
	result.tooltip = nullptr;

	if ( parent && inheritParentFrameOpacity )
	{
		setOpacity(static_cast<Frame*>(parent)->getOpacity());
	}

	if (disabled) {
		return result;
	}

#ifndef EDITOR // bad editor no cookie
	if ( players[getOwner()]->shootmode ) {
		return result;
	}
#endif

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

	int entrySize = 20;
	Font* _font = Font::get(font.c_str());
	if (_font != nullptr) {
		entrySize = _font->height();
		entrySize += entrySize / 2;
	}

	SDL_Rect fullSize = _size;
	fullSize.h += (actualSize.w > size.w) ? sliderSize : 0;
	fullSize.w += (actualSize.h > size.h) ? sliderSize : 0;

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
			if (dropDown) {
				toBeDeleted = true;
			}
		}

		// choose a selection
		if (list.size()) {
			if (selection == -1) {
				if (input.consumeBinaryToggle("MenuUp") || 
					input.consumeBinaryToggle("MenuDown")) {
					selection = 0;
					scrollToSelection();
				}
			} else {
				if (input.consumeBinaryToggle("MenuUp")) {
					selection = std::max(0, selection - 1);
					scrollToSelection();
				}
				if (input.consumeBinaryToggle("MenuDown")) {
					selection = std::min((int)list.size() - 1, selection + 1);
					scrollToSelection();
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

	// scroll with right stick
	if (allowScrolling && allowScrollBinds) {
		Input& input = Input::inputs[owner];

		// x scroll
		if (this->actualSize.w > size.w) {
			if (input.binary("MenuScrollRight")) {
				this->actualSize.x += std::min(this->actualSize.x + 5, this->actualSize.w - _size.w);
			}
			else if (input.binary("MenuScrollLeft")) {
				this->actualSize.x -= std::max(this->actualSize.x - 5, 0);
			}
		}

		// y scroll
		if (this->actualSize.h > size.h) {
			if (input.binary("MenuScrollDown")) {
				this->actualSize.y = std::min(this->actualSize.y + 5, this->actualSize.h - _size.h);
			}
			else if (input.binary("MenuScrollUp")) {
				this->actualSize.y = std::max(this->actualSize.y - 5, 0);
			}
		}
	}

	// scroll with mouse wheel
	if (parent != nullptr && !hollow && rectContainsPoint(fullSize, omousex, omousey) && usable && allowScrolling && allowScrollBinds) {
		// x scroll with mouse wheel
		if (this->actualSize.w > size.w) {
			if (this->actualSize.h <= size.h) {
				if (mousestatus[SDL_BUTTON_WHEELDOWN]) {
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					this->actualSize.x += std::min(entrySize * 4, size.w);
					usable = result.usable = false;
				} else if (mousestatus[SDL_BUTTON_WHEELUP]) {
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					this->actualSize.x -= std::min(entrySize * 4, size.w);
					usable = result.usable = false;
				}
			}
		}

		// y scroll with mouse wheel
		if (this->actualSize.h > size.h) {
			if (mousestatus[SDL_BUTTON_WHEELDOWN]) {
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				this->actualSize.y += std::min(entrySize * 4, size.h);
				usable = result.usable = false;
			} else if (mousestatus[SDL_BUTTON_WHEELUP]) {
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				this->actualSize.y -= std::min(entrySize * 4, size.h);
				usable = result.usable = false;
			}
		}

		// bound
		this->actualSize.x = std::min(std::max(0, this->actualSize.x), std::max(0, this->actualSize.w - size.w));
		this->actualSize.y = std::min(std::max(0, this->actualSize.y), std::max(0, this->actualSize.h - size.h));
	}

	// process frames
	{
		for (int i = frames.size() - 1; i >= 0; --i) {
			Frame* frame = frames[i];
			result_t frameResult = frame->process(_size, actualSize, selectedWidgets, usable);
			usable = result.usable = frameResult.usable;
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

	// process (frame view) sliders
	if (parent != nullptr && !hollow && usable && scrollbars) {
		// filler in between sliders
		if (actualSize.w > size.w && actualSize.h > size.h) {
			SDL_Rect sliderRect;
			sliderRect.x = _size.x + _size.w; sliderRect.w = sliderSize;
			sliderRect.y = _size.y + _size.h; sliderRect.h = sliderSize;
			if (rectContainsPoint(sliderRect, omousex, omousey)) {
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
			int handleSize = std::max((int)(size.w * winFactor), sliderSize);
			int sliderPos = winFactor * actualSize.x;
			SDL_Rect handleRect;
			handleRect.x = _size.x + sliderPos;
			handleRect.y = _size.y + _size.h;
			handleRect.w = handleSize;
			handleRect.h = sliderSize;

			// click & drag
			if (draggingHSlider) {
				if (!mousestatus[SDL_BUTTON_LEFT]) {
					draggingHSlider = false;
				} else {
					float winFactor = ((float)_size.w / (float)this->actualSize.w);
					this->actualSize.x = (mousex - omousex) / winFactor + oldSliderX;
					this->actualSize.x = std::min(std::max(0, this->actualSize.x), std::max(0, this->actualSize.w - size.w));
				}
				usable = result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if (rectContainsPoint(handleRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						draggingHSlider = true;
						oldSliderX = this->actualSize.x;
					}
					usable = result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				} else if (rectContainsPoint(sliderRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						this->actualSize.x += omousex < handleRect.x ? -std::min(entrySize * 4, size.w) : std::min(entrySize * 4, size.w);
						this->actualSize.x = std::min(std::max(0, this->actualSize.x), std::max(0, this->actualSize.w - size.w));
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					usable = result.usable = false;
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
			int handleSize = std::max((int)(size.h * winFactor), sliderSize);
			int sliderPos = winFactor * actualSize.y;
			SDL_Rect handleRect;
			handleRect.x = _size.x + _size.w;
			handleRect.y = _size.y + sliderPos;
			handleRect.w = sliderSize;
			handleRect.h = handleSize;

			// click & drag
			if (draggingVSlider) {
				if (!mousestatus[SDL_BUTTON_LEFT]) {
					draggingVSlider = false;
				} else {
					float winFactor = ((float)_size.h / (float)this->actualSize.h);
					this->actualSize.y = (mousey - omousey) / winFactor + oldSliderY;
					this->actualSize.y = std::min(std::max(0, this->actualSize.y), std::max(0, this->actualSize.h - size.h));
				}
				usable = result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if (rectContainsPoint(handleRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						draggingVSlider = true;
						oldSliderY = this->actualSize.y;
					}
					usable = result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				} else if (rectContainsPoint(sliderRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						this->actualSize.y += omousey < handleRect.y ? -std::min(entrySize * 4, size.h) : std::min(entrySize * 4, size.h);
						this->actualSize.y = std::min(std::max(0, this->actualSize.y), std::max(0, this->actualSize.h - size.h));
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					usable = result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				}
			}
		}
	}

	// process buttons
	{
		for (int i = buttons.size() - 1; i >= 0; --i) {
			Button* button = buttons[i];
			if (!destWidget) {
				destWidget = button->handleInput();
			}

			Button::result_t buttonResult = button->process(_size, actualSize, usable);
			if (usable && buttonResult.highlighted) {
				result.highlightTime = buttonResult.highlightTime;
				result.tooltip = buttonResult.tooltip;
				if (buttonResult.clicked) {
					button->activate();
				}
				result.usable = usable = false;
			}

			if (destWidget && button->isSelected()) {
				button->deselect();
			}
		}
	}

	// process (widget) sliders
	{
		for (int i = sliders.size() - 1; i >= 0; --i) {
			Slider* slider = sliders[i];

			if (!destWidget && !slider->isActivated()) {
				destWidget = slider->handleInput();
			} else {
				slider->control();
			}

			Slider::result_t sliderResult = slider->process(_size, actualSize, usable);
			if (usable && sliderResult.highlighted) {
				result.highlightTime = sliderResult.highlightTime;
				result.tooltip = sliderResult.tooltip;
				if (sliderResult.clicked) {
					slider->fireCallback();
				}
				result.usable = usable = false;
			}

			if (destWidget && slider->isSelected()) {
				slider->deselect();
			}
		}
	}

	// process the frame's list entries
	if (usable && list.size() > 0) {
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
			entryRect.x = _size.x + border - actualSize.x; entryRect.w = _size.w - border * 2;
			entryRect.y = _size.y + border + i * entrySize - actualSize.y; entryRect.h = entrySize;

			if (rectContainsPoint(_size, omousex, omousey) && rectContainsPoint(entryRect, omousex, omousey)) {
				result.highlightTime = entry->highlightTime;
				result.tooltip = entry->tooltip.c_str();
				if (mousestatus[SDL_BUTTON_LEFT]) {
					if (!entry->pressed) {
						entry->pressed = true;
						activateEntry(*entry);
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
				result.usable = usable = false;
			} else {
				entry->highlightTime = SDL_GetTicks();
				entry->highlighted = false;
				entry->pressed = false;
			}
		}
	}

	// process fields
	{
		for (int i = fields.size() - 1; i >= 0; --i) {
			Field* field = fields[i];

			// widget capture input
			if (!destWidget) {
				destWidget = field->handleInput();
			}

			Field::result_t fieldResult = field->process(_size, actualSize, usable);
			if (usable) {
				if (field->isSelected() && fieldResult.highlighted) {
					result.usable = usable = false;
				}
			}

			if (fieldResult.entered || (destWidget && field->isSelected())) {
				if (field->getCallback()) {
					(*field->getCallback())(*field);
				} else {
					printlog("modified field with no callback");
				}
			}

			if (destWidget && field->isSelected()) {
				field->deselect();
			}
		}
	}

	if (rectContainsPoint(_size, omousex, omousey) && !hollow) {
		result.usable = usable = false;
	}

	if (toBeDeleted) {
		result.removed = true;
	} else {
		++ticks;
		if (destWidget) {
			destWidget->select();
		}
	}

	return result;
}

void Frame::postprocess() {
	Widget::process();

	// TODO: which player owns the mouse
	if (dropDown && owner == 0) {
		if (!dropDownClicked) {
			for (int c = 0; c < sizeof(mousestatus) / sizeof(mousestatus[0]); ++c) {
				if (mousestatus[c]) {
					dropDownClicked |= 1 << c;
				}
			}
		} else {
			for (int c = 0; c < sizeof(mousestatus) / sizeof(mousestatus[0]); ++c) {
				if (!mousestatus[c]) {
					dropDownClicked &= ~(1 << c);
				}
			}
			if (!dropDownClicked && ticks > 0) {
				toBeDeleted = true;
			}
		}
	}
	for (auto frame : frames) {
		frame->postprocess();
	}
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
	entry_t* entry = new entry_t();
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

Frame* Frame::findFrame(const char* name) {
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
	int entrySize = 20;
	Font* _font = Font::get(font.c_str());
	if (_font != nullptr) {
		entrySize = _font->height();
		entrySize += entrySize / 2;
	}
	actualSize.w = size.w;
	actualSize.h = (Uint32)list.size() * entrySize;
	actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
}

bool Frame::capturesMouse(SDL_Rect* curSize, SDL_Rect* curActualSize) {
#ifdef NINTENDO
	return false;
#else
	SDL_Rect newSize = SDL_Rect{0, 0, xres, yres};
	SDL_Rect newActualSize = SDL_Rect{0, 0, xres, yres};
	SDL_Rect& _size = curSize ? *curSize : newSize;
	SDL_Rect& _actualSize = curActualSize ? *curActualSize : newActualSize;

	if (parent) {
		auto pframe = static_cast<Frame*>(parent);
		if (pframe->capturesMouse(&_size, &_actualSize)) {
			_size.x += std::max(0, size.x - _actualSize.x);
			_size.y += std::max(0, size.y - _actualSize.y);
			if (size.h < actualSize.h && allowScrolling) {
				_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			} else {
				_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			if (size.w < actualSize.w && allowScrolling) {
				_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			} else {
				_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			if (_size.w <= 0 || _size.h <= 0) {
				return false;
			} else {
#ifdef EDITOR
				Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
#else
				Sint32 omousex = (inputs.getMouse(owner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 omousey = (inputs.getMouse(owner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
#endif
				if (rectContainsPoint(_size, omousex, omousey)) {
					return true;
				} else {
					return false;
				}
			}
		} else {
			return false;
		}
	} else {
		return true;
	}
#endif
}

void Frame::warpMouseToFrame(const int player, Uint32 flags) const
{
	SDL_Rect _size = getAbsoluteSize();
	inputs.warpMouse(player,
		(_size.x + _size.w / 2) * ((float)xres / (float)Frame::virtualScreenX),
		(_size.y + _size.h / 2) * ((float)yres / (float)Frame::virtualScreenY),
		flags);
}

SDL_Rect Frame::getAbsoluteSize() const
{
	SDL_Rect _size{ size.x, size.y, size.w, size.h };
	auto _parent = this->parent;
	while ( _parent ) {
		auto pframe = static_cast<Frame*>(_parent);
		_size.x += std::max(0, pframe->size.x);
		_size.y += std::max(0, pframe->size.y);
		_parent = pframe->parent;
	}
	return _size;
}

bool Frame::capturesMouseInRealtimeCoords(SDL_Rect* curSize, SDL_Rect* curActualSize) {
	SDL_Rect newSize = SDL_Rect{ 0, 0, xres, yres };
	SDL_Rect newActualSize = SDL_Rect{ 0, 0, xres, yres };
	SDL_Rect& _size = curSize ? *curSize : newSize;
	SDL_Rect& _actualSize = curActualSize ? *curActualSize : newActualSize;

	if ( parent ) {
		auto pframe = static_cast<Frame*>(parent);
		if ( pframe->capturesMouseInRealtimeCoords(&_size, &_actualSize) ) {
			_size.x += std::max(0, size.x - _actualSize.x);
			_size.y += std::max(0, size.y - _actualSize.y);
			if ( size.h < actualSize.h && allowScrolling ) {
				_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			else {
				_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			if ( size.w < actualSize.w && allowScrolling ) {
				_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			else {
				_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			if ( _size.w <= 0 || _size.h <= 0 ) {
				return false;
			}
			else {
#ifdef EDITOR
				Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
#else
				Sint32 mousex = (inputs.getMouse(owner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 mousey = (inputs.getMouse(owner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
#endif
				if ( rectContainsPoint(_size, mousex, mousey) ) {
					return true;
				}
				else {
					return false;
				}
			}
		}
		else {
			return false;
		}
	}
	else {
		return true;
	}
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

void Frame::scrollToSelection() {
	if (selection == -1) {
		return;
	}
	int index = 0;
	for (auto entry : list) {
		if (entry == list[selection]) {
			break;
		}
		++index;
	}
	int entrySize = 20;
	Font* _font = Font::get(font.c_str());
	if (_font != nullptr) {
		entrySize = _font->height();
		entrySize += entrySize / 2;
	}
	if (actualSize.y > index * entrySize) {
		actualSize.y = index * entrySize;
	}
	if (actualSize.y + size.h < (index + 1) * entrySize) {
		actualSize.y = (index + 1) * entrySize - size.h;
	}
}

void Frame::activateEntry(entry_t& entry) {
	if (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) {
		if (entry.ctrlClick) {
			(*entry.ctrlClick)(entry);
		}
	} else {
		if (entry.click) {
			(*entry.click)(entry);
		}
	}
}

void createTestUI() {
	Frame* window = gui->addFrame("window");
	window->setSize(SDL_Rect{(Frame::virtualScreenX - 500) / 2, (Frame::virtualScreenY - 400) / 2, 500, 400});
	window->setActualSize(SDL_Rect{0, 0, 1500, 1500});
	window->setColor(SDL_MapRGBA(mainsurface->format, 128, 128, 160, 255));

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
		textBox->setColor(SDL_MapRGBA(mainsurface->format, 96, 96, 128, 255));

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
		frame->setColor(SDL_MapRGBA(mainsurface->format, 96, 96, 128, 255));
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

void Frame::drawImage(image_t* image, const SDL_Rect& _size, const SDL_Rect& scroll) {
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
			src.x = std::max(0.f, (_size.x - pos.x) * ((float)actualImage->getWidth() / image->pos.w));
			src.y = std::max(0.f, (_size.y - pos.y) * ((float)actualImage->getHeight() / image->pos.h));
			src.w = ((float)dest.w / pos.w) * actualImage->getWidth();
			src.h = ((float)dest.h / pos.h) * actualImage->getHeight();
		}
		if (src.w <= 0 || src.h <= 0) {
			return;
		}

		if ( getOpacity() < 100.0 )
		{
			Uint8 r, g, b, a;
			SDL_GetRGBA(image->color, mainsurface->format, &r, &g, &b, &a);
			a *= getOpacity() / 100.0;
			actualImage->drawColor(&src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY },
				SDL_MapRGBA(mainsurface->format, r, g, b, a));
		}
		else
		{
			actualImage->drawColor(&src, scaledDest, SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY }, image->color);
		}
	}
}