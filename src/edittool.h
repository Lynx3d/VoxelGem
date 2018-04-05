/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_EDITTOOL_H
#define VG_EDITTOOL_H

#include "voxelgem.h"

#include <QString>
#include <QIcon>

class QMouseEvent;
class SceneRayHit;
class VoxelScene;
class SceneProxy;

/*! Wraps and extends the QMouseEvent with viewport details */
class ToolEvent
{
	public:
		ToolEvent(QMouseEvent *qmEvent, ray_t cRay);
		const ray_t& getCursorRay() const { return cursorRay; }
		bool isShiftPressed() const;
		bool isAltPressed() const;
		bool isControlPressed() const;
	protected:
		QMouseEvent *mouseEvent;
		ray_t cursorRay;
};

class EditTool
{
	public:
		void setSceneProxy(SceneProxy *proxy) { sceneProxy = proxy; }
		virtual void mouseMoved(const ToolEvent &event, VoxelScene &scene);
		virtual void mouseDown(const ToolEvent &event, VoxelScene &scene);
		virtual void mouseUp(const ToolEvent &event, VoxelScene &scene);
	protected:
		/*! this applies the recorded voxel changes and creates an undo step */
		void completeAction();
		SceneProxy *sceneProxy = 0;
};

class ToolInstance
{
	public:
		EditTool *tool;
		QIcon icon;
		QString toolTip;
		QString statusTip;
};

#endif // VG_EDITTOOL_H
