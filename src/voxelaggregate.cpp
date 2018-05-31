/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelaggregate.h"

#include <iostream>

uint64_t VoxelAggregate::setVoxel(const IVector3D &pos, const VoxelEntry &voxel)
{
	uint64_t id = blockID(pos.x, pos.y, pos.z);
	blockMap_t::iterator grid = blockMap.find(id);
	if (grid == blockMap.end())
	{
		IVector3D gridPos(
			pos.x & ~(int)(GRID_LEN - 1),
			pos.y & ~(int)(GRID_LEN - 1),
			pos.z & ~(int)(GRID_LEN - 1) );
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
	grid->second->setVoxel(pos.x & (int)(GRID_LEN - 1), pos.y & (int)(GRID_LEN - 1), pos.z & (int)(GRID_LEN - 1), voxel);
	return id;
}

const VoxelEntry* VoxelAggregate::getVoxel(const IVector3D &pos) const
{
	uint64_t id = blockID(pos[0], pos[1], pos[2]);
	blockMap_t::const_iterator grid = blockMap.find(id);
	if (grid == blockMap.end())
		return 0;
	IVector3D gridPos(pos.x & (int)(GRID_LEN - 1), pos.y & (int)(GRID_LEN - 1), pos.z & (int)(GRID_LEN - 1));
	return grid->second->getVoxel(gridPos);
}

bool VoxelAggregate::rayIntersect(const ray_t &ray, SceneRayHit &hit) const
{
	// TODO: this is a brute force place holder implementation
	bool didHit = false;
	for (auto &grid: blockMap)
	{
		SceneRayHit isect;
		if (grid.second->rayIntersect(ray, isect))
		{
			if (!didHit || isect.rayT < hit.rayT)
			{
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
			voxelGridPtr_t posGrid(new VoxelGrid(toolGrid.second->getGridPos()));
			if (posGrid->applyChanges(*toolGrid.second, nullptr) == 0)
			{
				std::cout << "grid still empty, deleting grid and memento\n";
				// posGrid will be deleted automatically due to refcounting
				delete gridMem;
				continue;
			}
			grid = blockMap.emplace(toolGrid.first, posGrid).first;
		}
		else
		{
			// TODO: this creates an unnecessary copy when erasing grind with use_count > 1
			// but we don't know yet if it will be empty
			if (grid->second.use_count() > 1) // need to copy or we modify multiple aggregates
			{
				grid->second = voxelGridPtr_t(new VoxelGrid(*grid->second));
			}
			if (grid->second->applyChanges(*toolGrid.second, gridMem) == 0)
			{
				//std::cout << "grid now empty, erasing from aggregate\n";
				blockMap.erase(toolGrid.first);
			}
		}
		memento->blockMap.emplace(toolGrid.first, gridMementoPtr_t(gridMem));
	}
}

void VoxelAggregate::restoreState(AggregateMemento *memento, std::unordered_set<uint64_t> &changed)
{
	for (auto &memGrid: memento->blockMap)
	{
		if (!memGrid.second.get()->isEmpty())
		{
			blockMap_t::iterator grid = blockMap.find(memGrid.first);
			if (grid == blockMap.end())
			{
				// restore deleted grid; memento will be empty after construction
				IVector3D pos;
				blockPos(memGrid.first, pos);
				voxelGridPtr_t posGrid(new VoxelGrid(pos, memGrid.second.get()));
				grid = blockMap.emplace(memGrid.first, posGrid).first;
				//std::cout << "restored emptied grid; mememto.isEmpty(): " << memGrid.second.get()->isEmpty() << std::endl;
			}
			else
			{
				// this modifies the GridMemento to reflect its previous state!
				grid->second->restoreState(memGrid.second.get());
			}
		}
		// delete grid
		else
		{
			blockMap_t::iterator grid = blockMap.find(memGrid.first);
			if (grid == blockMap.end())
			{
				std::cout << "(!) unexpected erasing of already empty grid!\n";
			}
			else
			{
				// note: we could probably move the state rather than copy,
				// but grid access would be invalid until scene is updated.
				grid->second->saveState(memGrid.second.get());
			}
			blockMap.erase(memGrid.first);
		}
		changed.insert(memGrid.first);
	}
}

void VoxelAggregate::getNeighbours(const IVector3D &gridPos, const VoxelGrid* neighbours[27])
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

bool VoxelAggregate::getBound(IBBox &bound) const
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

void RenderAggregate::updateBlock(QOpenGLFunctions_3_3_Core &glf, uint64_t blockId, const VoxelGrid* grid)
{
	renderBlockMap_t::iterator rgrid = renderBlocks.find(blockId);
	const VoxelGrid* neighbours[27];
	if (rgrid == renderBlocks.end())
	{
		std::cout << "    allocating new RenderGrid" << std::endl;
		rgrid = renderBlocks.emplace(blockId, new RenderGrid).first;
	}
	aggregate->getNeighbours(grid->getGridPos(), neighbours);
	rgrid->second->update(glf, neighbours, options);
}

void RenderAggregate::updateBlockSliced(QOpenGLFunctions_3_3_Core &glf, uint64_t blockId, const VoxelGrid* grid)
{
	renderBlockMap_t::iterator rgrid = renderBlocks.find(blockId);
	// TODO: check if sliced
	const IBBox &bound = grid->getBound();
	if (bound.pMin[options.axis] > options.level || bound.pMax[options.axis] <= options.level)
	{
		if (rgrid != renderBlocks.end())
			rgrid->second->clear(glf);
		return;
	}

	const VoxelGrid* neighbours[27];
	if (rgrid == renderBlocks.end())
	{
		std::cout << "    allocating new RenderGrid" << std::endl;
		rgrid = renderBlocks.emplace(blockId, new RenderGrid).first;
	}
	aggregate->getNeighbours(grid->getGridPos(), neighbours);
	// TODO: sliced update function
	rgrid->second->update(glf, neighbours, options);
}

void RenderAggregate::update(QOpenGLFunctions_3_3_Core &glf, const blockSet_t &dirtyBlocks)
{
	auto updateFunc = &RenderAggregate::updateBlock;
	if (options.mode == RenderOptions::MODE_SLICE)
		updateFunc = &RenderAggregate::updateBlockSliced;

	for (auto &blockId: dirtyBlocks)
	{
		const VoxelGrid* blockGrid = aggregate->getBlock(blockId);
		if (blockGrid)
		{
			(this->*updateFunc)(glf, blockId, blockGrid);
		}
		else
		{
			// does not exist (anymore)
			renderBlockMap_t::iterator rgrid = renderBlocks.find(blockId);
			if (rgrid != renderBlocks.end())
			{
				rgrid->second->cleanupGL(glf);
				delete rgrid->second;
				renderBlocks.erase(rgrid);
			}
		}
	}
}

void RenderAggregate::rebuild(QOpenGLFunctions_3_3_Core &glf, const RenderOptions *opt)
{
	if (opt)
		options = *opt;
	auto updateFunc = &RenderAggregate::updateBlock;
	if (options.mode == RenderOptions::MODE_SLICE)
		updateFunc = &RenderAggregate::updateBlockSliced;

	clear(glf); // TODO: only delete RenderGrids for non-existing VoxelGrids
	const blockMap_t &blocks = aggregate->getBlockMap();
	for (auto block: blocks)
	{
		(this->*updateFunc)(glf, block.first, block.second.get());
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
