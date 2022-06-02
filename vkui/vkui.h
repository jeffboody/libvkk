/*
 * Copyright (c) 2019 Jeff Boody
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

#ifndef vkui_H
#define vkui_H

#include "../../libcc/math/cc_vec4f.h"
#include "../../libcc/cc_list.h"
#include "../vkk.h"

/*
 * constants
 */

#define VKUI_HLINE_SIZE_SMALL  0
#define VKUI_HLINE_SIZE_MEDIUM 1
#define VKUI_HLINE_SIZE_LARGE  2

#define VKUI_LISTBOX_ORIENTATION_VERTICAL   0
#define VKUI_LISTBOX_ORIENTATION_HORIZONTAL 1

#define VKUI_SCREEN_SCALE_XSMALL 1
#define VKUI_SCREEN_SCALE_SMALL  2
#define VKUI_SCREEN_SCALE_MEDIUM 3
#define VKUI_SCREEN_SCALE_LARGE  4
#define VKUI_SCREEN_SCALE_XLARGE 5

#define VKUI_TEXT_FONTTYPE_REGULAR 0
#define VKUI_TEXT_FONTTYPE_BOLD    1
#define VKUI_TEXT_FONTTYPE_MEDIUM  2

#define VKUI_TEXT_SIZE_XSMALL -1
#define VKUI_TEXT_SIZE_SMALL  0
#define VKUI_TEXT_SIZE_MEDIUM 1
#define VKUI_TEXT_SIZE_LARGE  2

#define VKUI_TEXT_SPACING_NONE   0x00
#define VKUI_TEXT_SPACING_SMALL  0x10
#define VKUI_TEXT_SPACING_MEDIUM 0x20
#define VKUI_TEXT_SPACING_LARGE  0x40

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

/*
 * opaque objects
 */

// screen
typedef struct vkui_screen_s vkui_screen_t;

// widgets
typedef struct vkui_bulletbox_s    vkui_bulletbox_t;
typedef struct vkui_checkbox_s     vkui_checkbox_t;
typedef struct vkui_hline_s        vkui_hline_t;
typedef struct vkui_infoPanel_s    vkui_infoPanel_t;
typedef struct vkui_layer_s        vkui_layer_t;
typedef struct vkui_listbox_s      vkui_listbox_t;
typedef struct vkui_radiolist_s    vkui_radiolist_t;
typedef struct vkui_sprite_s       vkui_sprite_t;
typedef struct vkui_statusBar_s    vkui_statusBar_t;
typedef struct vkui_text_s         vkui_text_t;
typedef struct vkui_textbox_s      vkui_textbox_t;
typedef struct vkui_viewbox_s      vkui_viewbox_t;
typedef struct vkui_widget_s       vkui_widget_t;
typedef struct vkui_widgetLayout_s vkui_widgetLayout_t;

/*
 * function calllbacks
 */

typedef void (*vkui_screen_playClickFn)(void* sound_fx);
typedef void (*vkui_text_enterFn)(vkui_widget_t* widget,
                                  void* priv,
                                  const char* string);
typedef int (*vkui_widget_clickFn)(vkui_widget_t* widget,
                                   void* priv, int state,
                                   float x, float y);
typedef void (*vkui_widget_refreshFn)(vkui_widget_t* widget,
                                      void* priv);

/*
 * parameter structures
 */

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

typedef struct vkui_widgetFn_s
{
	// priv and functions may be NULL

	void*                 priv;
	vkui_widget_clickFn   click_fn;
	vkui_widget_refreshFn refresh_fn;
} vkui_widgetFn_t;

typedef struct vkui_textLayout_s
{
	int   border;
	int   anchor;
	int   wrapx;
	float stretchx;
} vkui_textLayout_t;

typedef struct vkui_textFn_s
{
	// functions may be NULL

	vkui_widgetFn_t   fn;
	vkui_text_enterFn enter_fn;
} vkui_textFn_t;

typedef struct vkui_textStyle_s
{
	int        font_type;
	int        size;
	int        spacing;
	cc_vec4f_t color;
} vkui_textStyle_t;

typedef struct vkui_bulletboxStyle_s
{
	cc_vec4f_t       color_icon;
	vkui_textStyle_t text_style;
} vkui_bulletboxStyle_t;

typedef struct vkui_viewboxStyle_s
{
	cc_vec4f_t color_header;
	cc_vec4f_t color_body;
	cc_vec4f_t color_footer;
	vkui_bulletboxStyle_t bulletbox_style;
} vkui_viewboxStyle_t;

typedef struct vkui_widgetStyle_s
{
	cc_vec4f_t color_primary;
	cc_vec4f_t color_secondary;
	cc_vec4f_t color_text;
	cc_vec4f_t color_background;
} vkui_widgetStyle_t;

/*
 * screen API
 */

vkui_screen_t* vkui_screen_new(vkk_engine_t* engine,
                               vkk_renderer_t* renderer,
                               const char* resource,
                               void* sound_fx,
                               vkui_screen_playClickFn playClick,
                               vkui_widgetStyle_t* widget_style);
void           vkui_screen_delete(vkui_screen_t** _self);
vkui_widget_t* vkui_screen_top(vkui_screen_t* self,
                               vkui_widget_t* top);
void           vkui_screen_contentRect(vkui_screen_t* self,
                                       int t, int l,
                                       int b, int r);
void           vkui_screen_focus(vkui_screen_t* self,
                                 vkui_widget_t* focus);
void           vkui_screen_move(vkui_screen_t* self,
                                vkui_widget_t* move);
void           vkui_screen_density(vkui_screen_t* self,
                                   float density);
void           vkui_screen_rescale(vkui_screen_t* self,
                                   int scale);
int            vkui_screen_pointerDown(vkui_screen_t* self,
                                       float x, float y,
                                       double t0);
int            vkui_screen_pointerUp(vkui_screen_t* self,
                                     float x, float y,
                                     double t0);
int            vkui_screen_pointerMove(vkui_screen_t* self,
                                       float x, float y,
                                       double t0);
int            vkui_screen_keyPress(vkui_screen_t* self,
                                    int keycode, int meta);
void           vkui_screen_draw(vkui_screen_t* self);
void           vkui_screen_colorPageItem(vkui_screen_t* self,
                                         cc_vec4f_t* color);
void           vkui_screen_colorPageHeading(vkui_screen_t* self,
                                            cc_vec4f_t* color);
void           vkui_screen_colorPageEntry(vkui_screen_t* self,
                                          cc_vec4f_t* color);
void           vkui_screen_colorFooterItem(vkui_screen_t* self,
                                           cc_vec4f_t* color);
void           vkui_screen_colorBackground(vkui_screen_t* self,
                                           cc_vec4f_t* color);

/*
 * widget API
 */

// bulletbox
vkui_bulletbox_t* vkui_bulletbox_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     int anchor,
                                     vkui_widgetFn_t* fn,
                                     vkui_bulletboxStyle_t* bulletbox_style,
                                     const char** sprite_array);
vkui_bulletbox_t* vkui_bulletbox_newPageItem(vkui_screen_t* screen,
                                             size_t wsize,
                                             vkui_widgetFn_t* fn,
                                             const char** sprite_array);
vkui_bulletbox_t* vkui_bulletbox_newFooterItem(vkui_screen_t* screen,
                                               size_t wsize,
                                               vkui_widgetFn_t* fn,
                                               const char** sprite_array);
void              vkui_bulletbox_delete(vkui_bulletbox_t** _self);
void              vkui_bulletbox_select(vkui_bulletbox_t* self,
                                        uint32_t index);
void              vkui_bulletbox_colorIcon(vkui_bulletbox_t* self,
                                           cc_vec4f_t* color);
void              vkui_bulletbox_colorText(vkui_bulletbox_t* self,
                                           cc_vec4f_t* color);
void              vkui_bulletbox_label(vkui_bulletbox_t* self,
                                       const char* fmt, ...);

// checkbox
vkui_checkbox_t* vkui_checkbox_new(vkui_screen_t* screen,
                                   size_t wsize,
                                   vkui_bulletboxStyle_t* bulletbox_style,
                                   int* pvalue);
vkui_checkbox_t* vkui_checkbox_newPageItem(vkui_screen_t* screen,
                                           size_t wsize,
                                           int* pvalue);
void             vkui_checkbox_delete(vkui_checkbox_t** _self);
void             vkui_checkbox_label(vkui_checkbox_t* self,
                                     const char* fmt, ...);

// hline
vkui_hline_t* vkui_hline_new(vkui_screen_t* screen,
                             size_t wsize,
                             int size,
                             cc_vec4f_t* color);
vkui_hline_t* vkui_hline_newPageItem(vkui_screen_t* screen);
vkui_hline_t* vkui_hline_newInfoItem(vkui_screen_t* screen);
void          vkui_hline_delete(vkui_hline_t** _self);

// layer
vkui_layer_t*  vkui_layer_new(vkui_screen_t* screen,
                              size_t wsize,
                              vkui_widgetLayout_t* layout,
                              cc_vec4f_t* color);
void           vkui_layer_delete(vkui_layer_t** _self);
void           vkui_layer_clear(vkui_layer_t* self);
int            vkui_layer_add(vkui_layer_t* self,
                              vkui_widget_t* widget);
cc_listIter_t* vkui_layer_head(vkui_layer_t* self);
vkui_widget_t* vkui_layer_remove(vkui_layer_t* self,
                                 cc_listIter_t** _iter);

// listbox
vkui_listbox_t* vkui_listbox_new(vkui_screen_t* screen,
                                 size_t wsize,
                                 vkui_widgetLayout_t* layout,
                                 vkui_widgetScroll_t* scroll,
                                 vkui_widgetFn_t* fn,
                                 int orientation,
                                 cc_vec4f_t* color);
void            vkui_listbox_delete(vkui_listbox_t** _self);
void            vkui_listbox_clear(vkui_listbox_t* self);
int             vkui_listbox_add(vkui_listbox_t* self,
                                 vkui_widget_t* widget);
int             vkui_listbox_addSorted(vkui_listbox_t* self,
                                       cc_listcmp_fn compare,
                                       vkui_widget_t* widget);
cc_listIter_t*  vkui_listbox_head(vkui_listbox_t* self);
vkui_widget_t*  vkui_listbox_remove(vkui_listbox_t* self,
                                    cc_listIter_t** _iter);

// radiolist
vkui_radiolist_t* vkui_radiolist_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     vkui_widgetLayout_t* layout,
                                     vkui_widgetScroll_t* scroll,
                                     vkui_bulletboxStyle_t* bulletbox_style,
                                     int* pvalue);
vkui_radiolist_t* vkui_radiolist_newPageItem(vkui_screen_t* screen,
                                             int* pvalue);
void              vkui_radiolist_delete(vkui_radiolist_t** _self);
void              vkui_radiolist_clear(vkui_radiolist_t* self);
void              vkui_radiolist_add(vkui_radiolist_t* self,
                                     int value,
                                     const char* fmt, ...);

// sprite
vkui_sprite_t* vkui_sprite_new(vkui_screen_t* screen,
                               size_t wsize,
                               vkui_widgetLayout_t* layout,
                               vkui_widgetFn_t* fn,
                               cc_vec4f_t* color,
                               const char** sprite_array);
void          vkui_sprite_delete(vkui_sprite_t** _self);
void          vkui_sprite_select(vkui_sprite_t* self,
                                 uint32_t index);
void          vkui_sprite_rotate(vkui_sprite_t* self,
                                 float theta);
void          vkui_sprite_color(vkui_sprite_t* self,
                                cc_vec4f_t* color);
void          vkui_sprite_fill(vkui_sprite_t* self,
                               cc_vec4f_t* color);

// text
vkui_text_t* vkui_text_new(vkui_screen_t* screen,
                           size_t wsize,
                           vkui_textLayout_t* text_layout,
                           vkui_textStyle_t* text_style,
                           vkui_textFn_t* text_fn,
                           cc_vec4f_t* color_fill);
vkui_text_t* vkui_text_newPageHeading(vkui_screen_t* screen);
vkui_text_t* vkui_text_newPageTextEntry(vkui_screen_t* screen,
                                        void* priv,
                                        vkui_text_enterFn enter_fn);
vkui_text_t* vkui_text_newInfoHeading(vkui_screen_t* screen);
vkui_text_t* vkui_text_newInfoItem(vkui_screen_t* screen);
void         vkui_text_delete(vkui_text_t** _self);
void         vkui_text_color(vkui_text_t* self,
                             cc_vec4f_t* color);
int          vkui_text_label(vkui_text_t* self,
                             const char* fmt, ...);

// textbox
vkui_textbox_t* vkui_textbox_new(vkui_screen_t* screen,
                                 size_t wsize,
                                 vkui_widgetLayout_t* layout,
                                 vkui_widgetScroll_t* scroll,
                                 vkui_widgetFn_t* fn,
                                 vkui_textStyle_t* text_style);
void           vkui_textbox_delete(vkui_textbox_t** _self);
void           vkui_textbox_clear(vkui_textbox_t* self);
void           vkui_textbox_printf(vkui_textbox_t* self,
                                   const char* fmt, ...);

// viewbox
vkui_viewbox_t* vkui_viewbox_new(vkui_screen_t* screen,
                                 size_t wsize,
                                 vkui_widgetLayout_t* layout,
                                 vkui_widgetFn_t* fn,
                                 vkui_viewboxStyle_t* viewbox_style,
                                 const char** sprite_array,
                                 vkui_widget_t* body,
                                 vkui_widget_t* footer);
void            vkui_viewbox_delete(vkui_viewbox_t** _self);
void            vkui_viewbox_select(vkui_viewbox_t* self,
                                    uint32_t index);
void            vkui_viewbox_label(vkui_viewbox_t* self,
                                   const char* fmt, ...);

// statusBar
vkui_statusBar_t* vkui_statusBar_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     vkui_widgetFn_t* widget_fn);
void              vkui_statusBar_delete(vkui_statusBar_t** _self);
void              vkui_statusBar_add(vkui_statusBar_t* self,
                                     vkui_widget_t* widget);
void              vkui_statusBar_clear(vkui_statusBar_t* self);

// infoPanel
vkui_infoPanel_t* vkui_infoPanel_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     vkui_widgetFn_t* widget_fn);
void              vkui_infoPanel_delete(vkui_infoPanel_t** _self);
void              vkui_infoPanel_add(vkui_infoPanel_t* self,
                                     vkui_widget_t* widget);
void              vkui_infoPanel_clear(vkui_infoPanel_t* self);

#endif
