// Field.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Frame.hpp"
#include "Field.hpp"
#include "Text.hpp"

Field::Field(const int _textLen) {
	textlen = std::max(_textLen, 1);
	text = new char[textlen + 1];
	memset(text, 0, textlen + 1);
}

Field::Field(const char* _text) {
	textlen = strlen(_text) + 1;
	text = new char[textlen];
	setText(_text);
}

Field::Field(Frame& _parent, const int _textLen) : Field(_textLen) {
	parent = &_parent;
	_parent.getFields().push_back(this);
	_parent.adoptWidget(*this);
}

Field::Field(Frame& _parent, const char* _text) : Field(_text) {
	parent = &_parent;
	_parent.getFields().push_back(this);
	_parent.adoptWidget(*this);
}

Field::~Field() {
	deselect();
	if (callback) {
		delete callback;
		callback = nullptr;
	}
	if (text) {
		if (inputstr == text) {
			inputstr = nullptr;
			inputlen = 0;
			SDL_StopTextInput();
		}
		delete[] text;
		text = nullptr;
	}
}

void Field::select() {
	selected = true;
	inputstr = text;
	inputlen = textlen;
	SDL_StartTextInput();
}

void Field::deselect() {
	selectAll = false;
	selected = false;
	if (inputstr == text) {
		inputstr = nullptr;
		inputlen = 0;
		SDL_StopTextInput();
	}
}

void Field::draw(SDL_Rect _size, SDL_Rect _actualSize) {
	SDL_Rect rect;
	rect.x = _size.x + std::max(0, size.x - _actualSize.x);
	rect.y = _size.y + std::max(0, size.y - _actualSize.y);
	rect.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	rect.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (rect.w <= 0 || rect.h <= 0)
		return;

	SDL_Rect scaledRect;
	scaledRect.x = rect.x * (float)xres / (float)Frame::virtualScreenX;
	scaledRect.y = rect.y * (float)yres / (float)Frame::virtualScreenY;
	scaledRect.w = rect.w * (float)xres / (float)Frame::virtualScreenX;
	scaledRect.h = rect.h * (float)yres / (float)Frame::virtualScreenY;

	if (selected) {
		drawRect(&scaledRect, SDL_MapRGB(mainsurface->format, 0, 0, 127), 255);
	}

	bool showCursor = (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2;

	std::string str;
	if (selected && showCursor) {
		str.reserve((Uint32)strlen(text) + 2);
		str.assign(text);
		str.append("_");
	} else if (selected) {
		str.reserve((Uint32)strlen(text) + 2);
		str.assign(text);
		str.append(" ");
	} else {
		str.assign(text);
	}

	Text* text = nullptr;
	if (!str.empty()) {
		text = Text::get(str.c_str(), font.c_str());
		if (!text) {
			return;
		}
	} else {
		return;
	}
	Font* actualFont = Font::get(font.c_str());
	if (!actualFont) {
		return;
	}

	// get the size of the rendered text
	int textSizeW = text->getWidth();
	int textSizeH = text->getHeight();

	if (selected) {
		textSizeH += 2;
		if (hjustify == RIGHT || hjustify == BOTTOM) {
			textSizeH -= 4;
		} else if (hjustify == CENTER) {
			textSizeH -= 2;
		}
		if (!showCursor) {
			int w;
			actualFont->sizeText("_", &w, nullptr);
			textSizeW += w;
			textSizeH += 2;
		}
	}

	SDL_Rect pos;
	if (hjustify == LEFT || hjustify == TOP) {
		pos.x = _size.x + size.x - _actualSize.x;
	} else if (hjustify == CENTER) {
		pos.x = _size.x + size.x + size.w / 2 - textSizeW / 2 - _actualSize.x;
	} else if (hjustify == RIGHT || hjustify == BOTTOM) {
		pos.x = _size.x + size.x + size.w - textSizeW - _actualSize.x;
	}
	if (vjustify == LEFT || vjustify == TOP) {
		pos.y = _size.y + size.y - _actualSize.y;
	} else if (vjustify == CENTER) {
		pos.y = _size.y + size.y + size.h / 2 - textSizeH / 2 - _actualSize.y;
	} else if (vjustify == RIGHT || vjustify == BOTTOM) {
		pos.y = _size.y + size.y + size.h - textSizeH - _actualSize.y;
	}
	pos.w = textSizeW;
	pos.h = textSizeH;

	SDL_Rect dest;
	dest.x = std::max(rect.x, pos.x);
	dest.y = std::max(rect.y, pos.y);
	dest.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (rect.x + rect.w));
	dest.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (rect.y + rect.h));

	SDL_Rect src;
	src.x = std::max(0, rect.x - pos.x);
	src.y = std::max(0, rect.y - pos.y);
	src.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (rect.x + rect.w));
	src.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (rect.y + rect.h));

	// fit text to window
	if ((hjustify == LEFT || hjustify == TOP) && scroll && selected) {
		src.x = std::max(src.x, textSizeW - rect.w);
	}

	if (src.w <= 0 || src.h <= 0 || dest.w <= 0 || dest.h <= 0)
		return;

	if (selectAll && selected) {
		drawRect(&scaledRect, SDL_MapRGB(mainsurface->format, 127, 127, 0), 255);
	}

	SDL_Rect scaledDest;
	scaledDest.x = dest.x * (float)xres / (float)Frame::virtualScreenX;
	scaledDest.y = dest.y * (float)yres / (float)Frame::virtualScreenY;
	scaledDest.w = dest.w * (float)xres / (float)Frame::virtualScreenX;
	scaledDest.h = dest.h * (float)yres / (float)Frame::virtualScreenY;
	text->drawColor(src, scaledDest, color);
}

Field::result_t Field::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	result_t result;
	result.highlighted = false;
	result.entered = false;
	if (!editable) {
		if (selected) {
			result.entered = true;
			deselect();
		}
		return result;
	}

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		return result;
	}

	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;

	if (selected) {
		if (inputstr != text) {
			result.entered = true;
			deselect();
			if (inputstr == nullptr) {
				SDL_StopTextInput();
			}
		}
		if (keystatus[SDL_SCANCODE_RETURN] || keystatus[SDL_SCANCODE_KP_ENTER]) {
			result.entered = true;
			deselect();
		}
		if (keystatus[SDL_SCANCODE_ESCAPE] || mousestatus[SDL_BUTTON_RIGHT]) {
			result.entered = true;
			deselect();
		}

		/*if (selectAll) {
			if (mainEngine->getAnyKeyStatus()) {
				const char* keys = lastkeypressed;
				if (keys) {
					text = keys;
					selectAll = false;
				}
			}
		}*/
	}

	if (omousex >= _size.x && omousex < _size.x + _size.w &&
		omousey >= _size.y && omousey < _size.y + _size.h) {
		result.highlighted = true;
	}

	if (!result.highlighted && mousestatus[SDL_BUTTON_LEFT]) {
		if (selected) {
			result.entered = true;
			deselect();
		}
	} else if (result.highlighted && mousestatus[SDL_BUTTON_LEFT]) {
		select();
		/*if (doubleclick_mousestatus[SDL_BUTTON_LEFT]) {
			selectAll = true;
		}*/
	}

	return result;
}

void Field::setText(const char* _text) {
	if (_text == nullptr || textlen <= 1) {
		return;
	}
	int size = std::min(std::max(0, (int)strlen(_text)), (int)textlen - 1);
	if (size > 0) {
		memcpy(text, _text, size);
		text[size] = '\0';
	} else {
		text[0] = '\0';
	}
}