// Copyright (c) 2019 Jeff Boody

#version 450

layout(location=0) in vec4 varying_xyuv;

layout(std140, set=1, binding=0) uniform uniformColor
{
	vec4 color;
};

layout(std140, set=2, binding=0) uniform uniformMultiply
{
	bool multiply;
};

layout(set=3, binding=0) uniform sampler2D image;

layout(location=0) out vec4 fragColor;

void main()
{
	vec4 texel = texture(image, varying_xyuv.zw);
	if(multiply)
	{
		fragColor  = color*texel;
	}
	else
	{
		fragColor  = vec4(color.rgb, color.a*texel.r);
	}
}
