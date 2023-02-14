/*
 * Copyright (c) 2020 Jeff Boody
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

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static const char*
vkk_uiFileList_getPath(vkk_uiFileList_t* self)
{
	ASSERT(self);

	return self->bulletbox_path->text->string;
}

static void
vkk_uiFileList_setPath(vkk_uiFileList_t* self,
                       const char* path)
{
	ASSERT(self);
	ASSERT(path);

	// check if the path ends in slash
	int has_slash = 0;
	int len       = strlen(path);
	if((len > 0) && (path[len - 1] == '/'))
	{
		has_slash = 1;
	}

	if(has_slash)
	{
		vkk_uiBulletBox_label(self->bulletbox_path, "%s", path);
	}
	else
	{
		vkk_uiBulletBox_label(self->bulletbox_path, "%s/", path);
	}
}

static const char*
vkk_uiFileList_getName(vkk_uiFileList_t* self)
{
	ASSERT(self);

	return self->text_name->string;
}

static void
vkk_uiFileList_setName(vkk_uiFileList_t* self,
                       const char* name)
{
	ASSERT(self);
	ASSERT(name);

	const char* start = name;

	// remove directory prefix
	int idx = 0;
	while(name[idx] != '\0')
	{
		if(name[idx] == '/')
		{
			start = &name[idx];
		}
		++idx;
	}

	char copy[256];
	snprintf(copy, 256, "%s", start);

	// remove extension suffix
	char* ext = strstr(copy, ".");
	if(ext)
	{
		ext[0] = '\0';
	}

	vkk_uiText_label(self->text_name, "%s", copy);
}

static const char*
vkk_uiFileList_getExt(vkk_uiFileList_t* self)
{
	ASSERT(self);

	return self->text_ext->string;
}

static void
vkk_uiFileList_setExt(vkk_uiFileList_t* self,
                      const char* ext)
{
	ASSERT(self);
	ASSERT(ext);

	if(ext[0] == '.')
	{
		vkk_uiText_label(self->text_ext, "%s", ext);
	}
	else
	{
		vkk_uiText_label(self->text_ext, ".%s", ext);
	}
}

static void
vkk_uiFileList_clickItem(vkk_uiWidget_t* widget,
                         float x, float y)
{
	ASSERT(widget);

	vkk_uiFileList_t* self;
	self = (vkk_uiFileList_t*) vkk_uiWidget_priv(widget);

	vkk_uiBulletBox_t* item = (vkk_uiBulletBox_t*) widget;

	const char* path = vkk_uiFileList_getPath(self);
	const char* name = item->text->string;

	// update path
	if((name[0] == '.') &&
	   (name[1] == '.') &&
	   (name[2] == '\0'))
	{
		char label[256];
		snprintf(label, 256, "%s", path);

		// find the final directory
		int i    = 0;
		int last = 0;
		while(label[i] != '\0')
		{
			if((label[i]     == '/') &&
			   (label[i + 1] != '\0'))
			{
				last = i + 1;
			}

			++i;
		}

		// dir can't be ".." for "/"
		ASSERT(last != 0);

		// remove the last directory
		label[last] = '\0';
		vkk_uiFileList_setPath(self, label);
		self->dirty = 1;
	}
	else if(strstr(name, vkk_uiFileList_getExt(self)))
	{
		vkk_uiFileList_setName(self, name);
	}
	else
	{
		char label[256];
		snprintf(label, 256, "%s%s/", path, name);
		vkk_uiFileList_setPath(self, label);
		self->dirty = 1;
	}
}

static int validateName(const char* string, char* name)
{
	ASSERT(string);
	ASSERT(name);

	snprintf(name, 256, "%s", string);
	if(strlen(name) == 0)
	{
		LOGW("invalid string=%s", string);
		return 0;
	}

	int  idx = 0;
	char c   = name[idx];
	while(c != '\0')
	{
		if(((c >= 'a') && (c <= 'z')) ||
		   ((c >= 'A') && (c <= 'Z')) ||
		   ((c >= '0') && (c <= '9')) ||
		   ((c == '_') || (c == '-')))
		{
			// OK
		}
		else
		{
			// replace with '_'
			name[idx] = '_';
		}

		++idx;
		c = name[idx];
	}

	return 1;
}

static void
vkk_uiFileList_size(vkk_uiWidget_t* widget,
                    float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiFileList_t* self;
	vkk_uiWidget_t*   heading_name;
	vkk_uiWidget_t*   text_name;
	vkk_uiWidget_t*   text_ext;
	vkk_uiWidget_t*   heading_path;
	vkk_uiWidget_t*   bulletbox_path;
	vkk_uiWidget_t*   listbox_files;
	self           = (vkk_uiFileList_t*) widget;
	heading_name   = (vkk_uiWidget_t*) self->heading_name;
	text_name      = (vkk_uiWidget_t*) self->text_name;
	text_ext       = (vkk_uiWidget_t*) self->text_ext;
	heading_path   = (vkk_uiWidget_t*) self->heading_path;
	bulletbox_path = (vkk_uiWidget_t*) self->bulletbox_path;
	listbox_files  = (vkk_uiWidget_t*) self->listbox_files;

	// get separator size
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkk_uiScreen_layoutBorder(widget->screen,
	                          VKK_UI_WIDGET_BORDER_LARGE,
	                          &h_bo, &v_bo);

	float hnw = *w;
	float hnh = *h;
	vkk_uiWidget_layoutSize(heading_name, &hnw, &hnh);

	float ew = *w;
	float eh = *h;
	vkk_uiWidget_layoutSize(text_ext, &ew, &eh);

	float left = *w - ew;
	float nw   = (left > 0.0f) ? left : 0.0f;
	float nh   = *h;
	vkk_uiWidget_layoutSize(text_name, &nw, &nh);

	float hpw = *w;
	float hph = *h;
	vkk_uiWidget_layoutSize(heading_path, &hpw, &hph);

	float pw = *w;
	float ph = *h;
	vkk_uiWidget_layoutSize(bulletbox_path, &pw, &ph);

	float fw = *w;
	float fh = *h - (v_bo + hnh + hph + ph);
	vkk_uiWidget_layoutSize(listbox_files, &fw, &fh);

	*h = hnh + nh + hph + ph + fh;
}

static void
vkk_uiFileList_layout(vkk_uiWidget_t* widget,
                      int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiFileList_t* self;
	vkk_uiWidget_t*   heading_name;
	vkk_uiWidget_t*   text_name;
	vkk_uiWidget_t*   text_ext;
	vkk_uiWidget_t*   heading_path;
	vkk_uiWidget_t*   bulletbox_path;
	vkk_uiWidget_t*   listbox_files;
	self           = (vkk_uiFileList_t*) widget;
	heading_name   = (vkk_uiWidget_t*) self->heading_name;
	text_name      = (vkk_uiWidget_t*) self->text_name;
	text_ext       = (vkk_uiWidget_t*) self->text_ext;
	heading_path   = (vkk_uiWidget_t*) self->heading_path;
	bulletbox_path = (vkk_uiWidget_t*) self->bulletbox_path;
	listbox_files  = (vkk_uiWidget_t*) self->listbox_files;

	// get separator size
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkk_uiScreen_layoutBorder(widget->screen,
	                          VKK_UI_WIDGET_BORDER_LARGE,
	                          &h_bo, &v_bo);

	// initialize the layout
	float x   = 0.0f;
	float y   = 0.0f;
	float t   = widget->rect_draw.t;
	float l   = widget->rect_draw.l;
	float hnw = heading_name->rect_border.w;
	float hnh = heading_name->rect_border.h;
	float nw  = text_name->rect_border.w;
	float nh  = text_name->rect_border.h;
	float ew  = text_ext->rect_border.w;
	float eh  = text_ext->rect_border.h;
	float hpw = heading_path->rect_border.w;
	float hph = heading_path->rect_border.h;
	float pw  = bulletbox_path->rect_border.w;
	float ph  = bulletbox_path->rect_border.h;
	float fw  = listbox_files->rect_border.w;
	float fh  = listbox_files->rect_border.h;

	// layout heading_name
	cc_rect1f_t rect_draw =
	{
		.t = t + v_bo,
		.l = l + h_bo,
		.w = hnw,
		.h = hnh
	};
	cc_rect1f_t rect_clip;
	vkk_uiWidget_layoutAnchor(heading_name, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(heading_name, x, y, &rect_clip,
	                          dragx, dragy);

	// layout text_name
	rect_draw.t += rect_draw.h;
	rect_draw.w = nw;
	rect_draw.h = nh;
	vkk_uiWidget_layoutAnchor(text_name, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(text_name, x, y, &rect_clip,
	                          dragx, dragy);

	// layout text_ext
	rect_draw.l = l + nw;
	rect_draw.w = ew;
	rect_draw.h = eh;
	vkk_uiWidget_layoutAnchor(text_ext, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(text_ext, x, y, &rect_clip,
	                          dragx, dragy);
	rect_draw.l = l + h_bo;

	// layout heading_path
	rect_draw.t += rect_draw.h;
	rect_draw.w  = hpw;
	rect_draw.h  = hph;
	vkk_uiWidget_layoutAnchor(heading_path, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(heading_path, x, y, &rect_clip,
	                          dragx, dragy);

	// layout bulletbox_path
	rect_draw.t += rect_draw.h;
	rect_draw.w  = pw;
	rect_draw.h  = ph;
	vkk_uiWidget_layoutAnchor(bulletbox_path, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(bulletbox_path, x, y, &rect_clip,
	                          dragx, dragy);

	// layout listbox_files
	rect_draw.t += rect_draw.h;
	rect_draw.l  = l;
	rect_draw.w  = fw;
	rect_draw.h  = fh;
	vkk_uiWidget_layoutAnchor(listbox_files, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(listbox_files, x, y, &rect_clip,
	                          dragx, dragy);
}

static void
vkk_uiFileList_drag(vkk_uiWidget_t* widget,
                    float x, float y,
                    float dx, float dy)
{
	ASSERT(widget);

	vkk_uiFileList_t* self;
	vkk_uiWidget_t*   heading_name;
	vkk_uiWidget_t*   text_name;
	vkk_uiWidget_t*   text_ext;
	vkk_uiWidget_t*   heading_path;
	vkk_uiWidget_t*   bulletbox_path;
	vkk_uiWidget_t*   listbox_files;
	self           = (vkk_uiFileList_t*) widget;
	heading_name   = (vkk_uiWidget_t*) self->heading_name;
	text_name      = (vkk_uiWidget_t*) self->text_name;
	text_ext       = (vkk_uiWidget_t*) self->text_ext;
	heading_path   = (vkk_uiWidget_t*) self->heading_path;
	bulletbox_path = (vkk_uiWidget_t*) self->bulletbox_path;
	listbox_files  = (vkk_uiWidget_t*) self->listbox_files;

	vkk_uiWidget_drag(heading_name, x, y, dx, dy);
	vkk_uiWidget_drag(text_name, x, y, dx, dy);
	vkk_uiWidget_drag(text_ext, x, y, dx, dy);
	vkk_uiWidget_drag(heading_path, x, y, dx, dy);
	vkk_uiWidget_drag(bulletbox_path, x, y, dx, dy);
	vkk_uiWidget_drag(listbox_files, x, y, dx, dy);
}

static void
vkk_uiFileList_scrollTop(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiFileList_t* self = (vkk_uiFileList_t*) widget;

	vkk_uiWidget_scrollTop((vkk_uiWidget_t*)
	                       self->listbox_files);
}

static void vkk_uiFileList_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiFileList_t* self;
	vkk_uiWidget_t*   heading_name;
	vkk_uiWidget_t*   text_name;
	vkk_uiWidget_t*   text_ext;
	vkk_uiWidget_t*   heading_path;
	vkk_uiWidget_t*   bulletbox_path;
	vkk_uiWidget_t*   listbox_files;
	self           = (vkk_uiFileList_t*) widget;
	heading_name   = (vkk_uiWidget_t*) self->heading_name;
	text_name      = (vkk_uiWidget_t*) self->text_name;
	text_ext       = (vkk_uiWidget_t*) self->text_ext;
	heading_path   = (vkk_uiWidget_t*) self->heading_path;
	bulletbox_path = (vkk_uiWidget_t*) self->bulletbox_path;
	listbox_files  = (vkk_uiWidget_t*) self->listbox_files;

	vkk_uiWidget_draw(heading_name);
	vkk_uiWidget_draw(text_name);
	vkk_uiWidget_draw(text_ext);
	vkk_uiWidget_draw(heading_path);
	vkk_uiWidget_draw(bulletbox_path);
	vkk_uiWidget_draw(listbox_files);
}

static vkk_uiWidget_t*
vkk_uiFileList_action(vkk_uiWidget_t* widget,
                      vkk_uiWidgetActionInfo_t* info)
{
	ASSERT(widget);
	ASSERT(info);

	vkk_uiFileList_t* self;
	vkk_uiWidget_t*   heading_name;
	vkk_uiWidget_t*   text_name;
	vkk_uiWidget_t*   text_ext;
	vkk_uiWidget_t*   heading_path;
	vkk_uiWidget_t*   bulletbox_path;
	vkk_uiWidget_t*   listbox_files;
	self           = (vkk_uiFileList_t*) widget;
	heading_name   = (vkk_uiWidget_t*) self->heading_name;
	text_name      = (vkk_uiWidget_t*) self->text_name;
	text_ext       = (vkk_uiWidget_t*) self->text_ext;
	heading_path   = (vkk_uiWidget_t*) self->heading_path;
	bulletbox_path = (vkk_uiWidget_t*) self->bulletbox_path;
	listbox_files  = (vkk_uiWidget_t*) self->listbox_files;

	vkk_uiWidget_t* tmp;
	tmp = vkk_uiWidget_action(heading_name, info);
	if(tmp)
	{
		return tmp;
	}

	tmp = vkk_uiWidget_action(text_name, info);
	if(tmp)
	{
		return tmp;
	}

	tmp = vkk_uiWidget_action(text_ext, info);
	if(tmp)
	{
		return tmp;
	}

	tmp = vkk_uiWidget_action(heading_path, info);
	if(tmp)
	{
		return tmp;
	}

	tmp = vkk_uiWidget_action(bulletbox_path, info);
	if(tmp)
	{
		return tmp;
	}

	return vkk_uiWidget_action(listbox_files, info);
}

static int
vkk_uiFileList_compare(const void* a, const void* b)
{
	ASSERT(a);
	ASSERT(b);

	vkk_uiBulletBox_t* aa = (vkk_uiBulletBox_t*) a;
	vkk_uiBulletBox_t* bb = (vkk_uiBulletBox_t*) b;

	return strcmp(aa->text->string, bb->text->string);
}

static void
vkk_uiFileList_addItem(vkk_uiFileList_t* self,
                       const char* label,
                       int is_file)
{
	ASSERT(self);
	ASSERT(label);

	vkk_uiScreen_t* screen = self->base.screen;

	vkk_uiBulletBoxFn_t bbfn =
	{
		.priv     = self,
		.click_fn = vkk_uiFileList_clickItem,
	};

	const char* sprite_array_path[] =
	{
		"vkk/ui/icons/ic_folder_open_white_24dp.png",
		NULL
	};

	const char* sprite_array_file[] =
	{
		"vkk/ui/icons/outline_insert_drive_file_white_48dp.png",
		NULL
	};

	const char** sprite_array_item = sprite_array_path;
	if(is_file)
	{
		sprite_array_item = sprite_array_file;
	}

	vkk_uiBulletBox_t* item;
	item = vkk_uiBulletBox_newPageItem(screen, &bbfn,
	                                   sprite_array_item);
	if(item == NULL)
	{
		return;
	}
	vkk_uiBulletBox_label(item, label);

	if(vkk_uiListBox_addSorted(self->listbox_files,
	                           vkk_uiFileList_compare,
	                           (vkk_uiWidget_t*) item) == 0)
	{
		vkk_uiBulletBox_delete(&item);
	}
}

static void
vkk_uiFileList_discard(vkk_uiFileList_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter;
	iter = vkk_uiListBox_head(self->listbox_files);
	while(iter)
	{
		vkk_uiBulletBox_t* item;
		item = (vkk_uiBulletBox_t*)
		       vkk_uiListBox_remove(self->listbox_files, &iter);
		vkk_uiBulletBox_delete(&item);
	}
}

static int
vkk_uiFileList_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiFileList_t* self = (vkk_uiFileList_t*) widget;

	if(self->dirty == 0)
	{
		return 0;
	}

	self->dirty = 0;

	// require a fully qualified path
	const char* path = vkk_uiFileList_getPath(self);
	if(path[0] != '/')
	{
		LOGE("invalid path=%s", path);
		return 0;
	}

	vkk_uiFileList_discard(self);

	// add prev directory
	const char* root_dir = "/";
	if(strcmp(path, root_dir) != 0)
	{
		vkk_uiFileList_addItem(self, "..", 0);
	}

	// add items
	DIR* dir = opendir(path);
	if(dir == NULL)
	{
		LOGE("opendir failed");
		return 0;
	}

	struct dirent* de = readdir(dir);
	while(de != NULL)
	{
		if(de->d_type == DT_DIR)
		{
			// ignore "." and ".."
			if((strcmp(de->d_name, ".") == 0) ||
			   (strcmp(de->d_name, "..") == 0))
			{
				de = readdir(dir);
				continue;
			}

			vkk_uiFileList_addItem(self, de->d_name, 0);
		}
		else if((de->d_type == DT_REG) &&
		        strstr(de->d_name,
		               vkk_uiFileList_getExt(self)))
		{
			vkk_uiFileList_addItem(self, de->d_name, 1);
		}

		de = readdir(dir);
	}
	closedir(dir);

	return 1;
}

static vkk_uiText_t*
vkk_uiFileList_newExtText(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_LARGE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageItem(screen, &style.color);

	vkk_uiTextFn_t tfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &tfn, &layout,
	                      &style, &clear);
}

static vkk_uiListBox_t*
vkk_uiFileList_newFileList(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiWidgetLayout_t list_layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_LARGE,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 1
	};
	vkk_uiScreen_colorScroll0(screen, &scroll.color0);
	vkk_uiScreen_colorScroll1(screen, &scroll.color1);

	vkk_uiListBoxFn_t lbfn =
	{
		.priv = NULL
	};

	int orientation = VKK_UI_LISTBOX_ORIENTATION_VERTICAL;

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	return vkk_uiListBox_new(screen, 0, &lbfn,
	                         &list_layout, &scroll,
	                         orientation, &clear);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiFileList_t*
vkk_uiFileList_new(vkk_uiScreen_t* screen,
                   vkk_uiFilePicker_t* parent)
{
	ASSERT(screen);
	ASSERT(parent);

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiWidgetLayout_t layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_NONE,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv         = parent,
		.action_fn    = vkk_uiFileList_action,
		.drag_fn      = vkk_uiFileList_drag,
		.draw_fn      = vkk_uiFileList_draw,
		.layout_fn    = vkk_uiFileList_layout,
		.refresh_fn   = vkk_uiFileList_refresh,
		.scrollTop_fn = vkk_uiFileList_scrollTop,
		.size_fn      = vkk_uiFileList_size,
	};

	vkk_uiFileList_t* self;
	self = (vkk_uiFileList_t*)
	       vkk_uiWidget_new(screen,
	                        sizeof(vkk_uiFileList_t),
	                        &clear, &layout, &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->heading_name = vkk_uiText_newPageHeading(screen);
	if(self->heading_name == NULL)
	{
		goto fail_heading_name;
	}

	vkk_uiTextFn_t tfn =
	{
		.priv     = parent,
		.input_fn = vkk_uiFileList_input,
	};

	self->text_name = vkk_uiText_newPageTextInput(screen,
	                                              &tfn);
	if(self->text_name == NULL)
	{
		goto fail_text_name;
	}
	vkk_uiWindow_focus(&parent->base, &self->text_name->base);

	self->text_ext = vkk_uiFileList_newExtText(screen);
	if(self->text_ext == NULL)
	{
		goto fail_text_ext;
	}

	self->heading_path = vkk_uiText_newPageHeading(screen);
	if(self->heading_path == NULL)
	{
		goto fail_heading_path;
	}

	vkk_uiBulletBoxFn_t bbfn =
	{
		.priv = NULL
	};

	const char* sprite_array_path[] =
	{
		"vkk/ui/icons/ic_folder_white_24dp.png",
		NULL
	};

	self->bulletbox_path = vkk_uiBulletBox_newPageItem(screen,
	                                                   &bbfn,
	                                                   sprite_array_path);
	if(self->bulletbox_path == NULL)
	{
		goto fail_path;
	}

	self->listbox_files = vkk_uiFileList_newFileList(screen);
	if(self->listbox_files == NULL)
	{
		goto fail_files;
	}

	vkk_uiText_label(self->heading_name, "Name");
	vkk_uiText_label(self->heading_path, "Path");

	// success
	return self;

	// failure
	fail_files:
		vkk_uiBulletBox_delete(&self->bulletbox_path);
	fail_path:
		vkk_uiText_delete(&self->heading_path);
	fail_heading_path:
		vkk_uiText_delete(&self->text_ext);
	fail_text_ext:
		vkk_uiText_delete(&self->text_name);
	fail_text_name:
		vkk_uiText_delete(&self->heading_name);
	fail_heading_name:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

void vkk_uiFileList_delete(vkk_uiFileList_t** _self)
{
	ASSERT(_self);

	vkk_uiFileList_t* self = *_self;
	if(self)
	{
		vkk_uiFileList_discard(self);
		vkk_uiListBox_delete(&self->listbox_files);
		vkk_uiBulletBox_delete(&self->bulletbox_path);
		vkk_uiText_delete(&self->heading_path);
		vkk_uiText_delete(&self->text_ext);
		vkk_uiText_delete(&self->text_name);
		vkk_uiText_delete(&self->heading_name);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiFileList_reset(vkk_uiFileList_t* self,
                          const char* path,
                          const char* name,
                          const char* ext)
{
	ASSERT(self);
	ASSERT(path);
	ASSERT(name);
	ASSERT(ext);

	vkk_uiFileList_setPath(self, path);
	vkk_uiFileList_setName(self, name);
	vkk_uiFileList_setExt(self, ext);
	self->dirty = 1;
}

void vkk_uiFileList_filepath(vkk_uiFileList_t* self,
                             char* filepath)
{
	ASSERT(self);
	ASSERT(filepath);

	const char* path = vkk_uiFileList_getPath(self);
	const char* name = vkk_uiFileList_getName(self);
	const char* ext  = vkk_uiFileList_getExt(self);
	snprintf(filepath, 256, "%s%s%s", path, name, ext);
}

void vkk_uiFileList_mkdir(vkk_uiWidget_t* widget,
                          const char* text)
{
	ASSERT(widget);
	ASSERT(text);

	vkk_uiFileList_t* self;
	self = (vkk_uiFileList_t*) vkk_uiWidget_priv(widget);

	char name[256];
	if(validateName(text, name))
	{
		const char* base = vkk_uiFileList_getPath(self);

		char path[256];
		snprintf(path, 256, "%s%s/", base, name);

		if(mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		{
			if(errno == EEXIST)
			{
				// already exists
			}
			else
			{
				LOGE("mkdir %s failed", path);
				return;
			}
		}

		vkk_uiFileList_setPath(self, path);
		self->dirty = 1;
	}
}

void
vkk_uiFileList_input(vkk_uiWidget_t* widget,
                     const char* string)
{
	ASSERT(widget);
	ASSERT(string);

	// widget may be either file_list or text_name

	vkk_uiFilePicker_t* picker;
	picker = (vkk_uiFilePicker_t*) vkk_uiWidget_priv(widget);

	vkk_uiScreen_t* screen = widget->screen;
	vkk_engine_t*   engine = screen->engine;

	// determine input string
	char fname[256];
	vkk_uiFileList_filepath(picker->file_list, fname);

	if(picker->mode == VKK_UI_FILEPICKER_MODE_CREATE)
	{
		vkk_engine_platformCmdDocumentCreate(engine,
		                                     picker->document_priv,
		                                     picker->document_fn,
		                                     fname);
	}
	else if(picker->mode == VKK_UI_FILEPICKER_MODE_OPEN)
	{
		vkk_engine_platformCmdDocumentOpen(engine,
		                                   picker->document_priv,
		                                   picker->document_fn,
		                                   fname);
	}
	else if(picker->mode == VKK_UI_FILEPICKER_MODE_NAME)
	{
		vkk_engine_platformCmdDocumentName(engine,
		                                   picker->document_priv,
		                                   picker->document_fn,
		                                   fname);
	}

	vkk_uiScreen_windowPop(screen);
}
