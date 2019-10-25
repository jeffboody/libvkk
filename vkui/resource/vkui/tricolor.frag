// Copyright (c) 2019 Jeff Boody

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
