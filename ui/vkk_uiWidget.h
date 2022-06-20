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

#define VKK_UI_WIDGET_WRAP_SHRINK               0
#define VKK_UI_WIDGET_WRAP_STRETCH_PARENT       1
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL  2
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM 3
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE  4
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL  5
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM 6
#define VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HLARGE  7

#define VKK_UI_WIDGET_BORDER_NONE   0x00
#define VKK_UI_WIDGET_BORDER_SMALL  0x11
#define VKK_UI_WIDGET_BORDER_MEDIUM 0x22
#define VKK_UI_WIDGET_BORDER_LARGE  0x44

#define VKK_UI_WIDGET_BORDER_HSMALL   0x01
#define VKK_UI_WIDGET_BORDER_HMEDIUM  0x02
#define VKK_UI_WIDGET_BORDER_HLARGE   0x04
#define VKK_UI_WIDGET_BORDER_VSMALL   0x10
#define VKK_UI_WIDGET_BORDER_VMEDIUM  0x20
#define VKK_UI_WIDGET_BORDER_VLARGE   0x40

#define VKK_UI_WIDGET_POINTER_UP   0
#define VKK_UI_WIDGET_POINTER_DOWN 1
#define VKK_UI_WIDGET_POINTER_MOVE 2

// vertices per corner
#define VKK_UI_WIDGET_BEZEL 8

typedef int  (*vkk_uiWidget_clickFn)(vkk_uiWidget_t* widget,
                                     int state,
                                     float x, float y);
typedef void (*vkk_uiWidget_refreshFn)(vkk_uiWidget_t* widget);
typedef void (*vkk_uiWidget_reflowFn)(vkk_uiWidget_t* widget,
                                      float w, float h);
typedef void (*vkk_uiWidget_sizeFn)(vkk_uiWidget_t* widget,
                                    float* w, float* h);
typedef void (*vkk_uiWidget_aspectRatioFn)(vkk_uiWidget_t* widget,
                                           float* ar);
typedef int  (*vkk_uiWidget_keyPressFn)(vkk_uiWidget_t* widget,
                                        int keycode, int meta);
typedef void (*vkk_uiWidget_layoutFn)(vkk_uiWidget_t* widget,
                                      int dragx, int dragy);
typedef void (*vkk_uiWidget_dragFn)(vkk_uiWidget_t* widget,
                                    float x, float y,
                                    float dx, float dy);
typedef void (*vkk_uiWidget_scrollTopFn)(vkk_uiWidget_t* widget);
typedef void (*vkk_uiWidget_drawFn)(vkk_uiWidget_t* widget);

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
	// priv, arg, msg and functions may be NULL

	void*                  priv;
	void*                  arg;
	char                   msg[256];
	vkk_uiWidget_clickFn   click_fn;
	vkk_uiWidget_refreshFn refresh_fn;
} vkk_uiWidgetFn_t;

typedef struct vkk_uiWidgetPrivFn_s
{
	// functions may be NULL

	// reflow_fn allows a derived widget to reflow
	// it's content in a resize (e.g. textbox)
	// called internally by vkk_uiWidget_layoutSize()
	vkk_uiWidget_reflowFn reflow_fn;

	// size_fn allows a derived widget to define
	// it's internal size (e.g. ignoring borders)
	// called internally by vkk_uiWidget_layoutSize()
	vkk_uiWidget_sizeFn size_fn;

	// aspect_fn allows a derived widget to define
	// it's unstretched aspect ratio
	// called internally by vkk_uiWidget_layoutSize()
	vkk_uiWidget_aspectRatioFn aspect_fn;

	// keyPress_fn allows a derived widget to define it's keyPress
	// behavior called internally by vkk_uiWidget_keyPress()
	// keyPress_fn uses the priv member from widgetFn base
	vkk_uiWidget_keyPressFn keyPress_fn;

	// layout_fn allows a derived widget to layout it's children
	// called internally by vkk_uiWidget_layoutXYClip
	vkk_uiWidget_layoutFn layout_fn;

	// drag_fn allows a derived widget to drag it's children
	// called internally by vkk_uiWidget_drag
	vkk_uiWidget_dragFn drag_fn;

	// scrollTop_fn ensures widgets are at the top when
	// brought forward in a layer
	// called internally by vkk_uiWidget_scrollTop
	vkk_uiWidget_scrollTopFn scrollTop_fn;

	// draw_fn allows a derived widget to define
	// it's draw behavior
	// called internally by vkk_uiWidget_draw
	vkk_uiWidget_drawFn draw_fn;
} vkk_uiWidgetPrivFn_t;

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

	// widget properties
	cc_vec4f_t           color;
	vkk_uiWidgetLayout_t layout;
	vkk_uiWidgetScroll_t scroll;
	vkk_uiWidgetFn_t     fn;
	vkk_uiWidgetPrivFn_t priv_fn;

	// sound fx for clicks
	int sound_fx;

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
                                 vkk_uiWidgetFn_t* fn,
                                 vkk_uiWidgetPrivFn_t* priv_fn);
vkk_uiWidget_t* vkk_uiWidget_newSpace(vkk_uiScreen_t* screen);
void           vkk_uiWidget_delete(vkk_uiWidget_t** _self);
void           vkk_uiWidget_layoutXYClip(vkk_uiWidget_t* self,
                                         float x, float y,
                                         cc_rect1f_t* clip,
                                         int dragx, int dragy);
void           vkk_uiWidget_layoutSize(vkk_uiWidget_t* self,
                                       float* w, float* h);
void           vkk_uiWidget_layoutAnchor(vkk_uiWidget_t* self,
                                         cc_rect1f_t* rect,
                                         float* x, float * y);
int            vkk_uiWidget_click(vkk_uiWidget_t* self,
                                  int state,
                                  float x, float y);
int            vkk_uiWidget_clickUrlFn(vkk_uiWidget_t* widget,
                                       int state,
                                       float x, float y);
int            vkk_uiWidget_keyPress(vkk_uiWidget_t* self,
                                     int keycode, int meta);
void           vkk_uiWidget_drag(vkk_uiWidget_t* self,
                                 float x, float y,
                                 float dx, float dy);
void           vkk_uiWidget_draw(vkk_uiWidget_t* self);
void           vkk_uiWidget_refresh(vkk_uiWidget_t* self);
void           vkk_uiWidget_soundFx(vkk_uiWidget_t* self,
                                    int sound_fx);
void           vkk_uiWidget_color(vkk_uiWidget_t* self,
                                  cc_vec4f_t* color);
int            vkk_uiWidget_tricolor(vkk_uiWidget_t* self,
                                     cc_vec4f_t* color0,
                                     cc_vec4f_t* color1,
                                     cc_vec4f_t* color2);
void           vkk_uiWidget_tricolorAB(vkk_uiWidget_t* self,
                                       float a, float b);
void           vkk_uiWidget_scrollTop(vkk_uiWidget_t* self);
int            vkk_uiWidget_hasFocus(vkk_uiWidget_t* self);
void           vkk_uiWidget_privReflowFn(vkk_uiWidget_t* self,
                                         vkk_uiWidget_reflowFn reflow_fn);
void*          vkk_uiWidget_widgetFnPriv(vkk_uiWidget_t* self);
void*          vkk_uiWidget_widgetFnArg(vkk_uiWidget_t* self);
const char*    vkk_uiWidget_widgetFnMsg(vkk_uiWidget_t* self);

#endif
