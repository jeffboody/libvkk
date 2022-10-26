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

#ifndef vkk_platformAndroid_H
#define vkk_platformAndroid_H

#include <android_native_app_glue.h>

#include "../platform/vkk_platformCmdInfo.h"
#include "../vkk.h"
#include "../vkk_platform.h"

#define VKK_EVENTQ_BUFSIZE 256

typedef struct vkk_platform_s
{
	struct android_app* app;

	int    running;
	int    paused;
	int    has_window;
	float  density;
	double escape_t0;

	// priv is typically only accessed by main thread
	// however GPS events are an exception because they
	// may be generated when the app/main thread is paused
	// priv_mutex protects priv from being destroyed while
	// a GPS event is being processed on the UI thread
	// priv_mutex also protects the document state
	pthread_mutex_t priv_mutex;

	// axis values
	float AX1;
	float AY1;
	float AX2;
	float AY2;
	float AHX;
	float AHY;
	float ART;
	float ALT;

	// event state
	pthread_mutex_t     event_mutex;
	pthread_cond_t      event_cond;
	int                 event_head;
	int                 event_tail;
	vkk_platformEvent_t event_buffer[VKK_EVENTQ_BUFSIZE];
	vkk_platformEvent_t event_document;

	// document state
	int                        document_fd;
	char                       document_uri[256];
	void*                      document_priv;
	vkk_platformCmd_documentFn document_fn;

	vkk_engine_t* engine;
	void*         priv;
} vkk_platform_t;

void vkk_platform_cmd(vkk_platform_t* self,
                      vkk_platformCmdInfo_t* info);

#endif
