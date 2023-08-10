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
#include "vkk.h"

/*
 * platform commands
 */

typedef void (*vkk_platformCmd_documentFn)
             (void* priv, const char* uri, int* _fd);

typedef enum vkk_platformCmd_s
{
	VKK_PLATFORM_CMD_ACCELEROMETER_OFF  = 1,
	VKK_PLATFORM_CMD_ACCELEROMETER_ON   = 2,
	VKK_PLATFORM_CMD_CHECK_PERMISSIONS  = 3,
	VKK_PLATFORM_CMD_EXIT               = 4,
	VKK_PLATFORM_CMD_GPS_OFF            = 5,
	VKK_PLATFORM_CMD_GPS_ON             = 6,
	VKK_PLATFORM_CMD_GPS_RECORD         = 7,
	VKK_PLATFORM_CMD_GPS_PAUSE          = 8,
	VKK_PLATFORM_CMD_GYROSCOPE_OFF      = 9,
	VKK_PLATFORM_CMD_GYROSCOPE_ON       = 10,
	VKK_PLATFORM_CMD_LOADURL            = 11,
	VKK_PLATFORM_CMD_MAGNETOMETER_OFF   = 12,
	VKK_PLATFORM_CMD_MAGNETOMETER_ON    = 13,
	VKK_PLATFORM_CMD_PLAY_CLICK         = 14,
	VKK_PLATFORM_CMD_PLAY_NOTIFY        = 15,
	VKK_PLATFORM_CMD_FINE_LOCATION_PERM = 16,
	VKK_PLATFORM_CMD_SOFTKEY_HIDE       = 17,
	VKK_PLATFORM_CMD_SOFTKEY_SHOW       = 18,
	VKK_PLATFORM_CMD_DOCUMENT_CREATE    = 19,
	VKK_PLATFORM_CMD_DOCUMENT_OPEN      = 20,
	VKK_PLATFORM_CMD_DOCUMENT_NAME      = 21,
	VKK_PLATFORM_CMD_MEMORY_INFO        = 22,
} vkk_platformCmd_e;

/*
 * event handling
 */

// type
typedef enum
{
	VKK_PLATFORM_EVENTTYPE_UNDEFINED         = -1,
	VKK_PLATFORM_EVENTTYPE_ACCELEROMETER     = 0,
	VKK_PLATFORM_EVENTTYPE_ACTION_DOWN       = 1,
	VKK_PLATFORM_EVENTTYPE_ACTION_MOVE       = 2,
	VKK_PLATFORM_EVENTTYPE_ACTION_UP         = 3,
	VKK_PLATFORM_EVENTTYPE_AXIS_MOVE         = 4,
	VKK_PLATFORM_EVENTTYPE_BUTTON_DOWN       = 5,
	VKK_PLATFORM_EVENTTYPE_BUTTON_UP         = 6,
	VKK_PLATFORM_EVENTTYPE_DENSITY           = 7,
	VKK_PLATFORM_EVENTTYPE_GPS               = 8,
	VKK_PLATFORM_EVENTTYPE_GYROSCOPE         = 9,
	VKK_PLATFORM_EVENTTYPE_KEY_DOWN          = 10,
	VKK_PLATFORM_EVENTTYPE_KEY_UP            = 11,
	VKK_PLATFORM_EVENTTYPE_MAGNETOMETER      = 12,
	VKK_PLATFORM_EVENTTYPE_CONTENT_RECT      = 13,
	VKK_PLATFORM_EVENTTYPE_PERMISSION_STATUS = 14,
	VKK_PLATFORM_EVENTTYPE_LOW_MEMORY        = 15,
	VKK_PLATFORM_EVENTTYPE_MEMORY_INFO       = 16,
} vkk_platformEventType_e;

// max actions supported
#define VKK_PLATFORM_EVENTACTION_COUNT 4

// axis ids
typedef enum
{
	VKK_PLATFORM_AXIS_X1 = 0x00,
	VKK_PLATFORM_AXIS_Y1 = 0x01,
	VKK_PLATFORM_AXIS_X2 = 0x0B,
	VKK_PLATFORM_AXIS_Y2 = 0x0E,
	VKK_PLATFORM_AXIS_RT = 0x12,
	VKK_PLATFORM_AXIS_LT = 0x11,
	VKK_PLATFORM_AXIS_HX = 0x0F,
	VKK_PLATFORM_AXIS_HY = 0x10,
} vkk_platformAxis_e;

// button ids
typedef enum
{
	VKK_PLATFORM_BUTTON_1      = 0xBC,
	VKK_PLATFORM_BUTTON_2      = 0xBD,
	VKK_PLATFORM_BUTTON_3      = 0xBE,
	VKK_PLATFORM_BUTTON_4      = 0xBF,
	VKK_PLATFORM_BUTTON_5      = 0xC0,
	VKK_PLATFORM_BUTTON_6      = 0xC1,
	VKK_PLATFORM_BUTTON_7      = 0xC2,
	VKK_PLATFORM_BUTTON_8      = 0xC3,
	VKK_PLATFORM_BUTTON_9      = 0xC4,
	VKK_PLATFORM_BUTTON_10     = 0xC5,
	VKK_PLATFORM_BUTTON_11     = 0xC6,
	VKK_PLATFORM_BUTTON_12     = 0xC7,
	VKK_PLATFORM_BUTTON_13     = 0xC8,
	VKK_PLATFORM_BUTTON_14     = 0xC9,
	VKK_PLATFORM_BUTTON_15     = 0xCA,
	VKK_PLATFORM_BUTTON_16     = 0xCB,
	VKK_PLATFORM_BUTTON_A      = 0x60,
	VKK_PLATFORM_BUTTON_B      = 0x61,
	VKK_PLATFORM_BUTTON_C      = 0x62,
	VKK_PLATFORM_BUTTON_L1     = 0x66,
	VKK_PLATFORM_BUTTON_R1     = 0x67,
	VKK_PLATFORM_BUTTON_L2     = 0x68,
	VKK_PLATFORM_BUTTON_R2     = 0x69,
	VKK_PLATFORM_BUTTON_MODE   = 0x6E,
	VKK_PLATFORM_BUTTON_SELECT = 0x6D,
	VKK_PLATFORM_BUTTON_START  = 0x6C,
	VKK_PLATFORM_BUTTON_THUMBL = 0x6A,
	VKK_PLATFORM_BUTTON_THUMBR = 0x6B,
	VKK_PLATFORM_BUTTON_X      = 0x63,
	VKK_PLATFORM_BUTTON_Y      = 0x64,
	VKK_PLATFORM_BUTTON_Z      = 0x65,
	VKK_PLATFORM_BUTTON_UP     = 0x13,
	VKK_PLATFORM_BUTTON_DOWN   = 0x14,
	VKK_PLATFORM_BUTTON_LEFT   = 0x15,
	VKK_PLATFORM_BUTTON_RIGHT  = 0x16,
	VKK_PLATFORM_BUTTON_CENTER = 0x17,
} vkk_platformButton_e;

// special keys
// normal keys use standard ASCII keycode
#define VKK_PLATFORM_KEYCODE_ENTER     0x00D
#define VKK_PLATFORM_KEYCODE_ESCAPE    0x01B
#define VKK_PLATFORM_KEYCODE_BACKSPACE 0x008
#define VKK_PLATFORM_KEYCODE_DELETE    0x07F
#define VKK_PLATFORM_KEYCODE_UP        0x100
#define VKK_PLATFORM_KEYCODE_DOWN      0x101
#define VKK_PLATFORM_KEYCODE_LEFT      0x102
#define VKK_PLATFORM_KEYCODE_RIGHT     0x103
#define VKK_PLATFORM_KEYCODE_HOME      0x104
#define VKK_PLATFORM_KEYCODE_END       0x105
#define VKK_PLATFORM_KEYCODE_PGUP      0x106
#define VKK_PLATFORM_KEYCODE_PGDOWN    0x107
#define VKK_PLATFORM_KEYCODE_INSERT    0x108

// meta key mask
#define VKK_PLATFORM_META_ALT     0x00000032
#define VKK_PLATFORM_META_ALT_L   0x00000010
#define VKK_PLATFORM_META_ALT_R   0x00000020
#define VKK_PLATFORM_META_CTRL    0x00007000
#define VKK_PLATFORM_META_CTRL_L  0x00002000
#define VKK_PLATFORM_META_CTRL_R  0x00004000
#define VKK_PLATFORM_META_SHIFT   0x000000C1
#define VKK_PLATFORM_META_SHIFT_L 0x00000040
#define VKK_PLATFORM_META_SHIFT_R 0x00000080
#define VKK_PLATFORM_META_CAPS    0x00100000

typedef struct
{
	float ax;
	float ay;
	float az;
	int   rotation;
} vkk_platformEventAccelerometer_t;

typedef struct
{
	int count;
	cc_vec2f_t coord[VKK_PLATFORM_EVENTACTION_COUNT];
} vkk_platformEventAction_t;

typedef struct
{
	int                id;
	vkk_platformAxis_e axis;
	float              value;
} vkk_platformEventAxis_t;

typedef struct
{
	int                  id;
	vkk_platformButton_e button;
} vkk_platformEventButton_t;

typedef struct
{
	double lat;
	double lon;
	float  accuracy;
	float  altitude;
	float  speed;
	float  bearing;
} vkk_platformEventGps_t;

typedef struct
{
	float ax;
	float ay;
	float az;
} vkk_platformEventGyroscope_t;

typedef struct
{
	int keycode;
	int meta;
	int repeat;
} vkk_platformEventKey_t;

typedef struct
{
	float mx;
	float my;
	float mz;
	float gfx;
	float gfy;
	float gfz;
} vkk_platformEventMagnetometer_t;

typedef struct
{
	int t;
	int l;
	int b;
	int r;
} vkk_platformEventContentRect_t;

// Android permissions
typedef enum
{
	VKK_PLATFORM_PERMISSION_FINE_LOCATION = 1,
} vkk_platformPermission_e;

typedef struct
{
	vkk_platformPermission_e permission;
	int status;
} vkk_platformEventPermission_t;

typedef struct
{
	size_t available;
	size_t threshold;
	size_t total;
	int    low;
} vkk_platformEventMemoryInfo_t;

typedef struct
{
	vkk_platformEventType_e type;
	double ts;
	union
	{
		vkk_platformEventAccelerometer_t accelerometer;
		vkk_platformEventAction_t        action;
		vkk_platformEventAxis_t          axis;
		vkk_platformEventButton_t        button;
		float                            density;
		vkk_platformEventGps_t           gps;
		vkk_platformEventGyroscope_t     gyroscope;
		vkk_platformEventKey_t           key;
		vkk_platformEventMagnetometer_t  magnetometer;
		vkk_platformEventContentRect_t   content_rect;
		vkk_platformEventPermission_t    permission;
		vkk_platformEventMemoryInfo_t    memory_info;
	};
} vkk_platformEvent_t;

/*
 * platform API
 * The platform callback functions are always called from
 * from the main thread EXCEPT for GPS events which are
 * called from the UI thread. This is because the app may
 * have requested GPS recording which can cause events to
 * be delivered when the app main thread is paused.
 * The onMain callback may be used on Linux when no display
 * is required (all other callbacks are unsupported).
 */

typedef void* (*vkk_platformOnCreate_fn)(vkk_engine_t* engine);
typedef void  (*vkk_platformOnDestroy_fn)(void** _priv);
typedef void  (*vkk_platformOnPause_fn)(void* priv);
typedef void  (*vkk_platformOnDraw_fn)(void* priv);
typedef int   (*vkk_platformOnEvent_fn)(void* priv,
                                        vkk_platformEvent_t* event);
typedef int   (*vkk_platformOnMain_fn)(vkk_engine_t* engine,
                                       int argc,
                                       char** argv);

typedef struct
{
	const char*              app_name;
	const char*              app_dir;
	vkk_version_t            app_version;
	vkk_platformOnCreate_fn  onCreate;
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnPause_fn   onPause;
	vkk_platformOnDraw_fn    onDraw;
	vkk_platformOnEvent_fn   onEvent;
	vkk_platformOnMain_fn    onMain;
} vkk_platformInfo_t;

extern vkk_platformInfo_t VKK_PLATFORM_INFO;

#endif
