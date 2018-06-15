/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extrude.h"
#include "voxelscene.h"
#include "sceneproxy.h"

VoxelEntry ExtrudeTool::rejEntry(0, Voxel::VF_NO_COLLISION);
VoxelEntry ExtrudeTool::selEntry(0, Voxel::VF_NON_EMPTY | Voxel::VF_TOOL_SELECT);

const int axis_map[3][3] =
{
	{ 0, 1, 2 },
	{ 1, 2, 0 },
	{ 2, 0, 1 }
};

template <typename T>
void ExtrudeTool::extrude(T &op, int start, int end)
{
	for (int z = selBound.low[2]; z <= selBound.high[2]; ++z)
		for (int y = selBound.low[1]; y <= selBound.high[1]; ++y)
			for (int x = selBound.low[0]; x <= selBound.high[0]; ++x)
	{
		IVector3D pos(x, y, z);
		const VoxelEntry *voxel = selection.getVoxel(pos);
		if (voxel && (voxel->flags & Voxel::VF_TOOL_SELECT))
		{
			op(pos, start, end);
		}
	}
}

void ExtrudeTool::mouseMoved(const ToolEvent &event)
{
	if (!selBound.valid)
		return;
	QVector3D dragEnd = event.cursorPos();
	int newHeight = (dragEnd - dragStart).length() / 30.f;

	if (newHeight < extrudeHeight)
	{
		VoxelEntry ve(0, 0);
		auto clearOp = [this, ve](IVector3D pos, int start, int end)
		{
			for (int i=start; i < end; ++i)
			{
				pos[axis] += (i + 1) * direction;
				scene->setVoxel(pos, ve);
			}
		};
		extrude(clearOp, newHeight, extrudeHeight);
	}
	else if (newHeight > extrudeHeight)
	{
		const VoxelEntry &vt = *scene->getVoxelTemplate();
		auto extrudeOp = [this, &vt](IVector3D pos, int start, int end)
		{
			for (int i=start; i < end; ++i)
			{
				pos[axis] += (i + 1) * direction;
				scene->setVoxel(pos, vt);
			}
		};
		extrude(extrudeOp, extrudeHeight, newHeight);
	}
	extrudeHeight = newHeight;
}

void ExtrudeTool::mouseDown(const ToolEvent &event)
{
	SceneRayHit hit;
	scene->rayIntersect(event.getCursorRay(), hit);
	if (!(hit.flags & SceneRayHit::HIT_VOXEL))
		return;

	dragStart = event.cursorPos();
	extrudeHeight = 0;
	const VoxelLayer* layer = sceneProxy->getLayer(sceneProxy->activeLayer());
	selectFace(hit, layer->aggregate);
}

void ExtrudeTool::mouseUp(const ToolEvent &event)
{
	if (!selBound.valid || extrudeHeight == 0)
		return;

	completeAction();
}

void ExtrudeTool::selectFace(SceneRayHit &hit, const VoxelAggregate *aggregate)
{
	selection.clear();
	selBound.valid = false;
	if (!(hit.flags & SceneRayHit::HIT_VOXEL))
		return;
	axis = hit.hitFaceAxis();
	direction = hit.hitFaceOrientation();
	std::vector<IVector3D> queue;
	// offset of extrude dir
	IVector3D top_offset(0, 0, 0);
	top_offset[axis] += direction;
	// offset positions to search the face boundaries
	std::vector<IVector3D> n_offset(4, IVector3D(0,0,0));
	n_offset[0][axis_map[axis][1]] += 1;
	n_offset[1][axis_map[axis][1]] -= 1;
	n_offset[2][axis_map[axis][2]] += 1;
	n_offset[3][axis_map[axis][2]] -= 1;
	queue.push_back(hit.voxelPos);

	while (queue.size())
	{
		IVector3D pos = queue.back();
		queue.pop_back();
		// skip if we already visited
		const VoxelEntry *sel_voxel = selection.getVoxel(pos);
		if (sel_voxel && sel_voxel->flags != 0)
			continue;
		const VoxelEntry *voxel = aggregate->getVoxel(pos);
		IVector3D top_pos = pos + top_offset;
		const VoxelEntry *top_voxel = aggregate->getVoxel(top_pos);
		// reject if the voxel is empty or the top is non-empty
		if (!voxel || !(voxel->flags & Voxel::VF_NON_EMPTY) ||
			(top_voxel && (top_voxel->flags & Voxel::VF_NON_EMPTY)))
		{
			selection.setVoxel(pos, rejEntry);
			continue;
		}
		selection.setVoxel(pos, selEntry);
		selBound.addPosition(pos);
		for (auto &offset: n_offset)
			queue.push_back(pos + offset);
	}
}

ToolInstance* ExtrudeTool::getInstance()
{
	ToolInstance* instance = new ToolInstance;
	instance->tool = new ExtrudeTool();
	instance->icon.addFile(QStringLiteral(":/images/gfx/icons/extrude.svg"));
	instance->toolTip = "Extrude Voxels";
	instance->statusTip = "<TBD>";
	return instance;
}
