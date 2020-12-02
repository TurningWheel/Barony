// Button.cpp

#include "../main.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"
#include "Text.hpp"

Button::Button() {
	size.x = 0; size.w = 32;
	size.y = 0; size.h = 32;
	color = WideVector(.5f, .5f, .5f, 1.f);
	textColor = WideVector(1.f);
}

Button::Button(Frame& _parent) : Button() {
	parent = &_parent;
	_parent.getButtons().addNodeLast(this);
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
	Script::Args args(params);
	if (callback) {
		(*callback)(args);
	} else if (parent) {
		Frame* fparent = static_cast<Frame*>(parent);
		Script* script = fparent->getScript();
		if (script) {
			script->dispatch(name.get(), &args);
		}
	} else {
		mainEngine->fmsg(Engine::MSG_ERROR, "button clicked with no callback (script or otherwise)");
	}
}

void Button::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	_size.x += max(0, size.x - _actualSize.x);
	_size.y += max(0, size.y - _actualSize.y);
	_size.w = min(size.w, _size.w - size.x + _actualSize.x) + min(0, size.x - _actualSize.x);
	_size.h = min(size.h, _size.h - size.y + _actualSize.y) + min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0)
		return;

	glm::vec4 _color = disabled ? color * .5f : (highlighted ? color * 1.5f : color);
	glm::vec4 _borderColor = _color;
	if (borderColor.w) {
		_borderColor = disabled ? borderColor * .5f : (highlighted ? borderColor * 1.5f : borderColor);
	}
	if (selected) {
		_color *= 1.25f;
		_borderColor *= 1.25f;
	}
	if (border) {
		if (pressed) {
			renderer.drawFrame(_size, border, _color);
			renderer.drawLowFrame(_size, border, _borderColor, true);
		} else {
			renderer.drawFrame(_size, border, _color);
			renderer.drawHighFrame(_size, border, _borderColor, true);
		}
	} else {
		if (pressed) {
			renderer.drawFrame(_size, border, selected ? color * 1.25f * .9f : color * .9f);
			renderer.drawFrame(_size, 2, _borderColor * .9f, true);
		} else {
			renderer.drawFrame(_size, border, selected ? color * 1.25f : color);
			renderer.drawFrame(_size, 2, _borderColor, true);
		}
	}

	if (!text.empty() && style != STYLE_CHECKBOX) {
		Text* _text = Text::get(text.get(), font.get());
		if (_text) {
			Rect<int> pos;
			int textX = style == STYLE_DROPDOWN ? 5 + border : _size.w / 2 - _text->getWidth() / 2;
			int textY = _size.h / 2 - _text->getHeight() / 2;
			pos.x = _size.x + textX; pos.w = min((int)_text->getWidth(), _size.w);
			pos.y = _size.y + textY; pos.h = min((int)_text->getHeight(), _size.h);
			if (pos.w <= 0 || pos.h <= 0) {
				return;
			}
			_text->drawColor(Rect<int>(), pos, textColor);
		}
	} else if (icon.get()) {
		// we check a second time, just incase the cache was dumped and the original pointer invalidated.
		Image* iconImg = mainEngine->getImageResource().dataForString(icon.get());
		if (iconImg) {
			if (style != STYLE_CHECKBOX || pressed == true) {
				Rect<int> pos;
				pos.x = _size.x + border; pos.w = _size.w - border * 2;
				pos.y = _size.y + border; pos.h = _size.h - border * 2;
				if (pos.w <= 0 || pos.h <= 0) {
					return;
				}

				float w = iconImg->getWidth();
				float h = iconImg->getHeight();

				Rect<Sint32> section;
				section.x = size.x - _actualSize.x < 0 ? -(size.x - _actualSize.x) * (w / (size.w - border * 2)) : 0;
				section.y = size.y - _actualSize.y < 0 ? -(size.y - _actualSize.y) * (h / (size.h - border * 2)) : 0;
				section.w = ((float)pos.w / (size.w - border * 2)) * w;
				section.h = ((float)pos.h / (size.h - border * 2)) * h;

				iconImg->draw(&section, pos);
			}
		}
	}

	// drop down buttons have an image on the right side (presumably a down arrow)
	if (style == STYLE_DROPDOWN) {
		Image* iconImg = mainEngine->getImageResource().dataForString(icon.get());
		if (iconImg) {
			Rect<int> pos;
			pos.y = _size.y + border; pos.h = _size.h - border * 2;
			pos.w = pos.h; pos.x = _size.x + _size.w - border - pos.w;
			if (pos.w <= 0 || pos.h <= 0) {
				return;
			}

			float w = iconImg->getWidth();
			float h = iconImg->getHeight();

			// TODO scale the drop-down image
			Rect<Sint32> section;
			section.x = 0;
			section.y = size.y - _actualSize.y < 0 ? -(size.y - _actualSize.y) * (h / (size.h - border * 2)) : 0;
			section.w = ((float)pos.w / (size.h - border * 2)) * w;
			section.h = ((float)pos.h / (size.h - border * 2)) * h;

			iconImg->draw(&section, pos);
		}
	}
}

Button::result_t Button::process(Rect<int> _size, Rect<int> _actualSize, const bool usable) {
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

	_size.x += max(0, size.x - _actualSize.x);
	_size.y += max(0, size.y - _actualSize.y);
	_size.w = min(size.w, _size.w - size.x + _actualSize.x) + min(0, size.x - _actualSize.x);
	_size.h = min(size.h, _size.h - size.y + _actualSize.y) + min(0, size.y - _actualSize.y);
	if (_size.w <= 0 || _size.h <= 0) {
		highlightTime = result.highlightTime;
		return result;
	}

	Sint32 mousex = (mainEngine->getMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
	Sint32 mousey = (mainEngine->getMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;
	Sint32 omousex = (mainEngine->getOldMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
	Sint32 omousey = (mainEngine->getOldMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;

	if (_size.containsPoint(omousex, omousey)) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.get();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}

	result.clicked = false;
	if (highlighted) {
		if (mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
			select();
			if (_size.containsPoint(mousex, mousey)) {
				result.pressed = pressed = (reallyPressed == false);
			} else {
				pressed = reallyPressed;
			}
		} else {
			if (pressed != reallyPressed) {
				result.clicked = true;
			}
			pressed = reallyPressed;
		}
	} else {
		pressed = reallyPressed;
	}

	return result;
}