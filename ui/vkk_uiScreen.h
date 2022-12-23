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

#ifndef vkk_uiScreen_H
#define vkk_uiScreen_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/cc_map.h"
#include "../../libcc/cc_multimap.h"

#define VKK_UI_SCREEN_SCALE_XSMALL 1
#define VKK_UI_SCREEN_SCALE_SMALL  2
#define VKK_UI_SCREEN_SCALE_MEDIUM 3
#define VKK_UI_SCREEN_SCALE_LARGE  4
#define VKK_UI_SCREEN_SCALE_XLARGE 5

#define VKK_UI_SCREEN_BIND_NONE     0
#define VKK_UI_SCREEN_BIND_COLOR    1
#define VKK_UI_SCREEN_BIND_IMAGE    2
#define VKK_UI_SCREEN_BIND_TEXT     3
#define VKK_UI_SCREEN_BIND_TRICOLOR 4

#define VKK_UI_SCREEN_ACTION_STATE_UP     0
#define VKK_UI_SCREEN_ACTION_STATE_DOWN   1
#define VKK_UI_SCREEN_ACTION_STATE_DRAG   2
#define VKK_UI_SCREEN_ACTION_STATE_ROTATE 3
#define VKK_UI_SCREEN_ACTION_STATE_SCALE  4

typedef struct vkk_uiScreen_s
{
	vkk_engine_t*            engine;
	vkk_renderer_t*          renderer;
	vkk_uniformSetFactory_t* usf0_mvp;
	vkk_uniformSetFactory_t* usf1_color;
	vkk_uniformSetFactory_t* usf2_multiplyImage;
	vkk_uniformSetFactory_t* usf3_tricolor;
	vkk_pipelineLayout_t*    pl;
	vkk_graphicsPipeline_t*  gp_color;
	vkk_graphicsPipeline_t*  gp_image;
	vkk_graphicsPipeline_t*  gp_text;
	vkk_graphicsPipeline_t*  gp_tricolor;
	vkk_buffer_t*            ub00_mvp;
	vkk_buffer_t*            ub10_color;
	vkk_buffer_t*            ub20_multiply;
	vkk_image_t*             img21;
	vkk_buffer_t*            ub30_color0;
	vkk_buffer_t*            ub31_color1;
	vkk_buffer_t*            ub32_color2;
	vkk_buffer_t*            ub33_ab;
	vkk_uniformSet_t*        us0_mvp;
	vkk_uniformSet_t*        us1_color;
	vkk_uniformSet_t*        us2_multiplyImage;
	vkk_uniformSet_t*        us3_tricolor;

	// graphics pipeline bound
	int gp_bound;

	// screen size
	uint32_t w;
	uint32_t h;
	float    density;

	// action event
	int        action_state;
	int        action_count;
	double     action_ts;
	cc_vec2f_t action_coord0;
	cc_vec2f_t action_coord1;
	cc_vec2f_t action_dragv;

	// content rect
	uint32_t content_rect_top;
	uint32_t content_rect_left;
	uint32_t content_rect_width;
	uint32_t content_rect_height;

	// UI scale factor
	int scale;

	// widget(s)
	cc_list_t*      window_stack;
	vkk_uiWidget_t* focus_widget;
	vkk_uiWidget_t* action_widget; // for action events

	// action bar/popup
	vkk_uiActionBar_t*   action_bar;
	vkk_uiActionPopup_t* action_popup;

	// layout dirty flag
	int layout_dirty;

	// depth dirty widgets
	cc_list_t* depth_dirty;

	// recource.bfs
	char resource[256];

	// widget style used by helper functions
	vkk_uiWidgetStyle_t widget_style;

	// sprite images
	cc_map_t* sprite_map;

	// font
	vkk_uiFont_t* font_array[3];

	// text vb
	cc_multimap_t* map_text_vb;
} vkk_uiScreen_t;

vkk_uiScreen_t* vkk_uiScreen_new(size_t wsize,
                                 vkk_engine_t* engine,
                                 vkk_renderer_t* renderer,
                                 const char* resource,
                                 vkk_uiWidgetStyle_t* widget_style);
void            vkk_uiScreen_delete(vkk_uiScreen_t** _self);
void            vkk_uiScreen_eventAction(vkk_uiScreen_t* self,
                                         vkk_platformEvent_t* event);
void            vkk_uiScreen_eventContentRect(vkk_uiScreen_t* self,
                                              vkk_platformEventContentRect_t* e);
void            vkk_uiScreen_eventDensity(vkk_uiScreen_t* self,
                                          float density);
int             vkk_uiScreen_eventKey(vkk_uiScreen_t* self,
                                      vkk_platformEventKey_t* e);
vkk_uiWindow_t* vkk_uiScreen_windowPeek(vkk_uiScreen_t* self);
void            vkk_uiScreen_windowPush(vkk_uiScreen_t* self,
                                        vkk_uiWindow_t* window);
int             vkk_uiScreen_windowPop(vkk_uiScreen_t* self);
void            vkk_uiScreen_windowReset(vkk_uiScreen_t* self,
                                         vkk_uiWindow_t* window);
void            vkk_uiScreen_popupGet(vkk_uiScreen_t* self,
                                      vkk_uiActionBar_t** _action_bar,
                                      vkk_uiActionPopup_t** _action_popup);
void            vkk_uiScreen_popupSet(vkk_uiScreen_t* self,
                                      vkk_uiActionBar_t* action_bar,
                                      vkk_uiActionPopup_t* action_popup);
void            vkk_uiScreen_detach(vkk_uiScreen_t* self,
                                    vkk_uiWidget_t* widget);
void            vkk_uiScreen_focus(vkk_uiScreen_t* self,
                                   vkk_uiWidget_t* focus);
void            vkk_uiScreen_rescale(vkk_uiScreen_t* self,
                                     int scale);
void            vkk_uiScreen_draw(vkk_uiScreen_t* self);
void            vkk_uiScreen_colorPageItem(vkk_uiScreen_t* self,
                                           cc_vec4f_t* color);
void            vkk_uiScreen_colorPageHeading(vkk_uiScreen_t* self,
                                              cc_vec4f_t* color);
void            vkk_uiScreen_colorPageImage(vkk_uiScreen_t* self,
                                            cc_vec4f_t* color);
void            vkk_uiScreen_colorPageLink(vkk_uiScreen_t* self,
                                           cc_vec4f_t* color);
void            vkk_uiScreen_colorPageEntry(vkk_uiScreen_t* self,
                                            cc_vec4f_t* color);
void            vkk_uiScreen_colorBanner(vkk_uiScreen_t* self,
                                         cc_vec4f_t* color);
void            vkk_uiScreen_colorBannerText(vkk_uiScreen_t* self,
                                             cc_vec4f_t* color);
void            vkk_uiScreen_colorActionIcon0(vkk_uiScreen_t* self,
                                              cc_vec4f_t* color);
void            vkk_uiScreen_colorActionIcon1(vkk_uiScreen_t* self,
                                              cc_vec4f_t* color);
void            vkk_uiScreen_colorStatusIcon(vkk_uiScreen_t* self,
                                             cc_vec4f_t* color);
void            vkk_uiScreen_colorFooterItem(vkk_uiScreen_t* self,
                                             cc_vec4f_t* color);
void            vkk_uiScreen_colorBackground(vkk_uiScreen_t* self,
                                             cc_vec4f_t* color);
void            vkk_uiScreen_colorScroll0(vkk_uiScreen_t* self,
                                          cc_vec4f_t* color);
void            vkk_uiScreen_colorScroll1(vkk_uiScreen_t* self,
                                          cc_vec4f_t* color);
void            vkk_uiScreen_sizei(vkk_uiScreen_t* self,
                                   int* w, int* h);
void            vkk_uiScreen_sizef(vkk_uiScreen_t* self,
                                   float* w, float* h);
int             vkk_uiScreen_scalei(vkk_uiScreen_t* self);
float           vkk_uiScreen_scalef(vkk_uiScreen_t* self);
void            vkk_uiScreen_layoutDirty(vkk_uiScreen_t* self);
void            vkk_uiScreen_layoutBorder(vkk_uiScreen_t* self,
                                          int border,
                                          float* hborder,
                                          float* vborder);
float           vkk_uiScreen_layoutText(vkk_uiScreen_t* self,
                                        int size);
int             vkk_uiScreen_depthDirty(vkk_uiScreen_t* self,
                                        vkk_uiWidget_t* widget);
void            vkk_uiScreen_depthMark(vkk_uiScreen_t* self,
                                       vkk_uiWidget_t* widget);
void            vkk_uiScreen_depthReset(vkk_uiScreen_t* self);
void            vkk_uiScreen_bind(vkk_uiScreen_t* self,
                                  int bind);
void            vkk_uiScreen_scissor(vkk_uiScreen_t* self,
                                     cc_rect1f_t* rect);
vkk_uiFont_t*   vkk_uiScreen_font(vkk_uiScreen_t* self,
                                  int font_type);
vkk_buffer_t*   vkk_uiScreen_textVb(vkk_uiScreen_t* self,
                                    size_t size,
                                    vkk_buffer_t* vb);
vkk_image_t*    vkk_uiScreen_spriteImage(vkk_uiScreen_t* self,
                                         const char* name,
                                         texgz_tex_t** _tex);

#endif
