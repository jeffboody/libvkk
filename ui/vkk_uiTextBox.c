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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiTextBox_print(vkk_uiTextBox_t* self,
                    const char* string)
{
	ASSERT(self);
	ASSERT(string);

	vkk_uiWidget_t* widget = (vkk_uiWidget_t*) self;

	vkk_uiTextLayout_t text_layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE
	};

	vkk_uiTextFn_t tfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiText_t* text;
	if((string[0] == '\0') &&
	   (self->text_style.size > VKK_UI_TEXT_SIZE_SMALL))
	{
		// use a smaller size/spacing between paragraphs
		vkk_uiTextStyle_t text_style;
		memcpy(&text_style, &self->text_style,
		       sizeof(vkk_uiTextStyle_t));
		text_style.size    = self->text_style.size - 1;
		text_style.spacing = VKK_UI_TEXT_SPACING_NONE;

		text = vkk_uiText_new(widget->screen, 0, &tfn,
		                      &text_layout, &text_style,
		                      &clear);
	}
	else
	{
		text = vkk_uiText_new(widget->screen, 0, &tfn,
		                      &text_layout, &self->text_style,
		                      &clear);
	}

	if(text == NULL)
	{
		return;
	}

	if(vkk_uiListBox_add(&self->base, (vkk_uiWidget_t*) text) == 0)
	{
		goto fail_add;
	}

	vkk_uiText_label(text, "%s", string);

	// success
	return;

	// failure
	fail_add:
		vkk_uiText_delete(&text);
}

#define VKK_UI_TOKEN_END   0
#define VKK_UI_TOKEN_TEXT  1
#define VKK_UI_TOKEN_BREAK 2

static int
getToken(const char* src, char* tok, int* _srci, int* _toki)
{
	ASSERT(src);
	ASSERT(tok);
	ASSERT(_srci);
	ASSERT(_toki);

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
		return (srci == 0) ? VKK_UI_TOKEN_BREAK : VKK_UI_TOKEN_END;
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
	return VKK_UI_TOKEN_TEXT;
}

static void
vkk_uiTextBox_reflow(vkk_uiWidget_t* widget, float w, float h)
{
	ASSERT(widget);

	vkk_uiTextBox_t*   self       = (vkk_uiTextBox_t*) widget;
	vkk_uiTextStyle_t* text_style = &self->text_style;

	// subtract the spacing which is added when
	// printing text lines
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkk_uiScreen_layoutBorder(widget->screen,
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
	vkk_uiFont_t* font;
	font = vkk_uiScreen_font(widget->screen,
	                         text_style->font_type);
	float size   = vkk_uiScreen_layoutText(widget->screen,
	                                       text_style->size);
	float height = (float) vkk_uiFont_height(font);

	// clear the text
	cc_listIter_t* iter;
	iter = vkk_uiListBox_head(&self->base);
	while(iter)
	{
		vkk_uiText_t* text;
		text = (vkk_uiText_t*)
		       vkk_uiListBox_remove(&self->base, &iter);
		vkk_uiText_delete(&text);
	}

	// initialize parser
	char tok[256];
	char dst[256];
	char tmp[256];
	int  srci = 0;
	int  toki = 0;
	int  dsti = 0;
	int  type = VKK_UI_TOKEN_END;

	// reflow the string(s)
	iter = cc_list_head(self->strings);
	while(iter)
	{
		const char* src = (const char*) cc_list_peekIter(iter);

		srci = 0;
		type = getToken(src, tok, &srci, &toki);
		while(type != VKK_UI_TOKEN_END)
		{
			if(type == VKK_UI_TOKEN_BREAK)
			{
				if(dsti > 0)
				{
					// print current line and line break
					vkk_uiTextBox_print(self, dst);
					vkk_uiTextBox_print(self, "");
				}
				else
				{
					// print line break
					vkk_uiTextBox_print(self, "");
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
				float width = (float) vkk_uiFont_measure(font, tmp);

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
					vkk_uiTextBox_print(self, dst);

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
		vkk_uiTextBox_print(self, dst);
	}
}

static int
vkk_uiTextBox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	// ignore
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiTextBox_t*
vkk_uiTextBox_new(vkk_uiScreen_t* screen, size_t wsize,
                  vkk_uiTextBoxFn_t* tbfn,
                  vkk_uiWidgetLayout_t* layout,
                  vkk_uiWidgetScroll_t* scroll,
                  vkk_uiTextStyle_t* text_style)
{
	ASSERT(screen);
	ASSERT(tbfn);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(text_style);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiTextBox_t);
	}

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiListBoxFn_t lbfn =
	{
		.priv       = tbfn->priv,
		.refresh_fn = vkk_uiTextBox_refresh,
	};

	vkk_uiTextBox_t* self;
	self = (vkk_uiTextBox_t*)
	vkk_uiListBox_new(screen, wsize, &lbfn, layout,
	                  scroll,
	                  VKK_UI_LISTBOX_ORIENTATION_VERTICAL,
	                  &clear);
	if(self == NULL)
	{
		return NULL;
	}

	// optionially override the reflow function
	vkk_uiWidgetReflow_fn reflow_fn = NULL;
	if(layout->wrapx != VKK_UI_WIDGET_WRAP_SHRINK)
	{
		reflow_fn = vkk_uiTextBox_reflow;
	}

	// override functions
	vkk_uiWidgetFn_t fn_new =
	{
		.click_fn  = tbfn->click_fn,
		.reflow_fn = reflow_fn,
	};
	vkk_uiWidgetFn_t fn_old =
	{
		.priv = NULL
	};
	vkk_uiWidget_t* widget = (vkk_uiWidget_t*) self;
	vkk_uiWidget_override(widget, &fn_new, &fn_old);

	self->strings = cc_list_new();
	if(self->strings == NULL)
	{
		goto fail_strings;
	}

	self->dirty  = 1;
	self->last_w = 0.0f;
	self->last_h = 0.0f;

	memcpy(&self->text_style, text_style,
	       sizeof(vkk_uiTextStyle_t));

	// success
	return self;

	// failure
	fail_strings:
		vkk_uiListBox_delete((vkk_uiListBox_t**) &self);
	return NULL;
}

vkk_uiTextBox_t*
vkk_uiTextBox_newPageButton(vkk_uiScreen_t* screen,
                            vkk_uiTextBoxFn_t* tbfn)
{
	ASSERT(screen);
	ASSERT(tbfn);

	vkk_uiWidgetLayout_t layout =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageLink(screen, &style.color);

	return vkk_uiTextBox_new(screen, 0, tbfn, &layout,
	                         &scroll, &style);
}

vkk_uiTextBox_t*
vkk_uiTextBox_newPageParagraph(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiWidgetLayout_t layout =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiTextBoxFn_t tbfn =
	{
		.priv = NULL
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageItem(screen, &style.color);

	return vkk_uiTextBox_new(screen, 0, &tbfn, &layout,
	                         &scroll, &style);
}

void vkk_uiTextBox_delete(vkk_uiTextBox_t** _self)
{
	ASSERT(_self);

	vkk_uiTextBox_t* self = *_self;
	if(self)
	{
		vkk_uiTextBox_clear(self);
		cc_list_delete(&self->strings);
		vkk_uiListBox_delete((vkk_uiListBox_t**) _self);
		*_self = NULL;
	}
}

void vkk_uiTextBox_clear(vkk_uiTextBox_t* self)
{
	ASSERT(self);

	// clear the text
	cc_listIter_t* iter;
	iter = vkk_uiListBox_head(&self->base);
	while(iter)
	{
		vkk_uiText_t* text;
		text = (vkk_uiText_t*)
		       vkk_uiListBox_remove(&self->base, &iter);
		vkk_uiText_delete(&text);
	}

	iter = cc_list_head(self->strings);
	while(iter)
	{
		void* string;
		string = (void*) cc_list_remove(self->strings, &iter);
		FREE(string);
	}

	vkk_uiWidget_t* widget = (vkk_uiWidget_t*) self;
	vkk_uiScreen_layoutDirty(widget->screen);
}

void vkk_uiTextBox_printf(vkk_uiTextBox_t* self,
                          const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

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

	vkk_uiTextBox_print(self, string);

	// success
	return;

	// failure
	fail_append:
		FREE(string);
}
