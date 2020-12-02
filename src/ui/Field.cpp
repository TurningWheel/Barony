// Field.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Frame.hpp"
#include "Field.hpp"

Field::Field() : Field("") {
}

Field::Field(const int _textLen) {
	text.alloc(_textLen + 1);
}

Field::Field(const char* _text) {
	text = _text;
}

Field::Field(Frame& _parent) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
	_parent.adoptWidget(*this);
}

Field::Field(Frame& _parent, const int _textLen) : Field(_textLen) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
	_parent.adoptWidget(*this);
}

Field::Field(Frame& _parent, const char* _text) : Field(_text) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
	_parent.adoptWidget(*this);
}

Field::~Field() {
	deselect();
	if (callback) {
		delete callback;
		callback = nullptr;
	}
}

void Field::select() {
	selected = true;
	mainEngine->setInputStr(const_cast<char*>(text.get()));
	mainEngine->setInputLen(text.getSize());
	mainEngine->setInputNumbersOnly(numbersOnly);
	SDL_StartTextInput();
}

void Field::deselect() {
	selectAll = false;
	selected = false;
	if (mainEngine->getInputStr() == text) {
		mainEngine->setInputStr(nullptr);
		mainEngine->setInputLen(0);
		SDL_StopTextInput();
	}
}

void Field::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	Rect<int> rect;
	rect.x = _size.x + max(0, size.x - _actualSize.x);
	rect.y = _size.y + max(0, size.y - _actualSize.y);
	rect.w = min(size.w, _size.w - size.x + _actualSize.x) + min(0, size.x - _actualSize.x);
	rect.h = min(size.h, _size.h - size.y + _actualSize.y) + min(0, size.y - _actualSize.y);
	if (rect.w <= 0 || rect.h <= 0)
		return;

	if (selected) {
		renderer.drawRect(&rect, glm::vec4(0.f, 0.f, .5f, 1.f));
	}

	String str;
	if (selected && mainEngine->isCursorVisible()) {
		str.alloc((Uint32)strlen(text) + 2);
		str.assign(text);
		str.append("_");
	} else if (selected) {
		str.alloc((Uint32)strlen(text) + 2);
		str.assign(text);
		str.append(" ");
	} else {
		str.assign(text);
	}

	Text* text = nullptr;
	if (!str.empty()) {
		text = Text::get(str.get(), font.get());
		if (!text) {
			return;
		}
	} else {
		return;
	}
	Font* actualFont = mainEngine->getFontResource().dataForString(font.get());
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
		if (!mainEngine->isCursorVisible()) {
			int w;
			actualFont->sizeText("_", &w, nullptr);
			textSizeW += w;
			textSizeH += 2;
		}
	}

	Rect<int> pos;
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

	Rect<int> dest;
	dest.x = max(rect.x, pos.x);
	dest.y = max(rect.y, pos.y);
	dest.w = pos.w - (dest.x - pos.x) - max(0, (pos.x + pos.w) - (rect.x + rect.w));
	dest.h = pos.h - (dest.y - pos.y) - max(0, (pos.y + pos.h) - (rect.y + rect.h));

	Rect<int> src;
	src.x = max(0, rect.x - pos.x);
	src.y = max(0, rect.y - pos.y);
	src.w = pos.w - (dest.x - pos.x) - max(0, (pos.x + pos.w) - (rect.x + rect.w));
	src.h = pos.h - (dest.y - pos.y) - max(0, (pos.y + pos.h) - (rect.y + rect.h));

	// fit text to window
	if ((hjustify == LEFT || hjustify == TOP) && scroll) {
		src.x = max(src.x, textSizeW - rect.w);
	}

	if (src.w <= 0 || src.h <= 0 || dest.w <= 0 || dest.h <= 0)
		return;

	if (selectAll && selected) {
		renderer.drawRect(&rect, glm::vec4(.5f, .5f, 0.f, 1.f));
	}

	text->drawColor(src, dest, glm::vec4(color.x, color.y, color.z, color.w));
}

Field::result_t Field::process(Rect<int> _size, Rect<int> _actualSize, const bool usable) {
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

	_size.x += max(0, size.x - _actualSize.x);
	_size.y += max(0, size.y - _actualSize.y);
	_size.w = min(size.w, _size.w - size.x + _actualSize.x) + min(0, size.x - _actualSize.x);
	_size.h = min(size.h, _size.h - size.y + _actualSize.y) + min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		return result;
	}

	Sint32 omousex = (mainEngine->getOldMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
	Sint32 omousey = (mainEngine->getOldMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;

	if (selected) {
		if (mainEngine->getInputStr() != text) {
			result.entered = true;
			deselect();
			if (mainEngine->getInputStr() == nullptr) {
				SDL_StopTextInput();
			}
		}
		if (mainEngine->getKeyStatus(SDL_SCANCODE_RETURN) || mainEngine->getKeyStatus(SDL_SCANCODE_KP_ENTER)) {
			result.entered = true;
			deselect();
		}
		if (mainEngine->getKeyStatus(SDL_SCANCODE_ESCAPE) || mainEngine->getMouseStatus(SDL_BUTTON_RIGHT)) {
			result.entered = true;
			deselect();
		}

		if (selectAll) {
			if (mainEngine->getAnyKeyStatus()) {
				const char* keys = mainEngine->getLastInput();
				if (keys) {
					if (!Engine::charsHaveLetters(keys, (Uint32)strlen(keys)) || !numbersOnly) {
						text = keys;
						selectAll = false;
					}
				}
			}
		}
	}

	if (omousex >= _size.x && omousex < _size.x + _size.w &&
		omousey >= _size.y && omousey < _size.y + _size.h) {
		result.highlighted = true;
	}

	if (!result.highlighted && mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
		if (selected) {
			result.entered = true;
			deselect();
		}
	} else if (result.highlighted && mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
		select();
		if (mainEngine->getDBCMouseStatus(SDL_BUTTON_LEFT)) {
			selectAll = true;
		}
	}

	return result;
}