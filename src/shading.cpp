/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelgem.h"

#include <QOpenGLFunctions_3_3_Core>

GLuint g_normal_tex;

#define MAP_RES 64
#define UV_LO (0.5f / MAP_RES)
#define UV_HI (1.f - 0.5f / MAP_RES)

// generate vetex attributes required for normal mapping;
// UBO layout aligns to vec4, so there are some padding values
// and since normal/tangent are per face, it's a bit redundant.
extern const float VERTEX_ATTRIBS[3*24][4] = {
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

// handles only one quadrant, mirror accordingly for the other 3

QVector3D calculateNormal(float x, float y, bool roundX, bool roundY, float edgeRadius, float cornerRadius)
{
	if (x < edgeRadius && roundY && (!roundX || y > cornerRadius))
	{
		// y edge rounding
		return QVector3D(x - edgeRadius, 0, edgeRadius).normalized();
	}
	else if (y < edgeRadius && roundX && (!roundY || x > cornerRadius))
	{
		// x edge rounding
		return QVector3D(0, y - edgeRadius, edgeRadius).normalized();
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
		float nx = x - cx;
		float ny = y - cy;
		float nz = (x > y) ? cy : cx;
		return QVector3D(nx, ny, nz).normalized();
	}
	return QVector3D(0.f, 0.f, 1.f);
}

// TODO: RGB10_A2 would give more precise normals than RGB8
void genNormalMap(int size, int edgeMask, uint8_t *data)
{
	int halfSize = size/2;
	float er = 5.f/size;
	float cr = 8.f/size;

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
		uint8_t *pixel = data + 3 * (x + y * size);
		pixel[0] = 127 + n.x() * 127;
		pixel[1] = 127 + n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		// lower right Quadrant
		bevelX = edgeMask & 0x1;
		bevelY = edgeMask & 0x2;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 3 * (size - x - 1 + y * size);
		pixel[0] = 127 - n.x() * 127;
		pixel[1] = 127 + n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		// upper right Quadrant
		bevelX = edgeMask & 0x4;
		bevelY = edgeMask & 0x2;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 3 * (size - x - 1 + (size - y - 1) * size);
		pixel[0] = 127 - n.x() * 127;
		pixel[1] = 127 - n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
		// upper left Quadrant
		bevelX = edgeMask & 0x4;
		bevelY = edgeMask & 0x1;
		n = calculateNormal(xf, yf, bevelX, bevelY, er, cr);
		pixel = data + 3 * (x + (size - y - 1) * size);
		pixel[0] = 127 + n.x() * 127;
		pixel[1] = 127 - n.y() * 127;
		pixel[2] = 127 + n.z() * 127;
	}
}

void genNormalTex(QOpenGLFunctions_3_3_Core &glf)
{
	uint8_t *data = new uint8_t[3 * MAP_RES * MAP_RES];
	genNormalMap(MAP_RES, 0xf, data);
	glf.glGenTextures(1, &g_normal_tex);
	glf.glActiveTexture(GL_TEXTURE0);
	glf.glBindTexture(GL_TEXTURE_2D, g_normal_tex);
	glf.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glf.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glf.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glf.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glf.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MAP_RES, MAP_RES, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	delete[] data;
}
