/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "renderobject.h"
#include "glviewport.h"

#include <iostream>
#include <cmath>

#include <QOpenGLShaderProgram>

// Note: leaves the array buffer bound
void GLRenderable::uploadBuffer(QOpenGLFunctions_3_3_Core &glf, void *data, GLsizeiptr size)
{
	bool realloc = size > glBufferSize || glBufferSize - size > 4096;
	if (realloc)
	{
		if (glBufferSize == 0)
			glf.glGenBuffers(1, &glVBO);
		glf.glBindBuffer(GL_ARRAY_BUFFER, glVBO);
		glf.glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
		glBufferSize = size;
		std::cout << "uploading " << size << " Bytes to buffer #" << glVBO << std::endl;
		setup(glf);
	}
	else
	{
		glf.glBindBuffer(GL_ARRAY_BUFFER, glVBO);
		glf.glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}
}

void GLRenderable::deleteBuffer(QOpenGLFunctions_3_3_Core &glf)
{
	glf.glDeleteBuffers(1, &glVBO);
	glVBO = 0;
	glBufferSize = 0;
}

void LineGrid::setup(QOpenGLFunctions_3_3_Core &glf)
{
	// Attribute 0: vertex position
	glf.glEnableVertexAttribArray(0);
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, pos));
	// Attribute 1: vertex color
	glf.glEnableVertexAttribArray(1);
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
}

void LineGrid::render(QOpenGLFunctions_3_3_Core &glf)
{
	if (!glVAO.isCreated())
		glVAO.create();

	glVAO.bind();

	if (dirty) // create and upload buffer
	{
		numVert = 8 + 8 * radius;
		GlVertex_t *vertices = new GlVertex_t[numVert];
		// coordinate cross positive
		vertices[0] = {{ 0.f, 0.f, 0.f }, 			rgba_t(200, 0, 0, 255)};
		vertices[1] = {{ (float)radius, 0.f, 0.f }, rgba_t(200, 0, 0, 255)};
		vertices[2] = {{ 0.f, 0.f, 0.f }, 			rgba_t(0, 0, 200, 255)};
		vertices[3] = {{ 0.f, 0.f, (float)radius }, rgba_t(0, 0, 200, 255)};
		// coordinate cross negative
		vertices[4] = {{ (float)-radius, 0.f, 0.f }, rgba_t(150, 50, 50, 255)};
		vertices[5] = {{ 0.f, 0.f, 0.f }, 			 rgba_t(150, 50, 50, 255)};
		vertices[6] = {{ 0.f, 0.f, (float)-radius }, rgba_t(50, 50, 150, 255)};
		vertices[7] = {{ 0.f, 0.f, 0.f }, 			 rgba_t(50, 50, 150, 255)};
		int v = 8;
		for (int i = 1; i <= radius; ++i)
		{
			rgba_t lineCol = (i & 0xf) ? rgba_t(128, 128, 128, 128) : rgba_t(80, 80, 80, 80);
			// x-direction
			vertices[v++] = {{ (float)-radius, 0.f, (float)i }, lineCol};
			vertices[v++] = {{ (float)radius, 0.f, (float)i }, lineCol};
			vertices[v++] = {{ (float)-radius, 0.f, (float)-i }, lineCol};
			vertices[v++] = {{ (float)radius, 0.f, (float)-i }, lineCol};
			// z-direction
			vertices[v++] = {{ (float)i, 0.f, (float)-radius }, lineCol};
			vertices[v++] = {{ (float)i, 0.f, (float)radius }, lineCol};
			vertices[v++] = {{ (float)-i, 0.f, (float)-radius }, lineCol};
			vertices[v++] = {{ (float)-i, 0.f, (float)radius }, lineCol};
		}
		uploadBuffer(glf, vertices, numVert * sizeof(GlVertex_t));
		dirty = false;
		delete[] vertices;
	}
	glf.glDrawArrays(GL_LINES, 0, numVert);
	glVAO.release();
}

bool LineGrid::rayIntersect(const ray_t &ray, SceneRayHit &hit)
{
	// TODO: implement axis
	int axis = 1;
	int axisDirFlag = 0;
	float tHit;
	//std::cout << "ray.dir[axis]: " << ray.dir[axis] << std::endl;
	if (ray.dir[axis] > 1e-7)
	{
		tHit = (/*gridLevel[axis]*/ - ray.from[axis]) / ray.dir[axis];
	}
	else if (ray.dir[axis] < -1e-7)
	{
		axisDirFlag = SceneRayHit::AXIS_NEGATIVE;
		tHit = (/*gridLevel[axis]*/ - ray.from[axis]) / ray.dir[axis];
	}
	else return false;

	//std::cout << "tHit: " << tHit << std::endl;
	if (tHit >= ray.t_min && tHit <= ray.t_max)
	{
		QVector3D pHit = ray.from + tHit * ray.dir;
		for (int i = 0; i < 3; ++i)
			hit.voxelPos[i] = floor(pHit[i]);
		// floor() is not suitable for plane axis, but we know the value anyway...
		hit.voxelPos[axis] = 0; /*gridLevel[axis]*/
		// a plane has no volume, so if we shoot towards negative axis
		// we actually hit one voxel pos lower than in opposite direction
		if (axisDirFlag)
			hit.voxelPos[axis] -= 1;
		hit.flags = axis | axisDirFlag;
		hit.rayT = tHit;
		return true;
	}
	return false;
}

void LineGrid::setSize(int gridSize)
{
	radius = gridSize;
	dirty = true;
}
