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
	Widget* result = nullptr;
	while (selected) {
		Input& input = Input::inputs[owner];

		// find search root
		Frame* root = nullptr;

		// tab to next element
		if (keystatus[SDL_SCANCODE_TAB]) {
			keystatus[SDL_SCANCODE_TAB] = 0;
			if (!widgetTab.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetTab.c_str(), true);
				if (result) {
					break;
				}
			}
		}

		// directional move to next element
		const char* moves[4][2] = {
			{ "MenuRight", widgetRight.c_str() },
			{ "MenuDown", widgetDown.c_str() },
			{ "MenuLeft", widgetLeft.c_str() },
			{ "MenuUp", widgetUp.c_str() }
		};
		for (int c = 0; c < sizeof(moves) / sizeof(moves[0]); ++c) {
			if (input.consumeBinaryToggle(moves[c][0])) {
				if (moves[c][1] && moves[c][1][0] != '\0') {
					root = root ? root : findSearchRoot();
					result = root->findWidget(moves[c][1], true);
					if (result) {
						break;
					}
				}
			}
		}

		// next tab
		if (input.consumeBinaryToggle("MenuPageRight")) {
			if (!widgetPageRight.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageRight.c_str(), true);
				if (result) {
					result->activate();
					break;
				}
			}
		}

		// previous tab
		if (input.consumeBinaryToggle("MenuPageLeft")) {
			if (!widgetPageLeft.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageLeft.c_str(), true);
				if (result) {
					result->activate();
					break;
				}
			}
		}

		// confirm selection
		if (input.consumeBinaryToggle("MenuConfirm")) {
			activate();
			break;
		}

		// cancel selection
		if (input.consumeBinaryToggle("MenuCancel")) {
			if (!widgetBack.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetBack.c_str(), true);
				if (result) {
					result->activate();
					break;
				}
			}
		}

		break;
	}
	return result;
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