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
    bool			isHighlighted() const { return selected || highlighted; }
    bool			isSelected() const { return selected; }
    bool			isDisabled() const { return disabled; }
    bool            isInvisible() const { return invisible; }
    Uint32          getHighlightTime() const { return highlightTime; }
    Sint32          getOwner() const { return owner; }
    void			(*getTickCallback() const)(Widget&) { return callback; }
    const char*     getWidgetSearchParent() const { return widgetSearchParent.c_str(); }
    auto&           getWidgetActions() const { return widgetActions; }
    auto&           getWidgetMovements() const { return widgetMovements; }
    auto&           getWidgets() const { return widgets; }

    void	setName(const char* _name) { name = _name; }
    void	setPressed(bool _pressed) { reallyPressed = pressed = _pressed; }
    void    setSelected(bool _selected) { selected = _selected; }
    void	setDisabled(bool _disabled) { disabled = _disabled; }
    void    setInvisible(bool _invisible) { invisible = _invisible; }
    void    setOwner(Sint32 _owner) { owner = _owner; }
    void	setTickCallback(void (*const fn)(Widget&)) { callback = fn; }
    void    setWidgetTab(const char* s) { widgetMovements.emplace("MenuTab", s); }
    void    setWidgetRight(const char* s) { widgetMovements.emplace("MenuRight", s); widgetMovements.emplace("AltMenuRight", s); }
    void    setWidgetDown(const char* s) { widgetMovements.emplace("MenuDown", s); widgetMovements.emplace("AltMenuDown", s); }
    void    setWidgetLeft(const char* s) { widgetMovements.emplace("MenuLeft", s); widgetMovements.emplace("AltMenuLeft", s); }
    void    setWidgetUp(const char* s) { widgetMovements.emplace("MenuUp", s); widgetMovements.emplace("AltMenuUp", s); }
    void    setWidgetPageLeft(const char* s) { widgetActions.emplace("MenuPageLeft", s); }
    void    setWidgetPageRight(const char* s) { widgetActions.emplace("MenuPageRight", s); }
    void    setWidgetBack(const char* s) { widgetActions.emplace("MenuCancel", s); }
    void    setWidgetSearchParent(const char* s) { widgetSearchParent = s; }
    void    addWidgetAction(const char* binding, const char* action) { widgetActions.emplace(binding, action); }
    void    addWidgetMovement(const char* binding, const char* action) { widgetMovements.emplace(binding, action); }

    //! recursively locates the head widget for this widget
    //! @return the head widget, which may be this widget
    Widget* findHead();

    //! activate this widget
    virtual void activate();

    //! select this widget
    virtual void select();

    //! deselect this widget
    virtual void deselect();

    //! update this widget for one tick
    void process();

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

    //! find the selected widget amongst our children
    //! @return the selected widget, or nullptr if it was not found
    Widget* findSelectedWidget();

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
    void (*callback)(Widget&) = nullptr;			//!< the callback to use each frame for this widget

    std::unordered_map<std::string, std::string>
        widgetActions;                              //!< widgets to select and activate when input is pressed
    std::unordered_map<std::string, std::string>
        widgetMovements;                            //!< widgets to select when input is pressed
    std::string widgetSearchParent;                 //!< widget to search from for actions and movements

    Frame* findSearchRoot();

    void drawGlyphs(const SDL_Rect size, const Widget* selectedWidget);
};