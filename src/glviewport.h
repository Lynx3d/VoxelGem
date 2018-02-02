/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_GLVIEWPORT_H
#define VG_GLVIEWPORT_H

#include "renderobject.h"

//#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QMatrix4x4>


enum DragType
{
	DRAG_NONE = 0,
	DRAG_ROTATE = 1,
	DRAG_PAN = 2,
	DRAG_TOOL = 3
};

class GlViewportWidget;
class VoxelScene;

class ViewportSettings
{
	public:
		ViewportSettings(const GlViewportWidget *parentW):
			heading(180), pitch(0), roll(0), fov(45), camPos(0.f, 0.f, -10.f), parent(parentW)
			{
				updateViewport();
				updateViewMatrix();
			}
		QMatrix4x4 getGlMatrix();
		ray_t unproject(const QVector3D &vec);
		void rotateBy(float dHead, float dPitch);
		void panBy(float dX, float dY);
		void updateViewport();
	protected:
		void updateViewMatrix();
		QMatrix4x4 proj;
		QMatrix4x4 view;
		float heading, pitch, roll;
		float fov;
		QVector3D camPos;
		const GlViewportWidget *parent;
};

// QOpenGLExtraFunctions not available before Qt 5.6
class GlViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
	public:
		GlViewportWidget(VoxelScene *pscene): scene(pscene), dragStatus(DRAG_NONE) {};
		static float sRGB_LUT[1024];
	protected:
		void initializeGL();
		void paintGL();
		void resizeGL(int w, int h);
		void mousePressEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

		VoxelScene *scene;
		QOpenGLVertexArrayObject m_vertexSpec; // buffered vertex attribute layout
		GLuint matrixID;
		GLuint m_vbo;
		GLuint m_ubo_LUT;
		ViewportSettings *vpSettings;
		DragType dragStatus;
		QPoint dragStart;
};

#endif // VG_GLVIEWPORT_H
