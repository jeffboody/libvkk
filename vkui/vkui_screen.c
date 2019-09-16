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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/cc_log.h"
#include "vkui_key.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static const char VKUI_SHIFTKEYS[128] =
{
	0x00,
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x0D,
	0x0E,
	0x0F,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x17,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x1C,
	0x1D,
	0x1E,
	0x1F,
	0x20,
	0x21,
	0x22,
	0x23,
	0x24,
	0x25,
	0x26,
	'\"',
	0x28,
	0x29,
	0x2A,
	0x2B,
	'<',
	'_',
	'>',
	'?',
	')',
	'!',
	'@',
	'#',
	'$',
	'%',
	'^',
	'&',
	'*',
	'(',
	0x3A,
	':',
	0x3C,
	'+',
	0x3E,
	0x3F,
	0x40,
	0x41,
	0x42,
	0x43,
	0x44,
	0x45,
	0x46,
	0x47,
	0x48,
	0x49,
	0x4A,
	0x4B,
	0x4C,
	0x4D,
	0x4E,
	0x4F,
	0x50,
	0x51,
	0x52,
	0x53,
	0x54,
	0x55,
	0x56,
	0x57,
	0x58,
	0x59,
	0x5A,
	'{',
	'|',
	'}',
	0x5E,
	0x5F,
	'~',
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	0x7B,
	0x7C,
	0x7D,
	0x7E,
	0x7F,
};

static int vkui_shiftkeycode(int keycode, int meta)
{
	if((keycode >= 0) && (keycode < 128))
	{
		int shiftchar = meta & (VKUI_KEY_SHIFT | VKUI_KEY_CAPS);
		int shiftsym  = meta & VKUI_KEY_SHIFT;
		if(shiftchar && (keycode >= 'a') && (keycode <= 'z'))
		{
			return VKUI_SHIFTKEYS[keycode];
		}
		else if(shiftsym)
		{
			return VKUI_SHIFTKEYS[keycode];
		}
	}
	else if(keycode == VKUI_KEY_DELETE)
	{
		return VKUI_KEY_INSERT;
	}
	return keycode;
}

/***********************************************************
* protected                                                *
***********************************************************/

void vkui_screen_sizei(vkui_screen_t* self, int* w, int* h)
{
	assert(self);

	*w = self->w;
	*h = self->h;
}

void
vkui_screen_sizef(vkui_screen_t* self, float* w, float* h)
{
	assert(self);

	*w = (float) self->w;
	*h = (float) self->h;
}

int vkui_screen_scalei(vkui_screen_t* self)
{
	assert(self);

	return self->scale;
}

float vkui_screen_scalef(vkui_screen_t* self)
{
	assert(self);

	if(self->scale == VKUI_SCREEN_SCALE_XSMALL)
	{
		return 0.79f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_SMALL)
	{
		return 0.89f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_LARGE)
	{
		return 1.13f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_XLARGE)
	{
		return 1.27f;
	}
	return 1.0f;
}

void vkui_screen_dirty(vkui_screen_t* self)
{
	assert(self);

	self->dirty = 1;
}

void
vkui_screen_layoutBorder(vkui_screen_t* self, int border,
                         float* hborder, float* vborder)
{
	assert(self);
	assert(hborder);
	assert(vborder);

	*hborder = 0.0f;
	*vborder = 0.0f;

	float size = vkui_screen_layoutText(self,
	                                    VKUI_TEXT_SIZE_MEDIUM);

	// horizontal
	if(border & VKUI_WIDGET_BORDER_HSMALL)
	{
		*hborder = 0.125f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_HMEDIUM)
	{
		*hborder = 0.25f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_HLARGE)
	{
		*hborder = 0.5f*size;
	}

	// vertical
	if(border & VKUI_WIDGET_BORDER_VSMALL)
	{
		*vborder = 0.125f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_VMEDIUM)
	{
		*vborder = 0.25f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_VLARGE)
	{
		*vborder = 0.5f*size;
	}
}

float vkui_screen_layoutHLine(vkui_screen_t* self, int size)
{
	assert(self);

	if(size == VKUI_HLINE_SIZE_SMALL)
	{
		return 3.0f;
	}
	else if(size == VKUI_HLINE_SIZE_LARGE)
	{
		return 9.0f;
	}
	return 6.0f;
}

float vkui_screen_layoutText(vkui_screen_t* self, int size)
{
	assert(self);

	// default size is 24 px at density 1.0
	float sizef = 24.0f*self->density*vkui_screen_scalef(self);
	if(size == VKUI_TEXT_SIZE_SMALL)
	{
		return 0.66f*sizef;
	}
	else if(size == VKUI_TEXT_SIZE_LARGE)
	{
		return 1.5f*sizef;
	}
	return sizef;
}

void vkui_screen_bind(vkui_screen_t* self, int bind)
{
	assert(self);

	if(bind == self->gp_bound)
	{
		return;
	}

	if(bind == VKUI_SCREEN_BIND_COLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_color);

		vkk_renderer_bindUniformSets(self->renderer,
		                             self->pl, 1,
		                             &self->us_mvp);
	}
	else if(bind == VKUI_SCREEN_BIND_IMAGE)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_image);
	}
	else if(bind == VKUI_SCREEN_BIND_TRICOLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_tricolor);
	}

	self->gp_bound = bind;
}

void
vkui_screen_scissor(vkui_screen_t* self, cc_rect1f_t* rect)
{
	assert(self);
	assert(rect);

	// TODO - verify scissor
	vkk_renderer_scissor(self->renderer,
	                     (uint32_t) (rect->l + 0.5f),
	                     (uint32_t) (rect->t + 0.5f),
	                     (uint32_t) (rect->w + 0.5f),
	                     (uint32_t) (rect->h + 0.5f));
}

void vkui_screen_playClick(vkui_screen_t* self)
{
	assert(self);

	self->clicked = 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_screen_t*
vkui_screen_new(vkk_engine_t* engine,
                const char* resource,
                void* sound_fx,
                vkui_screen_playClickFn playClick)
{
	assert(engine);
	assert(resource);
	assert(sound_fx);
	assert(playClick);

	vkui_screen_t* self;
	self = (vkui_screen_t*) calloc(1, sizeof(vkui_screen_t));
	if(self == NULL)
	{
		LOGE("calloc failed");
		return NULL;
	}

	self->renderer = vkk_engine_renderer(engine);

	self->sampler = vkk_engine_newSampler(engine,
	                                      VKK_SAMPLER_FILTER_LINEAR,
	                                      VKK_SAMPLER_FILTER_LINEAR,
	                                      VKK_SAMPLER_MIPMAP_MODE_NEAREST);
	if(self->sampler == NULL)
	{
		goto fail_sampler;
	}

	vkk_uniformBinding_t ub0_array[1] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_VS,
			.sampler  = NULL
		},
	};

	self->usf0_mvp = vkk_engine_newUniformSetFactory(engine,
	                                                 VKK_UPDATE_MODE_DEFAULT,
	                                                 1,
	                                                 ub0_array);
	if(self->usf0_mvp == NULL)
	{
		goto fail_usf0_mvp;
	}

	vkk_uniformBinding_t ub1_array[1] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
	};

	self->usf1_color = vkk_engine_newUniformSetFactory(engine,
	                                                   VKK_UPDATE_MODE_STATIC,
	                                                   1,
	                                                   ub1_array);
	if(self->usf1_color == NULL)
	{
		goto fail_usf1_color;
	}

	vkk_uniformBinding_t ub2_array[1] =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
	};

	self->usf2_multiply = vkk_engine_newUniformSetFactory(engine,
	                                                      VKK_UPDATE_MODE_DEFAULT,
	                                                      1,
	                                                      ub2_array);
	if(self->usf2_multiply == NULL)
	{
		goto fail_usf2_multiply;
	}

	vkk_uniformBinding_t ub3_array[1] =
	{
		// layout(set=3, binding=0) uniform sampler2D image;
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_SAMPLER,
			.stage    = VKK_STAGE_FS,
			.sampler  = self->sampler
		},
	};

	self->usf3_image = vkk_engine_newUniformSetFactory(engine,
	                                                   VKK_UPDATE_MODE_STATIC,
	                                                   1,
	                                                   ub3_array);
	if(self->usf3_image == NULL)
	{
		goto fail_usf3_image;
	}

	vkk_uniformBinding_t ub4_array[4] =
	{
		// layout(std140, set=4, binding=0) uniform uniformHeader
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
		// layout(std140, set=4, binding=1) uniform uniformBody
		{
			.binding  = 1,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
		// layout(std140, set=4, binding=2) uniform uniformFooter
		{
			.binding  = 2,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
		// layout(std140, set=4, binding=3) uniform uniformAb
		{
			.binding  = 3,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
			.sampler  = NULL
		},
	};

	self->usf4_tricolor = vkk_engine_newUniformSetFactory(engine,
	                                                      VKK_UPDATE_MODE_DEFAULT,
	                                                      4,
	                                                      ub4_array);
	if(self->usf4_tricolor == NULL)
	{
		goto fail_usf4_tricolor;
	}

	vkk_uniformSetFactory_t* usf_array[5] =
	{
		self->usf0_mvp,
		self->usf1_color,
		self->usf2_multiply,
		self->usf3_image,
		self->usf4_tricolor,
	};

	self->pl = vkk_engine_newPipelineLayout(engine, 5,
	                                        usf_array);
	if(self->pl == NULL)
	{
		goto fail_pl;
	}

	vkk_vertexBufferInfo_t vbi =
	{
		.location   = 0,
		.components = 4,
		.format     = VKK_VERTEX_FORMAT_FLOAT
	};

	vkk_graphicsPipelineInfo_t gpi_color =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkui/default_vert.spv",
		.fs                = "vkui/color_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_color = vkk_engine_newGraphicsPipeline(engine,
	                                                &gpi_color);
	if(self->gp_color == NULL)
	{
		goto fail_gp_color;
	}

	vkk_graphicsPipelineInfo_t gpi_image =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkui/default_vert.spv",
		.fs                = "vkui/image_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_image = vkk_engine_newGraphicsPipeline(engine,
	                                                &gpi_image);
	if(self->gp_image == NULL)
	{
		goto fail_gp_image;
	}

	vkk_graphicsPipelineInfo_t gpi_tricolor =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkui/default_vert.spv",
		.fs                = "vkui/tricolor_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_tricolor = vkk_engine_newGraphicsPipeline(engine,
	                                                   &gpi_tricolor);
	if(self->gp_tricolor == NULL)
	{
		goto fail_gp_tricolor;
	}

	self->ub_mvp = vkk_engine_newBuffer(engine,
	                                    VKK_UPDATE_MODE_DEFAULT,
	                                    VKK_BUFFER_USAGE_UNIFORM,
	                                    sizeof(cc_mat4f_t),
	                                    NULL);
	if(self->ub_mvp == NULL)
	{
		goto fail_ub_mvp;
	}

	vkk_uniformAttachment_t ua_array_mvp[1] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub_mvp
		}
	};

	self->us_mvp = vkk_engine_newUniformSet(engine, 0, 1,
	                                        ua_array_mvp,
	                                        self->usf0_mvp);
	if(self->us_mvp == NULL)
	{
		goto fail_us_mvp;
	}

	self->engine        = engine;
	self->density       = 1.0f;
	self->scale         = VKUI_SCREEN_SCALE_MEDIUM;
	self->top_widget    = NULL;
	self->focus_widget  = NULL;
	self->dirty         = 1;
	self->pointer_state = VKUI_WIDGET_POINTER_UP;
	self->sound_fx      = sound_fx;
	self->playClick     = playClick;

	snprintf(self->resource, 256, "%s", resource);

	// success
	return self;

	// failure
	fail_us_mvp:
		vkk_engine_deleteBuffer(engine, &self->ub_mvp);
	fail_ub_mvp:
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_tricolor);
	fail_gp_tricolor:
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_image);
	fail_gp_image:
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_color);
	fail_gp_color:
		vkk_engine_deletePipelineLayout(engine, &self->pl);
	fail_pl:
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf4_tricolor);
	fail_usf4_tricolor:
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf3_image);
	fail_usf3_image:
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf2_multiply);
	fail_usf2_multiply:
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf1_color);
	fail_usf1_color:
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf0_mvp);
	fail_usf0_mvp:
		vkk_engine_deleteSampler(engine, &self->sampler);
	fail_sampler:
		free(self);
	return NULL;
}

void vkui_screen_delete(vkui_screen_t** _self)
{
	assert(_self);

	vkui_screen_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		vkk_engine_deleteUniformSet(engine, &self->us_mvp);
		vkk_engine_deleteBuffer(engine, &self->ub_mvp);
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_tricolor);
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_image);
		vkk_engine_deleteGraphicsPipeline(engine,
		                                  &self->gp_color);
		vkk_engine_deletePipelineLayout(engine, &self->pl);
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf4_tricolor);
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf3_image);
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf2_multiply);
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf1_color);
		vkk_engine_deleteUniformSetFactory(engine,
		                                   &self->usf0_mvp);
		vkk_engine_deleteSampler(engine, &self->sampler);
		free(self);
		*_self = NULL;
	}
}

void
vkui_screen_top(vkui_screen_t* self, vkui_widget_t* top)
{
	assert(self);

	if(self->top_widget == top)
	{
		return;
	}

	self->top_widget = top;
	self->dirty      = 1;
}

void
vkui_screen_focus(vkui_screen_t* self, vkui_widget_t* focus)
{
	// focus may be NULL
	assert(self);

	self->focus_widget = focus;
}

void vkui_screen_resize(vkui_screen_t* self, int w, int h)
{
	assert(self);

	if((self->w == w) && (self->h == h))
	{
		return;
	}

	self->w     = w;
	self->h     = h;
	self->dirty = 1;
}

void vkui_screen_density(vkui_screen_t* self, float density)
{
	assert(self);

	if(self->density == density)
	{
		return;
	}

	self->density = density;
	self->dirty   = 1;
}

void vkui_screen_rescale(vkui_screen_t* self, int scale)
{
	assert(self);

	if(self->scale == scale)
	{
		return;
	}

	if((scale >= VKUI_SCREEN_SCALE_XSMALL) &&
	   (scale <= VKUI_SCREEN_SCALE_XLARGE))
	{
		self->scale = scale;
		self->dirty = 1;
	}
}

int vkui_screen_pointerDown(vkui_screen_t* self,
                            float x, float y, double t0)
{
	assert(self);

	if((self->top_widget == NULL) ||
	   (self->pointer_state != VKUI_WIDGET_POINTER_UP))
	{
		// ignore
		return 0;
	}

	if(vkui_widget_click(self->top_widget,
	                     VKUI_WIDGET_POINTER_DOWN, x, y))
	{
		self->pointer_state = VKUI_WIDGET_POINTER_DOWN;
		self->pointer_x0    = x;
		self->pointer_y0    = y;
		self->pointer_t0    = t0;
		self->pointer_vx    = 0.0f;
		self->pointer_vy    = 0.0f;
		return 1;
	}

	return 0;
}

int vkui_screen_pointerUp(vkui_screen_t* self,
                          float x, float y, double t0)
{
	assert(self);

	int touch = self->pointer_state != VKUI_WIDGET_POINTER_UP;
	if(self->top_widget &&
	   (self->pointer_state == VKUI_WIDGET_POINTER_DOWN))
	{
		vkui_widget_click(self->top_widget,
		                  VKUI_WIDGET_POINTER_UP, x, y);
	}
	self->pointer_state = VKUI_WIDGET_POINTER_UP;

	return touch;
}

int vkui_screen_pointerMove(vkui_screen_t* self,
                            float x, float y, double t0)
{
	assert(self);

	if((self->top_widget == NULL) ||
	   (self->pointer_state == VKUI_WIDGET_POINTER_UP))
	{
		// ignore
		return 0;
	}

	float dx = x - self->pointer_x0;
	float dy = y - self->pointer_y0;
	if(self->pointer_state == VKUI_WIDGET_POINTER_DOWN)
	{
		// reject small motions
		float d = sqrtf(dx*dx + dy*dy);
		float s = 0.2f*vkui_screen_layoutText(self,
		                                      VKUI_TEXT_SIZE_MEDIUM);
		if(d < s)
		{
			// ignore
			return 1;
		}

		// initialize move state
		self->pointer_state = VKUI_WIDGET_POINTER_MOVE;
		self->pointer_x0    = x;
		self->pointer_y0    = y;
		self->pointer_t0    = t0;
		self->pointer_vx    = 0.0f;
		self->pointer_vy    = 0.0f;
		return 1;
	}

	// ignore events with less than 8ms time delta
	float dt = (float) (t0 - self->pointer_t0);
	if(dt >= 0.008f)
	{
		// update the move state
		self->pointer_x0 = x;
		self->pointer_y0 = y;
		self->pointer_t0 = t0;
		self->pointer_vx = dx/dt;
		self->pointer_vy = dy/dt;
		self->dirty      = 1;
	}

	return 1;
}

int vkui_screen_keyPress(vkui_screen_t* self,
                         int keycode, int meta)
{
	assert(self);

	if(self->focus_widget == NULL)
	{
		return 0;
	}

	keycode = vkui_shiftkeycode(keycode, meta);
	return vkui_widget_keyPress(self->focus_widget,
	                            keycode, meta);
}

void vkui_screen_draw(vkui_screen_t* self)
{
	assert(self);

	vkk_renderer_surfaceSize(self->renderer,
	                         &self->w, &self->h);

	vkui_widget_t* top = self->top_widget;
	if(top == NULL)
	{
		// ignore
		return;
	}

	vkui_widget_refresh(top);

	// dragging
	float w  = (float) self->w;
	float h  = (float) self->h;
	float dt = 1.0f/60.0f;
	if((self->pointer_vx != 0.0f) ||
	   (self->pointer_vy != 0.0f))
	{
		float x  = self->pointer_x0;
		float y  = self->pointer_y0;
		float vx = self->pointer_vx;
		float vy = self->pointer_vy;

		// clamp the speed to be proportional to the range
		float range  = 4.0f*sqrtf(w*w + h*h);
		float speed1 = sqrtf(vx*vx + vy*vy);
		float drag   = 1.0f*range*dt;
		if(speed1 > range)
		{
			vx *= range/speed1;
			vy *= range/speed1;
			speed1 = range;
		}

		// TODO - change drag to return status for
		// bump animation and to minimize dirty updates
		vkui_widget_drag(self->top_widget,
		                 x, y, vx*dt, vy*dt);

		// update the speed
		if((speed1 > drag) && (speed1 > 0.1f))
		{
			float speed2 = speed1 - drag;
			float coef   = speed2/speed1;
			self->pointer_vx *= coef;
			self->pointer_vy *= coef;
		}
		else
		{
			self->pointer_vx = 0.0f;
			self->pointer_vy = 0.0f;
		}

		self->dirty = 1;
	}

	if(self->dirty)
	{
		cc_rect1f_t clip =
		{
			.t = 0.0f,
			.l = 0.0f,
			.w = w,
			.h = h,
		};

		vkui_widget_layoutSize(top, &w, &h);
		vkui_widget_layoutXYClip(top, 0.0f, 0.0f, &clip, 1, 1);
		self->dirty = 0;
	}

	cc_mat4f_t mvp;
	cc_mat4f_ortho(&mvp, 1, 0.0f, w, h, 0.0f, 0.0f, 2.0f);
	vkk_renderer_updateBuffer(self->renderer, self->ub_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);

	vkui_widget_draw(self->top_widget);
	vkk_renderer_scissor(self->renderer,
	                     0, 0, w, h);
	vkui_screen_bind(self, VKUI_SCREEN_BIND_NONE);

	// play sound fx
	if(self->clicked)
	{
		vkui_screen_playClickFn playClick = self->playClick;
		(*playClick)(self->sound_fx);
		self->clicked = 0;
	}
}