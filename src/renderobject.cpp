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

void GLRenderable::cleanupGL(QOpenGLFunctions_3_3_Core &glf)
{
	if (glVBO)
		deleteBuffer(glf);
	glVAO.destroy();
}

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

//======= LineGrid =========== //

void LineGrid::setup(QOpenGLFunctions_3_3_Core &glf)
{
	// Attribute 0: vertex position
	glf.glEnableVertexAttribArray(0);
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, pos));
	// Attribute 1: vertex color
	glf.glEnableVertexAttribArray(1);
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
}

void LineGrid::setShape(int gridPlane, const IBBox &bounds)
{
	axis = gridPlane;
	bound = bounds;
	dirty = true;
}

void LineGrid::buildLines(std::vector<GlVertex_t> &vertices, int lineAxis, int spaceAxis)
{
	static const rgba_t axisCol[3][2] = {
		{ rgba_t(150, 50, 50, 255), rgba_t(200, 0, 0, 255) },
		{ rgba_t(50, 150, 50, 255), rgba_t(0, 200, 0, 255) },
		{ rgba_t(50, 50, 150, 255), rgba_t(0, 0, 200, 255) } };
	float gridLevel = bound.pMin[axis];
	for (int i = bound.pMin[spaceAxis]; i <= bound.pMax[spaceAxis]; ++i)
	{
		if (i == 0)
			continue;
		rgba_t lineCol = (i & 0xf) ? rgba_t(128, 128, 128, 128) : rgba_t(80, 80, 80, 80);
		vertices.push_back(GlVertex_t());
		GlVertex_t &vertex = vertices.back();
		vertex.pos[axis] = gridLevel;
		vertex.pos[spaceAxis] = i;
		vertex.pos[lineAxis] = bound.pMin[lineAxis];
		vertex.col = lineCol;
		vertices.push_back(vertex);
		vertices.back().pos[lineAxis] = bound.pMax[lineAxis];
	}
	// colored coordinate axis
	if (bound.pMin[spaceAxis] <= 0 && bound.pMax[spaceAxis] >= 0)
	{
		rgba_t lineCol = (bound.pMin[lineAxis] < 0) ? axisCol[lineAxis][0] : axisCol[lineAxis][1];
		vertices.push_back(GlVertex_t());
		GlVertex_t &vertex = vertices.back();
		vertex.pos[axis] = gridLevel;
		vertex.pos[spaceAxis] = 0;
		vertex.pos[lineAxis] = bound.pMin[lineAxis];
		vertex.col = lineCol;
		if (bound.pMin[lineAxis] < 0 && bound.pMax[lineAxis] > 0)
		{
			vertices.push_back(vertex);
			vertices.back().pos[lineAxis] = 0;
			vertices.push_back(vertices.back());
			lineCol = axisCol[lineAxis][1];
			vertices.back().col = lineCol;
		}
		vertices.push_back(vertices.back());
		vertices.back().pos[lineAxis] = bound.pMax[lineAxis];
		vertices.back().col = lineCol;
	}
}

void LineGrid::rebuild(QOpenGLFunctions_3_3_Core &glf)
{
	static const int axisMap[3][2] = { { 1, 2 }, { 0, 2 }, { 0, 1 } };
	int gridX = axisMap[axis][0], gridY = axisMap[axis][1];
	// avoid reallocations (and issues referencing elements in push_back())
	int maxVert = 2 * (bound.pMax[gridX] - bound.pMin[gridX] + bound.pMax[gridY] - bound.pMin[gridY] + 4);
	std::vector<GlVertex_t> vertices;
	vertices.reserve(maxVert);
	buildLines(vertices, gridX, gridY);
	buildLines(vertices, gridY, gridX);
	uploadBuffer(glf, vertices.data(), vertices.size() * sizeof(GlVertex_t));
	numVert = vertices.size();
}

void LineGrid::render(QOpenGLFunctions_3_3_Core &glf)
{
	if (!glVAO.isCreated())
		glVAO.create();

	glVAO.bind();

	if (dirty) // create and upload buffer
	{
		rebuild(glf);
		dirty = false;
	}
	glf.glDrawArrays(GL_LINES, 0, numVert);
	glVAO.release();
}

bool LineGrid::rayIntersect(const ray_t &ray, SceneRayHit &hit)
{
	float gridLevel = bound.pMin[axis];
	int axisDirFlag = 0;
	float tHit;
	//std::cout << "ray.dir[axis]: " << ray.dir[axis] << std::endl;
	if (ray.dir[axis] > 1e-7)
	{
		tHit = (gridLevel - ray.from[axis]) / ray.dir[axis];
	}
	else if (ray.dir[axis] < -1e-7)
	{
		axisDirFlag = SceneRayHit::AXIS_NEGATIVE;
		tHit = (gridLevel - ray.from[axis]) / ray.dir[axis];
	}
	else return false;

	//std::cout << "tHit: " << tHit << std::endl;
	if (tHit >= ray.t_min && tHit <= ray.t_max)
	{
		QVector3D pHit = ray.from + tHit * ray.dir;
		for (int i = 0; i < 3; ++i)
			hit.voxelPos[i] = floor(pHit[i]);
		// floor() is not suitable for plane axis, but we know the value anyway...
		hit.voxelPos[axis] = gridLevel;
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

//======= WireCube =========== //

GLuint WireCube::s_indexBuffer = 0;

void WireCube::initializeStaticGL(QOpenGLFunctions_3_3_Core &glf)
{
	static const uint16_t index_array[24] =
	{ 0,1, 1,2, 2,3, 3,0,   7,6, 6,5, 5,4, 4,7,  0,4, 1,5, 2,6, 3,7 };
	glf.glGenBuffers(1, &s_indexBuffer);
	glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_indexBuffer);
	glf.glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(uint16_t), index_array, GL_STATIC_DRAW);
}

void WireCube::setColor(rgba_t color)
{
	col = color;
	dirty = true;
}

void WireCube::setShape(const IBBox &box, float margin)
{
	QVector3D offset(margin, margin, margin);
	pMin = QVector3D(box.pMin.x, box.pMin.y, box.pMin.z) - offset;
	pMax = QVector3D(box.pMax.x, box.pMax.y, box.pMax.z) + offset;
	dirty = true;
}

// TODO: make 1 function per vertex type rather than copy for each implementation
void WireCube::setup(QOpenGLFunctions_3_3_Core &glf)
{
	// TODO: should only be required when VAO is created
	glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_indexBuffer);
	// Attribute 0: vertex position
	glf.glEnableVertexAttribArray(0);
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, pos));
	// Attribute 1: vertex color
	glf.glEnableVertexAttribArray(1);
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
}

void WireCube::render(QOpenGLFunctions_3_3_Core &glf)
{
	if (!glVAO.isCreated())
		glVAO.create();

	glVAO.bind();

	if (dirty) // create and upload buffer
	{
		rebuild(glf);
		dirty = false;
	}
	//glf.glDrawArrays(GL_LINES, 0, numVert);
	glf.glDrawElements(GL_LINES, 12 * 2, GL_UNSIGNED_SHORT, nullptr);
	glVAO.release();
}

void WireCube::rebuild(QOpenGLFunctions_3_3_Core &glf)
{
	std::vector<GlVertex_t> vertices;
	vertices.resize(8);
	vertices[0] = { { pMin.x(), pMin.y(), pMin.z() }, col };
	vertices[1] = { { pMin.x(), pMin.y(), pMax.z() }, col };
	vertices[2] = { { pMin.x(), pMax.y(), pMax.z() }, col };
	vertices[3] = { { pMin.x(), pMax.y(), pMin.z() }, col };
	vertices[4] = { { pMax.x(), pMin.y(), pMin.z() }, col };
	vertices[5] = { { pMax.x(), pMin.y(), pMax.z() }, col };
	vertices[6] = { { pMax.x(), pMax.y(), pMax.z() }, col };
	vertices[7] = { { pMax.x(), pMax.y(), pMin.z() }, col };

	uploadBuffer(glf, vertices.data(), vertices.size() * sizeof(GlVertex_t));
}
