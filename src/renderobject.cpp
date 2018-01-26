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

extern QOpenGLShaderProgram* m_program;

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

void LineGrid::setup(QOpenGLFunctions_3_3_Core &glf)
{
	m_program->bind();
	m_program->enableAttributeArray("v_position");
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, pos));
	m_program->enableAttributeArray("v_color");
	//glf.glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
}

void LineGrid::render(QOpenGLFunctions_3_3_Core &glf)
{
	if (!glVAO.isCreated())
		glVAO.create();

	glVAO.bind();

	if (dirty) // create and upload buffer
	{
		numVert = 4 + 8 * radius;
		GlVertex_t *vertices = new GlVertex_t[numVert];
		// coordinate cross
		vertices[0] = {{ (float)-radius, 0.f, 0.f }, { 200, 0, 0, 255 }};
		vertices[1] = {{ (float)radius, 0.f, 0.f }, { 200, 0, 0, 255 }};
		vertices[2] = {{ 0.f, 0.f, (float)-radius }, { 0, 0, 200, 255 }};
		vertices[3] = {{ 0.f, 0.f, (float)radius }, { 0, 0, 200, 255 }};
		int v = 4;
		for (int i = 1; i <= radius; ++i)
		{
			// x-direction
			vertices[v++] = {{ (float)-radius, 0.f, (float)i }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)radius, 0.f, (float)i }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)-radius, 0.f, (float)-i }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)radius, 0.f, (float)-i }, { 128, 128, 128, 255 }};
			// z-direction
			vertices[v++] = {{ (float)i, 0.f, (float)-radius }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)i, 0.f, (float)radius }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)-i, 0.f, (float)-radius }, { 128, 128, 128, 255 }};
			vertices[v++] = {{ (float)-i, 0.f, (float)radius }, { 128, 128, 128, 255 }};
		}
		uploadBuffer(glf, vertices, numVert * sizeof(GlVertex_t));
		dirty = false;
		delete[] vertices;
	}
	glf.glDrawArrays(GL_LINES, 0, numVert);
	glVAO.release();
}

bool LineGrid::rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit)
{
	// TODO: implement axis
	int axis = 1;
	int axisDirFlag = 0;
	float tHit;
	std::cout << "ray.dir[axis]: " << ray.dir[axis] << std::endl;
	if (ray.dir[axis] > 1e-7)
	{
		tHit = (/*gridLevel[axis]*/ - ray.from[axis]) / ray.dir[axis];
	}
	else if (ray.dir[axis] < -1e-7)
	{
		axisDirFlag = intersect_t::AXIS_NEGATIVE;
		tHit = (/*gridLevel[axis]*/ - ray.from[axis]) / ray.dir[axis];
	}
	else return false;
	
	std::cout << "tHit: " << tHit << std::endl;
	if (tHit >= ray.t_min && tHit <= ray.t_max)
	{
		QVector3D pHit = ray.from + tHit * ray.dir;
		for (int i = 0; i < 3; ++i)
			hitPos[i] = floor(pHit[i]);
		// a plane has no volume, so if we shoot towards negative axis
		// we actually hit one voxel pos lower than in opposite direction
		if (axisDirFlag)
			hitPos[axis] -= 1;
		hit.entryAxis = axis | axisDirFlag;
		hit.tNear = tHit;
		hit.tFar = tHit;
		return true;
	}
	return false;
}
