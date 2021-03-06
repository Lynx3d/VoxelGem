/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "transform.h"
#include "voxelaggregate.h"

void VTTranslate::operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target)
{
	target->setVoxel(pos + offset, *voxel);
}

void VTMirror::operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target)
{
	IVector3D mPos(pos);
	mPos[axis] = -pos[axis] + 2 * center;
	target->setVoxel(mPos, *voxel);
}

VTRotate::VTRotate(int axis, Rotation rotation)
{
	int planeXMap, planeYMap;
	int planeXDir, planeYDir;

	switch (rotation)
	{
		case Rot90:
			planeXMap = 1;
			planeYMap = 0;
			planeXDir = 1;
			planeYDir = -1;
			break;
		case Rot180:
			planeXMap = 0;
			planeYMap = 1;
			planeXDir = -1;
			planeYDir = -1;
			break;
		case Rot270:
			planeXMap = 1;
			planeYMap = 0;
			planeXDir = -1;
			planeYDir = 1;
	}
	if (axis == 0)
	{
		axisMap = IVector3D(0, 1 + planeXMap, 1 + planeYMap);
		axisScale = IVector3D(1, planeXDir, planeYDir);
	}
	else if (axis == 1)
	{
		axisMap = IVector3D((2 + planeYMap)%3, 1, (2 + planeXMap)%3);
		axisScale = IVector3D(planeYDir, 1, planeXDir);
	}
	else
	{
		axisMap = IVector3D(planeXMap, planeYMap, 2);
		axisScale = IVector3D(planeXDir, planeYDir, 1);
	}
}

void VTRotate::operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target)
{
	IVector3D mPos;
	for (int i = 0; i < 3; ++i)
		mPos[axisMap[i]] = pos[i] * axisScale[i];
	target->setVoxel(mPos, *voxel);
}

VoxelAggregate* transformAggregate(const VoxelAggregate *ag, VoxelTransform &xform)
{
	VoxelAggregate *transformed = new VoxelAggregate();
	const blockMap_t& blockMap = ag->getBlockMap();
	for (auto &grid: blockMap)
	{
		IVector3D gridPos = grid.second->getGridPos();
		for (int z = 0; z < GRID_LEN; ++z)
			for (int y = 0; y < GRID_LEN; ++y)
				for (int x = 0; x < GRID_LEN; ++x)
		{
			const VoxelEntry *voxel = grid.second->getVoxel(IVector3D(x, y, z));
			if (!(voxel->flags & Voxel::VF_NON_EMPTY))
				continue;
			xform(IVector3D(gridPos.x + x, gridPos.y + y, gridPos.z + z), voxel, transformed);
		}
	}
	return transformed;
}
