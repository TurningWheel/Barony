// Slider.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Slider.hpp"
#include "Frame.hpp"
#include "Button.hpp"

Slider::Slider(Frame& _parent) {
	parent = &_parent;
	_parent.adoptWidget(*this);
}

void Slider::draw(SDL_Rect _size, SDL_Rect _actualSize) {
	SDL_Rect _handleSize, _railSize;

	handleSize.x = railSize.x - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.w;
	handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;

	// draw rail
	_railSize.x = _size.x + std::max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + std::max(0, railSize.y - _actualSize.y);
	_railSize.w = std::min(railSize.w, _size.w - railSize.x + _actualSize.x) + std::min(0, railSize.x - _actualSize.x);
	_railSize.h = std::min(railSize.h, _size.h - railSize.y + _actualSize.y) + std::min(0, railSize.y - _actualSize.y);
	if (_railSize.w > 0 && _railSize.h > 0) {
		int x = (_railSize.x) * (float)xres / (float)Frame::virtualScreenX;
		int y = (_railSize.y) * (float)yres / (float)Frame::virtualScreenY;
		int w = (_railSize.x + _railSize.w) * (float)xres / (float)Frame::virtualScreenX;
		int h = (_railSize.y + _railSize.h) * (float)yres / (float)Frame::virtualScreenY;
		drawDepressed(x, y, w, h);
	}
	
	// draw handle
	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);
	if (_handleSize.w > 0 && _handleSize.h > 0) {
		int x = (_handleSize.x) * (float)xres / (float)Frame::virtualScreenX;
		int y = (_handleSize.y) * (float)yres / (float)Frame::virtualScreenY;
		int w = (_handleSize.x + _handleSize.w) * (float)xres / (float)Frame::virtualScreenX;
		int h = (_handleSize.y + _handleSize.h) * (float)yres / (float)Frame::virtualScreenY;
		drawWindow(x, y, w, h);
	}
}

Slider::result_t Slider::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	result_t result;
	result.tooltip = nullptr;
	result.highlightTime = SDL_GetTicks();
	result.highlighted = false;
	result.clicked = false;
	if (disabled) {
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
	_size.x = std::max(_size.x, _railSize.x - _handleSize.w / 2);
	_size.y = std::max(_size.y, _railSize.y + _railSize.h / 2 - _handleSize.h / 2);
	_size.w = std::min(_size.w, _railSize.w + _handleSize.w);
	_size.h = _handleSize.h;

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
			pressed = true;
			float oldValue = value;
			value = ((float)(mousex - offX) / railSize.w) * (float)(maxValue - minValue) + minValue;
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
	Widget::Args args;
	args.addFloat(value);
	if (callback) {
		(*callback)(args);
	}
}

void Slider::control() {
	if (!activated) {
		moveStartTime = ticks;
		return;
	}
	if (keystatus[SDL_SCANCODE_RETURN]) {
		keystatus[SDL_SCANCODE_RETURN] = 0;
		activated = false;
	} else if (keystatus[SDL_SCANCODE_ESCAPE]) {
		keystatus[SDL_SCANCODE_ESCAPE] = 0;
		activated = false;
	} else {
		if (keystatus[SDL_SCANCODE_RIGHT] || keystatus[SDL_SCANCODE_LEFT]) {
			Uint32 timeMoved = ticks - moveStartTime;
			Uint32 lastMove = ticks - lastMoveTime;
			Uint32 sec = TICKS_PER_SECOND;
			float inc = keystatus[SDL_SCANCODE_RIGHT] ? 1.f : -1.f;
			float ovalue = value;
			if (timeMoved < sec) {
				if (lastMove > sec / 5) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 2) {
				if (lastMove > sec / 10) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 3) {
				if (lastMove > sec / 20) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 4) {
				if (lastMove > sec / 40) {
					value += inc;
				}
			} else {
				if (lastMove > sec / 80) {
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