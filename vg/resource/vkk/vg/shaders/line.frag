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

layout(location=0) in vec2 varying_st;

layout(std140, set=2, binding=0) uniform uniformColor
{
	vec4 color;
};

layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
{
	vec4 brush12WidthCap;
};

layout(std140, set=3, binding=0) uniform uniformDist
{
	float dist;
};

layout(location=0) out vec4 fragColor;

void main()
{
	float width  = brush12WidthCap.z;
	float brush1 = width*brush12WidthCap.x;
	float brush2 = width*brush12WidthCap.y;
	float cap    = brush12WidthCap.w;
	float t      = abs(varying_st.y);
	float ds     = brush1 + brush2;
	float dist1  = dist;
	float dist2  = ds*round(dist1/ds);
	float s1     = varying_st.x;
	float s2     = s1*(dist2/dist1);
	float phase  = mod(s2, ds);

	// ignore empty space between dashes/dots
	if(phase > brush1)
	{
		discard;
		return;
	}

	// compute rounded end caps
	float xx;
	float yy = t;
	float w2 = width/2.0;
	if(cap > 0.5f)
	{
		if(s2 < w2)
		{
			xx = (w2 - s2)/w2;
			t  = clamp(sqrt(xx*xx + yy*yy), 0.0, 1.0);
		}
		else if((s2 + w2) > dist2)
		{
			xx = (s2 - dist2 + w2)/w2;
			t  = clamp(sqrt(xx*xx + yy*yy), 0.0, 1.0);
		}
		else if(brush2 == 0.0)
		{
			// ignore
		}
		else if(phase < w2)
		{
			xx = (w2 - phase)/w2;
			t  = clamp(sqrt(xx*xx + yy*yy), 0.0, 1.0);
		}
		else if((phase + w2) > brush1)
		{
			xx = (phase + w2 - brush1)/w2;
			t  = clamp(sqrt(xx*xx + yy*yy), 0.0, 1.0);
		}
	}

	// anti-alias edge
	//
	// cross section:
	// t: interpolation from 0 to 1 (center to edge)
	// c: line
	// e: edge
	//
	// |       width in px       |
	// |                         |
	// T<-----------t----------->T
	// |  CcccccccccccccccccccC  |
	// |eeE                   Eee|
	// |                         |

	float E     = 0.25;
	float C     = w2 - E;
	float T     = w2*t;
	if(T > C)
	{
		float tc = 1.0 - (T - C)/E;
		fragColor = vec4(color.rgb, tc*color.a);
	}
	else
	{
		fragColor = color;
	}
}
