/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_EDITTOOL_H
#define VG_EDITTOOL_H

class QMouseEvent;
class SceneRayHit;
class VoxelScene;

/*! Wraps and extends the QMouseEvent with viewport details */
class ToolEvent
{
	public:
		ToolEvent(QMouseEvent *qmEvent, SceneRayHit *srHit);
		const SceneRayHit* getCursorHit() const;
		bool getAdjacentVoxel(int pos[3]) const;
		bool isShiftPressed() const;
		bool isAltPressed() const;
		bool isControlPressed() const;
	protected:
		QMouseEvent *mouseEvent;
		SceneRayHit *rayHit;
};

class EditTool
{
	public:
		virtual void mouseMoved(const ToolEvent &event);
		virtual void mouseDown(const ToolEvent &event, VoxelScene &scene);
		virtual void mouseUp(const ToolEvent &event, VoxelScene &scene);
};

class PaintTool: public EditTool
{
	public:
		void mouseDown(const ToolEvent &event, VoxelScene &scene) override;
	private:
		bool painting = false;
};


#endif // VG_EDITTOOL_H
