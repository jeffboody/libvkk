// Copyright (c) 2019 Jeff Boody

#version 450

layout(location=0) in vec4 varying_xyuv;

layout(std140, set=4, binding=0) uniform uniformHeader
{
	vec4 header;
};

layout(std140, set=4, binding=1) uniform uniformBody
{
	vec4 body;
};

layout(std140, set=4, binding=2) uniform uniformFooter
{
	vec4 footer;
};

layout(std140, set=4, binding=3) uniform uniformAb
{
	vec2 ab;
};

layout(location=0) out vec4 fragColor;

void main()
{
	if(varying_xyuv.y < ab.x)
	{
		fragColor = header;
	}
	if(varying_xyuv.y > ab.y)
	{
		fragColor = footer;
	}
	else
	{
		fragColor = body;
	}
}
