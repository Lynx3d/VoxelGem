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
#include <QOpenGLDebugLogger>


enum DragType
{
	DRAG_NONE = 0,
	DRAG_ROTATE = 1,
	DRAG_PAN = 2,
	DRAG_TOOL = 3
};

class EditTool;
class GlViewportWidget;
class VoxelScene;
class SceneRayHit;
class GLRenderable;
class LineGrid;

class ViewportSettings
{
	public:
		ViewportSettings(const GlViewportWidget *parentW):
			heading(180), pitch(0), roll(0), fov(45), camDistance(10), camPivot(0.f, 2.f, 0.f), parent(parentW)
			{
				updateViewport();
				updateViewMatrix();
			}
		QMatrix4x4 getGlMatrix() const;
		QMatrix4x4 getViewMatrix() { return view; }
		ray_t unproject(const QVector3D &vec) const;
		void rotateBy(float dHead, float dPitch);
		void panBy(float dX, float dY);
		void zoomBy(float dZ);
		void updateViewport();
	protected:
		void updateViewMatrix();
		QMatrix4x4 proj;
		QMatrix4x4 view;
		float heading, pitch, roll;
		float fov, camDistance;
		QVector3D camPivot;
		const GlViewportWidget *parent;
};

// QOpenGLExtraFunctions not available before Qt 5.6
class GlViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
	public:
		GlViewportWidget(VoxelScene *pscene, QWidget *parent = nullptr);
		void setSamples(int numSamples);
		void setViewMode(RenderOptions::Modes mode);
		static float sRGB_LUT[1024];
		GLRenderable* getGrid();
		const RenderOptions& getRenderOptions() { return renderOptions; }
		void activeLayerChanged(int layerN);
	public Q_SLOTS:
		void on_activeToolChanged(EditTool *tool);
		void on_layerSettingsChanged(int layerN, int change_flags);
		void on_renderDataChanged();
	protected:
		void generateUBOs();
		void initializeGL() override;
		void paintGL() override;
		void resizeGL(int w, int h) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void wheelEvent(QWheelEvent *event) override;

		VoxelScene *scene;
		GLuint m_ubo_LUT;
		GLuint m_ubo_material;
		GLuint m_normal_tex;
		ViewportSettings *vpSettings;
		WireCube *boundCube;
		LineGrid *grid; // TODO: move to ViewportSettings?
		RenderOptions renderOptions; // TODO: move to ViewportSettings?
		bool tesselationChanged; // TODO: move to ViewportSettings?
		DragType dragStatus;
		QPoint dragStart;
		EditTool *currentTool;
		QOpenGLDebugLogger *glDebug;
};

#endif // VG_GLVIEWPORT_H
