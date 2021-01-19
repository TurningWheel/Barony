//! @file Button.hpp

#pragma once

#include "../main.hpp"
#include "Widget.hpp"
#include "Font.hpp"

class Frame;

static inline bool rectContainsPoint(SDL_Rect r, int x, int y) {
	return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

//! A Button lives in a Frame and can have scripted actions or a native callback.
class Button : public Widget {
public:
	Button();
	Button(Frame& _parent);
	Button(const Button&) = delete;
	Button(Button&&) = delete;
	virtual ~Button();

	Button& operator=(const Button&) = delete;
	Button& operator=(Button&&) = delete;

	//! the result of the button process
	struct result_t {
		bool highlighted;				//!< was highlighted this frame
		bool pressed;					//!< was pressed this frame
		bool clicked;					//!< was activated this frame
		Uint32 highlightTime;			//!< time since button was highlighted
		const char* tooltip = nullptr;	//!< button tooltip to be displayed
	};

	//! button style
	enum style_t {
		STYLE_NORMAL,
		STYLE_TOGGLE,
		STYLE_CHECKBOX,
		STYLE_DROPDOWN,
		STYLE_MAX
	};

	//! draws the button
	//! @param renderer the renderer object used to draw the button
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	virtual void draw(SDL_Rect _size, SDL_Rect _actualSize);

	//! handles button clicks, etc.
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the button after processing
	result_t process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable);

	//! activates the button
	virtual void activate() override;

	virtual type_t              getType() const override { return WIDGET_BUTTON; }
	const char*					getText() const { return text.c_str(); }
	const char*					getFont() const { return font.c_str(); }
	int							getBorder() const { return border; }
	const SDL_Rect&				getSize() const { return size; }
	int							getStyle() const { return style; }
	Widget::Args&				getParams() { return params; }
	const Widget::Callback*		getCallback() const { return callback; }

	void	setBorder(int _border) { border = _border; }
	void	setPos(int x, int y) { size.x = x; size.y = y; }
	void	setSize(SDL_Rect _size) { size = _size; }
	void	setColor(const Uint32& _color) { color = _color; }
	void	setTextColor(const Uint32& _color) { textColor = _color; }
	void	setBorderColor(const Uint32& _color) { borderColor = _color; }
	void	setText(const char* _text) { text = _text; }
	void	setFont(const char* _font) { font = _font; }
	void	setIcon(const char* _icon);
	void	setTooltip(const char* _tooltip) { tooltip = _tooltip; }
	void	setStyle(int _style) { style = static_cast<style_t>(_style); }
	void	setCallback(const Widget::Callback* fn) { callback = fn; }

private:
	const Widget::Callback* callback = nullptr;		//!< native callback for clicking
	std::string text;								//!< button text, if any
	std::string font = Font::defaultFont;			//!< button font
	std::string icon;								//!< icon, if any (supersedes text content)
	std::string tooltip;							//!< if empty, button has no tooltip; otherwise, it does
	Widget::Args params;							//!< optional function parameters to use when the button function is called	
	int border = 2;									//!< size of the button border in pixels
	SDL_Rect size;									//!< size and position of the button within its parent frame
	Uint32 color;									//!< the button's color
	Uint32 textColor;								//!< text color
	Uint32 borderColor;								//!< (optional) border color
	style_t style = STYLE_NORMAL;					//!< button style
};