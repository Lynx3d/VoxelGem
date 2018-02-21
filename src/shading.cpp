/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "shading.h"

#include <cstring>

#define MAP_RES 64
#define UV_LO (0.5f / MAP_RES)
#define UV_HI (1.f - 0.5f / MAP_RES)

// generate vetex attributes required for normal mapping;
// UBO layout aligns to vec4, so there are some padding values
// and since normal/tangent are per face, it's a bit redundant.
const float VERTEX_ATTRIBS[3*24][4] = {
	// normal		tangent			uv
	// Face 0
	{ -1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_LO, UV_LO, 0, 0 },
	{ -1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_HI, UV_LO, 0, 0 },
	{ -1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_HI, UV_HI, 0, 0 },
	{ -1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_LO, UV_HI, 0, 0 },
	// Face 1
	{  1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_LO, UV_LO, 0, 0 },
	{  1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_HI, UV_LO, 0, 0 },
	{  1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_HI, UV_HI, 0, 0 },
	{  1, 0, 0, 0 }, { 0, 0, 1, 0 }, { UV_LO, UV_HI, 0, 0 },
	// Face 2
	{ 0, -1, 0, 0 }, { 1, 0, 0, 0 }, { UV_LO, UV_LO, 0, 0 },
	{ 0, -1, 0, 0 }, { 1, 0, 0, 0 }, { UV_HI, UV_LO, 0, 0 },
	{ 0, -1, 0, 0 }, { 1, 0, 0, 0 }, { UV_HI, UV_HI, 0, 0 },
	{ 0, -1, 0, 0 }, { 1, 0, 0, 0 }, { UV_LO, UV_HI, 0, 0 },
	// Face 3
	{ 0,  1, 0, 0 }, { 1, 0, 0, 0 }, { UV_LO, UV_LO, 0, 0 },
	{ 0,  1, 0, 0 }, { 1, 0, 0, 0 }, { UV_HI, UV_LO, 0, 0 },
	{ 0,  1, 0, 0 }, { 1, 0, 0, 0 }, { UV_HI, UV_HI, 0, 0 },
	{ 0,  1, 0, 0 }, { 1, 0, 0, 0 }, { UV_LO, UV_HI, 0, 0 },
	// Face 4
	{ 0, 0, -1, 0 }, { 0, 1, 0, 0 }, { UV_LO, UV_LO, 0, 0 },
	{ 0, 0, -1, 0 }, { 0, 1, 0, 0 }, { UV_HI, UV_LO, 0, 0 },
	{ 0, 0, -1, 0 }, { 0, 1, 0, 0 }, { UV_HI, UV_HI, 0, 0 },
	{ 0, 0, -1, 0 }, { 0, 1, 0, 0 }, { UV_LO, UV_HI, 0, 0 },
	// Face 5
	{ 0, 0,  1, 0 }, { 0, 1, 0, 0 }, { UV_LO, UV_LO, 0, 0 },
	{ 0, 0,  1, 0 }, { 0, 1, 0, 0 }, { UV_HI, UV_LO, 0, 0 },
	{ 0, 0,  1, 0 }, { 0, 1, 0, 0 }, { UV_HI, UV_HI, 0, 0 },
	{ 0, 0,  1, 0 }, { 0, 1, 0, 0 }, { UV_LO, UV_HI, 0, 0 },
};

/* Generate a uniform buffer object that contains an array of 256 vec4 with
   the vertex attributes above, plus a sRGB lookup table in vec.a */
GLuint genVertexUBO(QOpenGLFunctions_3_3_Core &glf)
{
	float buff[256][4] {};
	std::memcpy(buff, VERTEX_ATTRIBS, sizeof(VERTEX_ATTRIBS));
	// init sRGB transfer function LUT (not exactly a gamma curve, but close)
	for (int i=0; i < 11; ++i) buff[i][3] = float(i)/(255.f * 12.92f);
	for (int i=11; i < 256; ++i) buff[i][3] = std::pow((float(i) + 0.055f)/(255.f * 1.055f), 2.4);

	GLuint ubo;
	glf.glGenBuffers(1, &ubo);
	glf.glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glf.glBufferData(GL_UNIFORM_BUFFER, sizeof(buff), buff, GL_STATIC_DRAW);
	return ubo;
}
// handles only one quadrant, mirror accordingly for the other 3

QVector3D calculateNormal(float x, float y, bool roundX, bool roundY, float edgeRadius, float cornerRadius)
{
	float nx = 0.f, ny = 0.f, nz = 1.f;
	if (x < edgeRadius && roundY && (!roundX || y > cornerRadius))
	{
		// y edge rounding
		nx = x - edgeRadius;
		nz = edgeRadius;
	}
	else if (y < edgeRadius && roundX && (!roundY || x > cornerRadius))
	{
		// x edge rounding
		ny = y - edgeRadius;
		nz = edgeRadius;
	}
	else if (x <= cornerRadius && y <= cornerRadius && roundX && roundY)
	{
		// corner rounding
		float innerRadius = cornerRadius - edgeRadius;
		float dx = cornerRadius - x;
		float dy = cornerRadius - y;
		float xyRadius = sqrt(dx * dx + dy * dy);
		if (xyRadius < innerRadius)
			return QVector3D(0.f, 0.f, 1.f);
		float scale = innerRadius/xyRadius;
		float cx = cornerRadius - scale * dx;
		float cy = cornerRadius - scale * dy;
		nx = x - cx;
		ny = y - cy;
		nz = (x > y) ? cy : cx;
	}
	// little tweak: normalize to z=1.0 and square x and y, this makes the the edge
	// a little sharper and smoothes out the transition from flat to beveled
	nx /= nz;
	ny /= nz;
	return QVector3D(-(nx*nx), -(ny*ny), 1.f).normalized();
}

// like normal, but no squaring and condense to single value (and we'll use larger radi)
float calculateGlow(float x, float y, bool roundX, bool roundY, float edgeRadius, float cornerRadius)
{
	if (x < edgeRadius && roundY && (!roundX || y > cornerRadius))
	{
		// y edge rounding
		return QVector3D(x - edgeRadius, 0, edgeRadius).normalized().z();
	}
	else if (y < edgeRadius && roundX && (!roundY || x > cornerRadius))
	{
		// x edge rounding
		return QVector3D(0, y - edgeRadius, edgeRadius).normalized().z();
	}
	else if (x <= cornerRadius && y <= cornerRadius && roundX && roundY)
	{
		// corner rounding
		float innerRadius = cornerRadius - edgeRadius;
		float dx = cornerRadius - x;
		float dy = cornerRadius - y;
		float xyRadius = sqrt(dx * dx + dy * dy);
		if (xyRadius < innerRadius)
			return 1.f;
		float scale = innerRadius/xyRadius;
		float cx = cornerRadius - scale * dx;
		float cy = cornerRadius - scale * dy;
		float nx = x - cx;
		float ny = y - cy;
		float nz = (x > y) ? cy : cx;
		return QVector3D(nx, ny, nz).normalized().z();
	}
	return 1.f;
}

// TODO: RGB10_A2 would give more precise normals than RGB8
void genNormalMap(int size, int edgeMask, uint8_t *data)
{
	int halfSize = size/2;
	float er = 5.f/size;
	float cr = 8.f/size;
	float er_g = 8.f/size;
	float cr_g = 16.f/size;

	for (int y = 0; y < halfSize; ++y)
		for (int x = 0; x < halfSize; ++x)
	{
		bool bevelX, bevelY;
		float xf = (float)x/(float)size;
		float yf = (float)y/(float)size;
		// lower left Quadrant
		bevelX = edgeMask & 0x1;
		bevelY = edgeMask & 0x8;
		QVector3D n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		uint8_t *pixel = data + 4 * (x + y * size);
		pixel[0] = 127 + n.x() * 127;
		pixel[1] = 127 + n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		pixel[3] = 254 * calculateGlow(xf, yf, bevelX, bevelY, er_g, cr_g);
		// lower right Quadrant
		bevelX = edgeMask & 0x1;
		bevelY = edgeMask & 0x2;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 4 * (size - x - 1 + y * size);
		pixel[0] = 127 - n.x() * 127;
		pixel[1] = 127 + n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		pixel[3] = 254 * calculateGlow(xf, yf, bevelX, bevelY, er_g, cr_g);
		// upper right Quadrant
		bevelX = edgeMask & 0x4;
		bevelY = edgeMask & 0x2;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 4 * (size - x - 1 + (size - y - 1) * size);
		pixel[0] = 127 - n.x() * 127;
		pixel[1] = 127 - n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		pixel[3] = 254 * calculateGlow(xf, yf, bevelX, bevelY, er_g, cr_g);
		// upper left Quadrant
		bevelX = edgeMask & 0x4;
		bevelY = edgeMask & 0x8;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 4 * (x + (size - y - 1) * size);
		pixel[0] = 127 + n.x() * 127;
		pixel[1] = 127 - n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		pixel[3] = 254 * calculateGlow(xf, yf, bevelX, bevelY, er_g, cr_g);
	}
}

GLuint genNormalTex(QOpenGLFunctions_3_3_Core &glf)
{
	GLuint normal_tex;
	uint8_t *data = new uint8_t[4 * MAP_RES * MAP_RES];
	glf.glGenTextures(1, &normal_tex);
	glf.glActiveTexture(GL_TEXTURE0);
	glf.glBindTexture(GL_TEXTURE_2D, normal_tex);
	glf.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, MAP_RES, MAP_RES, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glf.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glf.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glf.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glf.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 16; ++i)
	{
		genNormalMap(MAP_RES, ~i, data);
		glf.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, MAP_RES, MAP_RES, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	delete[] data;
	return normal_tex;
}
