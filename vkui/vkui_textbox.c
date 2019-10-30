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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "vkui_screen.h"
#include "vkui_text.h"
#include "vkui_textbox.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_textbox_print(vkui_textbox_t* self, const char* string)
{
	assert(self);
	assert(string);

	vkui_widget_t*  widget  = (vkui_widget_t*) self;
	vkui_listbox_t* listbox = (vkui_listbox_t*) self;

	vkui_textLayout_t text_layout =
	{
		.border = VKUI_WIDGET_BORDER_NONE
	};

	vkui_textFn_t text_fn;
	memset(&text_fn, 0, sizeof(vkui_textFn_t));

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_text_t* text;
	text = vkui_text_new(widget->screen, 0, &text_layout,
	                     &self->text_style, &text_fn, &clear);
	if(text == NULL)
	{
		return;
	}

	if(vkui_listbox_add(listbox, (vkui_widget_t*) text) == 0)
	{
		goto fail_add;
	}

	vkui_text_label(text, "%s", string);

	// success
	return;

	// failure
	fail_add:
		vkui_text_delete(&text);
}

#define VKUI_TOKEN_END   0
#define VKUI_TOKEN_TEXT  1
#define VKUI_TOKEN_BREAK 2

static int
getToken(const char* src, char* tok, int* _srci, int* _toki)
{
	assert(src);
	assert(tok);
	assert(_srci);
	assert(_toki);

	int srci = *_srci;
	int toki = 0;

	// skip whitespace
	while((src[srci] == '\n') ||
	      (src[srci] == '\r') ||
	      (src[srci] == '\t') ||
	      (src[srci] == ' '))
	{
		++srci;
	}

	// check for the end
	if(src[srci] == '\0')
	{
		*_srci = srci;
		*_toki = toki;
		return (srci == 0) ? VKUI_TOKEN_BREAK : VKUI_TOKEN_END;
	}

	// read text
	while((src[srci] != '\0') &&
	      (src[srci] != '\n') &&
	      (src[srci] != '\r') &&
	      (src[srci] != '\t') &&
	      (src[srci] != ' '))
	{
		tok[toki++] = src[srci++];
		tok[toki]   = '\0';
	}

	*_srci = srci;
	*_toki = toki;
	return VKUI_TOKEN_TEXT;
}

static void
vkui_textbox_reflow(vkui_widget_t* widget, float w, float h)
{
	assert(widget);

	vkui_textbox_t*   self       = (vkui_textbox_t*) widget;
	vkui_listbox_t*   listbox    = (vkui_listbox_t*) self;
	vkui_textStyle_t* text_style = &self->text_style;

	// subtract the spacing which is added when
	// printing text lines
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen,
	                        text_style->spacing,
	                        &h_bo, &v_bo);
	w = w - 2.0f*h_bo;

	// reflow text when changes occur
	if((self->dirty  == 0) &&
	   (self->last_w == w) && (self->last_h == h))
	{
		return;
	}
	self->dirty  = 0;
	self->last_w = w;
	self->last_h = h;

	// reflow text for variable width font
	vkui_font_t* font;
	font = vkui_screen_font(widget->screen,
	                        text_style->font_type);
	float size   = vkui_screen_layoutText(widget->screen,
	                                      text_style->size);
	float height = (float) vkui_font_height(font);

	// clear the text
	vkui_text_t* text;
	text = (vkui_text_t*) vkui_listbox_remove(listbox);
	while(text)
	{
		vkui_text_delete(&text);
		text = (vkui_text_t*) vkui_listbox_remove(listbox);
	}

	// initialize parser
	char tok[256];
	char dst[256];
	char tmp[256];
	int  srci = 0;
	int  toki = 0;
	int  dsti = 0;
	int  type = VKUI_TOKEN_END;

	// reflow the string(s)
	cc_listIter_t* iter = cc_list_head(self->strings);
	while(iter)
	{
		const char* src = (const char*) cc_list_peekIter(iter);

		srci = 0;
		type = getToken(src, tok, &srci, &toki);
		while(type != VKUI_TOKEN_END)
		{
			if(type == VKUI_TOKEN_BREAK)
			{
				if(dsti > 0)
				{
					// print current line and line break
					vkui_textbox_print(self, dst);
					vkui_textbox_print(self, "");
				}
				else
				{
					// print line break
					vkui_textbox_print(self, "");
				}
				dsti = 0;
				break;
			}

			if(dsti == 0)
			{
				// start a new line
				strncpy(dst, tok, 256);
				dst[255] = '\0';
				dsti = toki;
			}
			else
			{
				// measure width of "dst tok"
				int len = strlen(dst) + strlen(tok) + 1;
				snprintf(tmp, 256, "%s %s", dst, tok);
				tmp[255] = '\0';
				float width = (float) vkui_font_measure(font, tmp);

				if((size*(width/height) <= w) &&
				   (len <= 255))
				{
					// append to current line
					snprintf(dst, 256, "%s", tmp);
					dst[255] = '\0';
					dsti += toki + 1;
				}
				else
				{
					// print the current line
					vkui_textbox_print(self, dst);

					// start a new line
					strncpy(dst, tok, 256);
					dst[255] = '\0';
					dsti = toki;
				}
			}

			type = getToken(src, tok, &srci, &toki);
		}

		iter = cc_list_next(iter);
	}

	if(dsti > 0)
	{
		// print the last line
		vkui_textbox_print(self, dst);
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_textbox_t*
vkui_textbox_new(vkui_screen_t* screen, size_t wsize,
                 vkui_widgetLayout_t* layout,
                 vkui_widgetScroll_t* scroll,
                 vkui_widgetFn_t* fn,
                 vkui_textStyle_t* text_style)
{
	assert(screen);
	assert(layout);
	assert(scroll);
	assert(fn);
	assert(text_style);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_textbox_t);
	}

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_textbox_t* self;
	self = (vkui_textbox_t*)
	vkui_listbox_new(screen, wsize, layout, scroll, fn,
	                 VKUI_LISTBOX_ORIENTATION_VERTICAL,
	                 &clear);
	if(self == NULL)
	{
		return NULL;
	}

	vkui_widget_t* widget = (vkui_widget_t*) self;

	// optionially set the reflow function
	if(layout->wrapx != VKUI_WIDGET_WRAP_SHRINK)
	{
		vkui_widget_privReflowFn(widget, vkui_textbox_reflow);
	}

	// enable sound effects since textbox derives from listbox
	vkui_widget_soundFx((vkui_widget_t*) self,
	                    fn->click_fn ? 1 : 0);

	self->strings = cc_list_new();
	if(self->strings == NULL)
	{
		goto fail_strings;
	}

	self->dirty  = 1;
	self->last_w = 0.0f;
	self->last_h = 0.0f;

	memcpy(&self->text_style, text_style,
	       sizeof(vkui_textStyle_t));

	// success
	return self;

	// failure
	fail_strings:
		vkui_listbox_delete((vkui_listbox_t**) &self);
	return NULL;
}

void vkui_textbox_delete(vkui_textbox_t** _self)
{
	assert(_self);

	vkui_textbox_t* self = *_self;
	if(self)
	{
		vkui_textbox_clear(self);
		vkui_listbox_delete((vkui_listbox_t**) _self);
		*_self = NULL;
	}
}

void vkui_textbox_clear(vkui_textbox_t* self)
{
	assert(self);

	vkui_listbox_t* listbox = (vkui_listbox_t*) self;

	// clear the text
	vkui_text_t* text;
	text = (vkui_text_t*) vkui_listbox_remove(listbox);
	while(text)
	{
		vkui_text_delete(&text);
		text = (vkui_text_t*) vkui_listbox_remove(listbox);
	}

	cc_listIter_t* iter = cc_list_head(self->strings);
	while(iter)
	{
		void* string;
		string = (void*) cc_list_remove(self->strings, &iter);
		FREE(string);
	}

	vkui_widget_t* widget = (vkui_widget_t*) self;
	vkui_screen_dirty(widget->screen);
}

void vkui_textbox_printf(vkui_textbox_t* self,
                         const char* fmt, ...)
{
	assert(self);
	assert(fmt);

	// decode string
	char s[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(s, 256, fmt, argptr);
	va_end(argptr);

	// copy string
	size_t len    = strlen(s) + 1;
	char*  string = (char*)
	                CALLOC(len, sizeof(char));
	if(string == NULL)
	{
		LOGE("CALLOC failed");
		return;
	}
	snprintf(string, len, "%s", s);

	if(cc_list_append(self->strings, NULL,
	                  (const void*) string) == NULL)
	{
		goto fail_append;
	}

	self->dirty = 1;

	vkui_textbox_print(self, string);

	// success
	return;

	// failure
	fail_append:
		FREE(string);
}
