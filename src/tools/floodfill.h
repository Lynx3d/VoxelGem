/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_TOOL_FLOODFILL_H
#define VG_TOOL_FLOODFILL_H

#include "edittool.h"
#include "voxelaggregate.h"

class FloodFillTool: public EditTool
{
	public:
		void mouseDown(const ToolEvent &event) override;
		static ToolInstance* getInstance();
	protected:
		int fillVolume(const ToolEvent &event, const VoxelEntry *hitVoxel, IVector3D hitPos, const VoxelAggregate *aggregate);
		std::vector<IVector3D> getSearchOffsets(const ToolEvent &event);

		static VoxelEntry selEntry;
		VoxelAggregate selection;
		//DirtyVolume selBound;
};

#endif // VG_TOOL_FLOODFILL_H
