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

class QMouseEvent;
class SceneRayHit;
class VoxelScene;

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
		virtual void mouseMoved(const ToolEvent &event, VoxelScene &scene);
		virtual void mouseDown(const ToolEvent &event, VoxelScene &scene);
		virtual void mouseUp(const ToolEvent &event, VoxelScene &scene);
};

class PaintTool: public EditTool
{
	public:
		void mouseMoved(const ToolEvent &event, VoxelScene &scene) override;
		void mouseDown(const ToolEvent &event, VoxelScene &scene) override;
		void mouseUp(const ToolEvent &event, VoxelScene &scene) override;
	private:
		bool painting = false;
		bool deleting;
		bool haveLastPos;
		int lastPos[3];
};


#endif // VG_EDITTOOL_H
