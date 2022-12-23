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

#ifndef vkk_platformLinux_H
#define vkk_platformLinux_H

#include "../platform/vkk_platformCmdInfo.h"
#include "../vkk.h"
#include "../vkk_platform.h"

typedef struct vkk_platform_s
{
	int    running;
	int    paused;
	float  width;
	float  height;
	double escape_t0;

	// joystick state
	int           joy_id;
	SDL_Joystick* joy;

	// document state
	int                        document_ready;
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
