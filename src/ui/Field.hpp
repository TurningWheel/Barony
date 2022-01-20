//! @file Field.hpp

#pragma once

#include "../main.hpp"
#include "Font.hpp"
#include "Text.hpp"

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

	//! scroll the parent frame (if any) to be within our bounds
	virtual void scrollParent();

	//! activates the field for text editing
	virtual void activate() override;

	//! deactivate text editing
	void deactivate();

	//! deselects the field
	virtual void deselect() override;

	//! draws the field
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param selectedWidgets the currently selected widgets, if any
	void draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const;

	//! draws post elements in the field
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param selectedWidgets the currently selected widgets, if any
	void drawPost(SDL_Rect _size, SDL_Rect _actualSize,
	    const std::vector<const Widget*>& selectedWidgets,
	    const std::vector<const Widget*>& searchParents) const;

	//! handles clicks, etc.
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the field after processing
	result_t process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable);

	//! gets the physical screen-space x/y (not relative to current parent - but to the absolute root)
	SDL_Rect getAbsoluteSize() const;

	//! gets the last line number that fits within the field y + height (to check if overflowing container)
	int getLastLineThatFitsWithinHeight();

	//! gets longest line of field, measured by actual text width
	std::string getLongestLine();

	//! calls Text::get, passing in getText(), getFont(), getTextColor() getOutlineColor() as parameters
	Text* getTextObject() const;

	//! gets the number of lines occupied by text in the field
	//! @return number of lines of text
	int getNumTextLines() const;

	//! add a key value pair to the highlighted word map
	//! @param word the word 'index' in the sentence (first word is 0)
	//! @param color the color to set the word to
	void	addWordToHighlight(int word, Uint32 color) { wordsToHighlight[word] = color; }

	//! gets map for highlighted words
	const std::map<int, Uint32>&	getWordsToHighlight() const { return wordsToHighlight; }

	//! reset the highlighted word map
	void	clearWordsToHighlight() { wordsToHighlight.clear(); }

	static const int TEXT_HIGHLIGHT_WORDS_PER_LINE = 10000;

	virtual type_t              getType() const override { return WIDGET_FIELD; }
	const char*					getText() const { return text; }
	const char*					getFont() const { return font.c_str(); }
	const Uint32				getColor() const { return color; }
	const Uint32				getTextColor() const { return textColor; }
	const Uint32				getOutlineColor() const { return outlineColor; }
	const Uint32				getBackgroundColor() const { return backgroundColor; }
	const Uint32				getBackgroundActivatedColor() const { return backgroundActivatedColor; }
	const Uint32				getBackgroundSelectAllColor() const { return backgroundSelectAllColor; }
	const SDL_Rect				getSize() const { return size; }
	const int					getHJustify() const { return static_cast<int>(hjustify); }
	const int					getVJustify() const { return static_cast<int>(vjustify); }
	const bool					isEditable() const { return editable; }
	const bool					isNumbersOnly() const { return numbersOnly; }
	void						(*getCallback() const)(Field&) { return callback; }
	const char*					getGuide() const { return guide.c_str(); }
	const bool					isOntop() const { return ontop; }
	const bool                  isActivated() const { return activated; }

	void	setText(const char* _text);
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(const SDL_Rect _size) { size = _size; }
	void	setColor(const Uint32 _color) { color = _color; }
	void	setTextColor(const Uint32 _color) { textColor = _color; }
	void	setOutlineColor(const Uint32 _color) { outlineColor = _color; }
	void	setBackgroundColor(const Uint32 _color) { backgroundColor = _color; }
	void	setBackgroundActivatedColor(const Uint32 _color) { backgroundActivatedColor = _color; }
	void	setBackgroundSelectAllColor(const Uint32 _color) { backgroundSelectAllColor = _color; }
	void	setEditable(const bool _editable) { editable = _editable; }
	void	setNumbersOnly(const bool _numbersOnly) { numbersOnly = _numbersOnly; }
	void	setJustify(const int _justify) { hjustify = vjustify = static_cast<justify_t>(_justify); }
	void	setHJustify(const int _justify) { hjustify = static_cast<justify_t>(_justify); }
	void	setVJustify(const int _justify) { vjustify = static_cast<justify_t>(_justify); }
	void	setScroll(const bool _scroll) { scroll = _scroll; }
	void	setCallback(void (*const fn)(Field&)) { callback = fn; }
	void	setFont(const char* _font) { font = _font; }
	void	setGuide(const char* _guide) { guide = _guide; }
	void    reflowTextToFit(const int characterOffset);
	void	setOntop(const bool _ontop) { ontop = _ontop; }
	static char* tokenize(char* str, const char* const delimiters);

private:
	std::string font = Font::defaultFont;				//!< font to use for rendering the field
	std::string guide;									//!< string to use as a descriptive guide for the field (eg "Enter character's name");
	char* text = nullptr;								//!< internal text buffer
	size_t textlen = 0;									//!< length of internal text buffer
	Uint32 color;										//!< color mixed w/ final rendered text
	Uint32 textColor;									//!< text color
	Uint32 outlineColor;								//!< outline color
	Uint32 backgroundColor = 0;							//!< background color
	Uint32 backgroundActivatedColor = 0;				//!< background color (when activated)
	Uint32 backgroundSelectAllColor = 0;				//!< background color (when activated and all text selected)
	SDL_Rect size;										//!< size of the field in pixels
	justify_t hjustify = LEFT;							//!< horizontal text justification
	justify_t vjustify = TOP;							//!< vertical text justification
	bool editable = false;								//!< whether the field is read-only
	bool numbersOnly = false;							//!< whether the field can only contain numeric chars
	bool scroll = false;								//!< whether the field should scroll if the text is longer than its container
	bool selectAll = false;								//!< whether all the text is selected for editing
	bool activated = false;								//!< whether field is active for text editing
	void (*callback)(Field&) = nullptr;					//!< the callback to use after text is entered
	bool ontop = false;									//!< whether the field is drawn ontop of others
	std::map<int, Uint32> wordsToHighlight;				//!< word indexes in the field matching the keys in the map will be colored with the mapped value
};
