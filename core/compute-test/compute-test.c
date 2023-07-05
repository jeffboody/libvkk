/*
 * Copyright (c) 2023 Jeff Boody
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

#include <stdlib.h>

#define LOG_TAG "compute"
#include "libcc/cc_log.h"
#include "libvkk/vkk_platform.h"
#include "compute_test.h"

/***********************************************************
* callbacks                                                *
***********************************************************/

static void* compute_test_onCreate(vkk_engine_t* engine)
{
	ASSERT(engine);

	return (void*) compute_test_new(engine);
}

static void compute_test_onDestroy(void** _priv)
{
	ASSERT(_priv);

	compute_test_delete((compute_test_t**) _priv);
}

static void
compute_test_onMain(void* priv, int argc, char** argv)
{
	ASSERT(priv);

	compute_test_main((compute_test_t*) priv, argc, argv);
}

vkk_platformInfo_t VKK_PLATFORM_INFO =
{
	.app_name    = "Compute Test",
	.app_version =
	{
		.major = 1,
		.minor = 0,
		.patch = 0,
	},
	.app_dir   = "ComputeTest",
	.onCreate  = compute_test_onCreate,
	.onDestroy = compute_test_onDestroy,
	.onMain    = compute_test_onMain,
};
