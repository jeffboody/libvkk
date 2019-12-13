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

#ifndef vkui_screen_H
#define vkui_screen_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/cc_map.h"
#include "../../libcc/cc_multimap.h"
#include "vkui_font.h"
#include "vkui_sprite.h"
#include "vkui.h"

#define VKUI_SCREEN_BIND_NONE     0
#define VKUI_SCREEN_BIND_COLOR    1
#define VKUI_SCREEN_BIND_IMAGE    2
#define VKUI_SCREEN_BIND_TEXT     3
#define VKUI_SCREEN_BIND_TRICOLOR 4

typedef struct vkui_screen_s
{
	vkk_engine_t*            engine;
	vkk_renderer_t*          renderer;
	vkk_sampler_t*           sampler;
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
	vkk_uniformSet_t*        us0_mvp;

	// graphics pipeline bound
	int gp_bound;

	// screen size
	uint32_t w;
	uint32_t h;
	float    density;

	// UI scale factor
	int scale;

	// widget(s)
	vkui_widget_t* top_widget;
	vkui_widget_t* focus_widget;

	// layout dirty flag
	int dirty;

	// pointer generates click and drag events
	int    pointer_state;
	float  pointer_x0;
	float  pointer_y0;
	double pointer_t0;
	float  pointer_vx;
	float  pointer_vy;

	// resource.pak
	char resource[256];

	// sound fx
	int   clicked;
	void* sound_fx;
	vkui_screen_playClickFn playClick;

	// sprite images
	cc_map_t* sprite_map;

	// font
	vkui_font_t* font_array[2];

	// text vb
	cc_multimap_t* map_text_vb;
} vkui_screen_t;

void          vkui_screen_sizei(vkui_screen_t* self,
                                int* w, int* h);
void          vkui_screen_sizef(vkui_screen_t* self,
                                float* w, float* h);
int           vkui_screen_scalei(vkui_screen_t* self);
float         vkui_screen_scalef(vkui_screen_t* self);
void          vkui_screen_dirty(vkui_screen_t* self);
void          vkui_screen_layoutBorder(vkui_screen_t* self,
                                       int border,
                                       float* hborder,
                                       float* vborder);
float         vkui_screen_layoutHLine(vkui_screen_t* self,
                                      int size);
float         vkui_screen_layoutText(vkui_screen_t* self,
                                     int size);
void          vkui_screen_bind(vkui_screen_t* self,
                               int bind);
void          vkui_screen_scissor(vkui_screen_t* self,
                                  cc_rect1f_t* rect);
void          vkui_screen_playClick(vkui_screen_t* self);
vkui_font_t*  vkui_screen_font(vkui_screen_t* self,
                               int font_type);
vkk_buffer_t* vkui_screen_textVb(vkui_screen_t* self,
                                 uint32_t size,
                                 vkk_buffer_t* vb);

vkk_image_t*
vkui_screen_spriteImage(vkui_screen_t* self,
                        const char* name,
                        texgz_tex_t** _tex);

#endif
