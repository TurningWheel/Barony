//! @file Widget.hpp

#pragma once

#include "../main.hpp"

class Frame;

class Widget {
public:
    Widget() = default;
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    virtual ~Widget();

    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;

    //! widget type
    enum type_t {
        WIDGET_FRAME,
        WIDGET_BUTTON,
        WIDGET_FIELD,
        WIDGET_SLIDER
    };

    virtual type_t  getType() const = 0;
    Widget*         getParent() { return parent; }
    const char*		getName() const { return name.c_str(); }
    bool			isPressed() const { return reallyPressed; }
    bool			isHighlighted() const { return selected | highlighted; }
    bool			isSelected() const { return selected; }
    bool			isDisabled() const { return disabled; }
    bool            isInvisible() const { return invisible; }
    Uint32          getHighlightTime() const { return highlightTime; }
    Sint32          getOwner() const { return owner; }
    const char*     getWidgetRight() const { return widgetRight.c_str(); }
    const char*     getWidgetDown() const { return widgetDown.c_str(); }
    const char*     getWidgetLeft() const { return widgetLeft.c_str(); }
    const char*     getWidgetUp() const { return widgetUp.c_str(); }
    const char*     getWidgetPageLeft() const { return widgetPageLeft.c_str(); }
    const char*     getWidgetPageRight() const { return widgetPageRight.c_str(); }
    const char*     getWidgetBack() const { return widgetBack.c_str(); }
    const char*     getWidgetSearchParent() const { return widgetSearchParent.c_str(); }
    const char*     getWidgetTab() const { return widgetTab.c_str(); }

    void	setName(const char* _name) { name = _name; }
    void	setPressed(bool _pressed) { reallyPressed = pressed = _pressed; }
    void    setSelected(bool _selected) { selected = _selected; }
    void	setDisabled(bool _disabled) { disabled = _disabled; }
    void    setInvisible(bool _invisible) { invisible = _invisible; }
    void    setOwner(Sint32 _owner) { owner = _owner; }
    void    setWidgetRight(const char* s) { widgetRight = s; }
    void    setWidgetDown(const char* s) { widgetDown = s; }
    void    setWidgetLeft(const char* s) { widgetLeft = s; }
    void    setWidgetUp(const char* s) { widgetUp = s; }
    void    setWidgetPageLeft(const char* s) { widgetPageLeft = s; }
    void    setWidgetPageRight(const char* s) { widgetPageRight = s; }
    void    setWidgetBack(const char* s) { widgetBack = s; }
    void    setWidgetSearchParent(const char* s) { widgetSearchParent = s; }
    void    setWidgetTab(const char* s) { widgetTab = s; }

    //! recursively locates the head widget for this widget
    //! @return the head widget, which may be this widget
    Widget* findHead();

    //! activate this widget
    virtual void activate();

    //! select this widget
    virtual void select();

    //! deselect this widget
    virtual void deselect();

    //! handle inputs on the widget
    //! @return the next widget to select, or nullptr if no widget was selected
    Widget* handleInput();

    //! adopt a new widget as one of our children
    //! @param widget the widget to adopt
    void adoptWidget(Widget& widget);

    //! find a widget amongst our children
    //! @param name the name of the widget to find
    //! @param recursive true to search recursively or not
    //! @return the widget found, or nullptr if it was not found
    Widget* findWidget(const char* name, bool recursive);

protected:
    Widget* parent = nullptr;                       //!< parent widget
    std::list<Widget*> widgets;                     //!< widget children
    std::string name;                               //!< widget name
    bool pressed = false;							//!< pressed state
    bool reallyPressed = false;						//!< the "actual" pressed state, pre-mouse process
    bool highlighted = false;                       //!< if true, this widget has the mouse over it
    bool selected = false;							//!< if true, this widget has focus
    bool disabled = false;							//!< if true, the widget is unusable and grayed out
    bool invisible = false;                         //!< if true, widget is both unusable and invisible
	bool toBeDeleted = false;						//!< if true, the widget will be removed at the end of its process
    Uint32 highlightTime = 0u;						//!< records the time since the widget was highlighted
    Sint32 owner = 0;                               //!< which player owns this widget (0 = player 1, 1 = player 2, etc)

    std::string widgetSearchParent;                 //!< parent of widget to select (use to narrow search)
    std::string widgetRight;             			//!< next widget to select right
    std::string widgetDown;                         //!< next widget to select down
    std::string widgetLeft;                         //!< next widget to select left
    std::string widgetUp;                           //!< next widget to select up
    std::string widgetPageLeft;                     //!< widget to activate when you press MenuPageLeft
    std::string widgetPageRight;                    //!< widget to activate when you press MenuPageRight
    std::string widgetBack;                         //!< widget to activate when you press MenuCancel
    std::string widgetTab;                          //!< widget to select when you press tab

    Frame* findSearchRoot();
};