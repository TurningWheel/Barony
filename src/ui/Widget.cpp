// Widget.cpp

#include "../main.hpp"
#include "Widget.hpp"
#include "Frame.hpp"

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

		// find search root
		Frame* root = nullptr;

		// tab to next element
		if (keystatus[SDL_SCANCODE_TAB]) {
			if (!widgetTab.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetTab.c_str(), true);
				if (result) {
					break;
				}
			}
		}

		// directional move to next element
		struct move {
			int scancode;
			const char* widget;
		};
		move moves[4] = {
			{ SDL_SCANCODE_RIGHT, widgetRight.c_str() },
			{ SDL_SCANCODE_DOWN, widgetDown.c_str() },
			{ SDL_SCANCODE_LEFT, widgetLeft.c_str() },
			{ SDL_SCANCODE_UP, widgetUp.c_str() }
		};
		for (int c = 0; c < sizeof(moves) / sizeof(moves[0]); ++c) {
			if (keystatus[moves[c].scancode]) {
				if (moves[c].widget && moves[c].widget[0] != '\0') {
					root = root ? root : findSearchRoot();
					result = root->findWidget(moves[c].widget, true);
					if (result) {
						keystatus[moves[c].scancode] = 0;
						break;
					}
				}
			}
		}

		// next tab
		if (keystatus[SDL_SCANCODE_RIGHTBRACKET]) {
			if (!widgetPageRight.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageRight.c_str(), true);
				if (result) {
					keystatus[SDL_SCANCODE_RIGHTBRACKET] = 0;
					result->activate();
					break;
				}
			}
		}

		// previous tab
		if (keystatus[SDL_SCANCODE_LEFTBRACKET]) {
			if (!widgetPageLeft.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageLeft.c_str(), true);
				if (result) {
					keystatus[SDL_SCANCODE_LEFTBRACKET] = 0;
					result->activate();
					break;
				}
			}
		}

		// confirm selection
		if (keystatus[SDL_SCANCODE_RETURN]) {
			keystatus[SDL_SCANCODE_RETURN] = 0;
			activate();
			break;
		}

		// cancel selection
		if (keystatus[SDL_SCANCODE_ESCAPE]) {
			if (!widgetBack.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetBack.c_str(), true);
				if (result) {
					keystatus[SDL_SCANCODE_ESCAPE] = 0;
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