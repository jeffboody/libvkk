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

#ifndef vkk_ui_H
#define vkk_ui_H

typedef struct vkk_uiActionBar_s      vkk_uiActionBar_t;
typedef struct vkk_uiBulletboxStyle_s vkk_uiBulletboxStyle_t;
typedef struct vkk_uiBulletbox_s      vkk_uiBulletbox_t;
typedef struct vkk_uiCheckbox_s       vkk_uiCheckbox_t;
typedef struct vkk_uiFontcoords_s     vkk_uiFontcoords_t;
typedef struct vkk_uiFont_s           vkk_uiFont_t;
typedef struct vkk_uiHline_s          vkk_uiHline_t;
typedef struct vkk_uiInfoPanel_s      vkk_uiInfoPanel_t;
typedef struct vkk_uiLayer_s          vkk_uiLayer_t;
typedef struct vkk_uiListbox_s        vkk_uiListbox_t;
typedef struct vkk_uiRadiobox_s       vkk_uiRadiobox_t;
typedef struct vkk_uiRadiolist_s      vkk_uiRadiolist_t;
typedef struct vkk_uiScreen_s         vkk_uiScreen_t;
typedef struct vkk_uiSprite_s         vkk_uiSprite_t;
typedef struct vkk_uiStatusBar_s      vkk_uiStatusBar_t;
typedef struct vkk_uiTextFn_s         vkk_uiTextFn_t;
typedef struct vkk_uiTextLayout_s     vkk_uiTextLayout_s;
typedef struct vkk_uiTextStyle_s      vkk_uiTextStyle_t;
typedef struct vkk_uiText_s           vkk_uiText_t;
typedef struct vkk_uiTextbox_s        vkk_uiTextbox_t;
typedef struct vkk_uiTricolor_s       vkk_uiTricolor_t;
typedef struct vkk_uiWidgetFn_s       vkk_uiWidgetFn_t;
typedef struct vkk_uiWidgetLayout_s   vkk_uiWidgetLayout_t;
typedef struct vkk_uiWidgetScroll_s   vkk_uiWidgetScroll_t;
typedef struct vkk_uiWidgetStyle_s    vkk_uiWidgetStyle_t;
typedef struct vkk_uiWidgetPrivFn_s   vkk_uiWidgetPrivFn_t;
typedef struct vkk_uiWidget_s         vkk_uiWidget_t;
typedef struct vkk_uiWindowInfo_s     vkk_uiWindowInfo_t;
typedef struct vkk_uiWindow_s         vkk_uiWindow_t;

#include "ui/vkk_uiFont.h"
#include "ui/vkk_uiTricolor.h"
#include "ui/vkk_uiWidget.h"

// depends on widget
#include "ui/vkk_uiScreen.h"

// derive from widget
#include "ui/vkk_uiHline.h"
#include "ui/vkk_uiLayer.h"
#include "ui/vkk_uiListbox.h"
#include "ui/vkk_uiSprite.h"
#include "ui/vkk_uiText.h"
#include "ui/vkk_uiWindow.h"

// derive from widget and text
#include "ui/vkk_uiBulletbox.h"

// derive from bulletbox
#include "ui/vkk_uiCheckbox.h"
#include "ui/vkk_uiRadiobox.h"

// derive from listbox
#include "ui/vkk_uiActionBar.h"
#include "ui/vkk_uiInfoPanel.h"
#include "ui/vkk_uiRadiolist.h"
#include "ui/vkk_uiStatusBar.h"
#include "ui/vkk_uiTextbox.h"

#endif
