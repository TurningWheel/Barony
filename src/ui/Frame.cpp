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

static const Uint32 tooltip_background = 0x000000EE;
static const Uint32 tooltip_border_color = 0xFF8800FF;
static const int tooltip_border_width = 3;
static const Uint32 tooltip_text_color = 0xFFFFFFFF;
static const char* tooltip_text_font = Font::defaultFont;

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

Frame::entry_t::~entry_t() {
	if (listener) {
		listener->entry = nullptr;
	}
	if (click) {
		delete click;
		click = nullptr;
	}
	if (ctrlClick) {
		delete ctrlClick;
		ctrlClick = nullptr;
	}
	if (highlighting) {
		delete highlighting;
		highlighting = nullptr;
	}
	if (highlight) {
		delete highlight;
		highlight = nullptr;
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
	Frame::draw(size, actualSize);
}

void Frame::draw(SDL_Rect _size, SDL_Rect _actualSize) {
	if (disabled)
		return;

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	if (size.h < actualSize.h) {
		_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	} else {
		_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	}
	if (size.w < actualSize.w) {
		_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	} else {
		_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	}
	if (_size.w <= 0 || _size.h <= 0)
		return;

	// draw frame background
	if (!hollow) {
		SDL_Rect scaledSize;
		scaledSize.x = _size.x * (float)xres / (float)Frame::virtualScreenX;
		scaledSize.y = _size.y * (float)yres / (float)Frame::virtualScreenY;
		scaledSize.w = _size.w * (float)xres / (float)Frame::virtualScreenX;
		scaledSize.h = _size.h * (float)yres / (float)Frame::virtualScreenY;
		drawRect(&scaledSize, color, (Uint8)(color>>mainsurface->format->Ashift));
	}

	Sint32 mousex = (mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (omousey / (float)yres) * (float)Frame::virtualScreenY;

	// horizontal slider
	if (actualSize.w > size.w) {

		// slider rail
		SDL_Rect barRect;
		barRect.x = (_size.x) * (float)xres / (float)Frame::virtualScreenX;
		barRect.y = (_size.y + _size.h) * (float)yres / (float)Frame::virtualScreenY;
		barRect.w = (_size.w) * (float)xres / (float)Frame::virtualScreenX;
		barRect.h = (sliderSize) * (float)yres / (float)Frame::virtualScreenY;
		drawDepressed(barRect.x, barRect.y, barRect.x + barRect.w, barRect.y + barRect.h);

		// handle
		float winFactor = ((float)_size.w / (float)actualSize.w);
		int handleSize = std::max((int)(size.w * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.x;

		SDL_Rect handleRect;
		handleRect.x = _size.x + sliderPos;
		handleRect.y = _size.y + _size.h;
		handleRect.w = handleSize;
		handleRect.h = sliderSize;

		int x = (handleRect.x) * (float)xres / (float)Frame::virtualScreenX;
		int y = (handleRect.x) * (float)yres / (float)Frame::virtualScreenY;
		int w = (handleRect.x + handleRect.w) * (float)xres / (float)Frame::virtualScreenX;
		int h = (handleRect.x + handleRect.h) * (float)yres / (float)Frame::virtualScreenY;
		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			drawWindow(x, y, w, h);
		} else {
			drawWindow(x, y, w, h);
		}
	}

	// vertical slider
	if (actualSize.h > size.h && _size.y) {
		SDL_Rect barRect;
		barRect.x = (_size.x + _size.w) * (float)xres / (float)Frame::virtualScreenX;
		barRect.y = (_size.y) * (float)yres / (float)Frame::virtualScreenY;
		barRect.w = (sliderSize) * (float)xres / (float)Frame::virtualScreenX;
		barRect.h = (_size.h) * (float)yres / (float)Frame::virtualScreenY;
		drawRect(&barRect, color, (Uint8)(color>>mainsurface->format->Ashift));

		// handle
		float winFactor = ((float)_size.h / (float)actualSize.h);
		int handleSize = std::max((int)(size.h * winFactor), sliderSize);
		int sliderPos = winFactor * actualSize.y;

		SDL_Rect handleRect;
		handleRect.x = _size.x + _size.w;
		handleRect.y = _size.y + sliderPos;
		handleRect.w = sliderSize;
		handleRect.h = handleSize;

		int x = (handleRect.x) * (float)xres / (float)Frame::virtualScreenX;
		int y = (handleRect.x) * (float)yres / (float)Frame::virtualScreenY;
		int w = (handleRect.x + handleRect.w) * (float)xres / (float)Frame::virtualScreenX;
		int h = (handleRect.x + handleRect.h) * (float)yres / (float)Frame::virtualScreenY;
		if (rectContainsPoint(barRect, omousex, omousey)) {
			// TODO highlight
			drawWindow(x, y, w, h);
		} else {
			drawWindow(x, y, w, h);
		}
	}

	// slider filler (at the corner between sliders)
	if (actualSize.w > size.w && actualSize.h > size.h) {
		SDL_Rect barRect;
		barRect.x = (_size.x + _size.w) * (float)xres / (float)Frame::virtualScreenX;
		barRect.y = (_size.y + _size.h) * (float)yres / (float)Frame::virtualScreenY;
		barRect.w = (sliderSize) * (float)xres / (float)Frame::virtualScreenX;
		barRect.h = (sliderSize) * (float)yres / (float)Frame::virtualScreenY;
		// TODO different border styles
		if (border > 0) {
			switch (borderStyle) {
			case BORDER_FLAT:
				drawRect(&barRect, color, (Uint8)(color>>mainsurface->format->Ashift));
				break;
			case BORDER_BEVEL_HIGH:
				drawRect(&barRect, color, (Uint8)(color>>mainsurface->format->Ashift));
				break;
			case BORDER_BEVEL_LOW:
				drawRect(&barRect, color, (Uint8)(color>>mainsurface->format->Ashift));
				break;
			}
		} else {
			drawRect(&barRect, color, (Uint8)(color>>mainsurface->format->Ashift));
		}
	}

	SDL_Rect scroll = actualSize;
	if (size.x - _actualSize.x < 0) {
		scroll.x -= size.x - _actualSize.x;
	}
	if (size.y - _actualSize.y < 0) {
		scroll.y -= size.y - _actualSize.y;
	}

	// render images
	for (auto image : images) {
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
			scaledDest.x = dest.x * (float)xres / (float)Frame::virtualScreenX;
			scaledDest.y = dest.y * (float)yres / (float)Frame::virtualScreenY;
			scaledDest.w = dest.w * (float)xres / (float)Frame::virtualScreenX;
			scaledDest.h = dest.h * (float)yres / (float)Frame::virtualScreenY;

			SDL_Rect src;
			src.x = std::max(0, _size.x - pos.x);
			src.y = std::max(0, _size.y - pos.y);
			src.w = pos.w - (dest.x - pos.x) - std::max(0, (pos.x + pos.w) - (_size.x + _size.w));
			src.h = pos.h - (dest.y - pos.y) - std::max(0, (pos.y + pos.h) - (_size.y + _size.h));

			actualImage->drawColor(&src, scaledDest, image->color);
		}
	}

	// render list entries
	int listStart = std::min(std::max(0, scroll.y / entrySize), (int)list.size() - 1);
	int i = listStart;
	for (auto it = std::next(list.begin(), listStart); it != list.end(); ++it, ++i) {
		entry_t& entry = **it;
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
		int textSizeH = entrySize;

		SDL_Rect pos;
		pos.x = _size.x + border - scroll.x;
		pos.y = _size.y + border + i * entrySize - scroll.y;
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
			break;

		// TODO entry highlighting
		SDL_Rect entryback = dest;
		entryback.w = _size.w - border * 2;
		
		entryback.x = entryback.x * (float)xres / (float)Frame::virtualScreenX;
		entryback.y = entryback.y * (float)yres / (float)Frame::virtualScreenY;
		entryback.w = entryback.w * (float)xres / (float)Frame::virtualScreenX;
		entryback.h = entryback.h * (float)yres / (float)Frame::virtualScreenY;
		if (entry.pressed) {
			drawRect(&entryback, color, (Uint8)(color>>mainsurface->format->Ashift));
		} else if (entry.highlighted) {
			drawRect(&entryback, color, (Uint8)(color>>mainsurface->format->Ashift));
		} else if (selection == it) {
			drawRect(&entryback, color, (Uint8)(color>>mainsurface->format->Ashift));
		}

		SDL_Rect scaledDest;
		scaledDest.x = dest.x * (float)xres / (float)Frame::virtualScreenX;
		scaledDest.y = dest.y * (float)yres / (float)Frame::virtualScreenY;
		scaledDest.w = dest.w * (float)xres / (float)Frame::virtualScreenX;
		scaledDest.h = dest.h * (float)yres / (float)Frame::virtualScreenY;
		text->drawColor(src, scaledDest, entry.color);
	}

	// render fields
	for (auto field : fields) {
		field->draw(_size, scroll);
	}

	// draw buttons
	for (auto button : buttons) {
		button->draw(_size, scroll);
	}

	// draw sliders
	for (auto slider : sliders) {
		slider->draw(_size, scroll);
	}

	// draw subframes
	for (auto frame : frames) {
		frame->draw(_size, scroll);
	}

	// root frame draws tooltip
	if (!parent) {
		if (tooltip && tooltip[0] != '\0') {
			Font* font = Font::get(tooltip_text_font);
			if (font) {
				Text* text = Text::get(tooltip, font->getName());
				SDL_Rect src;
				src.x = mousex + 20 * ((float)Frame::virtualScreenX / xres);
				src.y = mousey;
				src.w = text->getWidth() + 2;
				src.h = text->getHeight() + 2;

				int border = tooltip_border_width;

				src.x = src.x * (float)xres / (float)Frame::virtualScreenX;
				src.y = src.y * (float)yres / (float)Frame::virtualScreenY;
				src.w = src.w * (float)xres / (float)Frame::virtualScreenX;
				src.h = src.h * (float)yres / (float)Frame::virtualScreenY;
				drawRect(&src, tooltip_border_color, (Uint8)(tooltip_border_color>>mainsurface->format->Ashift));

				SDL_Rect src2{src.x + border, src.y + border, src.w - border * 2, src.h - border * 2};
				src2.x = src2.x * (float)xres / (float)Frame::virtualScreenX;
				src2.y = src2.y * (float)yres / (float)Frame::virtualScreenY;
				src2.w = src2.w * (float)xres / (float)Frame::virtualScreenX;
				src2.h = src2.h * (float)yres / (float)Frame::virtualScreenY;
				drawRect(&src2, tooltip_background, (Uint8)(tooltip_background>>mainsurface->format->Ashift));

				text->drawColor(SDL_Rect{0,0,0,0}, SDL_Rect{src.x + 1, src.y + 1, 0, 0}, tooltip_text_color);
			}
		}
	}
}

Frame::result_t Frame::process() {
	result_t result = process(size, actualSize, true);

	tooltip = nullptr;
	if (result.tooltip && result.tooltip[0] != '\0') {
		if (SDL_GetTicks() - result.highlightTime >= tooltipTime) {
			tooltip = result.tooltip;
		}
	}
	postprocess();

	return result;
}

Frame::result_t Frame::process(SDL_Rect _size, SDL_Rect _actualSize, bool usable) {
	result_t result;
	result.removed = false;
	result.usable = usable;
	result.highlightTime = SDL_GetTicks();
	result.tooltip = nullptr;

	if (disabled) {
		return result;
	}
	if (shootmode) {
		return result;
	}

	_size.x += std::max(0, size.x - _actualSize.x);
	_size.y += std::max(0, size.y - _actualSize.y);
	if (size.h < actualSize.h) {
		_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	} else {
		_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
	}
	if (size.w < actualSize.w) {
		_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	} else {
		_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
	}
	if (_size.w <= 0 || _size.h <= 0)
		return result;

	SDL_Rect fullSize = _size;
	fullSize.h += (actualSize.w > size.w) ? sliderSize : 0;
	fullSize.w += (actualSize.h > size.h) ? sliderSize : 0;

	Sint32 mousex = (mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (omousey / (float)yres) * (float)Frame::virtualScreenY;

	if (selected) {
		// unselect list
		if (keystatus[SDL_SCANCODE_ESCAPE]) {
			keystatus[SDL_SCANCODE_ESCAPE] = 0;
			deselect();
			if (!widgetBack.empty()) {
				Frame* root = findSearchRoot(); assert(root);
				Widget* search = root->findWidget(widgetBack.c_str(), true);
				if (search) {
					search->select();
				}
			}
			if (dropDown) {
				toBeDeleted = true;
			}
		}

		// activate selection
		if (keystatus[SDL_SCANCODE_RETURN]) {
			keystatus[SDL_SCANCODE_RETURN] = 0;
			if (selection != list.end()) {
				activateEntry(**selection);
			}
			if (dropDown) {
				if (!widgetBack.empty()) {
					Frame* root = findSearchRoot(); assert(root);
					Widget* search = root->findWidget(widgetBack.c_str(), true);
					if (search) {
						search->select();
					}
				}
				toBeDeleted = true;
			}
		}

		// choose a selection
		if (selection == list.end()) {
			if (keystatus[SDL_SCANCODE_UP] || 
				keystatus[SDL_SCANCODE_DOWN]) {
				keystatus[SDL_SCANCODE_UP] = 0;
				keystatus[SDL_SCANCODE_DOWN] = 0;
				selection = list.begin();
				scrollToSelection();
			}
		} else {
			if (keystatus[SDL_SCANCODE_UP]) {
				keystatus[SDL_SCANCODE_UP] = 0;
				selection = selection == list.begin() ? list.begin() : --selection;
				scrollToSelection();
			}
			if (keystatus[SDL_SCANCODE_DOWN]) {
				keystatus[SDL_SCANCODE_DOWN] = 0;
				selection = selection == list.end() ? list.end() : ++selection;
				scrollToSelection();
			}
		}
	}

	// scroll with mouse wheel
	if (parent != nullptr && !hollow && rectContainsPoint(fullSize, omousex, omousey) && usable) {
		// x scroll with mouse wheel
		if (actualSize.w > size.w) {
			if (mousestatus[SDL_BUTTON_X1]) {
				actualSize.x += std::min(entrySize * 4, size.w);
				usable = result.usable = false;
			} else if (mousestatus[SDL_BUTTON_X2]) {
				actualSize.x -= std::min(entrySize * 4, size.w);
				usable = result.usable = false;
			}
			if (actualSize.h <= size.h) {
				if (mousestatus[SDL_BUTTON_WHEELDOWN]) {
					actualSize.x += std::min(entrySize * 4, size.w);
					usable = result.usable = false;
				} else if (mousestatus[SDL_BUTTON_WHEELUP]) {
					actualSize.x -= std::min(entrySize * 4, size.w);
					usable = result.usable = false;
				}
			}
		}

		// y scroll with mouse wheel
		if (actualSize.h > size.h) {
			if (mousestatus[SDL_BUTTON_WHEELDOWN]) {
				actualSize.y += std::min(entrySize * 4, size.h);
				usable = result.usable = false;
			} else if (mousestatus[SDL_BUTTON_WHEELUP]) {
				actualSize.y -= std::min(entrySize * 4, size.h);
				usable = result.usable = false;
			}
		}

		// bound
		actualSize.x = std::min(std::max(0, actualSize.x), std::max(0, actualSize.w - size.w));
		actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
	}

	// widget to move to after processing inputs
	Widget* destWidget = nullptr;

	// process frames
	{
		auto prev = frames.rbegin();
		for (auto it = frames.rbegin(); it != frames.rend(); it = prev) {
			Frame* frame = *it;
			prev = std::next(it);

			result_t frameResult = frame->process(_size, actualSize, usable);
			usable = result.usable = frameResult.usable;
			if (!frameResult.removed) {
				if (frameResult.tooltip != nullptr) {
					result = frameResult;
				}
			} else {
				delete frame;
				auto b = it.base(); ++b;
				frames.erase(b);
			}
		}
	}

	// process (frame view) sliders
	if (parent != nullptr && !hollow && usable) {
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
					float winFactor = ((float)_size.w / (float)actualSize.w);
					actualSize.x = (mousex - omousex) / winFactor + oldSliderX;
					actualSize.x = std::min(std::max(0, actualSize.x), std::max(0, actualSize.w - size.w));
				}
				usable = result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if (rectContainsPoint(handleRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						draggingHSlider = true;
						oldSliderX = actualSize.x;
					}
					usable = result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				} else if (rectContainsPoint(sliderRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						actualSize.x += omousex < handleRect.x ? -std::min(entrySize * 4, size.w) : std::min(entrySize * 4, size.w);
						actualSize.x = std::min(std::max(0, actualSize.x), std::max(0, actualSize.w - size.w));
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
					float winFactor = ((float)_size.h / (float)actualSize.h);
					actualSize.y = (mousey - omousey) / winFactor + oldSliderY;
					actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
				}
				usable = result.usable = false;
				ticks = -1; // hack to fix sliders in drop downs
			} else {
				if (rectContainsPoint(handleRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						draggingVSlider = true;
						oldSliderY = actualSize.y;
					}
					usable = result.usable = false;
					ticks = -1; // hack to fix sliders in drop downs
				} else if (rectContainsPoint(sliderRect, omousex, omousey)) {
					if (mousestatus[SDL_BUTTON_LEFT]) {
						actualSize.y += omousey < handleRect.y ? -std::min(entrySize * 4, size.h) : std::min(entrySize * 4, size.h);
						actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
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
		auto prev = buttons.rbegin();
		for (auto it = buttons.rbegin(); it != buttons.rend(); it = prev) {
			Button* button = *it;
			prev = std::next(it);

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
		auto prev = sliders.rbegin();
		for (auto it = sliders.rbegin(); it != sliders.rend(); it = prev) {
			Slider* slider = *it;
			prev = std::next(it);

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
		int num = 0;
		entry_t* prev = nullptr;
		auto next = list.begin();
		for (auto it = list.begin(); it != list.end(); it = next) {
			next = std::next(it);

			entry_t* entry = *it;
			if (entry->suicide) {
				if (selection == it) {
					selection = selection == list.begin() ? list.begin() : --selection;
				}
				delete entry;
				list.erase(it);
				continue;
			}

			SDL_Rect entryRect;
			entryRect.x = _size.x + border - actualSize.x; entryRect.w = _size.w - border * 2;
			entryRect.y = _size.y + border + num * entrySize - actualSize.y; entryRect.h = entrySize;

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
					Widget::Args args(entry->params);
					if (entry->highlighting) {
						(*entry->highlighting)(args);
					}
					if (!entry->highlighted) {
						entry->highlighted = true;
						if (entry->highlight) {
							(*entry->highlight)(args);
						}
					}
				}
				result.usable = usable = false;
			} else {
				entry->highlightTime = SDL_GetTicks();
				entry->highlighted = false;
				entry->pressed = false;
			}

			++num;
			prev = entry;
		}
	}

	// process fields
	{
		auto prev = fields.rbegin();
		for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
			Field* field = *it;
			prev = std::next(it);

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
				Widget::Args args(field->getParams());
				args.addString(field->getText());
				if (field->getCallback()) {
					(*field->getCallback())(args);
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

Frame::image_t* Frame::addImage(const SDL_Rect& pos, const Uint32& color, const char* image, const char* name) {
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
	entry->image = nullptr;
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
	selection = list.end();
}

void Frame::clearEntries() {
	while (list.size()) {
		delete list.front();
		list.erase(list.begin());
	}
	selection = list.end();
}

bool Frame::remove(const char* name) {
	for (auto it = frames.begin(); it != frames.end(); ++it) {
		Frame* frame = *it;
		if (strcmp(frame->getName(), name) == 0) {
			delete frame;
			frames.erase(it);
			return true;
		}
	}
	for (auto it = buttons.begin(); it != buttons.end(); ++it) {
		Button* button = *it;
		if (strcmp(button->getName(), name) == 0) {
			delete button;
			buttons.erase(it);
			return true;
		}
	}
	for (auto it = fields.begin(); it != fields.end(); ++it) {
		Field* field = *it;
		if (strcmp(field->getName(), name) == 0) {
			delete field;
			fields.erase(it);
			return true;
		}
	}
	for (auto it = images.begin(); it != images.end(); ++it) {
		image_t* image = *it;
		if (strcmp(image->name.c_str(), name) == 0) {
			delete image;
			images.erase(it);
			return true;
		}
	}
	for (auto it = sliders.begin(); it != sliders.end(); ++it) {
		Slider* slider = *it;
		if (strcmp(slider->getName(), name) == 0) {
			delete slider;
			sliders.erase(it);
			return true;
		}
	}
	return false;
}

bool Frame::removeEntry(const char* name, bool resizeFrame) {
	for (auto it = list.begin(); it != list.end(); ++it) {
		entry_t* entry = *it;
		if (entry->name == name) {
			if (selection == it) {
				selection = selection == list.begin() ? list.begin() : --selection;
			}
			delete entry;
			list.erase(it);
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
	actualSize.w = size.w;
	actualSize.h = (Uint32)list.size() * entrySize;
	actualSize.y = std::min(std::max(0, actualSize.y), std::max(0, actualSize.h - size.h));
}

bool Frame::capturesMouse(SDL_Rect* curSize, SDL_Rect* curActualSize) {
	int xres = xres;
	int yres = yres;
	SDL_Rect newSize = SDL_Rect{0, 0, xres, yres};
	SDL_Rect newActualSize = SDL_Rect{0, 0, xres, yres};
	SDL_Rect& _size = curSize ? *curSize : newSize;
	SDL_Rect& _actualSize = curActualSize ? *curActualSize : newActualSize;

	if (parent) {
		auto pframe = static_cast<Frame*>(parent);
		if (pframe->capturesMouse(&_size, &_actualSize)) {
			_size.x += std::max(0, size.x - _actualSize.x);
			_size.y += std::max(0, size.y - _actualSize.y);
			if (size.h < actualSize.h) {
				_size.w = std::min(size.w - sliderSize, _size.w - sliderSize - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			} else {
				_size.w = std::min(size.w, _size.w - size.x + _actualSize.x) + std::min(0, size.x - _actualSize.x);
			}
			if (size.w < actualSize.w) {
				_size.h = std::min(size.h - sliderSize, _size.h - sliderSize - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			} else {
				_size.h = std::min(size.h, _size.h - size.y + _actualSize.y) + std::min(0, size.y - _actualSize.y);
			}
			if (_size.w <= 0 || _size.h <= 0) {
				return false;
			} else {
				Sint32 omousex = (omousex / (float)xres) * (float)Frame::virtualScreenX;
				Sint32 omousey = (omousey / (float)yres) * (float)Frame::virtualScreenY;
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
}

Frame* Frame::getParent() {
	if (parent && parent->getType() == WIDGET_FRAME) {
		return static_cast<Frame*>(parent);
	} else {
		return nullptr;
	}
}

void Frame::deselect() {
	selected = false;
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

void Frame::setSelection(int index) {
	selection = std::next(list.begin(), index);
}

void Frame::scrollToSelection() {
	if (selection == list.end()) {
		return;
	}
	int index = 0;
	for (auto entry : list) {
		if (entry == *selection) {
			break;
		}
		++index;
	}
	if (actualSize.y > index * entrySize) {
		actualSize.y = index * entrySize;
	}
	if (actualSize.y + size.h < (index + 1) * entrySize) {
		actualSize.y = (index + 1) * entrySize - size.h;
	}
}

void Frame::activateEntry(entry_t& entry) {
	Widget::Args args(entry.params);
	if (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) {
		if (entry.ctrlClick) {
			(*entry.ctrlClick)(args);
		}
	} else {
		if (entry.click) {
			(*entry.click)(args);
		}
	}
}