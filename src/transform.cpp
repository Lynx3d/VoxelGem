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
