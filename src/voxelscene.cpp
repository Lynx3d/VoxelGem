/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelscene.h"
#include "voxelaggregate.h"
#include "glviewport.h"

#include <iostream>

UndoItem::~UndoItem()
{
	if (memento)
		std::cout << "deleting memento!" << std::endl;
	delete memento;
}

VoxelScene::VoxelScene(): viewport(0), voxelTemplate(128, 128, 255, 255)
{
	renderLayer = new VoxelAggregate();
	editingLayer = new VoxelAggregate();
	toolLayer = new VoxelAggregate();
	undoState = undoList.end();
}

VoxelScene::~VoxelScene()
{
	delete renderLayer;
	delete editingLayer;
	delete toolLayer;
}

/* TODO:
	So far we only have one editing layer plus a tool layer, merged to a rendering layer.
*/

void VoxelScene::setVoxel(const int pos[3], const VoxelEntry &voxel)
{
	uint64_t blockId = toolLayer->setVoxel(pos[0], pos[1], pos[2], voxel);
	changedBlocks.insert(blockId);
	dirtyVolumes[blockId].addPosition(pos);
}

void VoxelScene::eraseVoxel(const int pos[3])
{
	uint64_t blockId = toolLayer->setVoxel(pos[0], pos[1], pos[2], VoxelEntry(0, Voxel::VF_ERASED));
	changedBlocks.insert(blockId);
	dirtyVolumes[blockId].addPosition(pos);
}

// TODO!
const VoxelEntry* VoxelScene::getVoxel(const int pos[3])
{
	return 0;
}

void VoxelScene::completeToolAction()
{
	// rendering should not be affected by this as it effectively only transfers data
	// VoxelScene::update() will happen in editing updates, one more after this call, most likely
	// allocate undo memento:
	AggregateMemento *memento = new AggregateMemento;
	// TODO: for each visible scene layer instead of single editing layer
	editingLayer->applyChanges(*toolLayer, memento);
	if (undoState != undoList.end())
	{
		std::cout << "deleting outdated redo history\n";
		undoList.erase(undoState, undoList.end());
	}
	undoList.emplace_back(UndoItem(memento));
	undoState = undoList.end();
	toolLayer->clear();
}

void VoxelScene::update()
{
	renderLayer->clearBlocks(changedBlocks);
	// TODO: for each visible scene layer instead of single editing layer
	renderLayer->merge(*editingLayer, changedBlocks);
	renderLayer->merge(*toolLayer, changedBlocks);
	std::cout << "VoxelScene::update() editing layer block count:" << editingLayer->blockCount() << std::endl;
	changedBlocks.clear();
}

void VoxelScene::render(QOpenGLFunctions_3_3_Core &glf)
{
	// convert dirty volumes to dirty blocks
	for (auto &volume: dirtyVolumes)
	{
		VoxelAggregate::markDirtyBlocks(volume.second, dirtyBlocks);
	}
	dirtyVolumes.clear();
	//std::cout << "VoxelScene::render()" << std::endl;
	for (auto &blockId: dirtyBlocks)
	{
		std::cout << "dirty block: " << blockId << std::endl;
		const VoxelGrid* blockGrid = renderLayer->getBlock(blockId);
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
			renderLayer->getNeighbours(blockGrid->getGridPos(), neighbours);
			rgrid->second->update(glf, neighbours);
		}
		else
		{
			std::cout << "    block is empty." << std::endl;
			// does not exist (anymore)
			if (rgrid != renderBlocks.end())
			{
				rgrid->second->cleanupGL(glf);
				delete rgrid->second;
				renderBlocks.erase(rgrid);
			}
		}
	}
	for (auto &block: renderBlocks)
	{
		//std::cout << "redering block: " << block.first << std::endl;
		block.second->render(glf);
	}
	dirtyBlocks.clear();
}

bool VoxelScene::rayIntersect(const ray_t &ray, SceneRayHit &hit, int flags) const
{
	bool didHit = false;
	intersect_t hitInfo;
	if (flags & SceneRayHit::HIT_VOXEL)
	{
		didHit = editingLayer->rayIntersect(ray, hit.voxelPos, hitInfo);
		if (didHit)
			hit.flags |= SceneRayHit::HIT_VOXEL;
	}
	// TODO: currently it's purely a fallback, not a test which is hit first
	// tools probably should query separately if they want a fallback
	if (!didHit && (flags & SceneRayHit::HIT_LINEGRID))
	{
		didHit = viewport->getGrid()->rayIntersect(ray, hit.voxelPos, hitInfo);
		if (didHit)
			hit.flags |= SceneRayHit::HIT_LINEGRID;
	}
	if (didHit)
	{
		hit.flags |= hitInfo.entryAxis;
		hit.rayT = hitInfo.tNear;
	}
	return didHit;
}

void VoxelScene::undo()
{
	// on empty list, begin() == end()
	if (undoState != undoList.begin())
	{
		// undoState always points to the element past the last saved state.
		// i.e. in the case of no available redo, it will be undoList.end()
		--undoState;
		restoreState(*undoState);
	}
}

void VoxelScene::redo()
{
	if (undoState != undoList.end())
	{
		restoreState(*undoState);
		++undoState;
	}
}

void VoxelScene::restoreState(UndoItem &state)
{
	// TODO: determine layer of memento
	// TODO: get the dirty blocks caused by changedBlocks
	editingLayer->restoreState(state.getMemento(), changedBlocks);
	// convert changed blocks to dirty volumes, can't recover one (yet?) unfortunately
	for (auto &block: changedBlocks)
	{
		DirtyVolume vol;
		VoxelAggregate::blockPos(block, vol.low);
		for (int i = 0; i < 3; ++i)
			vol.high[i] = vol.low[i] + GRID_LEN - 1;
		vol.valid = true;
		dirtyVolumes[block] = vol;
	}
}
