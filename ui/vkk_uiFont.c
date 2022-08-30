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

#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libbfs/bfs_file.h"
#include "../../texgz/texgz_tex.h"
#include "../../libxmlstream/xml_istream.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_uiFont_parseStart(void* priv, int line, float progress,
                      const char* name, const char** atts)
{
	ASSERT(priv);
	ASSERT(name);
	ASSERT(atts);

	vkk_uiFont_t* self = (vkk_uiFont_t*) priv;

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
vkk_uiFont_parseEnd(void* priv, int line, float progress,
                    const char* name, const char* content)
{
	// content may be NULL
	ASSERT(priv);
	ASSERT(name);

	// ignore
	return 1;
}

static int
vkk_uiFont_parseXml(vkk_uiFont_t* self, const char* resource,
                    const char* key)
{
	ASSERT(self);
	ASSERT(resource);
	ASSERT(key);

	bfs_file_t* bfs;
	bfs = bfs_file_open(resource, 1, BFS_MODE_RDONLY);
	if(bfs == NULL)
	{
		return 0;
	}

	size_t size = 0;
	void*  data = NULL;
	if(bfs_file_blobGet(bfs, 0, key,
	                    &size, &data) == 0)
	{
		goto fail_get;
	}

	// check for empty data
	if(size == 0)
	{
		goto fail_empty;
	}

	if(xml_istream_parseBuffer((void*) self,
	                           vkk_uiFont_parseStart,
	                           vkk_uiFont_parseEnd,
	                           (const char*) data,
	                           size) == 0)
	{
		goto fail_parse;
	}

	FREE(data);
	bfs_file_close(&bfs);

	// success
	return 1;

	// failure
	fail_parse:
	fail_empty:
		FREE(data);
	fail_get:
		bfs_file_close(&bfs);
	return 0;
}

static int
vkk_uiFont_loadXml(vkk_uiFont_t* self,
                   const char* resource,
                   const char* xmlname)
{
	ASSERT(self);
	ASSERT(resource);
	ASSERT(xmlname);

	if(vkk_uiFont_parseXml(self, resource, xmlname) == 0)
	{
		return 0;
	}

	uint32_t W;
	uint32_t H;
	uint32_t D;
	vkk_image_size(self->img21, &W, &H, &D);

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

vkk_uiFont_t*
vkk_uiFont_new(vkk_uiScreen_t* screen, const char* resource,
               const char* texname, const char* xmlname)
{
	ASSERT(screen);
	ASSERT(resource);
	ASSERT(texname);
	ASSERT(xmlname);

	vkk_uiFont_t* self;
	self = (vkk_uiFont_t*)
	       CALLOC(1, sizeof(vkk_uiFont_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->img21 = vkk_uiScreen_spriteImage(screen, texname,
	                                       &self->tex);
	if(self->img21 == NULL)
	{
		goto fail_img21;
	}

	if(vkk_uiFont_loadXml(self, resource, xmlname) == 0)
	{
		goto fail_coords;
	}

	// measure printable ascii characters
	int c;
	int w = 0;
	int h = 0;
	for(c = 32; c <= 126; ++c)
	{
		w += vkk_uiFont_width(self, c);
		h += vkk_uiFont_height(self);
	}
	self->aspect_ratio_avg = (float) w/(float) h;

	// success
	return self;

	// failure
	fail_coords:
	fail_img21:
		FREE(self);
	return NULL;
}

void vkk_uiFont_delete(vkk_uiFont_t** _self)
{
	ASSERT(_self);

	vkk_uiFont_t* self = *_self;
	if(self)
	{
		// image is a reference
		texgz_tex_delete(&self->tex);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_uiFont_request(vkk_uiFont_t* self,
                        char c,
                        cc_rect2f_t* pc,
                        cc_rect2f_t* tc,
                        cc_rect2f_t* vc)
{
	ASSERT(self);
	ASSERT(pc);
	ASSERT(tc);
	ASSERT(vc);

	// check for a ascii/cursor character
	if((c < 31) || (c > 126))
	{
		c = VKK_UI_FONT_CURSOR;
	}

	float w = (float) self->coords[(int) c].w;
	float h = (float) self->h;

	uint32_t W;
	uint32_t H;
	uint32_t D;
	vkk_image_size(self->img21, &W, &H, &D);

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

float vkk_uiFont_aspectRatioAvg(vkk_uiFont_t* self)
{
	ASSERT(self);

	return self->aspect_ratio_avg;
}

int vkk_uiFont_width(vkk_uiFont_t* self, char c)
{
	ASSERT(self);

	// check for a ascii/cursor character
	if((c >= 31) && (c <= 126))
	{
		return self->coords[(int) c].w;
	}
	return 0;
}

int vkk_uiFont_height(vkk_uiFont_t* self)
{
	ASSERT(self);

	return self->h;
}

int vkk_uiFont_measure(vkk_uiFont_t* self, const char* s)
{
	ASSERT(self);
	ASSERT(s);

	int width = 0;
	while(s[0] != '\0')
	{
		width += vkk_uiFont_width(self, s[0]);
		++s;
	}
	return width;
}

texgz_tex_t*
vkk_uiFont_render(vkk_uiFont_t* self, const char* s)
{
	ASSERT(self);
	ASSERT(s);

	int width  = vkk_uiFont_measure(self, s);
	int height = vkk_uiFont_height(self);
	texgz_tex_t* tex = texgz_tex_new(width, height,
	                                 width, height,
	                                 self->tex->type,
	                                 self->tex->format,
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
		vkk_uiFont_request(self, s[0],
		                  &pc, &tc, &vc);

		xs    = pc.l;
		ys    = pc.t;
		width = vkk_uiFont_width(self, s[0]);

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
