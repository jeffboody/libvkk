/*
 * Copyright (c) 2022 Jeff Boody
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

#version 450

layout(location=0) in vec4 xyst;
layout(location=1) in vec2 dxdy;

layout(std140, set=0, binding=0) uniform uniformPm
{
	mat4 pm;
};

layout(std140, set=1, binding=0) uniform uniformMvm
{
	mat4 mvm;
};

layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
{
	vec4 brush12WidthCap;
};

layout(location=0) out vec2 varying_st;

void main()
{
	mat4  mvp   = pm*mvm;
	float width = brush12WidthCap.z;
	varying_st  = xyst.zw;
	gl_Position = mvp*vec4(xyst.xy + width*dxdy, -1.0, 1.0);
}
