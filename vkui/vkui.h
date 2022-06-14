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

typedef struct vkui_actionBar_s      vkui_actionBar_t;
typedef struct vkui_bulletboxStyle_s vkui_bulletboxStyle_t;
typedef struct vkui_bulletbox_s      vkui_bulletbox_t;
typedef struct vkui_checkbox_s       vkui_checkbox_t;
typedef struct vkui_fontcoords_s     vkui_fontcoords_t;
typedef struct vkui_font_s           vkui_font_t;
typedef struct vkui_hline_s          vkui_hline_t;
typedef struct vkui_infoPanel_s      vkui_infoPanel_t;
typedef struct vkui_layer_s          vkui_layer_t;
typedef struct vkui_listbox_s        vkui_listbox_t;
typedef struct vkui_radiobox_s       vkui_radiobox_t;
typedef struct vkui_radiolist_s      vkui_radiolist_t;
typedef struct vkui_screen_s         vkui_screen_t;
typedef struct vkui_sprite_s         vkui_sprite_t;
typedef struct vkui_statusBar_s      vkui_statusBar_t;
typedef struct vkui_textFn_s         vkui_textFn_t;
typedef struct vkui_textLayout_s     vkui_textLayout_s;
typedef struct vkui_textStyle_s      vkui_textStyle_t;
typedef struct vkui_text_s           vkui_text_t;
typedef struct vkui_textbox_s        vkui_textbox_t;
typedef struct vkui_tricolor_s       vkui_tricolor_t;
typedef struct vkui_widgetFn_s       vkui_widgetFn_t;
typedef struct vkui_widgetLayout_s   vkui_widgetLayout_t;
typedef struct vkui_widgetScroll_s   vkui_widgetScroll_t;
typedef struct vkui_widgetStyle_s    vkui_widgetStyle_t;
typedef struct vkui_widgetPrivFn_s   vkui_widgetPrivFn_t;
typedef struct vkui_widget_s         vkui_widget_t;
typedef struct vkui_windowInfo_s     vkui_windowInfo_t;
typedef struct vkui_window_s         vkui_window_t;

#include "vkui_font.h"
#include "vkui_tricolor.h"
#include "vkui_widget.h"

// depends on widget
#include "vkui_screen.h"

// derive from widget
#include "vkui_hline.h"
#include "vkui_layer.h"
#include "vkui_listbox.h"
#include "vkui_sprite.h"
#include "vkui_text.h"
#include "vkui_window.h"

// derive from widget and text
#include "vkui_bulletbox.h"

// derive from bulletbox
#include "vkui_checkbox.h"
#include "vkui_radiobox.h"

// derive from listbox
#include "vkui_actionBar.h"
#include "vkui_infoPanel.h"
#include "vkui_radiolist.h"
#include "vkui_statusBar.h"
#include "vkui_textbox.h"

#endif
