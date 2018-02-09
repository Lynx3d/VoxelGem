/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelaggregate.h"

#include <iostream>

void VoxelAggregate::setVoxel(int x, int y, int z, const VoxelEntry &voxel)
{
	uint64_t id = blockID(x, y, z);
	blockMap_t::iterator grid = blockMap.find(id);
	if (grid == blockMap.end())
	{
		// TODO: pass proper bound to voxel grid
		int gridPos[3] = {
			x & ~(int)(GRID_LEN - 1),
			y & ~(int)(GRID_LEN - 1),
			z & ~(int)(GRID_LEN - 1) };
		std::cout << "creating grid at (" << gridPos[0] << ", " << gridPos[1] << ", " << gridPos[2] << ")\n";
		voxelGridPtr_t posGrid(new VoxelGrid(gridPos));
		grid = blockMap.emplace(id, posGrid).first;
	}
//	std::cout << "setting voxel (" << (x & (int)(GRID_LEN - 1)) << ", " << (y & (int)(GRID_LEN - 1)) << ", " << (z & (int)(GRID_LEN - 1)) << ")\n";
	grid->second->setVoxel(x & (int)(GRID_LEN - 1), y & (int)(GRID_LEN - 1), z & (int)(GRID_LEN - 1), voxel);
}

bool VoxelAggregate::rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const
{
	// TODO: this is a brute force place holder implementation
	bool didHit = false;
	for (auto &grid: blockMap)
	{
		intersect_t isect;
		int iPos[3];
		if (grid.second->rayIntersect(ray, iPos, isect))
		{
			if (!didHit || isect.tNear < hit.tNear)
			{
				hitPos[0] = iPos[0];
				hitPos[1] = iPos[1];
				hitPos[2] = iPos[2];
				hit = isect;
			}
			didHit = true;
		}
	}
	return didHit;
}

void VoxelAggregate::render(QOpenGLFunctions_3_3_Core &glf)
{
	const VoxelGrid* neighbourGrids[27];
	for (auto &grid: blockMap)
	{
		if (grid.second->isDirty())
			getNeighbours(grid.second->getGridPos(), neighbourGrids);
		grid.second->render(glf, neighbourGrids);
	}
}

void VoxelAggregate::getNeighbours(const int gridPos[3], const VoxelGrid* neighbours[27])
{
	for (int z = -1, i = 0; z < 2; ++z)
		for (int y = -1; y < 2; ++y)
			for (int x = -1; x < 2; ++x, ++i)
	{
		uint64_t id = blockID(gridPos[0] + x * GRID_LEN, gridPos[1] + y * GRID_LEN, gridPos[2] + z * GRID_LEN);
		blockMap_t::iterator grid = blockMap.find(id);
		if (grid == blockMap.end())
			neighbours[i] = 0;
		else
			neighbours[i] = grid->second.get();
	}
}
