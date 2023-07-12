// Slider.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "../player.hpp"
#include "Slider.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"

#ifndef EDITOR
#include "MainMenu.hpp"
#endif

Slider::Slider(Frame& _parent) {
	parent = &_parent;
	_parent.adoptWidget(*this);
}

void Slider::draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const {
	if ( invisible || isDisabled() ) {
		return;
	}

	SDL_Rect _handleSize, _railSize;

#if defined(EDITOR) || defined(NINTENDO)
	const bool focused = (fingerdown && highlighted) || selected;
#else
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : owner;
	const bool focused = highlighted || (selected && !inputs.getVirtualMouse(mouseowner)->draw_cursor && (intro || !players[owner]->shootmode));
#endif

	auto white = Image::get("images/system/white.png");
	const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};

	// draw rail
	_railSize.x = _size.x + std::max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + std::max(0, railSize.y - _actualSize.y);
	_railSize.w = std::min(railSize.w, _size.w - railSize.x + _actualSize.x) + std::min(0, railSize.x - _actualSize.x);
	_railSize.h = std::min(railSize.h, _size.h - railSize.y + _actualSize.y) + std::min(0, railSize.y - _actualSize.y);
	if (_railSize.w > 0 && _railSize.h > 0) {
		auto& imageToUse = activated ?
			(handleImageActivated.empty() ? handleImage : handleImageActivated) :
			handleImage;
		if (railImage.empty() && imageToUse.empty()) {
			Uint8 r, g, b, a;
			::getColor(color, &r, &g, &b, &a);
			r = (r / 3) * 2;
			g = (g / 3) * 2;
			b = (b / 3) * 2;
			Uint32 darkColor = makeColor(r, g, b, a);
			white->drawColor(nullptr, _railSize, viewport, darkColor);
		} else if (!railImage.empty()) {
			Frame::image_t image;
			image.path = railImage;
			image.color = focused ? highlightColor : color;
			image.disabled = false;
			image.name = "temp";
			image.ontop = false;
			image.pos = {0, 0, railSize.w, railSize.h};
			image.tiled = false;
			auto frame = static_cast<Frame*>(parent);
			//bool isBlitToParent = frame->isBlitToParent();
			//frame->setBlitToParent(false);
			frame->drawImage(&image, _railSize,
				SDL_Rect{
					std::max(0, _actualSize.x - railSize.x),
					std::max(0, _actualSize.y - railSize.y),
					0, 0
				}
			);
			//frame->setBlitToParent(isBlitToParent);
		}
	}
	
	if (maxValue == minValue) {
		return;
	}

	// draw handle
	SDL_Rect handleSize = this->handleSize;
	if (handleSize.x == 0 && handleSize.y == 0) {
		if (orientation == SLIDER_HORIZONTAL) {
			handleSize.x = (railSize.x + border) - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.w - border * 2);
			handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;
		} else if (orientation == SLIDER_VERTICAL) {
			handleSize.x = railSize.x + railSize.w / 2 - handleSize.w / 2;
			handleSize.y = (railSize.y + border) - handleSize.h / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.h - border * 2);
		}
	}
	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);
	if (_handleSize.w > 0 && _handleSize.h > 0) {
		auto& imageToUse = activated ?
			(handleImageActivated.empty() ? handleImage : handleImageActivated) :
			handleImage;
		if (imageToUse.empty()) {
			white->drawColor(nullptr, _handleSize, viewport, color);
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
			//bool isBlitToParent = frame->isBlitToParent();
			//frame->setBlitToParent(false);
			frame->drawImage(&image, _handleSize,
				SDL_Rect{
					std::max(0, _actualSize.x - handleSize.x),
					std::max(0, _actualSize.y - handleSize.y),
					0, 0
				}
			);
			//frame->setBlitToParent(isBlitToParent);
		}
	}

	// draw user stuff
	SDL_Rect scaledHandle;
	scaledHandle.x = _handleSize.x;
	scaledHandle.y = _handleSize.y;
	scaledHandle.w = _handleSize.w;
	scaledHandle.h = _handleSize.h;
	if (drawCallback) {
		drawCallback(*this, scaledHandle);
	}
}

void Slider::drawPost(SDL_Rect _size, SDL_Rect _actualSize,
    const std::vector<const Widget*>& selectedWidgets,
    const std::vector<const Widget*>& searchParents) const {
	if (invisible) {
		return;
	}
	SDL_Rect _handleSize;
	SDL_Rect handleSize = this->handleSize;
	if (handleSize.x == 0 && handleSize.y == 0) {
		if (orientation == SLIDER_HORIZONTAL) {
			handleSize.x = (railSize.x + border) - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.w - border * 2);
			handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;
		} else if (orientation == SLIDER_VERTICAL) {
			handleSize.x = railSize.x + railSize.w / 2 - handleSize.w / 2;
			handleSize.y = (railSize.y + border) - handleSize.h / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.h - border * 2);
		}
	}
	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);
	if (_handleSize.w <= 0 || _handleSize.h <= 0) {
		return;
	}
	Widget::drawPost(_handleSize, selectedWidgets, searchParents);
}

void Slider::updateHandlePosition() {
	if (orientation == SLIDER_HORIZONTAL) {
		handleSize.x = (railSize.x + border) - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.w - border * 2);
		handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;
	} else if (orientation == SLIDER_VERTICAL) {
		handleSize.x = railSize.x + railSize.w / 2 - handleSize.w / 2;
		handleSize.y = (railSize.y + border) - handleSize.h / 2 + ((float)(value - minValue) / (maxValue - minValue)) * (railSize.h - border * 2);
	}
}

Slider::result_t Slider::process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable) {
	Widget::process();

	value = std::min(std::max(minValue, value), maxValue);

	updateHandlePosition();

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

	_railSize.x = _size.x + std::max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + std::max(0, railSize.y - _actualSize.y);
	_railSize.w = std::min(railSize.w, _size.w - railSize.x + _actualSize.x) + std::min(0, railSize.x - _actualSize.x);
	_railSize.h = std::min(railSize.h, _size.h - railSize.y + _actualSize.y) + std::min(0, railSize.y - _actualSize.y);

	_handleSize.x = _size.x + std::max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + std::max(0, handleSize.y - _actualSize.y);
	_handleSize.w = std::min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + std::min(0, handleSize.x - _actualSize.x);
	_handleSize.h = std::min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + std::min(0, handleSize.y - _actualSize.y);

	int offX = _size.x + (railSize.x + border) - _actualSize.x;
	int offY = _size.y + (railSize.y + border) - _actualSize.y;
	if (orientation == SLIDER_HORIZONTAL) {
		_size.x = std::max(_size.x, _railSize.x - _handleSize.w / 2 + border);
		_size.y = std::max(_size.y, _railSize.y + _railSize.h / 2 - _handleSize.h / 2);
		_size.w = std::min(_size.w, _railSize.w + _handleSize.w - border * 2);
		_size.h = _handleSize.h;
	} else if (orientation == SLIDER_VERTICAL) {
		_size.x = std::max(_size.x, _railSize.x + _railSize.w / 2 - _handleSize.w / 2);
		_size.y = std::max(_size.y, _railSize.y - _handleSize.h / 2 + border);
		_size.w = _handleSize.w;
		_size.h = std::min(_size.h, _railSize.h + _handleSize.h - border * 2);
	}

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
			pressed = true;
			float oldValue = value;
			if (orientation == SLIDER_HORIZONTAL) {
				value = ((float)(mousex - offX) / (railSize.w - border * 2)) * (float)(maxValue - minValue) + minValue;
			}
			else if (orientation == SLIDER_VERTICAL) {
				value = ((float)(mousey - offY) / (railSize.h - border * 2)) * (float)(maxValue - minValue) + minValue;
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

bool Slider::control() {
	if (!activated) {
		moveStartTime = ticks;
		return true;
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
			Uint32 sec = TICKS_PER_SECOND / 2;
			float inc = movePositive ? 1.f : -1.f;
			float ovalue = value;
			if (timeMoved < sec) {
				if (lastMove > sec / (10.f * valueSpeed)) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 2) {
				if (lastMove > sec / (20.f * valueSpeed)) {
					value += inc;
				}
			}
			else if (timeMoved < sec * 3) {
				if (lastMove > sec / (40.f * valueSpeed)) {
					value += inc;
				}
			}
			else {
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
	return false;
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

SDL_Rect Slider::getAbsoluteSize() const
{
	SDL_Rect _size{ handleSize.x, handleSize.y, handleSize.w, handleSize.h };
	auto _parent = static_cast<Frame*>(this->parent);
	if ( _parent ) {
		SDL_Rect absoluteSize = _parent->getAbsoluteSize();
		_size.x += std::max(0, absoluteSize.x);
		_size.y += std::max(0, absoluteSize.y);
	}
	return _size;
}