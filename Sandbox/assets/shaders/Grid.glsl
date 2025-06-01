﻿// 简单的网格纹理着色器（Simple Texture Shader）

#type vertex
#version 430


layout(location = 0) in vec3 a_Position;	// 顶点输入：位置
layout(location = 4) in vec2 a_TexCoord;	// 顶点输入：纹理坐标

// MVP 变换矩阵（模型-视图-投影）
uniform mat4 u_MVP;

// 传递到片元着色器的纹理坐标
out vec2 v_TexCoord;

void main()
{
	// 变换顶点位置到裁剪空间
	vec4 position = u_MVP * vec4(a_Position, 1.0);
	gl_Position = position;

	// 传递纹理坐标
	v_TexCoord = a_TexCoord;
}

#type fragment
#version 430

// 输出颜色
layout(location = 0) out vec4 color;


uniform sampler2D u_Texture;// 网格纹理采样器
uniform float u_Scale;		// 网格缩放因子
uniform float u_Res;		// 网格分辨率（线宽）

// 从顶点着色器传入的纹理坐标
in vec2 v_TexCoord;

/*
// 传统纹理采样（未启用，仅作参考）
// 取消注释可直接采样纹理
void main()
{
	color = texture(u_Texture, v_TexCoord * 8.0);
}
*/

// 生成网格线的函数
// st: 纹理坐标，res: 网格线宽
float grid(vec2 st, float res)
{
	vec2 grid = fract(st);							// 取小数部分，获得单元格内坐标
	return step(res, grid.x) * step(res, grid.y);	// 判断是否在网格线内
}
 
void main()
{
	float scale = u_Scale;         // 网格缩放
	float resolution = u_Res;      // 网格线宽

	// 计算当前像素是否在网格线上
	float x = grid(v_TexCoord * scale, resolution);

	// 网格线为透明灰色，非网格区域为全透明
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
}