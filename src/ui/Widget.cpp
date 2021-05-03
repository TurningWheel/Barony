// Widget.cpp

#include "../main.hpp"
#include "Widget.hpp"
#include "Frame.hpp"
#include "../input.hpp"

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
		if (callback) {
			(*callback)(*this);
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
		const char* moves[][2] = {
			{ "MenuTab", widgetTab.c_str() },
			{ "MenuRight", widgetRight.c_str() },
			{ "MenuDown", widgetDown.c_str() },
			{ "MenuLeft", widgetLeft.c_str() },
			{ "MenuUp", widgetUp.c_str() },
			{ "AltMenuRight", widgetRight.c_str() },
			{ "AltMenuDown", widgetDown.c_str() },
			{ "AltMenuLeft", widgetLeft.c_str() },
			{ "AltMenuUp", widgetUp.c_str() }
		};
		for (int c = 0; c < sizeof(moves) / sizeof(moves[0]); ++c) {
			if (input.consumeBinaryToggle(moves[c][0])) {
				if (moves[c][1] && moves[c][1][0] != '\0') {
					root = root ? root : findSearchRoot();
					Widget* result = root->findWidget(moves[c][1], true);
					if (result) {
						return result;
					}
				}
			}
		}

		// move to another widget and activate it
		const char* actions[][2] = {
			{ "MenuPageRight", widgetPageRight.c_str() },
			{ "MenuPageLeft", widgetPageLeft.c_str() },
			{ "MenuCancel", widgetBack.c_str() },
		};
		for (int c = 0; c < sizeof(actions) / sizeof(actions[0]); ++c) {
			if (input.consumeBinaryToggle(actions[c][0])) {
				if (actions[c][1] && actions[c][1][0] != '\0') {
					root = root ? root : findSearchRoot();
					Widget* result = root->findWidget(actions[c][1], true);
					if (result) {
						result->activate();
						return result;
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

void Widget::adoptWidget(Widget& widget) {
	if (widget.parent) {
		for (auto node = widget.parent->widgets.begin(); node != widget.parent->widgets.end(); ++node) {
			if (*node == &widget) {
				widget.parent->widgets.erase(node);
				break;
			}
		}
	}
	widget.parent = this;
	widgets.push_back(&widget);
}