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

#ifndef xsq_test_H
#define xsq_test_H

#include "libvkk/vkk.h"

typedef struct xsq_test_s
{
	vkk_engine_t*            engine;
	vkk_uniformSetFactory_t* usf0;
	vkk_buffer_t*            sb00_x;
	vkk_buffer_t*            sb01_xx;
	vkk_buffer_t*            sb02_count;
	vkk_uniformSet_t*        us0;
	vkk_pipelineLayout_t*    pl;
	vkk_compute_t*           compute;
	vkk_computePipeline_t*   cp;
} xsq_test_t;

xsq_test_t* xsq_test_new(vkk_engine_t* engine);
void        xsq_test_delete(xsq_test_t** _self);
int         xsq_test_main(xsq_test_t* self,
                          int argc, char** argv);

#endif
