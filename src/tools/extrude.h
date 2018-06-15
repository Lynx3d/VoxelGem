/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_TOOL_EXTRUDE_H
#define VG_TOOL_EXTRUDE_H

#include "edittool.h"
#include "voxelaggregate.h"

class ExtrudeTool: public EditTool
{
	public:
		void mouseMoved(const ToolEvent &event) override;
		void mouseDown(const ToolEvent &event) override;
		void mouseUp(const ToolEvent &event) override;
		static ToolInstance* getInstance();
	protected:
		template <typename T>
		void extrude(T &op, int start, int end);
		void selectFace(SceneRayHit &hit, const VoxelAggregate *aggregate);

		static VoxelEntry rejEntry, selEntry;
		VoxelAggregate selection;
		DirtyVolume selBound;
		int axis;
		int direction;
		int extrudeHeight;
		QVector3D dragStart;
};

#endif // VG_TOOL_EXTRUDE_H
