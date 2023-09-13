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
#include <android/window.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vulkan/vulkan.h>
#include <jni.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "vkk"
#include "../../libbfs/bfs_util.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_timestamp.h"
#include "../core/vkk_engine.h"
#include "../vkk.h"
#include "../vkk_platform.h"
#include "vkk_platformAndroid.h"

static vkk_platform_t* platform = NULL;

/***********************************************************
* protected Android list initialization                    *
***********************************************************/

extern int  cc_listPool_init(void);
extern void cc_listPool_destroy(void);

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
		int shiftchar = meta & (VKK_PLATFORM_META_SHIFT |
		                        VKK_PLATFORM_META_CAPS);
		int shiftsym  = meta & VKK_PLATFORM_META_SHIFT;
		if(shiftchar && (keycode >= 'a') && (keycode <= 'z'))
		{
			return SHIFTKEYS[keycode];
		}
		else if(shiftsym)
		{
			return SHIFTKEYS[keycode];
		}
	}
	else if(keycode == VKK_PLATFORM_KEYCODE_DELETE)
	{
		return VKK_PLATFORM_KEYCODE_INSERT;
	}
	return keycode;
}

/***********************************************************
* android app interface                                    *
***********************************************************/

static vkk_platformEvent_t*
vkk_platform_dequeue(vkk_platform_t* self);

static void vkk_platform_enqueue(vkk_platform_t* self);

static void
onAppCmd(struct android_app* app, int32_t cmd)
{
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnPause_fn   onPause;
	vkk_platformOnEvent_fn   onEvent;
	onDestroy = VKK_PLATFORM_INFO.onDestroy;
	onPause   = VKK_PLATFORM_INFO.onPause;
	onEvent   = VKK_PLATFORM_INFO.onEvent;

	if(cmd == APP_CMD_INIT_WINDOW)
	{
		LOGD("APP_CMD_INIT_WINDOW");

		ANativeActivity_setWindowFlags(app->activity,
		                               AWINDOW_FLAG_FORCE_NOT_FULLSCREEN |
		                               AWINDOW_FLAG_LAYOUT_IN_SCREEN     |
		                               AWINDOW_FLAG_LAYOUT_INSET_DECOR,
		                               AWINDOW_FLAG_FULLSCREEN);

		// recreate the window surface
		if(platform->engine)
		{
			if(vkk_engine_recreate(platform->engine) == 0)
			{
				if(platform->priv && (platform->paused == 0))
				{
					(*onPause)(platform->priv);
					platform->paused = 1;
				}

				// recreate renderer on failure
				vkk_engine_shutdown(platform->engine);
				pthread_mutex_lock(&platform->priv_mutex);
				(*onDestroy)(&platform->priv);
				pthread_mutex_unlock(&platform->priv_mutex);
				vkk_engine_delete(&platform->engine);
			}
		}
		platform->has_window = 1;
	}
	else if(cmd == APP_CMD_TERM_WINDOW)
	{
		LOGD("APP_CMD_TERM_WINDOW");

		// destroy the existing window surface
		if(platform->priv)
		{
			vkk_engine_recreate(platform->engine);
		}
		platform->has_window = 0;
	}
	else if(cmd == APP_CMD_RESUME)
	{
		LOGD("APP_CMD_RESUME");
		platform->running = 1;
	}
	else if((cmd == APP_CMD_PAUSE) ||
	        (cmd == APP_CMD_STOP))
	{
		LOGD("APP_CMD_PAUSE");

		if(platform->priv && (platform->paused == 0))
		{
			if(platform->engine)
			{
				vkk_engine_deviceWaitIdle(platform->engine);
			}

			(*onPause)(platform->priv);
			platform->paused = 1;
		}
		platform->running = 0;
	}
	else if(cmd == APP_CMD_DESTROY)
	{
		if(platform->priv && (platform->paused == 0))
		{
			(*onPause)(platform->priv);
			platform->paused = 1;
		}

		LOGD("APP_CMD_DESTROY");
		if(platform->engine)
		{
			vkk_engine_shutdown(platform->engine);
		}
		pthread_mutex_lock(&platform->priv_mutex);
		(*onDestroy)(&platform->priv);
		pthread_mutex_unlock(&platform->priv_mutex);
		vkk_engine_delete(&platform->engine);
		platform->running = 0;
	}
	else if(cmd == APP_CMD_CONTENT_RECT_CHANGED)
	{
		LOGD("APP_CMD_CONTENT_RECT_CHANGED: t=%i, l=%i, b=%i, r=%i",
		     app->contentRect.top, app->contentRect.left,
		     app->contentRect.bottom, app->contentRect.right);

		if(platform->priv)
		{
			vkk_platformEvent_t ve =
			{
				.type           = VKK_PLATFORM_EVENTTYPE_CONTENT_RECT,
				.ts             = cc_timestamp(),
				.content_rect.t = app->contentRect.top,
				.content_rect.l = app->contentRect.left,
				.content_rect.b = app->contentRect.bottom,
				.content_rect.r = app->contentRect.right,
			};

			(*onEvent)(platform->priv, &ve);
		}
	}
	else if(cmd == APP_CMD_LOW_MEMORY)
	{
		LOGD("APP_CMD_LOW_MEMORY");

		if(platform->priv)
		{
			vkk_platformEvent_t ve =
			{
				.type = VKK_PLATFORM_EVENTTYPE_LOW_MEMORY,
				.ts   = cc_timestamp(),
			};

			(*onEvent)(platform->priv, &ve);
		}
	}
}

static float
getAxisValue(AInputEvent* event, size_t idx, int axis)
{
	float value = AMotionEvent_getAxisValue(event, axis, idx);
	if(fabs(value) < 0.05F)
	{
		return 0.0f;
	}
	return value;
}

static int32_t
onInputEvent(struct android_app* app, AInputEvent* event)
{
	ASSERT(app);
	ASSERT(event);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_INFO.onEvent;

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

		double ns = (double) AMotionEvent_getEventTime(event);
		double ts = ns/1000000000.0;
		vkk_platformEvent_t ve =
		{
			.type = VKK_PLATFORM_EVENTTYPE_AXIS_MOVE,
			.ts   = ts,
			.axis =
			{
				.id = id,
			}
		};

		if(ax1 != platform->AX1)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_X1;
			ve.axis.value = ax1;
			platform->AX1 = ax1;
			(*onEvent)(platform->priv, &ve);
		}
		if(ay1 != platform->AY1)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_Y1;
			ve.axis.value = ay1;
			platform->AY1 = ay1;
			(*onEvent)(platform->priv, &ve);
		}
		if(ax2 != platform->AX2)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_X2;
			ve.axis.value = ax2;
			platform->AX2 = ax2;
			(*onEvent)(platform->priv, &ve);
		}
		if(ay2 != platform->AY2)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_Y2;
			ve.axis.value = ay2;
			platform->AY2 = ay2;
			(*onEvent)(platform->priv, &ve);
		}
		if(ahx != platform->AHX)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_HX;
			ve.axis.value = ahx;
			platform->AHX = ahx;
			(*onEvent)(platform->priv, &ve);
		}
		if(ahy != platform->AHY)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_HY;
			ve.axis.value = ahy;
			platform->AHY = ahy;
			(*onEvent)(platform->priv, &ve);
		}
		if(art != platform->ART)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_RT;
			ve.axis.value = art;
			platform->ART = art;
			(*onEvent)(platform->priv, &ve);
		}
		if(alt != platform->ALT)
		{
			ve.axis.axis  = VKK_PLATFORM_AXIS_LT;
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
		vkk_platformEventType_e type;
		if(action <= AMOTION_EVENT_ACTION_DOWN)
		{
			type = VKK_PLATFORM_EVENTTYPE_ACTION_DOWN;
		}
		else if(action == AMOTION_EVENT_ACTION_UP)
		{
			type = VKK_PLATFORM_EVENTTYPE_ACTION_UP;
		}
		else if(action == AMOTION_EVENT_ACTION_MOVE)
		{
			type = VKK_PLATFORM_EVENTTYPE_ACTION_MOVE;
		}
		else
		{
			// ignore
			return 0;
		}

		vkk_platformEvent_t ve =
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

	// PTHREAD_MUTEX_DEFAULT is not re-entrant
	if(pthread_mutex_init(&self->priv_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_priv_mutex;
	}

	// PTHREAD_MUTEX_DEFAULT is not re-entrant
	if(pthread_mutex_init(&self->event_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_event_mutex;
	}

	if(pthread_cond_init(&self->event_cond, NULL) != 0)
	{
		LOGE("pthread_cond_init failed");
		goto fail_cond_init;
	}

	self->app         = app;
	self->paused      = 1;
	self->document_fd = -1;
	app->userData     = self;
	app->onAppCmd     = onAppCmd;
	app->onInputEvent = onInputEvent;

	// success
	return self;

	// failure
	fail_cond_init:
		pthread_mutex_destroy(&self->event_mutex);
	fail_event_mutex:
		pthread_mutex_destroy(&self->priv_mutex);
	fail_priv_mutex:
		FREE(self);
	return NULL;
}

static void vkk_platform_delete(vkk_platform_t** _self)
{
	ASSERT(_self);

	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnPause_fn   onPause;
	onDestroy = VKK_PLATFORM_INFO.onDestroy;
	onPause   = VKK_PLATFORM_INFO.onPause;

	vkk_platform_t* self = *_self;
	if(self)
	{
		pthread_cond_destroy(&self->event_cond);
		pthread_mutex_destroy(&self->event_mutex);
		pthread_mutex_destroy(&self->priv_mutex);

		if(self->priv && (self->paused == 0))
		{
			(*onPause)(self->priv);
			self->paused = 1;
		}

		if(self->engine)
		{
			vkk_engine_shutdown(self->engine);
		}
		(*onDestroy)(&self->priv);
		vkk_engine_delete(&self->engine);
		FREE(self);
		*_self = NULL;
	}
}

static int
vkk_platform_poll(vkk_platform_t* self, vkk_platformEvent_t* e)
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

static vkk_platformEvent_t*
vkk_platform_dequeue(vkk_platform_t* self)
{
	ASSERT(self);

	vkk_platformEvent_t* event;
	event = &self->event_buffer[self->event_tail];
	memset((void*) event, 0, sizeof(vkk_platformEvent_t));
	return event;
}

static void vkk_platform_enqueue(vkk_platform_t* self)
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

	vkk_platformOnCreate_fn onCreate;
	vkk_platformOnDraw_fn   onDraw;
	onCreate = VKK_PLATFORM_INFO.onCreate;
	onDraw   = VKK_PLATFORM_INFO.onDraw;

	if(vkk_platform_rendering(self) == 0)
	{
		return;
	}

	// create engine
	if(self->engine == NULL)
	{
		const char* internal_path;
		const char* external_path;
		internal_path = self->app->activity->internalDataPath;
		external_path = self->app->activity->externalDataPath;
		self->engine = vkk_engine_new(self,
		                              VKK_PLATFORM_INFO.app_name,
		                              VKK_PLATFORM_INFO.app_dir,
		                              &VKK_PLATFORM_INFO.app_version,
		                              internal_path,
		                              external_path);
		if(self->engine == NULL)
		{
			return;
		}
	}

	// create renderer
	if(self->priv == NULL)
	{
		pthread_mutex_lock(&platform->priv_mutex);
		self->priv = (*onCreate)(self->engine);
		pthread_mutex_unlock(&platform->priv_mutex);
		if(self->priv == NULL)
		{
			return;
		}

		self->density = 1.0f;
	}

	(*onDraw)(self->priv);
	self->paused = 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_platform_cmd(vkk_platform_t* self,
                      vkk_platformCmdInfo_t* info)
{
	ASSERT(self);
	ASSERT(info);

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

	// save document callback
	if((info->cmd == VKK_PLATFORM_CMD_DOCUMENT_CREATE) ||
	   (info->cmd == VKK_PLATFORM_CMD_DOCUMENT_OPEN))
	{
		pthread_mutex_lock(&self->priv_mutex);
		int fd = self->document_fd;
		if(fd >= 0)
		{
			LOGE("invalid fd=%i", fd);
			pthread_mutex_unlock(&self->priv_mutex);
			return;
		}

		self->document_priv = info->priv;
		self->document_fn   = info->document_fn;
		pthread_mutex_unlock(&self->priv_mutex);
	}
	else if(info->cmd == VKK_PLATFORM_CMD_DOCUMENT_NAME)
	{
		LOGW("unsupported cmd=%i", info->cmd);
		ASSERT(0);
		return;
	}

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

	jstring jmsg;
	jmsg = (*env)->NewStringUTF(env, info->msg);
	if(jmsg == NULL)
	{
		LOGE("NewStringUTF failed");
		return;
	}

	(*env)->CallStaticVoidMethod(env, cls, mid,
	                             (int) info->cmd, jmsg);
	(*env)->DeleteLocalRef(env, jmsg);
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

	if(platform)
	{
		vkk_platformEvent_t* e    = vkk_platform_dequeue(platform);
		e->type                   = VKK_PLATFORM_EVENTTYPE_ACCELEROMETER;
		e->ts                     = ts;
		e->accelerometer.ax       = ax;
		e->accelerometer.ay       = ay;
		e->accelerometer.az       = az;
		e->accelerometer.rotation = rotation;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_BUTTON_DOWN;
		e->ts                  = ts;
		e->button.id           = id;
		e->button.button       = button;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_BUTTON_UP;
		e->ts                  = ts;
		e->button.id           = id;
		e->button.button       = button;
		vkk_platform_enqueue(platform);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeDensity(JNIEnv* env,
                                                       jobject obj,
                                                       jfloat density)
{
	ASSERT(env);

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_DENSITY;
		e->ts                  = 0.0;
		e->density             = density;
		vkk_platform_enqueue(platform);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKGpsService_NativeGps(JNIEnv* env, jobject obj,
                                               jdouble lat, jdouble lon,
                                               jfloat accuracy, jfloat altitude,
                                               jfloat speed, jfloat bearing, jdouble ts)
{
	ASSERT(env);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_INFO.onEvent;

	if(platform)
	{
		// trigger GPS events on the UI thread since the main
		// thread may be paused while the app is recording GPS
		// events
		pthread_mutex_lock(&platform->priv_mutex);
		if(platform->priv)
		{
			vkk_platformEvent_t e =
			{
				.type = VKK_PLATFORM_EVENTTYPE_GPS,
				.ts   = ts,
				.gps  =
				{
					.lat      = lat,
					.lon      = lon,
					.accuracy = accuracy,
					.altitude = altitude,
					.speed    = speed,
					.bearing  = bearing,
				}
			};
			(*onEvent)(platform->priv, &e);
		}
		pthread_mutex_unlock(&platform->priv_mutex);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativePermissionStatus(JNIEnv* env,
                                                               jobject obj,
                                                               jint permission,
                                                               jint status)
{
	ASSERT(env);

	if(platform)
	{
		vkk_platformEvent_t* e   = vkk_platform_dequeue(platform);
		e->type                  = VKK_PLATFORM_EVENTTYPE_PERMISSION_STATUS;
		e->ts                    = cc_timestamp();
		e->permission.permission = permission;
		e->permission.status     = status;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_GYROSCOPE;
		e->ts                  = ts;
		e->gyroscope.ax        = ax;
		e->gyroscope.ay        = ay;
		e->gyroscope.az = az;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_KEY_DOWN;
		e->ts                  = ts;
		e->key.keycode         = shiftKeycode(keycode, meta);
		e->key.meta            = meta;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_KEY_UP;
		e->ts                  = ts;
		e->key.keycode         = shiftKeycode(keycode, meta);
		e->key.meta            = meta;
		vkk_platform_enqueue(platform);
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

	if(platform)
	{
		vkk_platformEvent_t* e = vkk_platform_dequeue(platform);
		e->type                = VKK_PLATFORM_EVENTTYPE_MAGNETOMETER;
		e->ts                  = ts;
		e->magnetometer.mx     = mx;
		e->magnetometer.my     = my;
		e->magnetometer.mz     = mz;
		e->magnetometer.gfx    = gfx;
		e->magnetometer.gfy    = gfy;
		e->magnetometer.gfz    = gfz;
		vkk_platform_enqueue(platform);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeDocument(JNIEnv* env,
                                                        jobject obj,
                                                        jstring uri,
                                                        jint fd)
{
	ASSERT(env);
	ASSERT(uri);

	if(platform)
	{
		pthread_mutex_lock(&platform->priv_mutex);
		platform->document_fd = fd;

		const char* curi = (*env)->GetStringUTFChars(env, uri, NULL);
		snprintf(platform->document_uri, 256, "%s", curi);
		(*env)->ReleaseStringUTFChars(env, uri, curi);
		pthread_mutex_unlock(&platform->priv_mutex);
	}
}

JNIEXPORT void JNICALL
Java_com_jeffboody_vkk_VKKNativeActivity_NativeMemoryInfo(JNIEnv* env,
                                                          jobject obj,
                                                          jdouble available,
                                                          jdouble threshold,
                                                          jdouble total,
                                                          jint    low)
{
	ASSERT(env);

	if(platform)
	{
		vkk_platformEvent_t* e   = vkk_platform_dequeue(platform);
		e->type                  = VKK_PLATFORM_EVENTTYPE_MEMORY_INFO;
		e->ts                    = cc_timestamp();
		e->memory_info.available = (size_t) available;
		e->memory_info.threshold = (size_t) threshold;
		e->memory_info.total     = (size_t) total;
		e->memory_info.low       = low;
		vkk_platform_enqueue(platform);
	}
}

/***********************************************************
* utility functions                                        *
***********************************************************/

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

static void check_memory(void)
{
	size_t count = MEMCOUNT();
	size_t size  = MEMSIZE();
	if(count || size)
	{
		LOGW("memory leak detected");
		MEMINFO();
	}
}

static void
onContentRectChanged(ANativeActivity* activity,
                     const ARect* rect)
{
	struct android_app* android_app;
	android_app = (struct android_app*) activity->instance;
	pthread_mutex_lock(&android_app->mutex);
	android_app->pendingContentRect = *rect;
	pthread_mutex_unlock(&android_app->mutex);
}

/***********************************************************
* android_main                                             *
***********************************************************/

void android_main(struct android_app* app)
{
	ASSERT(app);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_INFO.onEvent;

	if(cc_listPool_init() == 0)
	{
		return;
	}

	if(bfs_util_initialize() == 0)
	{
		goto fail_bfs;
	}

	// workaround for android_native_app_glue which does not
	// implement APP_CMD_CONTENT_RECT_CHANGED
	app->activity->callbacks->onContentRectChanged = onContentRectChanged;

	// remove resource.pak
	char fname[256];
	snprintf(fname, 256, "%s/resource.pak",
	         app->activity->internalDataPath);
	unlink(fname);

	// update resource.bfs
	snprintf(fname, 256, "%s/resource.bfs",
	         app->activity->internalDataPath);
	if(updateResource(app, "resource.bfs", fname) == 0)
	{
		goto fail_update_resource;
	}

	platform = vkk_platform_new(app);
	if(platform == NULL)
	{
		goto fail_platform;
	}

	while(1)
	{
		// process document event
		pthread_mutex_lock(&platform->priv_mutex);
		if(platform->document_fd >= 0)
		{
			// cache the document state
			int                        document_fd;
			char                       document_uri[256];
			void*                      document_priv;
			vkk_platformCmd_documentFn document_fn;
			document_fd   = platform->document_fd;
			document_priv = platform->document_priv;
			document_fn   = platform->document_fn;
			snprintf(document_uri, 256, "%s",
			         platform->document_uri);

			// reset document event
			platform->document_fd   = -1;
			platform->document_priv = NULL;
			platform->document_fn   = NULL;
			snprintf(platform->document_uri, 256, "%s", "");
			pthread_mutex_unlock(&platform->priv_mutex);

			(document_fn)(document_priv,
			              document_uri,
			              &document_fd);

			// optionally close document_fd
			if(document_fd >= 0)
			{
				close(document_fd);
			}
		}
		else
		{
			pthread_mutex_unlock(&platform->priv_mutex);
		}

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
				bfs_util_shutdown();
				check_memory();
				cc_listPool_destroy();
				return;
			}

			rendering = vkk_platform_rendering(platform);
			id = ALooper_pollAll(rendering ? 0 : -1,
		                         NULL, &outEvents, &outData);
		}

		// workaround for android_native_app_glue which does not
		// implement APP_CMD_CONTENT_RECT_CHANGED
		pthread_mutex_lock(&app->mutex);
		if((app->contentRect.top    != app->pendingContentRect.top)    ||
		   (app->contentRect.left   != app->pendingContentRect.left)   ||
		   (app->contentRect.bottom != app->pendingContentRect.bottom) ||
		   (app->contentRect.right  != app->pendingContentRect.right))
		{
			app->contentRect = app->pendingContentRect;
			onAppCmd(app, APP_CMD_CONTENT_RECT_CHANGED);
		}
		pthread_mutex_unlock(&app->mutex);

		// poll for JNI events
		if(platform->priv)
		{
			vkk_platformEvent_t e;
			while(vkk_platform_poll(platform, &e))
			{
				int pressed = (*onEvent)(platform->priv, &e);
				if((e.type == VKK_PLATFORM_EVENTTYPE_KEY_UP) &&
				   (e.key.keycode == VKK_PLATFORM_KEYCODE_ESCAPE) &&
				   (pressed == 0))
				{
					// double tap back to exit
					if((e.ts - platform->escape_t0) < 0.5)
					{
						vkk_platformCmdInfo_t info =
						{
							.cmd = VKK_PLATFORM_CMD_EXIT,
						};
						vkk_platform_cmd(platform, &info);
					}
					else
					{
						platform->escape_t0 = e.ts;
					}
				}
			}
		}

		vkk_platform_draw(platform);
	}

	// dead code
	LOGE("dead code");
	return;

	// failure
	fail_platform:
	fail_update_resource:
		bfs_util_shutdown();
	fail_bfs:
		check_memory();
		cc_listPool_destroy();
}
