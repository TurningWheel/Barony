// Slider.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Slider.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"

Slider::Slider(Frame& _parent) {
	parent = &_parent;
	_parent.adoptWidget(*this);
}

void Slider::draw(SDL_Rect _size, SDL_Rect _actualSize, Widget* selectedWidget) {
	if (invisible) {
		return;
	}
	if (maxValue == minValue) {
		return;
	}

	SDL_Rect _handleSize, _railSize;

	if (orientation == SLIDER_HORIZONTAL) {
		handleSize.x = railSize.x - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.w;
		handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;
	} else if (orientation == SLIDER_VERTICAL) {
		handleSize.x = railSize.x + railSize.w / 2 - handleSize.w / 2;
		handleSize.y = railSize.y - handleSize.h / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.h;
	}

	bool focused = highlighted || selected;

	// draw rail
	_railSize.x = _size.x + std::max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + std::max(0, railSize.y - _actualSize.y);
	_railSize.w = std::min(railSize.w, _size.w - railSize.x + _actualSize.x) + std::min(0, railSize.x - _actualSize.x);
	_railSize.h = std::min(railSize.h, _size.h - railSize.y + _actualSize.y) + std::min(0, railSize.y - _actualSize.y);
	if (_railSize.w > 0 && _railSize.h > 0) {
		if (railImage.empty()) {
			int x = (_railSize.x);
			int y = (_railSize.y);
			int w = (_railSize.x + _railSize.w);
			int h = (_railSize.y + _railSize.h);
			drawDepressed(x, y, w, h);
		} else {
			Frame::image_t image;
			image.path = railImage;
			image.color = focused ? highlightColor : color;
			image.disabled = false;
			image.name = "temp";
			image.ontop = false;
			image.pos = {0, 0, railSize.w, railSize.h};
			image.tiled = false;
			auto frame = static_cast<Frame*>(parent);
			frame->drawImage(&image, _railSize,
				SDL_Rect{
					std::max(0, _actualSize.x - railSize.x),
					std::max(0, _actualSize.y - railSize.y),
					0, 0
				}
			);
		}
	}
	
	// draw handle
	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);
	if (_handleSize.w > 0 && _handleSize.h > 0) {
		auto& imageToUse = activated ?
			(handleImageActivated.empty() ? handleImage : handleImageActivated) :
			handleImage;
		if (imageToUse.empty()) {
			int x = (_handleSize.x);
			int y = (_handleSize.y);
			int w = (_handleSize.x + _handleSize.w);
			int h = (_handleSize.y + _handleSize.h);
			drawWindow(x, y, w, h);
		} else {
			Frame::image_t image;
			image.path = imageToUse;
			image.color = focused ? highlightColor : color;
			image.disabled = false;
			image.name = "temp";
			image.ontop = false;
			image.pos = {0, 0, handleSize.w, handleSize.h};
			image.tiled = false;
			auto frame = static_cast<Frame*>(parent);
			frame->drawImage(&image, _handleSize,
				SDL_Rect{
					std::max(0, _actualSize.x - handleSize.x),
					std::max(0, _actualSize.y - handleSize.y),
					0, 0
				}
			);
		}
	}

	drawGlyphs(_handleSize, selectedWidget);
}

Slider::result_t Slider::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	Widget::process();

	result_t result;
	result.tooltip = nullptr;
	result.highlightTime = SDL_GetTicks();
	result.highlighted = false;
	result.clicked = false;
	if (disabled || invisible || maxValue == minValue) {
		highlightTime = result.highlightTime;
		highlighted = false;
		pressed = false;
		return result;
	}
	if (!usable) {
		highlightTime = result.highlightTime;
		highlighted = false;
		pressed = false;
		return result;
	}

	SDL_Rect _handleSize, _railSize;

	handleSize.x = railSize.x - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.w;
	handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;

	_railSize.x = _size.x + std::max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + std::max(0, railSize.y - _actualSize.y);
	_railSize.w = std::min(railSize.w, _size.w - railSize.x + _actualSize.x) + std::min(0, railSize.x - _actualSize.x);
	_railSize.h = std::min(railSize.h, _size.h - railSize.y + _actualSize.y) + std::min(0, railSize.y - _actualSize.y);

	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);

	int offX = _size.x + railSize.x - _actualSize.x;
	int offY = _size.y + railSize.y - _actualSize.y;
	if (orientation == SLIDER_HORIZONTAL) {
		_size.x = std::max(_size.x, _railSize.x - _handleSize.w / 2);
		_size.y = std::max(_size.y, _railSize.y + _railSize.h / 2 - _handleSize.h / 2);
		_size.w = std::min(_size.w, _railSize.w + _handleSize.w);
		_size.h = _handleSize.h;
	} else if (orientation == SLIDER_VERTICAL) {
		_size.x = std::max(_size.x, _railSize.x + _railSize.w / 2 - _handleSize.w / 2);
		_size.y = std::max(_size.y, _railSize.y - _handleSize.h / 2);
		_size.w = _handleSize.w;
		_size.h = std::min(_size.h, _railSize.h + _handleSize.h);
	}

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
			pressed = true;
			float oldValue = value;
			if (orientation == SLIDER_HORIZONTAL) {
				value = ((float)(mousex - offX) / railSize.w) * (float)(maxValue - minValue) + minValue;
			}
			else if (orientation == SLIDER_VERTICAL) {
				value = ((float)(mousey - offY) / railSize.h) * (float)(maxValue - minValue) + minValue;
			}
			value = std::min(std::max(minValue, value), maxValue);
			if (oldValue != value) {
				result.clicked = true;
			}
		} else {
			pressed = false;
		}
	} else {
		pressed = false;
	}

	return result;
}

void Slider::activate() {
	activated = activated == false;
}

void Slider::fireCallback() {
	if (callback) {
		(*callback)(*this);
	}
}

void Slider::control() {
	if (!activated) {
		moveStartTime = ticks;
		return;
	}
	Input& input = Input::inputs[owner];
	if (input.consumeBinaryToggle("MenuCancel") ||
		input.consumeBinaryToggle("MenuConfirm")) {
		activated = false;
	} else {
		bool movePositive, moveNegative;
		if (orientation == SLIDER_HORIZONTAL) {
			movePositive = input.binary("MenuRight") || input.binary("AltMenuRight");
			moveNegative = input.binary("MenuLeft") || input.binary("AltMenuLeft");
		} else if (orientation == SLIDER_VERTICAL) {
			movePositive = input.binary("MenuDown") || input.binary("AltMenuDown");
			moveNegative = input.binary("MenuUp") || input.binary("AltMenuUp");
		}
		if (movePositive || moveNegative) {
			Uint32 timeMoved = ticks - moveStartTime;
			Uint32 lastMove = ticks - lastMoveTime;
			Uint32 sec = TICKS_PER_SECOND;
			float inc = movePositive ? 1.f : -1.f;
			float ovalue = value;
			if (timeMoved < sec) {
				if (lastMove > sec / (5.f * valueSpeed)) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 2) {
				if (lastMove > sec / (10.f * valueSpeed)) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 3) {
				if (lastMove > sec / (20.f * valueSpeed)) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 4) {
				if (lastMove > sec / (40.f * valueSpeed)) {
					value += inc;
				}
			} else {
				if (lastMove > sec / (80.f * valueSpeed)) {
					value += inc;
				}
			}
			value = std::min(std::max(minValue, value), maxValue);
			if (value != ovalue) {
				lastMoveTime = ticks;
				fireCallback();
			}
		} else {
			moveStartTime = ticks;
		}
	}
}

void Slider::deselect() {
	activated = false;
	Widget::deselect();
}

void Slider::scrollParent() {
	Frame* fparent = static_cast<Frame*>(parent);
	auto fActualSize = fparent->getActualSize();
	auto fSize = fparent->getSize();
	if (orientation == SLIDER_HORIZONTAL) {
		if (handleSize.y < fActualSize.y) {
			fActualSize.y = handleSize.y;
		}
		else if (handleSize.y + handleSize.h >= fActualSize.y + fSize.h) {
			fActualSize.y = (handleSize.y + handleSize.h) - fSize.h;
		}
		if (railSize.x < fActualSize.x) {
			fActualSize.x = railSize.x;
		}
		else if (railSize.x + railSize.w >= fActualSize.x + fSize.w) {
			fActualSize.x = (railSize.x + railSize.w) - fSize.w;
		}
	} else if (orientation == SLIDER_VERTICAL) {
		if (railSize.y < fActualSize.y) {
			fActualSize.y = railSize.y;
		}
		else if (railSize.y + railSize.h >= fActualSize.y + fSize.h) {
			fActualSize.y = (railSize.y + railSize.h) - fSize.h;
		}
		if (handleSize.x < fActualSize.x) {
			fActualSize.x = handleSize.x;
		}
		else if (handleSize.x + handleSize.w >= fActualSize.x + fSize.w) {
			fActualSize.x = (handleSize.x + handleSize.w) - fSize.w;
		}
	}
	fparent->setActualSize(fActualSize);
}