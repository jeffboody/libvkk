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
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libpak/pak_file.h"
#include "../../texgz/texgz_tex.h"
#include "../../libxmlstream/xml_istream.h"
#include "vkui_font.h"
#include "vkui_screen.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkui_font_parseStart(void* priv, int line,
                     const char* name, const char** atts)
{
	assert(priv);
	assert(name);
	assert(atts);

	vkui_font_t* self = (vkui_font_t*) priv;

	if(strcmp(name, "font") == 0)
	{
		int idx = 0;
		while(atts[idx] && atts[idx + 1])
		{
			const char* key = atts[idx];
			const char* val = atts[idx + 1];
			if(strcmp(key, "size") == 0)
			{
				self->size = (int) strtol(val, NULL, 0);
			}
			else if(strcmp(key, "h") == 0)
			{
				self->h = (int) strtol(val, NULL, 0);
			}
			idx += 2;
		}
	}
	else if(strcmp(name, "coords") == 0)
	{
		int c   = 0;
		int x   = 0;
		int y   = 0;
		int w   = 0;
		int idx = 0;
		while(atts[idx] && atts[idx + 1])
		{
			const char* key = atts[idx];
			const char* val = atts[idx + 1];
			if(strcmp(key, "c") == 0)
			{
				c = (int) strtol(val, NULL, 0);
			}
			else if(strcmp(key, "x") == 0)
			{
				x = (int) strtol(val, NULL, 0);
			}
			else if(strcmp(key, "y") == 0)
			{
				y = (int) strtol(val, NULL, 0);
			}
			else if(strcmp(key, "w") == 0)
			{
				w = (int) strtol(val, NULL, 0);
			}
			idx += 2;
		}

		// check for a ascii/cursor character
		if((c >= 31) && (c <= 126))
		{
			self->coords[c].x = x;
			self->coords[c].y = y;
			self->coords[c].w = w;
		}
	}

	return 1;
}

static int
vkui_font_parseEnd(void* priv, int line,
                   const char* name, const char* content)
{
	// content may be NULL
	assert(priv);
	assert(name);

	// ignore
	return 1;
}

static int
vkui_font_parseXml(vkui_font_t* self, const char* resource,
                   const char* key)
{
	assert(self);
	assert(resource);
	assert(key);

	pak_file_t* pak = pak_file_open(resource, PAK_FLAG_READ);
	if(pak == NULL)
	{
		return 0;
	}

	int size = pak_file_seek(pak, key);
	if(size == 0)
	{
		goto fail_seek;
	}

	if(xml_istream_parseFile((void*) self,
	                         vkui_font_parseStart,
	                         vkui_font_parseEnd,
	                         pak->f, size) == 0)
	{
		goto fail_parse;
	}

	pak_file_close(&pak);

	// success
	return 1;

	// failure
	fail_parse:
	fail_seek:
		pak_file_close(&pak);
	return 0;
}

static int
vkui_font_loadXml(vkui_font_t* self,
                  const char* resource,
                  const char* xmlname)
{
	assert(self);
	assert(resource);
	assert(xmlname);

	if(vkui_font_parseXml(self, resource, xmlname) == 0)
	{
		return 0;
	}

	uint32_t W;
	uint32_t H;
	vkk_image_size(self->image, &W, &H);

	// validate xml
	// check height
	if(self->h > H)
	{
		LOGE("invalid h=%i, H=%i", self->h, H);
		return 0;
	}

	// check font size
	if(self->size > self->h)
	{
		LOGE("invalid size=%i, h=%i", self->size, self->h);
		return 0;
	}

	// check ascii/cursor character(s)
	int c;
	for(c = 31; c <= 126; ++c)
	{
		int w = self->coords[c].w;
		int h = self->h;
		int t = self->coords[c].y;
		int l = self->coords[c].x;
		int b = self->coords[c].y + h - 1;
		int r = self->coords[c].x + w - 1;
		if((t <  0) || (t >= H) ||
		   (l <  0) || (l >= W) ||
		   (b <  0) || (b >= H) ||
		   (r <  0) || (r >= W) ||
		   (w <= 0) || (w > W)  ||
		   (h <= 0) || (h > H))
		{
			LOGE("invalid c=0x%X=%c, t=%i, l=%i, b=%i, r=%i, w=%i, h=%i, W=%i, H=%i",
			     c, (char) c, t, l, b, r, w, h, W, H);
			return 0;
		}
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_font_t*
vkui_font_new(vkui_screen_t* screen, const char* resource,
              const char* texname, const char* xmlname)
{
	assert(screen);
	assert(resource);
	assert(texname);
	assert(xmlname);

	vkui_font_t* self;
	self = (vkui_font_t*) MALLOC(sizeof(vkui_font_t));
	if(self == NULL)
	{
		LOGE("MALLOC failed");
		return NULL;
	}

	self->image = vkui_screen_spriteImage(screen, texname,
	                                      &self->tex);
	if(self->image == NULL)
	{
		goto fail_image;
	}

	if(vkui_font_loadXml(self, resource, xmlname) == 0)
	{
		goto fail_coords;
	}

	// measure printable ascii characters
	int c;
	int w = 0;
	int h = 0;
	for(c = 32; c <= 126; ++c)
	{
		w += vkui_font_width(self, c);
		h += vkui_font_height(self);
	}
	self->aspect_ratio_avg = (float) w/(float) h;

	// success
	return self;

	// failure
	fail_coords:
	fail_image:
		FREE(self);
	return NULL;
}

void vkui_font_delete(vkui_font_t** _self)
{
	assert(_self);

	vkui_font_t* self = *_self;
	if(self)
	{
		// image is a reference
		texgz_tex_delete(&self->tex);
		FREE(self);
		*_self = NULL;
	}
}

void vkui_font_request(vkui_font_t* self,
                       char c,
                       cc_rect2f_t* pc,
                       cc_rect2f_t* tc,
                       cc_rect2f_t* vc)
{
	assert(self);
	assert(pc);
	assert(tc);
	assert(vc);

	// check for a ascii/cursor character
	if((c < 31) || (c > 126))
	{
		c = VKUI_FONT_CURSOR;
	}

	float w = (float) self->coords[(int) c].w;
	float h = (float) self->h;

	uint32_t W;
	uint32_t H;
	vkk_image_size(self->image, &W, &H);

	// fill in the pixel coords
	pc->t = (float) self->coords[(int) c].y;
	pc->l = (float) self->coords[(int) c].x;
	pc->b = pc->t + h - 1.0f;
	pc->r = pc->l + w - 1.0f;

	// fill in tex coords
	float hf = (float) (H - 1);
	float wf = (float) (W - 1);
	tc->t = pc->t/hf;
	tc->l = pc->l/wf;
	tc->b = pc->b/hf;
	tc->r = pc->r/wf;

	// fill in vertex coords
	vc->t = 0.0f;
	vc->l = 0.0f;
	vc->b = 1.0f;
	vc->r = w/h;
}

float vkui_font_aspectRatioAvg(vkui_font_t* self)
{
	assert(self);

	return self->aspect_ratio_avg;
}

int vkui_font_width(vkui_font_t* self, char c)
{
	assert(self);

	// check for a ascii/cursor character
	if((c >= 31) && (c <= 126))
	{
		return self->coords[(int) c].w;
	}
	return 0;
}

int vkui_font_height(vkui_font_t* self)
{
	assert(self);

	return self->h;
}

int vkui_font_measure(vkui_font_t* self, const char* s)
{
	assert(self);
	assert(s);

	int width = 0;
	while(s[0] != '\0')
	{
		width += vkui_font_width(self, s[0]);
		++s;
	}
	return width;
}

texgz_tex_t*
vkui_font_render(vkui_font_t* self, const char* s)
{
	assert(self);
	assert(s);

	int width  = vkui_font_measure(self, s);
	int height = vkui_font_height(self);
	texgz_tex_t* tex = texgz_tex_new(width, height,
	                                 width, height,
	                                 TEXGZ_UNSIGNED_BYTE,
	                                 TEXGZ_ALPHA,
	                                 NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	// blit characters
	int xs = 0;
	int ys = 0;
	int xd = 0;
	int yd = 0;
	cc_rect2f_t pc;
	cc_rect2f_t tc;
	cc_rect2f_t vc;
	while(s[0] != '\0')
	{
		vkui_font_request(self, s[0],
		                  &pc, &tc, &vc);

		xs    = pc.l;
		ys    = pc.t;
		width = vkui_font_width(self, s[0]);

		if(texgz_tex_blit(self->tex, tex,
                          width, height,
                          xs, ys, xd, yd) == 0)
		{
			goto fail_blit;
		}

		xd += width;
		++s;
	}

	// success
	return tex;

	// failure
	fail_blit:
		texgz_tex_delete(&tex);
	return NULL;
}
