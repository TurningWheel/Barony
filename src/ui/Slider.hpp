//! @file Slider.hpp

#pragma once

#include "../main.hpp"
#include "Widget.hpp"

class Frame;

//! a Slider lives in a frame and allows a user to select a range of values
class Slider : public Widget {
public:
    Slider() = delete;
    Slider(Frame& _parent);
    Slider(const Slider&) = delete;
    Slider(Slider&&) = delete;
    virtual ~Slider() = default;

    Slider& operator=(const Slider&) = delete;
    Slider& operator=(Slider&&) = delete;

    //! the result of the slider process
    struct result_t {
        bool highlighted;				//!< was highlighted this frame
        bool clicked;					//!< was modified this frame
        Uint32 highlightTime;			//!< time since slider was highlighted
        const char* tooltip = nullptr;	//!< slider tooltip to be displayed
    };

    enum orientation_t {
        SLIDER_HORIZONTAL,
        SLIDER_VERTICAL
    };

    //! scroll the parent frame (if any) to be within our bounds
    virtual void scrollParent();

    //! draws the slider
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
	//! @param selectedWidgets the currently selected widgets, if any
    void draw(SDL_Rect _size, SDL_Rect _actualSize, const std::vector<const Widget*>& selectedWidgets) const;

    //! draws post elements in the slider
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    //! @param selectedWidgets the currently selected widgets, if any
    void drawPost(SDL_Rect _size, SDL_Rect _actualSize,
        const std::vector<const Widget*>& selectedWidgets,
        const std::vector<const Widget*>& searchParents) const;

    //! handles slider clicks, etc.
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    //! @param usable true if another object doesn't have the mouse's attention, false otherwise
    //! @return resultant state of the slider after processing
    result_t process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable);

    //! gets the physical screen-space x/y (not relative to current parent - but to the absolute root)
    SDL_Rect getAbsoluteSize() const;

    //! activates the slider
    virtual void activate() override;

    //! control slider with keyboard/gamepad inputs
    bool control();

    //! trigger callback
    void fireCallback();

    //! update the position of the handle
    void updateHandlePosition();

    //! deselect the slider
    virtual void deselect() override;

    virtual type_t              getType() const override { return WIDGET_SLIDER; }
    orientation_t               getOrientation() const { return orientation; }
    float                       getValue() const { return value; }
    float                       getMaxValue() const { return maxValue; }
    float                       getMinValue() const { return minValue; }
    float                       getValueSpeed() const { return valueSpeed; }
    int                         getBorder() const { return border; }
    const SDL_Rect&             getHandleSize() const { return handleSize; }
    const SDL_Rect&             getRailSize() const { return railSize; }
    const char*                 getTooltip() const { return tooltip.c_str(); }
    const Uint32&               getColor() const { return color; }
    const Uint32&               getHighlightColor() const { return highlightColor; }
    void						(*getCallback() const)(Slider&) { return callback; }
    bool                        isActivated() const { return activated; }
    const char*                 getHandleImageActivated() const { return handleImageActivated.c_str(); }
    const char*                 getHandleImage() const { return handleImage.c_str(); }
    const char*                 getRailImage() const { return railImage.c_str(); }
	const bool					isOntop() const { return ontop; }

    void    setOrientation(orientation_t o) { orientation = o; }
    void    setValue(float _value) { value = _value; }
    void    setMaxValue(float _value) { maxValue = _value; }
    void    setMinValue(float _value) { minValue = _value; }
    void    setValueSpeed(float _value) { valueSpeed = _value; }
    void    setBorder(int _border) { border = _border; }
    void    setHandleSize(const SDL_Rect rect) { handleSize = rect; }
    void    setRailSize(const SDL_Rect rect) { railSize = rect; }
    void    setTooltip(const char* _tooltip) { tooltip = _tooltip; }
    void    setColor(const Uint32& _color) { color = _color; }
    void    setHighlightColor(const Uint32& _color) { highlightColor = _color; }
    void	setCallback(void (*const fn)(Slider&)) { callback = fn; }
    void    setHandleImageActivated(const char* _image) { handleImageActivated = _image; }
    void    setHandleImage(const char* _image) { handleImage = _image; }
    void    setRailImage(const char* _image) { railImage = _image; }
	void	setOntop(const bool _ontop) { ontop = _ontop; }

private:
    void (*callback)(Slider&) = nullptr;		    //!< native callback for clicking
    orientation_t orientation = SLIDER_HORIZONTAL;  //!< horizontal or vertical slider?
    float value = 0.f;                              //!< value
    float maxValue = 0.f;                           //!< maximum value
    float minValue = 0.f;                           //!< minimum value
    float valueSpeed = 1.f;                         //!< how fast the slider moves when we press a button
    int border = 0;                                 //!< border size in pixels
    bool activated = false;                         //!< if true, the slider captures all input
    SDL_Rect handleSize{0, 0, 0, 0};                //!< size of the handle in pixels
    SDL_Rect railSize{0, 0, 0, 0};                  //!< size of the rail in pixels
    std::string tooltip;						    //!< if empty, slider has no tooltip; otherwise, it does
    Uint32 color = 0xffffffff;					    //!< the slider's color
    Uint32 highlightColor = 0xffffffff;             //!< slider's color when highlighted
    Uint32 moveStartTime = 0u;                      //!< when the player started holding a direction to move the slider
    Uint32 lastMoveTime = 0u;                       //!< last time the slider was moved
    std::string handleImageActivated;               //!< image to use for the handle (when activated)
    std::string handleImage;                        //!< image to use for the handle
    std::string railImage;                          //!< image to use for the rail
	bool ontop = false;								//!< whether the slider is drawn ontop of others
};
