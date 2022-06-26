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

#version 450

layout(location=0) in vec4 varying_xyuv;

layout(std140, set=3, binding=0) uniform uniformColor0
{
	vec4 color0;
};

layout(std140, set=3, binding=1) uniform uniformColor1
{
	vec4 color1;
};

layout(std140, set=3, binding=2) uniform uniformColor2
{
	vec4 color2;
};

layout(std140, set=3, binding=3) uniform uniformAb
{
	vec2 ab;
};

layout(location=0) out vec4 fragColor;

void main()
{
	if(varying_xyuv.y < ab.x)
	{
		fragColor = color0;
	}
	else if(varying_xyuv.y > ab.y)
	{
		fragColor = color2;
	}
	else
	{
		fragColor = color1;
	}
}
