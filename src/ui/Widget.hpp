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

	//! variable types
	enum var_t {
		TYPE_BOOLEAN,
		TYPE_INTEGER,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_POINTER,
		TYPE_NIL,
		TYPE_MAX
	};

	//! function parameter
	struct param_t {
		param_t() = default;
		param_t(const param_t&) = delete;
		param_t(param_t&&) = delete;
		virtual ~param_t() = default;

		param_t& operator=(const param_t&) = delete;
		param_t& operator=(param_t&&) = delete;

		virtual var_t getType() const = 0;
		virtual param_t* copy() const = 0;
		virtual const char* str() = 0;
		Uint32 strSize() const { return string.size(); }

	protected:
		std::string string;
	};

	//! boolean parameter
	struct param_bool_t : param_t {
		param_bool_t() {
			string.reserve(2);
		}
		param_bool_t(const bool _value) : value(_value) {
			string.reserve(2);
		}
		virtual ~param_bool_t() {}
		virtual var_t getType() const override { return TYPE_BOOLEAN; }
		virtual param_t* copy() const override {
			return new param_bool_t(value);
		}
		virtual const char* str() override {
			return value ? "tb" : "fb";
		}
		bool value = false;
	};

	//! integer parameter
	struct param_int_t : param_t {
		param_int_t() {
			string.reserve(5);
		}
		param_int_t(const int _value) : value(_value) {
			string.reserve(5);
		}
		virtual ~param_int_t() {}
		virtual var_t getType() const override { return TYPE_INTEGER; }
		virtual param_t* copy() const override {
			return new param_int_t(value);
		}
		virtual const char* str() override {
			Uint32* p = (Uint32*)(&string[0]);
			*p = value;
			string[4] = 'i';
			return string.c_str();
		}
		int value = 0;
	};

	//! float parameter
	struct param_float_t : param_t {
		param_float_t() {
			string.reserve(5);
		}
		param_float_t(const float _value) : value(_value) {
			string.reserve(5);
		}
		virtual ~param_float_t() {}
		virtual var_t getType() const override { return TYPE_FLOAT; }
		virtual param_t* copy() const override {
			return new param_float_t(value);
		}
		virtual const char* str() override {
			float* p = (float*)(&string[0]);
			*p = value;
			string[4] = 'f';
			return string.c_str();
		}
		float value = 0.f;
	};

	//! string parameter
	struct param_string_t : param_t {
		param_string_t() {}
		param_string_t(const std::string& _value) : value(_value) {}
		virtual ~param_string_t() {}
		virtual var_t getType() const override { return TYPE_STRING; }
		virtual param_t* copy() const override {
			return new param_string_t(value);
		}
		virtual const char* str() override {
			string.reserve(value.size() + 4);
			string = value;
			Uint32* p = (Uint32*)(&string[value.length()]);
			*p = value.length();
			string[string.size() - 1] = 's';
			return string.c_str();
		}
		std::string value;
	};

	//! pointer parameter
	struct param_pointer_t : param_t {
		param_pointer_t() {
			string.reserve(1);
		}
		param_pointer_t(void* _value) : value(_value) {
			string.reserve(1);
		}
		virtual ~param_pointer_t() {}
		virtual var_t getType() const override { return TYPE_POINTER; }
		virtual param_t* copy() const override {
			return new param_pointer_t(value);
		}
		virtual const char* str() override {
			return "p";
		}
		void* value = nullptr;
	};

	//! nil parameter
	struct param_nil_t : param_t {
		param_nil_t() {
			string.reserve(1);
		}
		virtual ~param_nil_t() {}
		virtual var_t getType() const override { return TYPE_NIL; }
		virtual param_t* copy() const override {
			return new param_nil_t();
		}
		virtual const char* str() override {
			return "n";
		}
	};

	//! function arguments
	class Args {
	public:
		Args() = default;
		Args(const Args& src) {
			copy(src);
		}
		Args(Args&&) = delete;
		~Args() {
			while (list.size() > 0) {
				delete list.back();
				list.pop_back();
			}
		}

		Args& operator=(const Args&) = delete;
		Args& operator=(Args&&) = delete;

		const std::vector<param_t*>&		getList() const { return list; }
		int									getSize() const { return (int)list.size(); }

		//! copy the args from one struct to another
		//! @param src the args to copy
		void copy(const Args& src) {
			while (list.size() > 0) {
				delete list.back();
				list.pop_back();
			}
			for (Uint32 c = 0; c < src.list.size(); ++c) {
				list.push_back(src.list[c]->copy());
			}
		}

		//! add a bool to the args list
		//! @param value the value to init with
		void addBool(const bool value) {
			list.push_back(new param_bool_t(value));
		}

		//! add an int to the args list
		//! @param value the value to init with
		void addInt(const int value) {
			list.push_back(new param_int_t(value));
		}

		//! add a float to the args list
		//! @param value the value to init with
		void addFloat(const float value) {
			list.push_back(new param_float_t(value));
		}

		//! add a string to the args list
		//! @param value the value to init with
		void addString(const std::string& value) {
			list.push_back(new param_string_t(value));
		}

		//! add a pointer to the args list
		//! @param value the value to init with
		void addPointer(void* value) {
			list.push_back(new param_pointer_t(value));
		}

		//! add a nil to the args list
		void addNil() {
			list.push_back(new param_nil_t());
		}

	private:
		std::vector<param_t*> list;
	};

    //! native callback function for processing args
    class Callback {
    public:
        virtual ~Callback() {}

        //! handle the args
        //! @param args the args to consume
        //! @return error code
        virtual int operator()(Args& args) const = 0;
    };

    //! widget type
    enum type_t {
        WIDGET_FRAME,
        WIDGET_BUTTON,
        WIDGET_FIELD,
        WIDGET_SLIDER
    };

    virtual type_t  getType() const = 0;
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