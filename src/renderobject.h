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

class GLRenderable
{
	public:
		GLRenderable(): glVBO(0), glBufferSize(0), dirty(true) {}
		virtual ~GLRenderable() {} // TODO: add checks to ensure OpenGL data was freed, now is too late!
		bool isDirty() const { return dirty; }
		virtual void cleanupGL(QOpenGLFunctions_3_3_Core &glf);
		virtual void setup(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual void render(QOpenGLFunctions_3_3_Core &glf) = 0;
		virtual bool rayIntersect(const ray_t &ray, SceneRayHit &hit) { return false; }
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
		LineGrid(): axis(1), bound(IVector3D(-4, 0, -4), IVector3D(4, 0, 4)) {}
		void setShape(int gridPlane, const IBBox &bounds);
		void setup(QOpenGLFunctions_3_3_Core &glf) override;
		void render(QOpenGLFunctions_3_3_Core &glf) override;
		bool rayIntersect(const ray_t &ray, SceneRayHit &hit) override;
	protected:
		void rebuild(QOpenGLFunctions_3_3_Core &glf);
		void buildLines(std::vector<GlVertex_t> &vertices, int lineAxis, int spaceAxis);
		int axis;
		IBBox bound;
		int numVert;
};

#endif // VG_RENDEROBJECT_H
