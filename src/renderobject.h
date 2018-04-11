/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_RENDEROBJECT_H
#define VG_RENDEROBJECT_H

#include "voxelgem.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
//#include <QVector3D>

struct GlVertex_t
{
	float pos[3];
	rgba_t col;
};

struct GlVoxelVertex_t
{
	float pos[3];
	unsigned char col[4];
	unsigned char index;
	unsigned char matIndex;
	unsigned char texIndex;
	unsigned char occlusion;
};

// TODO: cleanup
struct intersect_t
{
	static const int AXIS_NONE = 1 << 7;
	static const int AXIS_NEGATIVE = 1 << 2;
	intersect_t() {}
	intersect_t(float near, float far): tNear(near), tFar(far) {}
	float tNear;
	float tFar;
	int entryAxis = AXIS_NONE;
};

class GLRenderable
{
	public:
		GLRenderable(): glVBO(0), glBufferSize(0), dirty(true) {}
		virtual ~GLRenderable() {} // TODO: add checks to ensure OpenGL data was freed, now is too late!
		bool isDirty() const { return dirty; }
		virtual void setup(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual void render(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual bool rayIntersect(const ray_t &ray, IVector3D &hitPos, intersect_t &hit) { return false; }
	protected:
		virtual void uploadBuffer(QOpenGLFunctions_3_3_Core &glf, void *data, GLsizeiptr size);
		void deleteBuffer(QOpenGLFunctions_3_3_Core &glf);
		QOpenGLVertexArrayObject glVAO;
		GLuint glVBO;
		GLsizei glBufferSize;
		bool dirty;
};

class LineGrid : public GLRenderable
{
	public:
		LineGrid(): radius(4){}
		void setup(QOpenGLFunctions_3_3_Core &glf);
		void setSize(int gridSize);
		void render(QOpenGLFunctions_3_3_Core &glf);
		bool rayIntersect(const ray_t &ray, IVector3D &hitPos, intersect_t &hit);
	protected:
		int radius;
		int numVert;
};

#endif // VG_RENDEROBJECT_H
