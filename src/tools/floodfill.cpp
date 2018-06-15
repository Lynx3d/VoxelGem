/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "floodfill.h"
#include "voxelscene.h"
#include "sceneproxy.h"

VoxelEntry FloodFillTool::selEntry(0, Voxel::VF_NON_EMPTY | Voxel::VF_TOOL_SELECT);

const int axis_map[3][3] =
{
	{ 0, 1, 2 },
	{ 1, 2, 0 },
	{ 2, 0, 1 }
};

void FloodFillTool::mouseDown(const ToolEvent &event)
{
	IVector3D curPos;
	const VoxelEntry *voxel = getCursorVoxelEdit(event, curPos);
	if (!voxel)
		return;

	const VoxelLayer* layer = sceneProxy->getLayer(sceneProxy->activeLayer());
	// TODO: ensure layer...(curPos) == voxel
	int fillCount = fillVolume(event, voxel, curPos, layer->aggregate);
	if (fillCount > 0)
		completeAction();
}

std::vector<IVector3D> FloodFillTool::getSearchOffsets(const ToolEvent &event)
{
	bool sliceMode = event.getRenderOptions().mode == RenderOptions::MODE_SLICE;
	if (sliceMode)
	{
		int axis = event.getRenderOptions().axis;
		std::vector<IVector3D> n_offset(4, IVector3D(0,0,0));
		n_offset[0][axis_map[axis][1]] += 1;
		n_offset[1][axis_map[axis][1]] -= 1;
		n_offset[2][axis_map[axis][2]] += 1;
		n_offset[3][axis_map[axis][2]] -= 1;
		return n_offset;
	}
	else
	{
		std::vector<IVector3D> n_offset(6, IVector3D(0,0,0));
		n_offset[0][0] += 1;
		n_offset[1][0] -= 1;
		n_offset[2][1] += 1;
		n_offset[3][1] -= 1;
		n_offset[4][2] += 1;
		n_offset[5][2] -= 1;
		return n_offset;
	}
}

int FloodFillTool::fillVolume(const ToolEvent &event, const VoxelEntry *hitVoxel, IVector3D hitPos, const VoxelAggregate *aggregate)
{
	selection.clear();
	//selBound.valid = false;
	int fillCount = 0;

	std::vector<IVector3D> n_offset = getSearchOffsets(event);
	std::vector<IVector3D> queue;
	queue.push_back(hitPos);
	while (queue.size())
	{
		IVector3D pos = queue.back();
		queue.pop_back();
		// skip if we already visited
		const VoxelEntry *sel_voxel = selection.getVoxel(pos);
		if (sel_voxel && sel_voxel->flags != 0)
			continue;
		// mark visited
		selection.setVoxel(pos, selEntry);
		const VoxelEntry *voxel = aggregate->getVoxel(pos);
		if (!voxel || !(voxel->flags & Voxel::VF_NON_EMPTY))
		{
			//selection.setVoxel(pos, rejEntry);
			continue;
		}
		// decide if we floodfill
		if (voxel->col.raw != hitVoxel->col.raw || voxel->flags != hitVoxel->flags)
			continue;
		scene->setVoxel(pos, *scene->getVoxelTemplate());
		++fillCount;
		//selBound.addPosition(pos);
		for (auto &offset: n_offset)
			queue.push_back(pos + offset);
	}
	return fillCount;
}

ToolInstance* FloodFillTool::getInstance()
{
	ToolInstance* instance = new ToolInstance;
	instance->tool = new FloodFillTool();
	instance->icon.addFile(QStringLiteral(":/images/gfx/icons/bucket.svg"));
	instance->toolTip = "Flood Fill Voxels";
	instance->statusTip = "<TBD>";
	return instance;
}
