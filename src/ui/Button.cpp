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
	color = makeColor(127, 127, 127, 255);
	textColor = makeColor(255, 255, 255, 255);
	textHighlightColor = textColor;
	borderColor = makeColor(63, 63, 63, 255);
	highlightColor = makeColor(255, 255, 255, 255);
}

Button::Button(Frame& _parent) : Button() {
	parent = &_parent;
	_parent.getButtons().push_back(this);
	_parent.adoptWidget(*this);
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
	if (callback) {
		(*callback)(*this);
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

	bool focused = highlighted || selected;

	{
		SDL_Rect scaledSize;
		scaledSize.x = _size.x * (float)xres / (float)Frame::virtualScreenX;
		scaledSize.y = _size.y * (float)yres / (float)Frame::virtualScreenY;
		scaledSize.w = _size.w * (float)xres / (float)Frame::virtualScreenX;
		scaledSize.h = _size.h * (float)yres / (float)Frame::virtualScreenY;
		if (background.empty()) {
			SDL_Rect inner;
			inner.x = (_size.x + border) * (float)xres / (float)Frame::virtualScreenX;
			inner.y = (_size.y + border) * (float)yres / (float)Frame::virtualScreenY;
			inner.w = (_size.w - border*2) * (float)xres / (float)Frame::virtualScreenX;
			inner.h = (_size.h - border*2) * (float)yres / (float)Frame::virtualScreenY;
			Uint32 color = focused ? highlightColor : this->color;
			if (pressed) {
				drawRect(&scaledSize, color, (Uint8)(color>>mainsurface->format->Ashift));
				drawRect(&inner, borderColor, (Uint8)(borderColor>>mainsurface->format->Ashift));
			} else {
				drawRect(&scaledSize, borderColor, (Uint8)(borderColor>>mainsurface->format->Ashift));
				drawRect(&inner, color, (Uint8)(color>>mainsurface->format->Ashift));
			}
		} else {
			const char* path = pressed ?
				(backgroundActivated.empty() ?
					background.c_str() :
					backgroundActivated.c_str()) :
				background.c_str();
			Image* background_img = Image::get(path);
			background_img->drawColor(nullptr, scaledSize, focused ? highlightColor : color);
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
				int x, y, w, h;
				w = _text->getWidth();
				h = _text->getHeight();
				if (hjustify == LEFT || hjustify == TOP) {
					x = style == STYLE_DROPDOWN ? 5 + border : border;
				} else if (hjustify == CENTER) {
					x = (size.w - w) / 2;
				} else if (hjustify == RIGHT || hjustify == BOTTOM) {
					x = size.w - w - border;
				}
				if (vjustify == LEFT || vjustify == TOP) {
					y = border;
				} else if (vjustify == CENTER) {
					y = (size.h - h) / 2;
				} else if (vjustify == RIGHT || vjustify == BOTTOM) {
					y = size.h - h - border;
				}

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
				section.w = pos.w;
				section.h = pos.h;
				if (section.w == 0 || section.h == 0) {
					return;
				}

				SDL_Rect scaledPos;
				scaledPos.x = pos.x * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.y = pos.y * (float)yres / (float)Frame::virtualScreenY;
				scaledPos.w = pos.w * (float)xres / (float)Frame::virtualScreenX;
				scaledPos.h = pos.h * (float)yres / (float)Frame::virtualScreenY;
				if (focused) {
					_text->drawColor(section, scaledPos, textHighlightColor);
				} else {
					_text->drawColor(section, scaledPos, textColor);
				}
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
				section.w = pos.w;
				section.h = pos.h;
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

#ifndef NINTENDO
	if (rectContainsPoint(_size, omousex, omousey)) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.c_str();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}
#else
	result.highlighted = highlighted = false;
	result.highlightTime = highlightTime = SDL_GetTicks();
	result.tooltip = nullptr;
#endif

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