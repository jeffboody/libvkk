// Copyright (c) 2019 Jeff Boody

#version 450

layout(std140, set=1, binding=0) uniform uniformColor
{
	vec4 color;
};

layout(location=0) out vec4 fragColor;

void main()
{
	fragColor = color;
}
