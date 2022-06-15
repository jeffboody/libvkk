VKUI Library
============

The VKUI user interface is defined by creating a screen
and one or more widget objects.

A screen is required to manage shared resources, control
the window stack, handle events and perform rendering.

Widgets are designed as building blocks which the user may
inherit from to build more complex widgets. Inheritance is
accomplished by declaring the base object in the derived
object (not a pointer object). Construct the object by
passing the combined size in the wsize parameter of the
base object constructor followed by construction of the
derived class members. Similarly destruction is performed
by freeing the derived class members followed by calling
the base class destructor (e.g. perform destructor
operations in reverse order).

For example.

	typedef struct
	{
		vkui_widget_t base;
		// optional my_widget_t members
	} my_widget_t;

	my_widget_t* my_widget_new(vkui_screen_t* screen)
	{
		my_widget_t* self;
		self = (my_widget_t*)
		       vkui_widget_new(screen,
		                       sizeof(my_widget_t),
		                       ...);
		// optionally create my_widget_t members
		return self;
	}

	void my_widget_delete(my_widget_t** _self)
	{
		vkui_widget_t* self = *_self;
		if(self)
		{
			// optionally destroy my_widget_t members
			vkui_widget_delete((vkui_widget_t**) _self);
		}
	}

Some widgets also have constructor functions which allow
the user to create a commonly used subclass of the widget
(e.g. a page heading or paragraph).

Note that the VKUI library is optional and must be enabled
by setting/exporting the VKK\_USE\_VKUI variable.

The following sections briefly describe the screen and
standard widgets.

Screen
------

The screen accepts a VKK renderer which allows the app to
define where rendering occurs. Typically an app will render
directly to the display via the default renderer. However,
an app may also render to a display embedded in a 3D
environment via an image stream renderer (e.g. render to
texture).

The screen also accepts a vkui\_widgetStyle\_t color
palette which is used to derive colors the various widget
components. The derived colors are roughly based on the
[Material Design Dark Theme](https://material.io/design/color/dark-theme.html).
Here is an example color palette that seems to work well.

![Mint Color Palette](../doc/palette.jpg?raw=true "Mint Color Palette")

Widget
------

A widget is the basic building block used to create all
other derived widget types.

Window
------

A window is a composite widget type which supports flags
to automatically create a title bar, a page, workspace
layers and/or a footer. The constructor is typically used
to create a full screen page (e.g. a web page), but also
supports a couple specializations. The page sidebar is a
narrow page that may be used to display settings or
bookmarks on the main page. The page popup may be used in
conjunction with an actionBar to popup a small window for
additional commands or options.

Objects suitable to be placed in the window may be
constructed by the newPageXXX/newSidebar/newFooterXXX
constructor functions.

It is the users responsibility to manage the page, layers
and footer (e.g. you must clear all widgets added before
deleting the window).

Bullet Box
----------

A bullet box consists of icon and text object.

Check Box
---------

A check box is a specialization of a bullet box which
includes standard check box icons and a pointer to an
integer value for the check box state.

Layer
-----

A layer is a workspace that is internally represented as a
list of widgets that are drawn back-to-front to ensure the
correct order is maintained.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
layer).

List Box
--------

A list box is a workspace that draws a list of widgets from
top-to-bottom or left-to-right. A list box may optionally
have a scroll bar.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
listbox).

Radio List
----------

A radio list is a specialization of a list box and a bullet
box which includes standard radio icons and a pointer to an
integer value for the radio list state.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
radiolist).

Sprite
------

A sprite is container for images and icons.

The select function may be used to change the image
displayed (e.g. a checkbox may change between
checked/unchecked states).

Text
----

Text is simply a single line of text which may also be
used for text entry.

Textbox
-------

A textbox is a container for a paragraph of text which
automatically reflows.

HLine
-----

A hline is simply a horizontal separator line.

Info Panel
----------

An info panel is a vertical list of text items which are
not intended for user interaction.

Objects suitable to be placed in the info panel may be
constructed by the newInfoXXX constructor functions.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
info panel).

Status Bar
----------

A status bar is a horizontal list of icon items which are
not intended for user interaction.

Objects suitable to be placed in the status bar may be
constructed by the newStatusXXX constructor functions.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
status bar).

Action Bar
----------

An action bar is a horizontal or vertical list of icon
items which may be used to trigger actions such as showing
a new window, showing a popup window or performing some
other action. A horizontal action bar will place popup
windows above/below while a vertical action bar will place
popup windows to the side.

Objects suitable to be placed in the action bar may be
constructed by the newActionXXX constructor functions.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
action bar).
