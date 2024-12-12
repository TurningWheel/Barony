// Button.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"
#include "Text.hpp"
#include <cassert>

#ifndef EDITOR
#include "MainMenu.hpp"
#endif

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
	if (style == STYLE_NORMAL || style == STYLE_RADIO) {
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

static char* tokenize(char* str, const char* const delimiters) {
	if (!str || !delimiters) {
		return nullptr;
	}
	size_t del_len = strlen(delimiters);
	for (char* token = str;; ++token) {
		for (size_t c = 0; c < del_len; ++c) {
			if (*token == delimiters[c]) {
				*token = '\0';
				return token + 1;
			}
		}
		if (*token == '\0') {
			return nullptr;
		}
	}
}

void Button::draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const {
	if (invisible) {
		return;
	}
	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		return;
	}

#if defined(EDITOR) || defined(NINTENDO)
	const bool focused = (fingerdown && highlighted) || selected;
#else
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	const bool focused = highlighted || (selected && !inputs.getVirtualMouse(mouseowner)->draw_cursor && (intro || !players[owner]->shootmode));
#endif

	SDL_Rect scaledSize;
	scaledSize.x = _size.x;
	scaledSize.y = _size.y;
	scaledSize.w = _size.w;
	scaledSize.h = _size.h;

	const char* path;
	auto _background = background.empty() ? "images/system/white.png" : background.c_str();
	if (pressed) {
		if (!backgroundActivated.empty()) {
			path = backgroundActivated.c_str();
		} else if (!backgroundHighlighted.empty()) {
			path = backgroundHighlighted.c_str();
		} else {
			path = _background;
		}
	} else if (focused) {
		if (!backgroundHighlighted.empty()) {
			path = backgroundHighlighted.c_str();
		} else {
			path = _background;
		}
	} else {
		path = _background;
	}

	auto viewport = SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};

	Frame::image_t image;
	image.path = path;
	image.color = focused ? highlightColor : color;
	image.disabled = false;
	image.name = "temp";
	image.ontop = false;
	image.pos = {0, 0, size.w, size.h};
	image.tiled = false;
	image.noBlitParent = true;
	auto frame = static_cast<Frame*>(parent);
	frame->drawImage(&image, _size,
		SDL_Rect{
			std::max(0, _actualSize.x - size.x),
			std::max(0, _actualSize.y - size.y),
			0, 0
		}
	);

	SDL_Rect scroll{0, 0, 0, 0};
	if (size.x - _actualSize.x < 0) {
		scroll.x -= size.x - _actualSize.x;
	}
	if (size.y - _actualSize.y < 0) {
		scroll.y -= size.y - _actualSize.y;
	}

	if ((style != STYLE_CHECKBOX && style != STYLE_RADIO) || pressed) {
		if (!icon.empty()) {
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
					goto next;
				}

				SDL_Rect section;
				section.x = x - scroll.x < 0 ? -(x - scroll.x) : 0;
				section.y = y - scroll.y < 0 ? -(y - scroll.y) : 0;
				section.w = pos.w;
				section.h = pos.h;
				if (section.w == 0 || section.h == 0) {
					goto next;
				}

				SDL_Rect scaledPos;
				scaledPos.x = pos.x;
				scaledPos.y = pos.y;
				scaledPos.w = pos.w;
				scaledPos.h = pos.h;
				if (iconColor) {
				    iconImg->drawColor(&section, scaledPos, viewport, iconColor);
				} else {
				    iconImg->drawColor(&section, scaledPos, viewport, focused ? highlightColor : color);
				}
			}
		} else if (!text.empty()) {
			Font* _font = Font::get(font.c_str());

			int lines = 1;
			for (auto c : text) {
				if (c == '\n') {
					++lines;
				}
			}
			int fullH = lines * (_font->height(false) + paddingPerTextLine) + _font->getOutline() * 2;

			char* buf = (char*)malloc(text.size() + 1);
			memcpy(buf, text.c_str(), text.size() + 1);
			int yoff = 0;
			char* nexttoken;
			char* token = buf;
			do {
				nexttoken = tokenize(token, "\n");

				std::string str = token;

				Text* _text = Text::get(str.c_str(), font.c_str(),
					makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
				assert(_text);

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
					y = yoff + border + std::min(size.h - fullH, 0);
				} else if (vjustify == CENTER) {
					y = yoff + (size.h - fullH) / 2;
				} else if (vjustify == RIGHT || vjustify == BOTTOM) {
					y = yoff - border + std::max(size.h - fullH, 0);
				}

				x += textOffset.x;
				y += textOffset.y;

				yoff += _font->height(false) + paddingPerTextLine;

				SDL_Rect pos = _size;
				pos.x += std::max(0, x - scroll.x);
				pos.y += std::max(0, y - scroll.y);
				pos.w = std::min(w, _size.w - x + scroll.x) + std::min(0, x - scroll.x);
				pos.h = std::min(h, _size.h - y + scroll.y) + std::min(0, y - scroll.y);
				if (pos.w <= 0 || pos.h <= 0) {
					goto next;
				}

				SDL_Rect section;
				section.x = x - scroll.x < 0 ? -(x - scroll.x) : 0;
				section.y = y - scroll.y < 0 ? -(y - scroll.y) : 0;
				section.w = pos.w;
				section.h = pos.h;
				if (section.w == 0 || section.h == 0) {
					goto next;
				}

				SDL_Rect scaledPos;
				scaledPos.x = pos.x;
				scaledPos.y = pos.y;
				scaledPos.w = pos.w;
				scaledPos.h = pos.h;
				if (focused) {
					if ( parent && static_cast<Frame*>(parent)->getOpacity() < 100.0 )
					{
						Uint8 r, g, b, a;
						::getColor(textHighlightColor, &r, &g, &b, &a);
						a *= static_cast<Frame*>(parent)->getOpacity() / 100.0;
						_text->drawColor(section, scaledPos, viewport, makeColor(r, g, b, a));
					}
					else
					{
						_text->drawColor(section, scaledPos, viewport, textHighlightColor);
					}
				} else {
					if ( parent && static_cast<Frame*>(parent)->getOpacity() < 100.0 )
					{
						Uint8 r, g, b, a;
						::getColor(textColor, &r, &g, &b, &a);
						a *= static_cast<Frame*>(parent)->getOpacity() / 100.0;
						_text->drawColor(section, scaledPos, viewport, makeColor(r, g, b, a));
					}
					else
					{
						_text->drawColor(section, scaledPos, viewport, textColor);
					}
				}
			} while ((token = nexttoken) != NULL);
			free(buf);
		}
	}
next:

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
			scaledPos.x = pos.x;
			scaledPos.y = pos.y;
			scaledPos.w = pos.w;
			scaledPos.h = pos.h;
			iconImg->draw(&section, scaledPos, viewport);
		}
	}

	// draw user stuff
	if (drawCallback) {
		drawCallback(*this, scaledSize);
	}
}

void Button::drawPost(SDL_Rect _size, SDL_Rect _actualSize,
    const std::vector<const Widget*>& selectedWidgets,
    const std::vector<const Widget*>& searchParents) const {
	if (invisible) {
		return;
	}
	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		return;
	}
	Widget::drawPost(_size, selectedWidgets, searchParents);
}

Button::result_t Button::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	Widget::process();

	result_t result;
	if (style == STYLE_CHECKBOX || style == STYLE_RADIO || style == STYLE_TOGGLE) {
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
		if (style == STYLE_NORMAL || style == STYLE_DROPDOWN) {
			reallyPressed = pressed = false;
		}
		return result;
	}
	if (!usable) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if (style == STYLE_NORMAL || style == STYLE_DROPDOWN) {
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

#if defined(NINTENDO)
	const bool clicking = fingerdown;
	Sint32 mousex = (::fingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::fingery / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::ofingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::ofingery / (float)yres) * (float)Frame::virtualScreenY;
#elif defined(EDITOR)
	const bool clicking = mousestatus[SDL_BUTTON_LEFT];
	Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
#else
	const bool clicking = mousestatus[SDL_BUTTON_LEFT];
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	Sint32 mousex = (inputs.getMouse(mouseowner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (inputs.getMouse(mouseowner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (inputs.getMouse(mouseowner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (inputs.getMouse(mouseowner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
#endif

#ifndef EDITOR
#ifndef NINTENDO
	if (rectContainsPoint(_size, omousex, omousey) && inputs.getVirtualMouse(mouseowner)->draw_cursor) {
#else
	if (rectContainsPoint(_size, omousex, omousey)) {
#endif
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
		if (clicking) {
			select();
			if (rectContainsPoint(_size, mousex, mousey)) {
				if (style == STYLE_RADIO) {
					if (!reallyPressed) {
						result.pressed = pressed = reallyPressed = true;
						result.clicked = true;
					}
				} else {
					result.pressed = pressed = (reallyPressed == false);
				}
			} else {
				pressed = reallyPressed;
			}
		} else {
			if (pressed != reallyPressed) {
				result.clicked = true;
			}
			if (style == STYLE_NORMAL || style == STYLE_DROPDOWN) {
				reallyPressed = pressed = false;
			} else {
				pressed = reallyPressed;
			}
		}
	} else {
		if (style == STYLE_NORMAL || style == STYLE_DROPDOWN) {
			reallyPressed = pressed = false;
		} else {
			pressed = reallyPressed;
		}
	}

	return result;
}

void Button::scrollParent() {
	Frame* fparent = static_cast<Frame*>(parent);
	auto fActualSize = fparent->getActualSize();
	auto fSize = fparent->getSize();

	const auto y = size.y + scrollParentOffset.y;
	const auto h = size.h + scrollParentOffset.h;
	const auto x = size.x + scrollParentOffset.x;
	const auto w = size.w + scrollParentOffset.w;

	if ( y < fActualSize.y) {
		fActualSize.y = y;
	}
	else if ( size.y + h >= fActualSize.y + fSize.h) {
		fActualSize.y = (size.y + h) - fSize.h;
	}
	if (x < fActualSize.x) {
		fActualSize.x = x;
	}
	else if (size.x + w >= fActualSize.x + fSize.w) {
		fActualSize.x = (size.x + w) - fSize.w;
	}
	fparent->setActualSize(fActualSize);
}

SDL_Rect Button::getAbsoluteSize() const
{
	SDL_Rect _size{ size.x, size.y, size.w, size.h };
	auto _parent = static_cast<Frame*>(this->parent);
	if ( _parent ) {
		SDL_Rect absoluteSize = _parent->getAbsoluteSize();
		_size.x += std::max(0, absoluteSize.x);
		_size.y += std::max(0, absoluteSize.y);
	}
	return _size;
}
