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

    //! draws the slider
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    void draw(SDL_Rect _size, SDL_Rect _actualSize);

    //! handles slider clicks, etc.
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    //! @param usable true if another object doesn't have the mouse's attention, false otherwise
    //! @return resultant state of the slider after processing
    result_t process(SDL_Rect _size, SDL_Rect _actualSize, const bool usable);

    //! activates the slider
    virtual void activate() override;

    //! control slider with keyboard/gamepad inputs
    void control();

    //! trigger callback
    void fireCallback();

    //! deselect the slider
    virtual void deselect() override;

    virtual type_t              getType() const override { return WIDGET_SLIDER; }
    float                       getValue() const { return value; }
    float                       getMaxValue() const { return maxValue; }
    float                       getMinValue() const { return minValue; }
    int                         getBorder() const { return border; }
    const SDL_Rect&             getHandleSize() const { return handleSize; }
    const SDL_Rect&             getRailSize() const { return railSize; }
    const char*                 getTooltip() const { return tooltip.c_str(); }
    const Uint32&               getColor() const { return color; }
    const Widget::Callback*     getCallback() const { return callback; }
    bool                        isActivated() const { return activated; }

    void    setValue(float _value) { value = _value; }
    void    setMaxValue(float _value) { maxValue = _value; }
    void    setMinValue(float _value) { minValue = _value; }
    void    setBorder(int _border) { border = _border; }
    void    setHandleSize(const SDL_Rect rect) { handleSize = rect; }
    void    setRailSize(const SDL_Rect rect) { railSize = rect; }
    void    setTooltip(const char* _tooltip) { tooltip = _tooltip; }
    void    setColor(const Uint32& _color) { color = _color; }
    void	setCallback(const Widget::Callback* fn) { callback = fn; }

private:
    const Widget::Callback* callback = nullptr;		//!< native callback for clicking
    float value = 0.f;                              //!< value
    float maxValue = 0.f;                           //!< maximum value
    float minValue = 0.f;                           //!< minimum value
    int border = 2;                                 //!< border size in pixels
    bool activated = false;                         //!< if true, the slider captures all input
    SDL_Rect handleSize;                            //!< size of the handle in pixels
    SDL_Rect railSize;                              //!< size of the rail in pixels
    std::string tooltip;						    //!< if empty, button has no tooltip; otherwise, it does
    Uint32 color;					    			//!< the slider's color
    Uint32 moveStartTime = 0u;                      //!< when the player started holding a direction to move the slider
    Uint32 lastMoveTime = 0u;                       //!< last time the slider was moved
};