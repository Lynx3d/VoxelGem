/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_RENDEROBJECT_H
#define VG_RENDEROBJECT_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>

// TODO: move to renderobject.h
struct GlVertex_t
{
	float pos[3];
	unsigned char col[4];
};

struct GlVoxelVertex_t
{
	float pos[3];
	unsigned char col[4];
	float normal[3];
	unsigned char matIndex;
};

struct ray_t
{
	QVector3D dir;
	QVector3D from;
	float t_min;
	float t_max;
};

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
		GLRenderable(): glBufferSize(0), dirty(true) {}
		virtual void setup(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual void render(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) { return false; }
	protected:
		virtual void uploadBuffer(QOpenGLFunctions_3_3_Core &glf, void *data, GLsizeiptr size);
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
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit);
	protected:
		int radius;
		int numVert;
};

#endif // VG_RENDEROBJECT_H
