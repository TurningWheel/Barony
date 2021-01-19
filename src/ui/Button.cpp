// Button.cpp

#include "../main.hpp"
#include "../player.hpp"
#include "../draw.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"
#include "Text.hpp"

Button::Button() {
	size.x = 0; size.w = 32;
	size.y = 0; size.h = 32;
	color = SDL_MapRGBA(mainsurface->format, 127, 127, 127, 255);
	textColor = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255);
}

Button::Button(Frame& _parent) : Button() {
	parent = &_parent;
	_parent.getButtons().push_back(this);
	_parent.adoptWidget(*this);
}

Button::~Button() {
	if (callback) {
		delete callback;
		callback = nullptr;
	}
}

void Button::setIcon(const char* _icon) {
	icon = _icon;
}

void Button::activate() {
	if (style == STYLE_NORMAL) {
		setPressed(true);
	} else {
		setPressed(isPressed()==false);
	}
	Widget::Args args(params);
	if (callback) {
		(*callback)(args);
	} else {
		printlog("button clicked with no callback");
	}
}

void Button::draw(SDL_Rect _size, SDL_Rect _actualSize) {
	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		return;
	}

	{
		int x = (_size.x) * (float)xres / (float)Frame::virtualScreenX;
		int y = (_size.y) * (float)yres / (float)Frame::virtualScreenY;
		int w = (_size.x + _size.w) * (float)xres / (float)Frame::virtualScreenX;
		int h = (_size.y + _size.h) * (float)yres / (float)Frame::virtualScreenY;
		if (pressed) {
			drawDepressed(x, y, w, h);
		} else {
			drawWindow(x, y, w, h);
		}
	}

	SDL_Rect scroll{0, 0, 0, 0};
	if (size.x - _actualSize.x < 0) {
		scroll.x -= size.x - _actualSize.x;
	}
	if (size.y - _actualSize.y < 0) {
		scroll.y -= size.y - _actualSize.y;
	}

	if (style != STYLE_CHECKBOX || pressed) {
		if (!text.empty()) {
			Text* _text = Text::get(text.c_str(), font.c_str());
			if (_text) {
				int w = _text->getWidth();
				int h = _text->getHeight();
				int x = (style != STYLE_DROPDOWN) ?
					(size.w - w) / 2 :
					5 + border;
				int y = (size.h - h) / 2;

				SDL_Rect pos = _size;
				pos.x += std::max(0, x - scroll.x);
				pos.y += std::max(0, y - scroll.y);
				pos.w = std::min(w, _size.w - x + scroll.x) + std::min(0, x - scroll.x);
				pos.h = std::min(h, _size.h - y + scroll.y) + std::min(0, y - scroll.y);
				if (pos.w <= 0 || pos.h <= 0) {
					return;
				}

				SDL_Rect section;
				section.x = x - scroll.x < 0 ? -(x - scroll.x) : 0;
				section.y = y - scroll.y < 0 ? -(y - scroll.y) : 0;
				section.w = ((float)pos.w / (size.w - x * 2)) * w;
				section.h = ((float)pos.h / (size.h - y * 2)) * h;
				if (section.w == 0 || section.h == 0) {
					return;
				}

				SDL_Rect scaledPos;
				scaledPos.x = pos.x * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.y = pos.y * (float)yres / (float)Frame::virtualScreenY;
				scaledPos.w = pos.w * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.h = pos.h * (float)yres / (float)Frame::virtualScreenY;
				_text->drawColor(section, scaledPos, textColor);
			}
		} else if (icon.c_str()) {
			Image* iconImg = Image::get(icon.c_str());
			if (iconImg) {
				int w = iconImg->getWidth();
				int h = iconImg->getHeight();
				int x = (style != STYLE_DROPDOWN) ?
					(size.w - w) / 2 :
					5 + border;
				int y = (size.h - h) / 2;

				SDL_Rect pos = _size;
				pos.x += std::max(0, x - scroll.x);
				pos.y += std::max(0, y - scroll.y);
				pos.w = std::min(w, _size.w - x + scroll.x) + std::min(0, x - scroll.x);
				pos.h = std::min(h, _size.h - y + scroll.y) + std::min(0, y - scroll.y);
				if (pos.w <= 0 || pos.h <= 0) {
					return;
				}

				SDL_Rect section;
				section.x = x - scroll.x < 0 ? -(x - scroll.x) : 0;
				section.y = y - scroll.y < 0 ? -(y - scroll.y) : 0;
				section.w = ((float)pos.w / (size.w - x * 2)) * w;
				section.h = ((float)pos.h / (size.h - y * 2)) * h;
				if (section.w == 0 || section.h == 0) {
					return;
				}

				SDL_Rect scaledPos;
				scaledPos.x = pos.x * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.y = pos.y * (float)yres / (float)Frame::virtualScreenY;
				scaledPos.w = pos.w * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.h = pos.h * (float)yres / (float)Frame::virtualScreenY;
				iconImg->draw(&section, scaledPos);
			}
		}
	}

	// drop down buttons have an image on the right side (presumably a down arrow)
	if (style == STYLE_DROPDOWN) {
		Image* iconImg = Image::get(icon.c_str());
		if (iconImg) {
			SDL_Rect pos;
			pos.y = _size.y + border; pos.h = _size.h - border * 2;
			pos.w = pos.h; pos.x = _size.x + _size.w - border - pos.w;
			if (pos.w <= 0 || pos.h <= 0) {
				return;
			}

			float w = iconImg->getWidth();
			float h = iconImg->getHeight();

			// TODO scale the drop-down image
			SDL_Rect section;
			section.x = 0;
			section.y = size.y - _actualSize.y < 0 ? -(size.y - _actualSize.y) * (h / (size.h - border * 2)) : 0;
			section.w = ((float)pos.w / (size.h - border * 2)) * w;
			section.h = ((float)pos.h / (size.h - border * 2)) * h;
			if (section.w <= 0 || section.h <= 0) {
				return;
			}

			SDL_Rect scaledPos;
			scaledPos.x = pos.x * (float)xres / (float)Frame::virtualScreenX;
			scaledPos.y = pos.y * (float)yres / (float)Frame::virtualScreenY;
			scaledPos.w = pos.w * (float)xres / (float)Frame::virtualScreenX;
			scaledPos.h = pos.h * (float)yres / (float)Frame::virtualScreenY;
			iconImg->draw(&section, scaledPos);
		}
	}
}

Button::result_t Button::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	result_t result;
	if (style == STYLE_CHECKBOX || style == STYLE_TOGGLE) {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = false;
		result.clicked = false;
	} else {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = pressed;
		result.clicked = false;
	}
	if (disabled) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if (style != STYLE_CHECKBOX && style != STYLE_TOGGLE) {
			reallyPressed = pressed = false;
		}
		return result;
	}
	if (!usable) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if (style != STYLE_CHECKBOX && style != STYLE_TOGGLE) {
			reallyPressed = pressed = false;
		}
		return result;
	}

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		highlightTime = result.highlightTime;
		return result;
	}

	Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;

	if (rectContainsPoint(_size, omousex, omousey)) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.c_str();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}

	result.clicked = false;
	if (highlighted) {
		if (mousestatus[SDL_BUTTON_LEFT]) {
			select();
			if (rectContainsPoint(_size, mousex, mousey)) {
				result.pressed = pressed = (reallyPressed == false);
			} else {
				pressed = reallyPressed;
			}
		} else {
			if (pressed != reallyPressed) {
				result.clicked = true;
			}
			if (style != STYLE_CHECKBOX && style != STYLE_TOGGLE) {
				reallyPressed = pressed = false;
			} else {
				pressed = reallyPressed;
			}
		}
	} else {
		if (style != STYLE_CHECKBOX && style != STYLE_TOGGLE) {
			reallyPressed = pressed = false;
		} else {
			pressed = reallyPressed;
		}
	}

	return result;
}