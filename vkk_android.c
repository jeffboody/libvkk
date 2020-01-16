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

#include <android_native_app_glue.h>
#include <jni.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <vulkan_wrapper.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "../libcc/cc_timestamp.h"
#include "vkk_android.h"
#include "vkk.h"

/***********************************************************
* apply shift key                                          *
***********************************************************/

static const char SHIFTKEYS[128] =
{
	0x00,
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x0D,
	0x0E,
	0x0F,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x17,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x1C,
	0x1D,
	0x1E,
	0x1F,
	0x20,
	0x21,
	0x22,
	0x23,
	0x24,
	0x25,
	0x26,
	'\"',
	0x28,
	0x29,
	0x2A,
	0x2B,
	'<',
	'_',
	'>',
	'?',
	')',
	'!',
	'@',
	'#',
	'$',
	'%',
	'^',
	'&',
	'*',
	'(',
	0x3A,
	':',
	0x3C,
	'+',
	0x3E,
	0x3F,
	0x40,
	0x41,
	0x42,
	0x43,
	0x44,
	0x45,
	0x46,
	0x47,
	0x48,
	0x49,
	0x4A,
	0x4B,
	0x4C,
	0x4D,
	0x4E,
	0x4F,
	0x50,
	0x51,
	0x52,
	0x53,
	0x54,
	0x55,
	0x56,
	0x57,
	0x58,
	0x59,
	0x5A,
	'{',
	'|',
	'}',
	0x5E,
	0x5F,
	'~',
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	0x7B,
	0x7C,
	0x7D,
	0x7E,
	0x7F,
};

static int shiftKeycode(int keycode, int meta)
{
	if((keycode >= 0) && (keycode < 128))
	{
		int shiftchar = meta & (VKK_META_SHIFT | VKK_META_CAPS);
		int shiftsym  = meta & VKK_META_SHIFT;
		if(shiftchar && (keycode >= 'a') && (keycode <= 'z'))
		{
			return SHIFTKEYS[keycode];
		}
		else if(shiftsym)
		{
			return SHIFTKEYS[keycode];
		}
	}
	else if(keycode == VKK_KEYCODE_DELETE)
	{
		return VKK_KEYCODE_INSERT;
	}
	return keycode;
}

/***********************************************************
* eventq interface                                         *
***********************************************************/

#define VKK_EVENTQ_BUFSIZE 256

typedef struct
{
	// event state
	pthread_mutex_t event_mutex;
	pthread_cond_t  event_cond;
	int             event_head;
	int             event_tail;
	vkk_event_t     event_buffer[VKK_EVENTQ_BUFSIZE];
} vkk_eventq_t;

static vkk_eventq_t* eventq = NULL;

static vkk_eventq_t* vkk_eventq_new(void)
{
	vkk_eventq_t* self;
	self = (vkk_eventq_t*) CALLOC(1, sizeof(vkk_eventq_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// PTHREAD_MUTEX_DEFAULT is not re-entrant
	if(pthread_mutex_init(&self->event_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_mutex_init;
	}

	if(pthread_cond_init(&self->event_cond, NULL) != 0)
	{
		LOGE("pthread_cond_init failed");
		goto fail_cond_init;
	}

	self->event_head = 0;
	self->event_tail = 0;

	// success
	return self;

	// failure
	fail_cond_init:
		pthread_mutex_destroy(&self->event_mutex);
	fail_mutex_init:
		FREE(self);
	return NULL;
}

static void vkk_eventq_delete(vkk_eventq_t** _self)
{
	ASSERT(_self);

	vkk_eventq_t* self = *_self;
	if(self)
	{
		pthread_cond_destroy(&self->event_cond);
		pthread_mutex_destroy(&self->event_mutex);
		FREE(self);
		*_self = NULL;
	}
}

static int
vkk_eventq_poll(vkk_eventq_t* self, vkk_event_t* e)
{
	ASSERT(self);
	ASSERT(e);

	pthread_mutex_lock(&self->event_mutex);

	int has_event = 0;
	if(self->event_head == self->event_tail)
	{
		// buffer is empty
	}
	else
	{
		*e = self->event_buffer[self->event_head];
		self->event_head = (self->event_head + 1) %
		                   VKK_EVENTQ_BUFSIZE;
		has_event = 1;
		pthread_cond_signal(&self->event_cond);
	}

	pthread_mutex_unlock(&self->event_mutex);

	return has_event;
}

static vkk_event_t* vkk_eventq_dequeue(vkk_eventq_t* self)
{
	ASSERT(self);

	return &self->event_buffer[self->event_tail];
}

static void vkk_eventq_enqueue(vkk_eventq_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->event_mutex);

	self->event_tail = (self->event_tail + 1) %
	                   VKK_EVENTQ_BUFSIZE;
	if(self->event_tail == self->event_head)
	{
		// wait if the buffer is full
		pthread_cond_wait(&self->event_cond,
		                  &self->event_mutex);
	}

	pthread_mutex_unlock(&self->event_mutex);
}

/***********************************************************
* JNI interface                                            *
***********************************************************/

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeAccelerometer(JNIEnv* env,
                                                             jobject obj,
                                                             jfloat ax,
                                                             jfloat ay,
                                                             jfloat az,
                                                             jint rotation,
                                                             jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e            = vkk_eventq_dequeue(eventq);
		e->type                   = VKK_EVENT_TYPE_ACCELEROMETER;
		e->ts                     = ts;
		e->accelerometer.ax       = ax;
		e->accelerometer.ay       = ay;
		e->accelerometer.az       = az;
		e->accelerometer.rotation = rotation;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeButtonDown(JNIEnv* env,
                                                          jobject  obj,
                                                          jint id,
                                                          jint button,
                                                          jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e   = vkk_eventq_dequeue(eventq);
		e->type          = VKK_EVENT_TYPE_BUTTON_DOWN;
		e->ts            = ts;
		e->button.id     = id;
		e->button.button = button;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeButtonUp(JNIEnv* env,
                                                        jobject obj,
                                                        jint id,
                                                        jint button,
                                                        jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e   = vkk_eventq_dequeue(eventq);
		e->type          = VKK_EVENT_TYPE_BUTTON_UP;
		e->ts            = ts;
		e->button.id     = id;
		e->button.button = button;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeDensity(JNIEnv* env,
                                                       jobject obj,
                                                       jfloat density)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e = vkk_eventq_dequeue(eventq);
		e->type        = VKK_EVENT_TYPE_DENSITY;
		e->ts          = 0.0;
		e->density     = density;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeGrantPermission(JNIEnv* env,
                                                               jobject obj,
                                                               jint permission)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e = vkk_eventq_dequeue(eventq);
		e->type        = VKK_EVENT_TYPE_PERMISSION_GRANTED;
		e->ts          = cc_timestamp();
		e->permission  = permission;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeGyroscope(JNIEnv* env,
                                                         jobject obj,
                                                         jfloat ax,
                                                         jfloat ay,
                                                         jfloat az,
                                                         jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e  = vkk_eventq_dequeue(eventq);
		e->type         = VKK_EVENT_TYPE_GYROSCOPE;
		e->ts           = ts;
		e->gyroscope.ax = ax;
		e->gyroscope.ay = ay;
		e->gyroscope.az = az;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeKeyDown(JNIEnv* env,
                                                       jobject obj,
                                                       jint keycode,
                                                       jint meta,
                                                       jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e = vkk_eventq_dequeue(eventq);
		e->type        = VKK_EVENT_TYPE_KEY_DOWN;
		e->ts          = ts;
		e->key.keycode = shiftKeycode(keycode, meta);
		e->key.meta    = meta;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeKeyUp(JNIEnv* env,
                                                     jobject obj,
                                                     jint keycode,
                                                     jint meta,
                                                     jdouble ts)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e = vkk_eventq_dequeue(eventq);
		e->type        = VKK_EVENT_TYPE_KEY_UP;
		e->ts          = ts;
		e->key.keycode = shiftKeycode(keycode, meta);
		e->key.meta    = meta;
		vkk_eventq_enqueue(eventq);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeMagnetometer(JNIEnv* env,
                                                            jobject obj,
                                                            jfloat mx,
                                                            jfloat my,
                                                            jfloat mz,
                                                            jdouble ts,
                                                            jfloat gfx,
                                                            jfloat gfy,
                                                            jfloat gfz)
{
	ASSERT(env);

	if(eventq)
	{
		vkk_event_t* e      = vkk_eventq_dequeue(eventq);
		e->type             = VKK_EVENT_TYPE_MAGNETOMETER;
		e->ts               = ts;
		e->magnetometer.mx  = mx;
		e->magnetometer.my  = my;
		e->magnetometer.mz  = mz;
		e->magnetometer.gfx = gfx;
		e->magnetometer.gfy = gfy;
		e->magnetometer.gfz = gfz;
		vkk_eventq_enqueue(eventq);
	}
}

/***********************************************************
* android app interface                                    *
***********************************************************/

static void
onAppCmd(struct android_app* app, int32_t cmd)
{
	vkk_platform_t* platform;
	platform = (vkk_platform_t*) app->userData;

	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnEvent_fn   onEvent;
	onDestroy = VKK_PLATFORM_CALLBACKS.onDestroy;
	onEvent   = VKK_PLATFORM_CALLBACKS.onEvent;

	if(cmd == APP_CMD_INIT_WINDOW)
	{
		LOGI("APP_CMD_INIT_WINDOW");

		// recreate the window surface
		if(platform->priv)
		{
			vkk_event_t ve =
			{
				.type = VKK_EVENT_TYPE_RECREATE,
				.ts   = cc_timestamp(),
			};
			if((*onEvent)(platform->priv, &ve) == 0)
			{
				// recreate renderer on failure
				(*onDestroy)(&platform->priv);
			}
		}
		platform->has_window = 1;
	}
	else if(cmd == APP_CMD_TERM_WINDOW)
	{
		LOGI("APP_CMD_TERM_WINDOW");

		// destroy the existing window surface
		if(platform->priv)
		{
			vkk_event_t ve =
			{
				.type = VKK_EVENT_TYPE_RECREATE,
				.ts   = cc_timestamp(),
			};
			(*onEvent)(platform->priv, &ve);
		}
		platform->has_window = 0;
	}
	else if(cmd == APP_CMD_RESUME)
	{
		LOGI("APP_CMD_RESUME");
		platform->running = 1;
	}
	else if((cmd == APP_CMD_PAUSE) ||
	        (cmd == APP_CMD_STOP))
	{
		LOGI("APP_CMD_PAUSE");

		if(platform->running && platform->priv)
		{
			vkk_event_t ve =
			{
				.type = VKK_EVENT_TYPE_PAUSE,
				.ts   = cc_timestamp(),
			};
			(*onEvent)(platform->priv, &ve);
		}
		platform->running = 0;
	}
	else if(cmd == APP_CMD_DESTROY)
	{
		LOGI("APP_CMD_DESTROY");
		(*onDestroy)(&platform->priv);
		platform->running = 0;
	}
}

static float denoiseAxis(float value)
{
	if(fabs(value) < 0.05F)
	{
		return 0.0f;
	}
	return value;
}

static float
getAxisValue(AInputEvent* event, size_t idx, int axis)
{
	return denoiseAxis(AMotionEvent_getAxisValue(event,
	                                             axis, idx));
}

static int32_t
onInputEvent(struct android_app* app, AInputEvent* event)
{
	ASSERT(app);
	ASSERT(event);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_CALLBACKS.onEvent;

	vkk_platform_t* platform;
	platform = (vkk_platform_t*) app->userData;
	if(platform == NULL)
	{
		return 0;
	}

	int32_t source = AInputEvent_getSource(event);
	int32_t action = AMotionEvent_getAction(event) &
	                 AMOTION_EVENT_ACTION_MASK;
	int32_t id     = AInputEvent_getDeviceId(event);

	// 1) use JNI to handle key events because the native API
	//    doesn't seem to provide a complete set of keycodes
	//    e.g. '$' doesn't exist
	// 2) use native to handle touch events because the native
	//    window is needed to generate the correct offset
	int32_t atype = AInputEvent_getType(event);
	if((source & AINPUT_SOURCE_CLASS_JOYSTICK) &&
	   (action == AMOTION_EVENT_ACTION_MOVE))
	{
		size_t idx;
		idx = (size_t) (AMotionEvent_getAction(event) &
		                AMOTION_EVENT_ACTION_POINTER_INDEX_MASK);

		// process the joystick movement...
		float ax1 = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_X);
		float ay1 = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_Y);
		float ax2 = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_Z);
		float ay2 = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_RZ);
		float ahx = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_HAT_X);
		float ahy = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_HAT_Y);

		float art = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_RTRIGGER);
		if(art == 0.0f)
		{
			art = getAxisValue(event, idx,
			                   AMOTION_EVENT_AXIS_GAS);
			if(art == 0.0f)
			{
				art = getAxisValue(event, idx,
				                   AMOTION_EVENT_AXIS_THROTTLE);
			}
		}

		float alt = getAxisValue(event, idx,
		                         AMOTION_EVENT_AXIS_LTRIGGER);
		if(alt == 0.0f)
		{
			alt = getAxisValue(event, idx,
			                   AMOTION_EVENT_AXIS_BRAKE);
		}

		vkk_event_t ve =
		{
			.type = VKK_EVENT_TYPE_AXIS_MOVE,
			.ts   = cc_timestamp(),
			.axis =
			{
				.id = id,
			}
		};

		if(ax1 != platform->AX1)
		{
			ve.axis.axis  = VKK_AXIS_X1;
			ve.axis.value = ax1;
			platform->AX1 = ax1;
			(*onEvent)(platform->priv, &ve);
		}
		if(ay1 != platform->AY1)
		{
			ve.axis.axis  = VKK_AXIS_Y1;
			ve.axis.value = ay1;
			platform->AY1 = ay1;
			(*onEvent)(platform->priv, &ve);
		}
		if(ax2 != platform->AX2)
		{
			ve.axis.axis  = VKK_AXIS_X2;
			ve.axis.value = ax2;
			platform->AX2 = ax2;
			(*onEvent)(platform->priv, &ve);
		}
		if(ay2 != platform->AY2)
		{
			ve.axis.axis  = VKK_AXIS_Y2;
			ve.axis.value = ay2;
			platform->AY2 = ay2;
			(*onEvent)(platform->priv, &ve);
		}
		if(ahx != platform->AHX)
		{
			ve.axis.axis  = VKK_AXIS_HX;
			ve.axis.value = ahx;
			platform->AHX = ahx;
			(*onEvent)(platform->priv, &ve);
		}
		if(ahy != platform->AHY)
		{
			ve.axis.axis  = VKK_AXIS_HY;
			ve.axis.value = ahy;
			platform->AHY = ahy;
			(*onEvent)(platform->priv, &ve);
		}
		if(art != platform->ART)
		{
			ve.axis.axis  = VKK_AXIS_RT;
			ve.axis.value = art;
			platform->ART = art;
			(*onEvent)(platform->priv, &ve);
		}
		if(alt != platform->ALT)
		{
			ve.axis.axis  = VKK_AXIS_LT;
			ve.axis.value = alt;
			platform->ALT = alt;
			(*onEvent)(platform->priv, &ve);
		}
		return 1;
	}
	else if(platform && (atype == AINPUT_EVENT_TYPE_MOTION))
	{
		int     action = (int) AMotionEvent_getAction(event) &
		                       AMOTION_EVENT_ACTION_MASK;
		int64_t ns     = AMotionEvent_getEventTime(event);
		double  ts     = ((double) ns)/1000000000.0;
		int     count  = (int) AMotionEvent_getPointerCount(event);

		if(count < 1)
		{
			// ignore
			return 0;
		}

		// get points
		float x0 = 0.0f;
		float y0 = 0.0f;
		float x1 = 0.0f;
		float y1 = 0.0f;
		float x2 = 0.0f;
		float y2 = 0.0f;
		float x3 = 0.0f;
		float y3 = 0.0f;
		if(count >= 1)
		{
			x0 = AMotionEvent_getX(event, 0);
			y0 = AMotionEvent_getY(event, 0);
		}
		if(count >= 2)
		{
			x1 = AMotionEvent_getX(event, 1);
			y1 = AMotionEvent_getY(event, 1);
		}
		if(count >= 3)
		{
			x2 = AMotionEvent_getX(event, 2);
			y2 = AMotionEvent_getY(event, 2);
		}
		if(count >= 4)
		{
			count = 4;
			x3 = AMotionEvent_getX(event, 3);
			y3 = AMotionEvent_getY(event, 3);
		}

		// translate the action
		int type;
		if(action <= AMOTION_EVENT_ACTION_DOWN)
		{
			type = VKK_EVENT_TYPE_ACTION_DOWN;
		}
		else if(action == AMOTION_EVENT_ACTION_UP)
		{
			type = VKK_EVENT_TYPE_ACTION_UP;
		}
		else if(action == AMOTION_EVENT_ACTION_MOVE)
		{
			type = VKK_EVENT_TYPE_ACTION_MOVE;
		}
		else
		{
			// ignore
			return 0;
		}

		vkk_event_t ve =
		{
			.type = type,
			.ts   = ts,
			.action =
			{
				.count = count,
				.coord =
				{
					{ .x = x0, .y = y0 },
					{ .x = x1, .y = y1 },
					{ .x = x2, .y = y2 },
					{ .x = x3, .y = y3 }
				}
			}
		};
		(*onEvent)(platform->priv, &ve);

		return 1;
	}

	return 0;
}

/***********************************************************
* private                                                  *
***********************************************************/

static vkk_platform_t*
vkk_platform_new(struct android_app* app)
{
	ASSERT(app);

	vkk_platform_t* self;
	self = (vkk_platform_t*)
	       CALLOC(1, sizeof(vkk_platform_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// create the global eventq
	eventq = vkk_eventq_new();
	if(eventq == NULL)
	{
		goto fail_eventq;
	}

	self->app         = app;
	app->userData     = self;
	app->onAppCmd     = onAppCmd;
	app->onInputEvent = onInputEvent;

	// success
	return self;

	// failure
	fail_eventq:
		FREE(self);
	return NULL;
}

static void vkk_platform_delete(vkk_platform_t** _self)
{
	ASSERT(_self);

	vkk_platformOnDestroy_fn onDestroy;
	onDestroy = VKK_PLATFORM_CALLBACKS.onDestroy;

	vkk_platform_t* self = *_self;
	if(self)
	{
		// destroy the global eventq
		vkk_eventq_delete(&eventq);

		(*onDestroy)(&self->priv);
		FREE(self);
		*_self = NULL;
	}
}

static int vkk_platform_rendering(vkk_platform_t* self)
{
	ASSERT(self);

	if(self->running && self->has_window)
	{
		return 1;
	}

	return 0;
}

static void vkk_platform_draw(vkk_platform_t* self)
{
	ASSERT(self);

	vkk_platformOnCreate_fn  onCreate;
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnDraw_fn    onDraw;
	vkk_platformOnEvent_fn   onEvent;
	onCreate  = VKK_PLATFORM_CALLBACKS.onCreate;
	onDestroy = VKK_PLATFORM_CALLBACKS.onDestroy;
	onDraw    = VKK_PLATFORM_CALLBACKS.onDraw;
	onEvent   = VKK_PLATFORM_CALLBACKS.onEvent;

	if(vkk_platform_rendering(self) == 0)
	{
		return;
	}

	// create renderer
	ANativeWindow* window = self->app->window;
	if(self->priv == NULL)
	{
		self->priv = (*onCreate)(self);
		if(self->priv == NULL)
		{
			return;
		}

		self->density = 1.0f;
		self->width   = (int) ANativeWindow_getWidth(window);
		self->height  = (int) ANativeWindow_getHeight(window);
	}
	else
	{
		// check if the native window was resized
		int width;
		int height;
		width  = (int) ANativeWindow_getWidth(window);
		height = (int) ANativeWindow_getHeight(window);
		if((self->width  != width) ||
		   (self->height != height))
		{
			self->width  = width;
			self->height = height;

			vkk_event_t ve =
			{
				.type = VKK_EVENT_TYPE_RESIZE,
				.ts   = cc_timestamp()
			};
			if((*onEvent)(self->priv, &ve) == 0)
			{
				// recreate renderer on failure
				(*onDestroy)(&self->priv);
				return;
			}
		}
	}

	(*onDraw)(self->priv);
}

static int
isTimestampValid(struct android_app* app)
{
	ASSERT(app);

	// import ts1
	AAssetManager* am;
	am = app->activity->assetManager;
	AAsset* asset = AAssetManager_open(am, "timestamp.raw",
	                                   AASSET_MODE_BUFFER);
	if(asset == NULL)
	{
		LOGE("AAssetManager_open %s failed", "timestamp.raw");
		return 0;
	}

	size_t sz1 = (size_t) AAsset_getLength(asset);
	if(sz1 == 0)
	{
		LOGE("invalid sz1=%u", (unsigned int) sz1);
		goto fail_ts1_size;
	}

	char* ts1 = (char*) CALLOC(sz1, sizeof(char));
	if(ts1 == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_ts1_alloc;
	}

	if(AAsset_read(asset, ts1, sz1) != sz1)
	{
		LOGE("AAsset_read failed");
		goto fail_ts1_read;
	}

	// check if ts2 exists
	char fname[256];
	snprintf(fname, 256, "%s/timestamp.raw",
	         app->activity->internalDataPath);
	if(access(fname, F_OK) != 0)
	{
		LOGW("invalid %s", fname);
		goto fail_ts2_access;
	}

	// import ts2
	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGW("invalid %s", fname);
		goto fail_ts2_fopen;
	}

	fseek(f, (long) 0, SEEK_END);
	size_t sz2 = (size_t) ftell(f);
	fseek(f, 0, SEEK_SET);

	if(sz1 != sz2)
	{
		LOGW("invalid sz1=%i, sz2=%i", (int) sz1, (int) sz2);
		goto fail_compare_size;
	}

	char* ts2 = (char*) CALLOC(sz2, sizeof(char));
	if(ts2 == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_ts2_alloc;
	}

	if(fread(ts2, sz2, 1, f) != 1)
	{
		LOGE("fread failed");
		goto fail_ts2_read;
	}

	// compare timestamps
	for(int i = 0; i < sz1; ++i)
	{
		if(ts1[i] != ts2[i])
		{
			LOGW("invalid ts1=%s, ts2=%s", ts1, ts2);
			goto fail_compare;
		}
	}

	FREE(ts2);
	fclose(f);
	FREE(ts1);
	AAsset_close(asset);

	// success
	return 1;

	// failure
	fail_compare:
	fail_ts2_read:
		FREE(ts2);
	fail_ts2_alloc:
	fail_compare_size:
		fclose(f);
	fail_ts2_fopen:
	fail_ts2_access:
	fail_ts1_read:
		FREE(ts1);
	fail_ts1_alloc:
	fail_ts1_size:
		AAsset_close(asset);
	return 0;
}

static int
updateResource(struct android_app* app, const char* src,
               const char* dst)
{
	ASSERT(app);
	ASSERT(src);
	ASSERT(dst);

	// remove old resource
	unlink(dst);

	// import resource
	AAssetManager* am;
	am = app->activity->assetManager;
	AAsset* asset = AAssetManager_open(am, src,
	                                   AASSET_MODE_BUFFER);
	if(asset == NULL)
	{
		LOGE("AAssetManager_open %s failed", src);
		return 0;
	}

	size_t size = (size_t) AAsset_getLength(asset);
	if(size == 0)
	{
		LOGE("invalid size=%u", (unsigned int) size);
		goto fail_size;
	}

	char* buf;
	buf = (char*) CALLOC(size, sizeof(char));
	if(buf == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc;
	}

	if(AAsset_read(asset, buf, size) != size)
	{
		LOGE("AAsset_read failed");
		goto fail_read;
	}

	// export resource
	FILE* f = fopen(dst, "w");
	if(f == NULL)
	{
		LOGE("fopen %s failed", dst);
		goto fail_fopen;
	}

	if(fwrite(buf, size, 1, f) != 1)
	{
		LOGE("fwrite %s failed", dst);
		goto fail_fwrite;
	}

	fclose(f);
	FREE(buf);
	AAsset_close(asset);

	// success
	return 1;

	// failure
	fail_fwrite:
		fclose(f);
	fail_fopen:
	fail_read:
		FREE(buf);
	fail_alloc:
	fail_size:
		AAsset_close(asset);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_platform_cmd(vkk_platform_t* self, int cmd,
                      const char* msg)
{
	// msg may be NULL
	ASSERT(self);

	// This doesn't work ... and the JNI workaround still
	// doesn't work correctly.
	// See VKKNativeActvity.DrainCommandQueue() for more
	// details.
	// if(cmd == VKK_PLATFORM_CMD_SOFTKEY_SHOW)
	// {
	// 	ANativeActivity_showSoftInput(self->app->activity,
	// 	                              ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
	// 	return;
	// }
	// else if(cmd == VKK_PLATFORM_CMD_SOFTKEY_HIDE)
	// {
	// 	ANativeActivity_hideSoftInput(self->app->activity, 0);
	// 	return;
	// }

	JavaVM* vm = self->app->activity->vm;
	if(vm == NULL)
	{
		LOGE("vm is NULL");
		return;
	}

	JNIEnv* env = NULL;
	if((*vm)->AttachCurrentThread(vm, &env, NULL) != 0)
	{
		LOGE("AttachCurrentThread failed");
		return;
	}

	jobject clazz = self->app->activity->clazz;
	jclass  cls   = (*env)->GetObjectClass(env, clazz);
	if(cls == NULL)
	{
		LOGE("FindClass failed");
		return;
	}

	jmethodID mid = (*env)->GetStaticMethodID(env, cls,
	                                          "CallbackCmd",
	                                          "(ILjava/lang/String;)V");
	if(mid == NULL)
	{
		LOGE("GetStaticMethodID failed");
		return;
	}

	jstring jmsg = (*env)->NewStringUTF(env, msg ? msg : "");
	if(jmsg == NULL)
	{
		LOGE("NewStringUTF failed");
		return;
	}

	(*env)->CallStaticVoidMethod(env, cls, mid, cmd, jmsg);
	(*env)->DeleteLocalRef(env, jmsg);
}

/***********************************************************
* android_main                                             *
***********************************************************/

void android_main(struct android_app* app)
{
	ASSERT(app);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_CALLBACKS.onEvent;

	LOGI("InitVulkan=%i", InitVulkan());

	if(isTimestampValid(app) == 0)
	{
		char fname[256];
		snprintf(fname, 256, "%s/resource.pak",
		         app->activity->internalDataPath);
		if(updateResource(app, "resource.pak", fname) == 0)
		{
			return;
		}

		snprintf(fname, 256, "%s/timestamp.raw",
		         app->activity->internalDataPath);
		if(updateResource(app, "timestamp.raw", fname) == 0)
		{
			return;
		}
	}

	vkk_platform_t* platform = vkk_platform_new(app);
	if(platform == NULL)
	{
		LOGE("platform failed");
		return;
	}

	while(1)
	{
		// poll for native events
		int   id;
		int   outEvents;
		void* outData;
		int   rendering = vkk_platform_rendering(platform);
		id = ALooper_pollAll(rendering ? 0 : -1,
		                     NULL, &outEvents, &outData);
		while(id > 0)
		{
			if((id == LOOPER_ID_MAIN) ||
			   (id == LOOPER_ID_INPUT))
			{
				struct android_poll_source* source;
				source = (struct android_poll_source*)
				         outData;
				if(source)
				{
					source->process(app, source);
				}
			}
			else if(id == LOOPER_ID_USER)
			{
				// ignore user events
			}

			// check for exit
			if(app->destroyRequested)
			{
				vkk_platform_delete(&platform);
				return;
			}

			rendering = vkk_platform_rendering(platform);
			id = ALooper_pollAll(rendering ? 0 : -1,
		                         NULL, &outEvents, &outData);
		}

		// poll for JNI events
		if(platform->priv)
		{
			vkk_event_t e;
			while(vkk_eventq_poll(eventq, &e))
			{
					(*onEvent)(platform->priv, &e);
			}
		}

		vkk_platform_draw(platform);
	}
}