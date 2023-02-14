/*
 * Copyright (c) 2015 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef vkk_uiWidget_H
#define vkk_uiWidget_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/math/cc_vec4f.h"

#define VKK_UI_WIDGET_ANCHOR_TL 0
#define VKK_UI_WIDGET_ANCHOR_TC 1
#define VKK_UI_WIDGET_ANCHOR_TR 2
#define VKK_UI_WIDGET_ANCHOR_CL 3
#define VKK_UI_WIDGET_ANCHOR_CC 4
#define VKK_UI_WIDGET_ANCHOR_CR 5
#define VKK_UI_WIDGET_ANCHOR_BL 6
#define VKK_UI_WIDGET_ANCHOR_BC 7
#define VKK_UI_WIDGET_ANCHOR_BR 8

#define VKK_UI_WIDGET_WRAP_SHRINK                0
#define VKK_UI_WIDGET_WRAP_STRETCH_PARENT        1
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL   2
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM  3
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE   4
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL   5
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM  6
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HLARGE   7
#define VKK_UI_WIDGET_WRAP_STRETCH_SCREEN        8
#define VKK_UI_WIDGET_WRAP_STRETCH_SCREEN_NARROW 9

#define VKK_UI_WIDGET_BORDER_NONE   0x00
#define VKK_UI_WIDGET_BORDER_SMALL  0x11
#define VKK_UI_WIDGET_BORDER_MEDIUM 0x22
#define VKK_UI_WIDGET_BORDER_LARGE  0x44
#define VKK_UI_WIDGET_BORDER_XLARGE 0x88

#define VKK_UI_WIDGET_BORDER_HSMALL   0x01
#define VKK_UI_WIDGET_BORDER_HMEDIUM  0x02
#define VKK_UI_WIDGET_BORDER_HLARGE   0x04
#define VKK_UI_WIDGET_BORDER_HXLARGE  0x08
#define VKK_UI_WIDGET_BORDER_VSMALL   0x10
#define VKK_UI_WIDGET_BORDER_VMEDIUM  0x20
#define VKK_UI_WIDGET_BORDER_VLARGE   0x40
#define VKK_UI_WIDGET_BORDER_VXLARGE  0x80

#define VKK_UI_WIDGET_ACTION_UP     0
#define VKK_UI_WIDGET_ACTION_CLICK  1
#define VKK_UI_WIDGET_ACTION_DOWN   2
#define VKK_UI_WIDGET_ACTION_DRAG   3
#define VKK_UI_WIDGET_ACTION_ROTATE 4
#define VKK_UI_WIDGET_ACTION_SCALE  5

// vertices per corner
#define VKK_UI_WIDGET_BEZEL 8

typedef struct vkk_uiWidgetActionInfo_s
{
	int        action;
	int        count;
	double     ts;
	double     dt;
	cc_vec2f_t coord0;
	cc_vec2f_t coord1;

	union
	{
		cc_vec2f_t drag;
		float      scale;
		float      angle;
	};
} vkk_uiWidgetActionInfo_t;

typedef vkk_uiWidget_t* (*vkk_uiWidgetAction_fn)(vkk_uiWidget_t* widget,
                                                 vkk_uiWidgetActionInfo_t* info);
typedef void (*vkk_uiWidgetAspectRatio_fn)(vkk_uiWidget_t* widget,
                                           float* ar);
typedef void (*vkk_uiWidgetClick_fn)(vkk_uiWidget_t* widget,
                                     float x, float y);
typedef void (*vkk_uiWidgetDrag_fn)(vkk_uiWidget_t* widget,
                                    float x, float y,
                                    float dx, float dy);
typedef void (*vkk_uiWidgetDraw_fn)(vkk_uiWidget_t* widget);
typedef void (*vkk_uiWidgetInput_fn)(vkk_uiWidget_t* widget,
                                     const char* string);
typedef int  (*vkk_uiWidgetKeyPress_fn)(vkk_uiWidget_t* widget,
                                        int keycode, int meta);
typedef void (*vkk_uiWidgetLayout_fn)(vkk_uiWidget_t* widget,
                                      int dragx, int dragy);
typedef void (*vkk_uiWidgetReflow_fn)(vkk_uiWidget_t* widget,
                                      float w, float h);
typedef int  (*vkk_uiWidgetRefresh_fn)(vkk_uiWidget_t* widget);
typedef void (*vkk_uiWidgetScrollTop_fn)(vkk_uiWidget_t* widget);
typedef void (*vkk_uiWidgetSize_fn)(vkk_uiWidget_t* widget,
                                    float* w, float* h);
typedef void (*vkk_uiWidgetValue_fn)(vkk_uiWidget_t* widget,
                                     int value);

typedef struct vkk_uiWidgetLayout_s
{
	// anchor
	//    the anchor point defines how a widget is attached to
	//    its parent/container widget
	//    e.g. an CC anchor for a widget that is added to a
	//    layer will cause the center of the widget to be
	//    placed at the center of the layer
	// horizontal/vertical wrapping
	//    shrink:
	//       size of children plus border
	//       border is outset from children
	//       children must be shrink
	//       (except lists which may have both)
	//    stretch:
	//       size of container
	//       border is inset from container
	//       children may be stretch or shrink
	//       top level widget must be stretch
	int   border;
	int   anchor;
	int   wrapx;
	int   wrapy;
	float stretchx;
	float stretchy;
} vkk_uiWidgetLayout_t;

typedef struct vkk_uiWidgetScroll_s
{
	int        scroll_bar;
	cc_vec4f_t color0;
	cc_vec4f_t color1;
} vkk_uiWidgetScroll_t;

typedef struct vkk_uiWidgetStyle_s
{
	cc_vec4f_t color_primary;
	cc_vec4f_t color_secondary;
	cc_vec4f_t color_text;
	cc_vec4f_t color_background;
} vkk_uiWidgetStyle_t;

typedef struct vkk_uiWidgetFn_s
{
	// priv and functions may be NULL

	void* priv;

	// action_fn allows a derived widget to receive action
	// events
	vkk_uiWidgetAction_fn action_fn;

	// aspect_fn allows a derived widget to define
	// the unstretched aspect ratio
	vkk_uiWidgetAspectRatio_fn aspect_fn;

	// click_fn allows a derived widget to receive click
	// events
	vkk_uiWidgetClick_fn click_fn;

	// drag_fn allows container widgets to drag their
	// children
	vkk_uiWidgetDrag_fn drag_fn;

	// draw_fn allows a derived widget to define the draw
	// behavior
	vkk_uiWidgetDraw_fn draw_fn;

	// input_fn allows a derived widget to generate string
	// events
	vkk_uiWidgetInput_fn input_fn;

	// keyPress_fn allows a derived widget to respond to key
	// events when focused
	vkk_uiWidgetKeyPress_fn keyPress_fn;

	// layout_fn allows a container widget to layout their
	// children
	vkk_uiWidgetLayout_fn layout_fn;

	// reflow_fn allows a derived widget to reflow
	// the content on a resize event (e.g. textbox)
	vkk_uiWidgetReflow_fn reflow_fn;

	// refresh_fn allows a derived widget to update the
	// contents prior to drawing
	vkk_uiWidgetRefresh_fn refresh_fn;

	// scrollTop_fn ensures widgets are at the top when
	// brought forward in a layer
	vkk_uiWidgetScrollTop_fn scrollTop_fn;

	// size_fn allows a container widget to define their
	// internal size (e.g. ignoring borders)
	vkk_uiWidgetSize_fn size_fn;

	// value_fn allows a derived widget to generate integer
	// events
	vkk_uiWidgetValue_fn value_fn;
} vkk_uiWidgetFn_t;

typedef struct vkk_uiWidget_s
{
	vkk_uiScreen_t* screen;

	// dragable rules (implicit widget property of drag event)
	// 1. wrapping must be shrink
	// 2. if widget is wider than clip then
	//    restrict right >= clipright and left <= clipleft
	// 3. if widget is taller than clip then
	//    restrict top <= cliptop and bottom >= clipbottom
	float drag_dx;
	float drag_dy;

	// defines rect after a layout update
	cc_rect1f_t rect_draw;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_border;
	cc_rect1f_t rect_scissor;

	// widget properties
	cc_vec4f_t           color;
	vkk_uiWidgetLayout_t layout;
	vkk_uiWidgetScroll_t scroll;
	vkk_uiWidgetFn_t     fn;

	// shader data
	vkk_buffer_t*     vb_xyuv;
	vkk_buffer_t*     ub10_color;
	vkk_uniformSet_t* us1_color;

	// optional tricolor shader
	// used by scroll bars and viewbox
	vkk_uiTricolor_t* tricolor;
} vkk_uiWidget_t;

vkk_uiWidget_t* vkk_uiWidget_new(vkk_uiScreen_t* screen,
                                 size_t wsize, cc_vec4f_t* color,
                                 vkk_uiWidgetLayout_t* layout,
                                 vkk_uiWidgetScroll_t* scroll,
                                 vkk_uiWidgetFn_t* fn);
vkk_uiWidget_t* vkk_uiWidget_newSpace(vkk_uiScreen_t* screen);
void            vkk_uiWidget_delete(vkk_uiWidget_t** _self);
void            vkk_uiWidget_override(vkk_uiWidget_t* self,
                                      vkk_uiWidgetFn_t* fn_new,
                                      vkk_uiWidgetFn_t* fn_old);
void            vkk_uiWidget_layoutXYClip(vkk_uiWidget_t* self,
                                          float x, float y,
                                          cc_rect1f_t* clip,
                                          int dragx, int dragy);
void            vkk_uiWidget_layoutSize(vkk_uiWidget_t* self,
                                        float* w, float* h);
void            vkk_uiWidget_layoutAnchor(vkk_uiWidget_t* self,
                                          cc_rect1f_t* rect,
                                          float* x, float * y);
vkk_uiWidget_t* vkk_uiWidget_action(vkk_uiWidget_t* self,
                                    vkk_uiWidgetActionInfo_t* info);
int             vkk_uiWidget_keyPress(vkk_uiWidget_t* self,
                                      int keycode, int meta);
void            vkk_uiWidget_drag(vkk_uiWidget_t* self,
                                  float x, float y,
                                  float dx, float dy);
void            vkk_uiWidget_draw(vkk_uiWidget_t* self);
void            vkk_uiWidget_refresh(vkk_uiWidget_t* self);
void            vkk_uiWidget_color(vkk_uiWidget_t* self,
                                   cc_vec4f_t* color);
int             vkk_uiWidget_tricolor(vkk_uiWidget_t* self,
                                      cc_vec4f_t* color0,
                                      cc_vec4f_t* color1,
                                      cc_vec4f_t* color2);
void            vkk_uiWidget_tricolorAB(vkk_uiWidget_t* self,
                                        float a, float b);
void            vkk_uiWidget_scrollTop(vkk_uiWidget_t* self);
int             vkk_uiWidget_hasFocus(vkk_uiWidget_t* self);
cc_rect1f_t*    vkk_uiWidget_rectDraw(vkk_uiWidget_t* self);
cc_rect1f_t*    vkk_uiWidget_rectScissor(vkk_uiWidget_t* self);
void*           vkk_uiWidget_priv(vkk_uiWidget_t* self);

// standard click handlers
// clickBack:
//    priv: NULL
// clickUrl:
//    priv: const char* url;
// clickTransition:
//    priv: vkk_uiWindow_t** _window;
//    note: use double pointer for window to solve
//    construction ordering issues
// value:
//    priv: int* _value;
void vkk_uiWidget_clickBack(vkk_uiWidget_t* widget,
                            float x, float y);
void vkk_uiWidget_clickUrl(vkk_uiWidget_t* widget,
                           float x, float y);
void vkk_uiWidget_clickTransition(vkk_uiWidget_t* widget,
                                  float x, float y);
void vkk_uiWidget_value(vkk_uiWidget_t* widget,
                        int value);

#endif
