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

#ifndef vkui_widget_H
#define vkui_widget_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/math/cc_vec4f.h"

#define VKUI_WIDGET_ANCHOR_TL 0
#define VKUI_WIDGET_ANCHOR_TC 1
#define VKUI_WIDGET_ANCHOR_TR 2
#define VKUI_WIDGET_ANCHOR_CL 3
#define VKUI_WIDGET_ANCHOR_CC 4
#define VKUI_WIDGET_ANCHOR_CR 5
#define VKUI_WIDGET_ANCHOR_BL 6
#define VKUI_WIDGET_ANCHOR_BC 7
#define VKUI_WIDGET_ANCHOR_BR 8

#define VKUI_WIDGET_WRAP_SHRINK               0
#define VKUI_WIDGET_WRAP_STRETCH_PARENT       1
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_VSMALL  2
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM 3
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_VLARGE  4
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_HSMALL  5
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM 6
#define VKUI_WIDGET_WRAP_STRETCH_TEXT_HLARGE  7

#define VKUI_WIDGET_BORDER_NONE   0x00
#define VKUI_WIDGET_BORDER_SMALL  0x11
#define VKUI_WIDGET_BORDER_MEDIUM 0x22
#define VKUI_WIDGET_BORDER_LARGE  0x44

#define VKUI_WIDGET_BORDER_HSMALL   0x01
#define VKUI_WIDGET_BORDER_HMEDIUM  0x02
#define VKUI_WIDGET_BORDER_HLARGE   0x04
#define VKUI_WIDGET_BORDER_VSMALL   0x10
#define VKUI_WIDGET_BORDER_VMEDIUM  0x20
#define VKUI_WIDGET_BORDER_VLARGE   0x40

#define VKUI_WIDGET_POINTER_UP   0
#define VKUI_WIDGET_POINTER_DOWN 1
#define VKUI_WIDGET_POINTER_MOVE 2

// vertices per corner
#define VKUI_WIDGET_BEZEL 8

typedef int  (*vkui_widget_clickFn)(vkui_widget_t* widget,
                                    int state,
                                    float x, float y);
typedef void (*vkui_widget_refreshFn)(vkui_widget_t* widget);
typedef void (*vkui_widget_reflowFn)(vkui_widget_t* widget,
                                     float w, float h);
typedef void (*vkui_widget_sizeFn)(vkui_widget_t* widget,
                                   float* w, float* h);
typedef void (*vkui_widget_aspectRatioFn)(vkui_widget_t* widget,
                                          float* ar);
typedef int  (*vkui_widget_keyPressFn)(vkui_widget_t* widget,
                                       int keycode, int meta);
typedef void (*vkui_widget_layoutFn)(vkui_widget_t* widget,
                                     int dragx, int dragy);
typedef void (*vkui_widget_dragFn)(vkui_widget_t* widget,
                                   float x, float y,
                                   float dx, float dy);
typedef void (*vkui_widget_scrollTopFn)(vkui_widget_t* widget);
typedef void (*vkui_widget_drawFn)(vkui_widget_t* widget);

typedef struct vkui_widgetLayout_s
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
} vkui_widgetLayout_t;

typedef struct vkui_widgetScroll_s
{
	int        scroll_bar;
	cc_vec4f_t color0;
	cc_vec4f_t color1;
} vkui_widgetScroll_t;

typedef struct vkui_widgetStyle_s
{
	cc_vec4f_t color_primary;
	cc_vec4f_t color_secondary;
	cc_vec4f_t color_text;
	cc_vec4f_t color_background;
} vkui_widgetStyle_t;

typedef struct vkui_widgetFn_s
{
	// priv, arg, msg and functions may be NULL

	void*                 priv;
	void*                 arg;
	char                  msg[256];
	vkui_widget_clickFn   click_fn;
	vkui_widget_refreshFn refresh_fn;
} vkui_widgetFn_t;

typedef struct vkui_widgetPrivFn_s
{
	// functions may be NULL

	// reflow_fn allows a derived widget to reflow
	// it's content in a resize (e.g. textbox)
	// called internally by vkui_widget_layoutSize()
	vkui_widget_reflowFn reflow_fn;

	// size_fn allows a derived widget to define
	// it's internal size (e.g. ignoring borders)
	// called internally by vkui_widget_layoutSize()
	vkui_widget_sizeFn size_fn;

	// aspect_fn allows a derived widget to define
	// it's unstretched aspect ratio
	// called internally by vkui_widget_layoutSize()
	vkui_widget_aspectRatioFn aspect_fn;

	// keyPress_fn allows a derived widget to define it's keyPress
	// behavior called internally by vkui_widget_keyPress()
	// keyPress_fn uses the priv member from widgetFn base
	vkui_widget_keyPressFn keyPress_fn;

	// layout_fn allows a derived widget to layout it's children
	// called internally by vkui_widget_layoutXYClip
	vkui_widget_layoutFn layout_fn;

	// drag_fn allows a derived widget to drag it's children
	// called internally by vkui_widget_drag
	vkui_widget_dragFn drag_fn;

	// scrollTop_fn ensures widgets are at the top when
	// brought forward in a layer
	// called internally by vkui_widget_scrollTop
	vkui_widget_scrollTopFn scrollTop_fn;

	// draw_fn allows a derived widget to define
	// it's draw behavior
	// called internally by vkui_widget_draw
	vkui_widget_drawFn draw_fn;
} vkui_widgetPrivFn_t;

typedef struct vkui_widget_s
{
	vkui_screen_t* screen;

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
	cc_vec4f_t          color;
	vkui_widgetLayout_t layout;
	vkui_widgetScroll_t scroll;
	vkui_widgetFn_t     fn;
	vkui_widgetPrivFn_t priv_fn;

	// sound fx for clicks
	int sound_fx;

	// shader data
	vkk_buffer_t*     vb_xyuv;
	vkk_buffer_t*     ub10_color;
	vkk_uniformSet_t* us1_color;

	// optional tricolor shader
	// used by scroll bars and viewbox
	vkui_tricolor_t* tricolor;
} vkui_widget_t;

vkui_widget_t* vkui_widget_new(vkui_screen_t* screen,
                               size_t wsize, cc_vec4f_t* color,
                               vkui_widgetLayout_t* layout,
                               vkui_widgetScroll_t* scroll,
                               vkui_widgetFn_t* fn,
                               vkui_widgetPrivFn_t* priv_fn);
vkui_widget_t* vkui_widget_newSpace(vkui_screen_t* screen);
void           vkui_widget_delete(vkui_widget_t** _self);
void           vkui_widget_layoutXYClip(vkui_widget_t* self,
                                        float x, float y,
                                        cc_rect1f_t* clip,
                                        int dragx, int dragy);
void           vkui_widget_layoutSize(vkui_widget_t* self,
                                      float* w, float* h);
void           vkui_widget_layoutAnchor(vkui_widget_t* self,
                                        cc_rect1f_t* rect,
                                        float* x, float * y);
int            vkui_widget_click(vkui_widget_t* self,
                                 int state,
                                 float x, float y);
int            vkui_widget_clickUrlFn(vkui_widget_t* widget,
                                      int state,
                                      float x, float y);
int            vkui_widget_keyPress(vkui_widget_t* self,
                                    int keycode, int meta);
void           vkui_widget_drag(vkui_widget_t* self,
                                float x, float y,
                                float dx, float dy);
void           vkui_widget_draw(vkui_widget_t* self);
void           vkui_widget_refresh(vkui_widget_t* self);
void           vkui_widget_soundFx(vkui_widget_t* self,
                                   int sound_fx);
void           vkui_widget_color(vkui_widget_t* self,
                                 cc_vec4f_t* color);
int            vkui_widget_tricolor(vkui_widget_t* self,
                                    cc_vec4f_t* color0,
                                    cc_vec4f_t* color1,
                                    cc_vec4f_t* color2);
void           vkui_widget_tricolorAB(vkui_widget_t* self,
                                      float a, float b);
void           vkui_widget_scrollTop(vkui_widget_t* self);
int            vkui_widget_hasFocus(vkui_widget_t* self);
void           vkui_widget_privReflowFn(vkui_widget_t* self,
                                        vkui_widget_reflowFn reflow_fn);
void*          vkui_widget_widgetFnPriv(vkui_widget_t* self);
void*          vkui_widget_widgetFnArg(vkui_widget_t* self);
const char*    vkui_widget_widgetFnMsg(vkui_widget_t* self);

#endif
