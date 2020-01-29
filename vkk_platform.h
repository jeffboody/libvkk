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

#ifndef vkk_platform_H
#define vkk_platform_H

#include "../libcc/math/cc_vec2f.h"

/*
 * opaque objects
 */

typedef struct vkk_platform_s vkk_platform_t;

/*
 * platform commands
 */

#define VKK_PLATFORM_CMD_ACCELEROMETER_OFF 1
#define VKK_PLATFORM_CMD_ACCELEROMETER_ON  2
#define VKK_PLATFORM_CMD_CHECK_PERMISSIONS 3
#define VKK_PLATFORM_CMD_EXIT              4
#define VKK_PLATFORM_CMD_GPS_OFF           5
#define VKK_PLATFORM_CMD_GPS_ON            6
#define VKK_PLATFORM_CMD_GPS_RECORD        7
#define VKK_PLATFORM_CMD_GPS_PAUSE         8
#define VKK_PLATFORM_CMD_GYROSCOPE_OFF     9
#define VKK_PLATFORM_CMD_GYROSCOPE_ON      10
#define VKK_PLATFORM_CMD_LOADURL           11
#define VKK_PLATFORM_CMD_MAGNETOMETER_OFF  12
#define VKK_PLATFORM_CMD_MAGNETOMETER_ON   13
#define VKK_PLATFORM_CMD_PLAY_CLICK        14
#define VKK_PLATFORM_CMD_REQ_LOCATION_PERM 15
#define VKK_PLATFORM_CMD_REQ_STORAGE_PERM  16
#define VKK_PLATFORM_CMD_SOFTKEY_HIDE      17
#define VKK_PLATFORM_CMD_SOFTKEY_SHOW      18

void vkk_platform_cmd(vkk_platform_t* self, int cmd,
                      const char* msg);

/*
 * event handling
 */

// type
#define VKK_EVENT_TYPE_ACCELEROMETER      0
#define VKK_EVENT_TYPE_ACTION_DOWN        1
#define VKK_EVENT_TYPE_ACTION_MOVE        2
#define VKK_EVENT_TYPE_ACTION_UP          3
#define VKK_EVENT_TYPE_AXIS_MOVE          4
#define VKK_EVENT_TYPE_BUTTON_DOWN        5
#define VKK_EVENT_TYPE_BUTTON_UP          6
#define VKK_EVENT_TYPE_DENSITY            7
#define VKK_EVENT_TYPE_GPS                8
#define VKK_EVENT_TYPE_GYROSCOPE          9
#define VKK_EVENT_TYPE_KEY_DOWN           10
#define VKK_EVENT_TYPE_KEY_UP             11
#define VKK_EVENT_TYPE_MAGNETOMETER       12
#define VKK_EVENT_TYPE_PAUSE              13
#define VKK_EVENT_TYPE_PERMISSION_GRANTED 14
#define VKK_EVENT_TYPE_RECREATE           15
#define VKK_EVENT_TYPE_RESIZE             16

// max actions supported
#define VKK_EVENT_ACTION_COUNT 4

// axis ids
#define VKK_AXIS_X1 0x00
#define VKK_AXIS_Y1 0x01
#define VKK_AXIS_X2 0x0B
#define VKK_AXIS_Y2 0x0E
#define VKK_AXIS_RT 0x12
#define VKK_AXIS_LT 0x11
#define VKK_AXIS_HX 0x0F
#define VKK_AXIS_HY 0x10

// button ids
#define VKK_BUTTON_1      0xBC
#define VKK_BUTTON_2      0xBD
#define VKK_BUTTON_3      0xBE
#define VKK_BUTTON_4      0xBF
#define VKK_BUTTON_5      0xC0
#define VKK_BUTTON_6      0xC1
#define VKK_BUTTON_7      0xC2
#define VKK_BUTTON_8      0xC3
#define VKK_BUTTON_9      0xC4
#define VKK_BUTTON_10     0xC5
#define VKK_BUTTON_11     0xC6
#define VKK_BUTTON_12     0xC7
#define VKK_BUTTON_13     0xC8
#define VKK_BUTTON_14     0xC9
#define VKK_BUTTON_15     0xCA
#define VKK_BUTTON_16     0xCB
#define VKK_BUTTON_A      0x60
#define VKK_BUTTON_B      0x61
#define VKK_BUTTON_C      0x62
#define VKK_BUTTON_L1     0x66
#define VKK_BUTTON_R1     0x67
#define VKK_BUTTON_L2     0x68
#define VKK_BUTTON_R2     0x69
#define VKK_BUTTON_MODE   0x6E
#define VKK_BUTTON_SELECT 0x6D
#define VKK_BUTTON_START  0x6C
#define VKK_BUTTON_THUMBL 0x6A
#define VKK_BUTTON_THUMBR 0x6B
#define VKK_BUTTON_X      0x63
#define VKK_BUTTON_Y      0x64
#define VKK_BUTTON_Z      0x65
#define VKK_BUTTON_UP     0x13
#define VKK_BUTTON_DOWN   0x14
#define VKK_BUTTON_LEFT   0x15
#define VKK_BUTTON_RIGHT  0x16
#define VKK_BUTTON_CENTER 0x17

// special keys
// normal keys use standard ASCII keycode
#define VKK_KEYCODE_ENTER     0x00D
#define VKK_KEYCODE_ESCAPE    0x01B
#define VKK_KEYCODE_BACKSPACE 0x008
#define VKK_KEYCODE_DELETE    0x07F
#define VKK_KEYCODE_UP        0x100
#define VKK_KEYCODE_DOWN      0x101
#define VKK_KEYCODE_LEFT      0x102
#define VKK_KEYCODE_RIGHT     0x103
#define VKK_KEYCODE_HOME      0x104
#define VKK_KEYCODE_END       0x105
#define VKK_KEYCODE_PGUP      0x106
#define VKK_KEYCODE_PGDOWN    0x107
#define VKK_KEYCODE_INSERT    0x108

// meta key mask
#define VKK_META_ALT     0x00000032
#define VKK_META_ALT_L   0x00000010
#define VKK_META_ALT_R   0x00000020
#define VKK_META_CTRL    0x00007000
#define VKK_META_CTRL_L  0x00002000
#define VKK_META_CTRL_R  0x00004000
#define VKK_META_SHIFT   0x000000C1
#define VKK_META_SHIFT_L 0x00000040
#define VKK_META_SHIFT_R 0x00000080
#define VKK_META_CAPS    0x00100000

typedef struct
{
	float ax;
	float ay;
	float az;
	int   rotation;
} vkk_eventAccelerometer_t;

typedef struct
{
	int count;
	cc_vec2f_t coord[VKK_EVENT_ACTION_COUNT];
} vkk_eventAction_t;

typedef struct
{
	int   id;
	int   axis;
	float value;
} vkk_eventAxis_t;

typedef struct
{
	int id;
	int button;
} vkk_eventButton_t;

typedef struct
{
	double lat;
	double lon;
	float  accuracy;
	float  altitude;
	float  speed;
	float  bearing;
} vkk_eventGps_t;

typedef struct
{
	float ax;
	float ay;
	float az;
} vkk_eventGyroscope_t;

typedef struct
{
	int keycode;
	int meta;
	int repeat;
} vkk_eventKey_t;

typedef struct
{
	float mx;
	float my;
	float mz;
	float gfx;
	float gfy;
	float gfz;
} vkk_eventMagnetometer_t;

typedef struct
{
	int    type;
	double ts;
	union
	{
		vkk_eventAccelerometer_t accelerometer;
		vkk_eventAction_t        action;
		vkk_eventAxis_t          axis;
		vkk_eventButton_t        button;
		float                    density;
		vkk_eventGps_t           gps;
		vkk_eventGyroscope_t     gyroscope;
		vkk_eventKey_t           key;
		vkk_eventMagnetometer_t  magnetometer;
		int                      permission;
	};
} vkk_event_t;

// Android permissions
#define VKK_PERMISSION_LOCATION 1
#define VKK_PERMISSION_STORAGE  2

/*
 * platform API
 */

typedef void* (*vkk_platformOnCreate_fn)(vkk_platform_t* platform);
typedef void  (*vkk_platformOnDestroy_fn)(void** _priv);
typedef void  (*vkk_platformOnDraw_fn)(void* priv);
typedef int   (*vkk_platformOnEvent_fn)(void* priv,
                                        vkk_event_t* event);

typedef struct
{
	vkk_platformOnCreate_fn  onCreate;
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnDraw_fn    onDraw;
	vkk_platformOnEvent_fn   onEvent;
} vkk_platformCallbacks_t;

extern vkk_platformCallbacks_t VKK_PLATFORM_CALLBACKS;

#endif
