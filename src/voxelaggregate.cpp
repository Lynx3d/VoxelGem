/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelaggregate.h"

#include <iostream>

uint64_t VoxelAggregate::setVoxel(int x, int y, int z, const VoxelEntry &voxel)
{
	uint64_t id = blockID(x, y, z);
	blockMap_t::iterator grid = blockMap.find(id);
	if (grid == blockMap.end())
	{
		int gridPos[3] = {
			x & ~(int)(GRID_LEN - 1),
			y & ~(int)(GRID_LEN - 1),
			z & ~(int)(GRID_LEN - 1) };
		std::cout << "creating grid at (" << gridPos[0] << ", " << gridPos[1] << ", " << gridPos[2] << ")\n";
		voxelGridPtr_t posGrid(new VoxelGrid(gridPos));
		grid = blockMap.emplace(id, posGrid).first;
	}
	// TODO: this should only ever happen when render and editing layer share data, which would be okay.
	// we should find a way to ensure this and throw an error otherwise.
	else if (grid->second.use_count() > 1)
	{
		std::cout << "grid is currently shared, creating copy.\n";
		grid->second = voxelGridPtr_t(new VoxelGrid(*grid->second));
	}
//	std::cout << "setting voxel (" << (x & (int)(GRID_LEN - 1)) << ", " << (y & (int)(GRID_LEN - 1)) << ", " << (z & (int)(GRID_LEN - 1)) << ")\n";
	grid->second->setVoxel(x & (int)(GRID_LEN - 1), y & (int)(GRID_LEN - 1), z & (int)(GRID_LEN - 1), voxel);
	return id;
}

const VoxelEntry* VoxelAggregate::getVoxel(const int pos[3]) const
{
	uint64_t id = blockID(pos[0], pos[1], pos[2]);
	blockMap_t::const_iterator grid = blockMap.find(id);
	if (grid == blockMap.end())
		return 0;
	int gridPos[3] = { pos[0] & (int)(GRID_LEN - 1), pos[1] & (int)(GRID_LEN - 1), pos[2] & (int)(GRID_LEN - 1) };
	return grid->second->getVoxel(gridPos);
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

void VoxelAggregate::clear()
{
	//TODO! cache some grids for later use
	blockMap.clear();
}

void VoxelAggregate::clearBlocks(const std::unordered_set<uint64_t> &blocks)
{
	for (auto &id: blocks)
	{
		blockMap.erase(id);
	}
}

void VoxelAggregate::clearBlocks(const std::unordered_map<uint64_t, DirtyVolume> &blocks)
{
	for (auto &id: blocks)
	{
		blockMap.erase(id.first);
	}
}

void VoxelAggregate::clone(const VoxelAggregate &source)
{
	clear();
	for (auto &block: source.blockMap)
		blockMap.emplace(block.first, block.second);
}

void VoxelAggregate::merge(const VoxelAggregate &topLayer, const std::unordered_set<uint64_t> &blocks)
{
	for (auto &block_id: blocks)
	{
		blockMap_t::iterator grid = blockMap.find(block_id);
		blockMap_t::const_iterator topGrid = topLayer.blockMap.find(block_id);
		if (topGrid == topLayer.blockMap.end())
			continue;
		if (grid == blockMap.end())
		{
			// no need to merge grid
			blockMap.emplace(topGrid->first, topGrid->second);
		}
		else if (grid->second != topGrid->second) // only merge if we actually reference different grids
		{
			if (grid->second.use_count() > 1) // need to copy or we modify multiple aggregates
			{
				grid->second = voxelGridPtr_t(new VoxelGrid(*grid->second));
			}
			grid->second->merge(*topGrid->second);
		}
	}
}

void VoxelAggregate::merge(const VoxelAggregate &topLayer, const std::unordered_map<uint64_t, DirtyVolume> &blocks)
{
	for (auto &block_id: blocks)
	{
		blockMap_t::iterator grid = blockMap.find(block_id.first);
		blockMap_t::const_iterator topGrid = topLayer.blockMap.find(block_id.first);
		if (topGrid == topLayer.blockMap.end())
			continue;
		if (grid == blockMap.end())
		{
			// no need to merge grid
			blockMap.emplace(topGrid->first, topGrid->second);
		}
		else if (grid->second != topGrid->second) // only merge if we actually reference different grids
		{
			if (grid->second.use_count() > 1) // need to copy or we modify multiple aggregates
			{
				grid->second = voxelGridPtr_t(new VoxelGrid(*grid->second));
			}
			grid->second->merge(*topGrid->second);
		}
	}
}

void VoxelAggregate::applyChanges(const VoxelAggregate &toolLayer, AggregateMemento *memento)
{
	for (auto &toolGrid: toolLayer.blockMap)
	{
		blockMap_t::iterator grid = blockMap.find(toolGrid.first);
		GridMemento *gridMem = new GridMemento;
		if (grid == blockMap.end())
		{
			// TODO: implement empty grid memento handling and emplace tool grid
			voxelGridPtr_t posGrid(new VoxelGrid(toolGrid.second->getGridPos()));
			posGrid->applyChanges(*toolGrid.second, gridMem);
			grid = blockMap.emplace(toolGrid.first, posGrid).first;
		}
		else
		{
			if (grid->second.use_count() > 1) // need to copy or we modify multiple aggregates
			{
				grid->second = voxelGridPtr_t(new VoxelGrid(*grid->second));
			}
			grid->second->applyChanges(*toolGrid.second, gridMem);
		}
		memento->blockMap.emplace(toolGrid.first, gridMementoPtr_t(gridMem));
	}
}

void VoxelAggregate::restoreState(AggregateMemento *memento, std::unordered_set<uint64_t> &changed)
{
	for (auto &memGrid: memento->blockMap)
	{
		if (memGrid.second)
		{
			blockMap_t::iterator grid = blockMap.find(memGrid.first);
			if (grid == blockMap.end())
			{
				// TODO: restore deleted grid
				std::cout << "ERR: restoring erased grid unimplemented!\n";
			}
			else
			{
				// this modifies the GridMemento to reflect its previous state!
				grid->second->restoreState(memGrid.second.get());
			}
		}
		// TODO: null pointer denotes deleted grid
		else
		{
			std::cout << "ERR: grid erasing unimplemented!\n";
		}
		changed.insert(memGrid.first);
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

bool VoxelAggregate::getBound(BBox &bound) const
{
	bool haveBound = false;
	for (auto &grid: blockMap)
	{
		if (!haveBound)
		{
			bound = grid.second->getBound();
			haveBound = true;
			continue;
		}
		bound.join(grid.second->getBound());
	}
	return haveBound;
}

// important! the dirty volume must not span blocks!
// while we could adjust the calculation, it would defeat the purpose of of saving adjacent block re-tesselations
void VoxelAggregate::markDirtyBlocks(const DirtyVolume &vol, std::unordered_set<uint64_t> &blocks)
{
	int start[3] = { 0, 0, 0 };
	int end[3] = { 0, 0, 0 };
	for (int i = 0; i < 3; ++i)
	{
		int low = vol.low[i] & (GRID_LEN - 1);
		int high = vol.high[i] & (GRID_LEN - 1);
		if (low == 0)
			start[i] = -1;
		if (high == GRID_LEN - 1)
			end[i] = 1;
	}

	for (int z = start[2]; z <= end[2]; ++z)
		for (int y = start[1]; y <= end[1]; ++y)
			for (int x = start[0]; x <= end[0]; ++x)
				blocks.insert(blockID(vol.low[0] + x * GRID_LEN, vol.low[1] + y * GRID_LEN, vol.low[2] + z * GRID_LEN));
}


const VoxelGrid* VoxelAggregate::getBlock(uint64_t blockId) const
{
	blockMap_t::const_iterator block = blockMap.find(blockId);
	if (block != blockMap.end())
		return block->second.get();
	return 0;
}

/*=================
  RenderAggregate
=================*/

void RenderAggregate::clear(QOpenGLFunctions_3_3_Core &glf)
{
	for (auto &rgrid: renderBlocks)
	{
		rgrid.second->cleanupGL(glf);
		delete rgrid.second;
	}
	renderBlocks.clear();
}

void RenderAggregate::update(QOpenGLFunctions_3_3_Core &glf, const blockSet_t &dirtyBlocks)
{
	for (auto &blockId: dirtyBlocks)
	{
		const VoxelGrid* blockGrid = aggregate->getBlock(blockId);
		// TODO: Typedef ^^
		std::unordered_map<uint64_t, RenderGrid*>::iterator rgrid = renderBlocks.find(blockId);
		if (blockGrid)
		{
			const VoxelGrid* neighbours[27];
			if (rgrid == renderBlocks.end())
			{
				std::cout << "    allocating new RenderGrid" << std::endl;
				rgrid = renderBlocks.emplace(blockId, new RenderGrid).first;
			}
			aggregate->getNeighbours(blockGrid->getGridPos(), neighbours);
			rgrid->second->update(glf, neighbours);
		}
		else
		{
			// does not exist (anymore)
			if (rgrid != renderBlocks.end())
			{
				rgrid->second->cleanupGL(glf);
				delete rgrid->second;
				renderBlocks.erase(rgrid);
			}
		}
	}
}

void RenderAggregate::render(QOpenGLFunctions_3_3_Core &glf)
{
	for (auto &block: renderBlocks)
	{
		block.second->render(glf);
	}
}

void RenderAggregate::renderTransparent(QOpenGLFunctions_3_3_Core &glf)
{
	for (auto &block: renderBlocks)
	{
		block.second->renderTransparent(glf);
	}
}
