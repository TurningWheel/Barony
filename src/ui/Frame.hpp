//! @file Frame.hpp

#pragma once

#include "../main.hpp"
#include "Font.hpp"
#include "Widget.hpp"

#include <memory>

class Field;
class Button;
class Field;
class Slider;

//! A container for a gui (ie, a window)
//! When a frame's size is smaller than its actual size, sliders will automatically be placed in the frame.
//! Frame objects can be populated with Field objects, Button objects, other Frame objects, and more.
class Frame : public Widget {
public:
	Frame() = delete;
	Frame(const char* _name = "");
	Frame(Frame& parent, const char* _name = "");
	Frame(const Frame&) = delete;
	Frame(Frame&&) = delete;
	virtual ~Frame();

	Frame& operator=(const Frame&) = delete;
	Frame& operator=(Frame&&) = delete;

	//! border style
	enum border_style_t {
		BORDER_FLAT,
		BORDER_BEVEL_HIGH,
		BORDER_BEVEL_LOW,
		BORDER_MAX
	};

	//! list text justification
	enum justify_t {
		LEFT,
		RIGHT,
		CENTER,
		JUSTIFY_TYPE_LENGTH
	};

	//! frame image
	struct image_t {
		std::string name;
		std::string path;
		Uint32 color;
		SDL_Rect pos;
		bool tiled = false;
		bool disabled = false;
		bool ontop = false;
	};

	struct entry_t;

	//! list entry listener
	struct listener_t {
		listener_t(entry_t* _entry) :
			entry(_entry) {}

		void onDeleted();
		void onChangeColor(bool selected, bool highlighted);
		void onChangeName(const char* name);

		//! Frame::entry_t*
		entry_t* entry = nullptr;
	};

	//! frame list entry
	struct entry_t {
		entry_t(Frame& _parent) : parent(_parent) {}
		Frame& parent;
		std::string name;
		std::string text;
		std::string tooltip;
		Uint32 color;
		std::string image;

		bool pressed = false;
		bool highlighted = false;
		Uint32 highlightTime = 0;
		bool suicide = false;

		void (*click)(entry_t&) = nullptr;
		void (*ctrlClick)(entry_t&) = nullptr;
		void (*highlight)(entry_t&) = nullptr;
		void (*highlighting)(entry_t&) = nullptr;

		std::shared_ptr<listener_t> listener;
	};

	//! frame processing result structure
	struct result_t {
		bool usable;			//! true if the frame is still usable after processing elements, false otherwise
		Uint32 highlightTime;	//! current time since the last frame element was pressed
		const char* tooltip;	//! tooltip for the parent frame to display
		bool removed;			//! the frame was removed during the process
	};

	//! amount of time (ms) it takes for a highlighted object to display a tooltip
	static const Uint32 tooltipTime = 500;

	//! width/height of the slider(s) that appear when actualSize > size (in pixels)
	static const Sint32 sliderSize;

	//! virtual screen size (width)
	static const int virtualScreenX = 1280;

	//! virtual screen size (height)
	static const int virtualScreenY = 720;

	//! init ui engine
	static void guiInit();
	static void fboInit();

	//! destroy ui engine
	static void guiDestroy();
	static void fboDestroy();

	//! draws the frame and all of its subelements
	void draw() const;

	//! handle clicks and other events
	//! @return compiled results of frame processing
	result_t process();

	//! to be performed recursively on every frame after process()
	void postprocess();

	//! adds a new frame to the current frame
	//! @param name internal name of the new frame
	//! @return the newly created frame
	Frame* addFrame(const char* name = "");

	//! adds a new button to the current frame
	//! @param name internal name of the new button
	//! @return the newly created button
	Button* addButton(const char* name);

	//! adds a new field to the current frame
	//! @param name internal name of the new field
	//! @param len the length of the field in characters
	//! @return the newly created field
	Field* addField(const char* name, const int len);

	//! adds a new image object to the current frame
	//! @param pos position of the image in the frame
	//! @param color the color of the image
	//! @param image the image to draw
	//! @param name the name of the image (unique id)
	//! @return the newly created image object
	image_t* addImage(const SDL_Rect pos, const Uint32 color, const char* image, const char* name = "");

	//! adds a new entry to the frame's list
	//! @param name internal name of the new entry
	//! @param resizeFrame if true, the size of the frame will be reduced after removing the entry
	//! @return the newly created entry object
	entry_t* addEntry(const char* name, bool resizeFrame);

	//! adds a new slider object to the current frame
	//! @param name the name of the slider
	//! @return the newly created slider object
	Slider* addSlider(const char* name);

	//! removes all objects and list entries from the frame
	void clear();

	//! remove all list entries from the frame
	void clearEntries();

	//! removes the frame itself, as well as all contained objects
	void removeSelf();

	//! remove an object from the frame
	//! @param name the name of the object to remove
	//! @return true if the object was successfully removed, false otherwise
	bool remove(const char* name);

	//! remove an entry from the frame list
	//! @param name the name of the object to remove
	//! @param resizeFrame if true, the size of the frame will be reduced after removing the entry
	//! @return true if the entry was successfully removed, false otherwise
	bool removeEntry(const char* name, bool resizeFrame);

	//! recursively searches all embedded frames for a specific frame
	//! @param name the name of the frame to find
	//! @return the frame with the given name, or nullptr if the frame could not be found
	Frame* findFrame(const char* name);

	//! find a button in this frame
	//! @param name the name of the button to find
	//! @return the button, or nullptr if it could not be found
	Button* findButton(const char* name);

	//! find a field in this frame
	//! @param name the name of the field to find
	//! @return the field, or nullptr if it could not be found
	Field* findField(const char* name);

	//! find an image in this frame
	//! @param name the name of the image to find
	//! @return the image, or nullptr if it could not be found
	image_t* findImage(const char* name);

	//! find an entry in this frame
	//! @param name the name of the entry to find
	//! @return the entry, or nullptr if it could not be found
	entry_t* findEntry(const char* name);

	//! find a slider in this frame
	//! @param name the name of the slider to find
	//! @return the slider, or nullptr if it could not be found
	Slider* findSlider(const char* name);

	//! get the frame's parent frame, if any
	//! @return the parent frame, or nullptr if there is none
	Frame* getParent();

	//! resizes the frame to accomodate all list entries
	void resizeForEntries();

	//! deselect all frame elements recursively
	virtual void deselect() override;

	//! activates the frame so we can select and activate list entries
	virtual void activate() override;

	//! activates the current entry selection
	void activateSelection();

	//! determines if the mouse is currently within the frame or not
	//! @param curSize used by the recursion algorithm, ignore or always pass nullptr
	//! @param curActualSize used by the recursion algorithm, ignore or always pass nullptr
	//! @return true if it is, false otherwise
	bool capturesMouse(SDL_Rect* curSize = nullptr, SDL_Rect* curActualSize = nullptr);

	//! determines if the mouse is currently within the frame or not - but uses X/Y not OX/OY (OX/OY remain constant when dragging)
	//! @param curSize used by the recursion algorithm, ignore or always pass nullptr
	//! @param curActualSize used by the recursion algorithm, ignore or always pass nullptr
	//! @return true if it is, false otherwise
	bool capturesMouseInRealtimeCoords(SDL_Rect* curSize = nullptr, SDL_Rect* curActualSize = nullptr);

	//! warps the player's mouse cursor to the center location of the frame
	void warpMouseToFrame(const int player, Uint32 flags) const;

	//! gets the physical screen-space x/y (not relative to current parent - but to the absolute root)
	SDL_Rect getAbsoluteSize() const;

	//! set the list selection to the given index
	//! @param index the index to set the list selection to
	void setSelection(int index);

	//! whether to enable/disable scrolling on this frame
	//! @param enabled whether to enable or disable scrolling
	void enableScroll(bool enabled);

	//! draw an image in the frame, clipping it within the given rectangles
	//! @param image the image to draw
	//! @param _size the size of the rectangle to clip against
	//! @param scroll the amount by which to offset the image in x/y
	void drawImage(const image_t* image, const SDL_Rect& _size, const SDL_Rect& scroll) const;

	//! scroll to the current list entry selection in the frame
	//! @param scroll_to_top if true, scroll the selection to the very top of the frame
	void scrollToSelection(bool scroll_to_top = false);

	virtual type_t					getType() const override { return WIDGET_FRAME; }
	const char*						getFont() const { return font.c_str(); }
	const int						getBorder() const { return border; }
	const SDL_Rect&					getSize() const { return size; }
	const SDL_Rect&					getActualSize() const { return actualSize; }
	int								getBorderStyle() const { return borderStyle; }
	std::vector<Frame*>&			getFrames() { return frames; }
	std::vector<Field*>&			getFields() { return fields; }
	std::vector<Button*>&			getButtons() { return buttons; }
	std::vector<Slider*>&			getSliders() { return sliders; }
	std::vector<entry_t*>&			getEntries() { return list; }
	std::vector<image_t*>&			getImages() { return images; }
	const bool						isDisabled() const { return disabled; }
	const bool						isHollow() const { return hollow; }
	const bool						isDropDown() const { return dropDown; }
	const bool						isScrollBarsEnabled() const { return scrollbars; }
	const bool						isAllowScrollBinds() const { return allowScrollBinds; }
	const bool						isActivated() const { return activated; }
	const SDL_Rect&					getListOffset() const { return listOffset; }
	int								getSelection() const { return selection; }
	real_t							getOpacity() const { return opacity; }
	const bool						getInheritParentFrameOpacity() const { return inheritParentFrameOpacity; }
	justify_t						getJustify() const { return justify; }
	const bool						isClickable() const { return clickable; }

	void	setFont(const char* _font) { font = _font; }
	void	setBorder(const int _border) { border = _border; }
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(SDL_Rect _size) { size = _size; }
	void	setActualSize(SDL_Rect _actualSize) { actualSize = _actualSize; allowScrolling = true; }
	void	setBorderStyle(int _borderStyle) { borderStyle = static_cast<border_style_t>(_borderStyle); }
	void	setHigh(bool b) { borderStyle = b ? BORDER_BEVEL_HIGH : BORDER_BEVEL_LOW; }
	void	setColor(const Uint32& _color) { color = _color; }
	void	setBorderColor(const Uint32& _color) { borderColor = _color; }
	void	setDisabled(const bool _disabled) { disabled = _disabled; }
	void	setHollow(const bool _hollow) { hollow = _hollow; }
	void	setDropDown(const bool _dropDown) { dropDown = _dropDown; }
	void	setScrollBarsEnabled(const bool _scrollbars) { scrollbars = _scrollbars; }
	void	setAllowScrollBinds(const bool _allow) { allowScrollBinds = _allow; }
	void	setListOffset(SDL_Rect _size) { listOffset = _size; }
	void	setInheritParentFrameOpacity(const bool _inherit) { inheritParentFrameOpacity = _inherit; }
	void	setOpacity(const real_t _opacity) { opacity = _opacity; }
	void	setListJustify(justify_t _justify) { justify = _justify; }
	void	setClickable(const bool _clickable) { clickable = _clickable; }

private:
	Uint32 ticks = 0;									//!< number of engine ticks this frame has persisted
	std::string font = Font::defaultFont;				//!< name of the font to use for frame entries
	int border = 2;										//!< size of the frame's border
	SDL_Rect size;										//!< size and position of the frame in its parent frame
	SDL_Rect actualSize;								//!< size of the frame's whole contents. when larger than size, activates sliders
	border_style_t borderStyle = BORDER_BEVEL_HIGH;		//!< border style
	Uint32 color;										//!< the frame's color
	Uint32 borderColor;									//!< the frame's border color (only used for flat border)
	const char* tooltip = nullptr;						//!< points to the tooltip that should be displayed by the (master) frame, or nullptr if none should be displayed
	bool hollow = false;								//!< if true, the frame doesn't have a solid background
	bool draggingHSlider = false;						//!< if true, we are dragging the horizontal slider
	bool draggingVSlider = false;						//!< if true, we are dragging the vertical slider
	int oldSliderX = 0;									//!< when you start dragging a slider, this is set
	int oldSliderY = 0;									//!< when you start dragging a slider, this is set
	bool dropDown = false;								//!< if true, the frame is destroyed when specific inputs register
	Uint32 dropDownClicked = 0;							//!< key states stored for removing drop downs
	int selection = -1;									//!< entry selection
	bool allowScrollBinds = true;						//!< if true, scroll wheel + right stick can scroll frame
	bool allowScrolling = false;						//!< must be enabled for any kind of scrolling/actualSize to work
	bool scrollbars = true;								//!< must be true for sliders to be drawn/usable
	bool activated = false;								//!< true if this frame is consuming input (to navigate list entries)
	SDL_Rect listOffset{0, 0, 0, 0};					//!< frame list offset in x, y
	real_t opacity = 100.0;								//!< opacity multiplier of elements within this frame (image/fields etc)
	bool inheritParentFrameOpacity = true;				//!< if true, uses parent frame opacity
	justify_t justify = justify_t::LEFT;				//!< frame list horizontal justification
	bool clickable = false;								//!< if true, you can activate the frame by clicking on it (used for lists)

	std::vector<Frame*> frames;
	std::vector<Button*> buttons;
	std::vector<Field*> fields;
	std::vector<image_t*> images;
	std::vector<Slider*> sliders;
	std::vector<entry_t*> list;

	//! activate the given list entry
	//! @param entry the entry to activate
	void activateEntry(entry_t& entry);

	//! draws the frame and all of its subelements
	//! @param _size real position of the frame onscreen
	//! @param _actualSize offset into the frame space (scroll)
	//! @param selectedWidgets the currently selected widgets, if any
	void draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const;

	//! handle clicks and other events
	//! @param _size real position of the frame onscreen
	//! @param _actualSize offset into the frame space (scroll)
	//! @param selectedWidgets the currently selected widgets, if any
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return compiled results of frame processing
	result_t process(SDL_Rect _size, SDL_Rect actualSize, const std::vector<Widget*>& selectedWidgets, const bool usable);
};

// root frame object
extern Frame* gui;
void createTestUI();