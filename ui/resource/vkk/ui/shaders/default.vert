// Copyright (c) 2019 Jeff Boody

#version 450

layout(location=0) in vec4 xyuv;

layout(std140, set=0, binding=0) uniform uniformMvp
{
	mat4 mvp;
};

layout(location=0) out vec4 varying_xyuv;

void main()
{
	varying_xyuv = xyuv;
	gl_Position  = mvp*vec4(xyuv.xy, -1.0, 1.0);
}
