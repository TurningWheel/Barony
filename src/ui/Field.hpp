//! @file Field.hpp

#pragma once

#include "../main.hpp"
#include "Font.hpp"

class Frame;

//! A Field is a text field that lives in a Frame. It can be edited, or locked for editing to just have some static text in a window.
class Field : public Widget {
public:
	Field(const int _textLen);
	Field(const char* _text);
	Field(Frame& _parent, const int _textLen);
	Field(Frame& _parent, const char* _text);
	Field(const Field&) = delete;
	Field(Field&&) = delete;
	virtual ~Field();

	Field& operator=(const Field&) = delete;
	Field& operator=(Field&&) = delete;

	//! text justification
	enum justify_t {
		TOP,
		BOTTOM,
		LEFT,
		RIGHT,
		CENTER,
		JUSTIFY_TYPE_LENGTH
	};

	//! the result of the field process
	struct result_t {
		bool highlighted;
		bool entered;
	};

	//! selects the field for text editing
	virtual void select() override;

	//! deselects the field
	virtual void deselect() override;

	//! draws the field
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	void draw(SDL_Rect _size, SDL_Rect _actualSize);

	//! handles clicks, etc.
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the field after processing
	result_t process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable);

	virtual type_t              getType() const override { return WIDGET_FIELD; }
	const char*					getText() const { return text; }
	const char*					getFont() const { return font.c_str(); }
	const Uint32				getColor() const { return color; }
	const SDL_Rect				getSize() const { return size; }
	const int					getHJustify() const { return static_cast<int>(hjustify); }
	const int					getVJustify() const { return static_cast<int>(vjustify); }
	const bool					isEditable() const { return editable; }
	const bool					isNumbersOnly() const { return numbersOnly; }
	Widget::Args&				getParams() { return params; }
	const Widget::Callback*		getCallback() const { return callback; }

	void	setText(const char* _text);
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(const SDL_Rect _size) { size = _size; }
	void	setColor(const Uint32 _color) { color = _color; }
	void	setEditable(const bool _editable) { editable = _editable; }
	void	setNumbersOnly(const bool _numbersOnly) { numbersOnly = _numbersOnly; }
	void	setJustify(const int _justify) { hjustify = vjustify = static_cast<justify_t>(_justify); }
	void	setHJustify(const int _justify) { hjustify = static_cast<justify_t>(_justify); }
	void	setVJustify(const int _justify) { vjustify = static_cast<justify_t>(_justify); }
	void	setScroll(const bool _scroll) { scroll = _scroll; }
	void	setCallback(const Widget::Callback* fn) { callback = fn; }
	void	setFont(const char* _font) { font = _font; }

private:
	Widget::Args params;								//!< script arguments to use when calling script
	std::string font = Font::defaultFont;				//!< font to use for rendering the field
	char* text = nullptr;								//!< internal text buffer
	size_t textlen = 0;									//!< length of internal text buffer
	Uint32 color = 0xFFFFFFFF;							//!< text color
	SDL_Rect size;										//!< size of the field in pixels
	justify_t hjustify = LEFT;							//!< horizontal text justification
	justify_t vjustify = TOP;							//!< vertical text justification
	bool editable = false;								//!< whether the field is read-only
	bool numbersOnly = false;							//!< whether the field can only contain numeric chars
	bool scroll = true;									//!< whether the field should scroll if the text is longer than its container
	bool selectAll = false;								//!< whether all the text is selected for editing
	const Widget::Callback* callback = nullptr;			//!< the callback to use after text is entered
};