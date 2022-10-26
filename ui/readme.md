VKK UI (User Interface)
=======================

VKK UI is an experimental widget library that is built on
top of the VKK Core. The widget library requires a screen to
manage shared resources, control the window stack, handle
events, perform rendering and create widgets. The library
includes widgets commonly found in other similar libraries
such as text boxes and windows. Widgets are designed as
building blocks which the user may also inherit from to
build more complex widgets.

Inheritance is accomplished by declaring the base object in
the derived object (not a pointer object). Construct the
object by passing the combined size in the wsize parameter
of the base object constructor followed by construction of
the derived class members. Similarly destruction is
performed by freeing the derived class members followed by
calling the base class destructor (e.g. perform destructor
operations in reverse order).

For example.

	typedef struct
	{
		vkk_uiWidget_t base;
		// optional my_widget_t members
	} my_widget_t;

	my_widget_t* my_widget_new(vkk_uiScreen_t* screen)
	{
		my_widget_t* self;
		self = (my_widget_t*)
		       vkk_uiWidget_new(screen,
		                        sizeof(my_widget_t),
		                       ...);
		// optionally create my_widget_t members
		return self;
	}

	void my_widget_delete(my_widget_t** _self)
	{
		vkk_uiWidget_t* self = *_self;
		if(self)
		{
			// optionally destroy my_widget_t members
			vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
		}
	}

Some widgets also have special constructor functions which
allow the user to create a commonly used subclass of the
widget (e.g. a page heading or paragraph).

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

The screen also accepts a vkk\_uiWidgetStyle\_t color
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

File Picker
-----------

A file picker is a special window that provides the user a
cross-platform mechanism to select a file (e.g. Open or
Save As).

Input Window
------------

A input window is a special window that allows the user to
enter a line of text and provides a callback for accepting
the input.

Bullet Box
----------

A bullet box consists of icon and text object.

Check Box
---------

A check box is a specialization of a bullet box which
includes standard check box icons and a pointer to an
integer value for the check box state.

Graphics Box
------------

A graphics box is a widget which allows an app to embed 3D
graphics content. The graphics box uses a cooperative
rendering model where the widget library initializes the
scissor, viewport and clears the depth buffer (as needed).
This ensures the 3D graphics content is composed seamlessly
with other widgets. The widget library also routes action
events including multi-touch gestures to the graphics box.

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

Text Box
--------

A text box is a container for a paragraph of text which
automatically reflows.

Separator
---------

A separator is a thin line which can be used to separate
content.

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

An action bar may simply contain action buttons.

It is the users responsibility to manage the list of widgets
(e.g. you must clear all widgets added before deleting the
action bar).

Action Button
-------------

An action button is a icon that may be placed in an action
bar. These buttons may perform a click function, open an
action popup windows or transition to a new window.

Action Popup
------------

An action popup is a window that may be opened by an action
button in an action bar to provide additional selection
options.

Setup
=====

VKK UI is an optional library and must be enabled
by exporting the VKK\_USE\_UI variable in the top level
Makefile or CMakeLists.txt.

VKK UI requires that resources are packed into the
app resource file.
