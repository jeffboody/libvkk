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

typedef struct vkk_uiActionBarFn_s    vkk_uiActionBarFn_t;
typedef struct vkk_uiActionBar_s      vkk_uiActionBar_t;
typedef struct vkk_uiActionButtonFn_s vkk_uiActionButtonFn_t;
typedef struct vkk_uiActionButton_s   vkk_uiActionButton_t;
typedef struct vkk_uiActionPopupFn_s  vkk_uiActionPopupFn_t;
typedef struct vkk_uiActionPopup_s    vkk_uiActionPopup_t;
typedef struct vkk_uiBulletBoxFn_s    vkk_uiBulletBoxFn_t;
typedef struct vkk_uiBulletBoxStyle_s vkk_uiBulletBoxStyle_t;
typedef struct vkk_uiBulletBox_s      vkk_uiBulletBox_t;
typedef struct vkk_uiCheckBoxFn_s     vkk_uiCheckBoxFn_t;
typedef struct vkk_uiCheckBox_s       vkk_uiCheckBox_t;
typedef struct vkk_uiFileList_s       vkk_uiFileList_t;
typedef struct vkk_uiFilePicker_s     vkk_uiFilePicker_t;
typedef struct vkk_uiFontcoords_s     vkk_uiFontcoords_t;
typedef struct vkk_uiFont_s           vkk_uiFont_t;
typedef struct vkk_uiGraphicsBox_s    vkk_uiGraphicsBox_t;
typedef struct vkk_uiInfoPanelFn_s    vkk_uiInfoPanelFn_t;
typedef struct vkk_uiInfoPanel_s      vkk_uiInfoPanel_t;
typedef struct vkk_uiLayerFn_s        vkk_uiLayerFn_t;
typedef struct vkk_uiLayer_s          vkk_uiLayer_t;
typedef struct vkk_uiListBoxFn_s      vkk_uiListBoxFn_t;
typedef struct vkk_uiListBox_s        vkk_uiListBox_t;
typedef struct vkk_uiRadioBoxFn_s     vkk_uiRadioBoxFn_t;
typedef struct vkk_uiRadioBox_s       vkk_uiRadioBox_t;
typedef struct vkk_uiRadioListFn_s    vkk_uiRadioListFn_t;
typedef struct vkk_uiRadioList_s      vkk_uiRadioList_t;
typedef struct vkk_uiScreen_s         vkk_uiScreen_t;
typedef struct vkk_uiSeparator_s      vkk_uiSeparator_t;
typedef struct vkk_uiSpriteFn_s       vkk_uiSpriteFn_t;
typedef struct vkk_uiSprite_s         vkk_uiSprite_t;
typedef struct vkk_uiStatusBarFn_s    vkk_uiStatusBarFn_t;
typedef struct vkk_uiStatusBar_s      vkk_uiStatusBar_t;
typedef struct vkk_uiInputWindowFn_s  vkk_uiInputWindowFn_t;
typedef struct vkk_uiInputWindow_s    vkk_uiInputWindow_t;
typedef struct vkk_uiTextFn_s         vkk_uiTextFn_t;
typedef struct vkk_uiTextLayout_s     vkk_uiTextLayout_s;
typedef struct vkk_uiTextStyle_s      vkk_uiTextStyle_t;
typedef struct vkk_uiText_s           vkk_uiText_t;
typedef struct vkk_uiTextBoxFn_s      vkk_uiTextBoxFn_t;
typedef struct vkk_uiTextBox_s        vkk_uiTextBox_t;
typedef struct vkk_uiTricolor_s       vkk_uiTricolor_t;
typedef struct vkk_uiWidgetFn_s       vkk_uiWidgetFn_t;
typedef struct vkk_uiWidgetLayout_s   vkk_uiWidgetLayout_t;
typedef struct vkk_uiWidgetScroll_s   vkk_uiWidgetScroll_t;
typedef struct vkk_uiWidgetStyle_s    vkk_uiWidgetStyle_t;
typedef struct vkk_uiWidget_s         vkk_uiWidget_t;
typedef struct vkk_uiWindowFn_s       vkk_uiWindowFn_t;
typedef struct vkk_uiWindow_s         vkk_uiWindow_t;

#include "vkk_platform.h"

#include "ui/vkk_uiFont.h"
#include "ui/vkk_uiTricolor.h"
#include "ui/vkk_uiWidget.h"

// depends on widget
#include "ui/vkk_uiScreen.h"

// derive from widget
#include "ui/vkk_uiFileList.h"
#include "ui/vkk_uiGraphicsBox.h"
#include "ui/vkk_uiSeparator.h"
#include "ui/vkk_uiLayer.h"
#include "ui/vkk_uiListBox.h"
#include "ui/vkk_uiSprite.h"
#include "ui/vkk_uiText.h"
#include "ui/vkk_uiWindow.h"

// derive from widget and text
#include "ui/vkk_uiBulletBox.h"

// derive from window
#include "ui/vkk_uiFilePicker.h"
#include "ui/vkk_uiInputWindow.h"
#include "ui/vkk_uiActionPopup.h"

// derive from bulletbox
#include "ui/vkk_uiCheckBox.h"
#include "ui/vkk_uiRadioBox.h"

// derive from listbox
#include "ui/vkk_uiActionBar.h"
#include "ui/vkk_uiInfoPanel.h"
#include "ui/vkk_uiRadioList.h"
#include "ui/vkk_uiStatusBar.h"
#include "ui/vkk_uiTextBox.h"

// derive from sprite
#include "ui/vkk_uiActionButton.h"

#endif
