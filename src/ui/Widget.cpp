// Widget.cpp

#include "../main.hpp"
#include "Widget.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "../input.hpp"
#include "../engine/audio/sound.hpp"

Widget::~Widget() {
	if (parent) {
		for (auto node = parent->widgets.begin(); node != parent->widgets.end(); ++node) {
			if (*node == this) {
				parent->widgets.erase(node);
				break;
			}
		}
	}
}

void Widget::select() {
	if (selected) {
		return;
	}
	Widget* head = findHead();
	if (head && head->getType() == WIDGET_FRAME) {
		Frame* f = static_cast<Frame*>(head);
		f->deselect(); // this deselects everything in the gui
	}
	selected = true;
}

void Widget::deselect() {
	selected = false;
}

void Widget::activate() {
	// no-op
}

void Widget::process() {
	if (!disabled) {
		if (tickCallback) {
			(*tickCallback)(*this);
		}
	}
}

Frame* Widget::findSearchRoot() {
	Widget* gui = findHead();
	if (gui && gui->getType() == WIDGET_FRAME) {
		if (widgetSearchParent.empty()) {
			return static_cast<Frame*>(gui);
		} else {
			auto search = gui->findWidget(widgetSearchParent.c_str(), true);
			if (search && search->getType() == WIDGET_FRAME) {
				return static_cast<Frame*>(search);
			} else {
				return static_cast<Frame*>(gui);
			}
		}
	} else {
		return nullptr;
	}
}

Widget* Widget::handleInput() {
	if (selected) {
		Input& input = Input::inputs[owner];

		// find search root
		Frame* root = nullptr;

		// move to another widget
		for (auto& move : widgetMovements) {
			if (input.consumeBinaryToggle(move.first.c_str())) {
				if (!move.second.empty()) {
					root = root ? root : findSearchRoot();
					Widget* result = root->findWidget(move.second.c_str(), true);
					if (result) {
						playSound(495, 64);
						result->scrollParent();
						return result;
					}
				}
			}
		}

		// move to another widget and activate it
		for (auto& action : widgetActions) {
			if (input.consumeBinaryToggle(action.first.c_str())) {
				if (!action.second.empty()) {
					root = root ? root : findSearchRoot();
					Widget* result = root->findWidget(action.second.c_str(), true);
					if (result) {
						result->activate();
						return nullptr;
					}
				}
			}
		}

		// activate current selection
		if (input.consumeBinaryToggle("MenuConfirm")) {
			activate();
			return nullptr;
		}
	}
	return nullptr;
}

Widget* Widget::findHead() {
    if (parent && parent->owner == owner) {
        return parent->findHead();
    } else {
        return this;
    }
}

Widget* Widget::findWidget(const char* name, bool recursive) {
	for (auto widget : widgets) {
		if (widget->owner != owner) {
			continue;
		}
		if (widget->name == name) {
			return widget;
		} else if (recursive) {
			auto result = widget->findWidget(name, recursive);
			if (result) {
				return result;
			}
		}
	}
	return nullptr;
}

void Widget::findSelectedWidgets(std::vector<Widget*>& outResult) {
	if (selected) {
		outResult.push_back(this);
	}
	for (auto widget : widgets) {
		widget->findSelectedWidgets(outResult);
	}
}

Widget* Widget::findSelectedWidget(int owner) {
	if (selected && owner == this->owner) {
		return this;
	} else {
		std::vector<Widget*> selectedWidgets;
		findSelectedWidgets(selectedWidgets);
		for (auto widget : selectedWidgets) {
			if (widget->owner == owner) {
				return widget;
			}
		}
	}
	return nullptr;
}

bool Widget::isChildOf(Widget& widget) {
	if (!parent) {
		return false;
	}
	else if (parent == &widget) {
		return true;
	}
	else {
		return parent->isChildOf(widget);
	}
}

void Widget::adoptWidget(Widget& widget) {
	if (widget.parent) {
		for (auto node = widget.parent->widgets.begin(); node != widget.parent->widgets.end(); ++node) {
			if (*node == &widget) {
				widget.parent->widgets.erase(node);
				break;
			}
		}
	}
	widget.owner = owner;
	widget.parent = this;
	widget.setOwner(this->getOwner());
	widgets.push_back(&widget);
}

void Widget::drawGlyphs(const SDL_Rect size, const std::vector<Widget*>& selectedWidgets) {
	if (drawCallback) {
		drawCallback(*this, size);
	}

#ifdef NINTENDO
	if (hideGlyphs) {
		return;
	}
	Widget* selectedWidget = nullptr;
	for (auto widget : selectedWidgets) {
		if (widget->owner == owner) {
			selectedWidget = widget;
			break;
		}
	}
	if (!selectedWidget) {
		return;
	} else {
		auto searchParent = selectedWidget->findSearchRoot();
		if (searchParent && !isChildOf(*searchParent)) {
			return;
		}
	}
	int x = size.x + size.w;
	int y = size.y + size.h;
	auto& actions = selectedWidget->getWidgetActions();
	auto action = actions.begin();
	if (selectedWidget == this) {
		auto image = Image::get("images/ui/Glyphs/G_Switch_A00.png");
		int w = image->getWidth();
		int h = image->getHeight();
		image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
		x -= w;
	}
	if ((action = actions.find("MenuCancel")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_B00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuAlt1")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_Y00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuAlt2")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_X00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuStart")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_+00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuSelect")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_-00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuPageLeft")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_L00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
	if ((action = actions.find("MenuPageRight")) != actions.end()) {
		if (action->second == name) {
			auto image = Image::get("images/ui/Glyphs/G_Switch_R00.png");
			int w = image->getWidth();
			int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h});
			x -= w;
		}
	}
#endif
}

void Widget::scrollParent() {
	// no-op
}