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

VoxelScene::VoxelScene(): viewport(0), voxelTemplate(128, 128, 255, 255), activeLayerN(0), dirty(true)
{
	// test
	editingLayer = new VoxelLayer;
	editingLayer->aggregate = new VoxelAggregate();
	//editingLayer->renderAg = new RenderAggregate(editingLayer->aggregate);
	editingLayer->name = "Default Layer";
	layers.push_back(editingLayer);
	renderLayer = new VoxelLayer;
	renderLayer->aggregate = new VoxelAggregate();
	editingLayer->renderAg = new RenderAggregate(renderLayer->aggregate);
	renderLayer->renderAg = editingLayer->renderAg; // note: only for independent layer mode
	//
	//renderLayer = new VoxelAggregate();
	//editingLayer = new VoxelAggregate();
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

void VoxelScene::setActiveLayer(int layerN)
{
	// TODO: make sure pending tool changes are handled properly
	// TODO: handler layer compositing mode
	// Reset current active editing layer to its own aggregate, and the new active to renderLayer
	editingLayer->renderAg->setAggregate(editingLayer->aggregate);
	editingLayer = layers[layerN];
	editingLayer->renderAg->setAggregate(renderLayer->aggregate);
	// render layer aggregate needs to be set to editing layer (shallow copy)
	renderLayer->aggregate->clone(*editingLayer->aggregate);
	activeLayerN = layerN;
}

void VoxelScene::setVoxel(const int pos[3], const VoxelEntry &voxel)
{
	uint64_t blockId = toolLayer->setVoxel(pos[0], pos[1], pos[2], voxel);
	//changedBlocks.insert(blockId);
	editingLayer->dirtyVolumes[blockId].addPosition(pos);
	dirty = true;
}

void VoxelScene::eraseVoxel(const int pos[3])
{
	uint64_t blockId = toolLayer->setVoxel(pos[0], pos[1], pos[2], VoxelEntry(0, Voxel::VF_ERASED));
	//changedBlocks.insert(blockId);
	editingLayer->dirtyVolumes[blockId].addPosition(pos);
	dirty = true;
}

// TODO!
const VoxelEntry* VoxelScene::getVoxel(const int pos[3])
{
	return 0;
}

const VoxelAggregate* VoxelScene::getAggregate(int layer)
{
	if (layer == 0)
		return toolLayer;
	if (layer == 1)
		return editingLayer->aggregate;
	return 0;
}

void VoxelScene::completeToolAction()
{
	// rendering should not be affected by this as it effectively only transfers data
	// VoxelScene::update() will happen in editing updates, one more after this call, most likely
	// allocate undo memento:
	AggregateMemento *memento = new AggregateMemento;
	// TODO: for each visible scene layer instead of single editing layer
	editingLayer->aggregate->applyChanges(*toolLayer, memento);
	if (undoState != undoList.end())
	{
		std::cout << "deleting outdated redo history\n";
		undoList.erase(undoState, undoList.end());
	}
	undoList.emplace_back(UndoItem(memento));
	undoState = undoList.end();
	toolLayer->clear();
}

void VoxelScene::applyToolChanges(AggregateMemento *memento)
{
	// rendering should not be affected by this as it effectively only transfers data
	// VoxelScene::update() will happen in editing updates, one more after this call, most likely
	editingLayer->aggregate->applyChanges(*toolLayer, memento);
	toolLayer->clear();
}

void VoxelScene::update()
{
	renderLayer->aggregate->clearBlocks(editingLayer->dirtyVolumes/* changedBlocks */);
	// TODO: for each visible scene layer instead of single editing layer
	renderLayer->aggregate->merge(*editingLayer->aggregate, editingLayer->dirtyVolumes/* changedBlocks */);
	renderLayer->aggregate->merge(*toolLayer, editingLayer->dirtyVolumes/* changedBlocks */);
	std::cout << "VoxelScene::update() editing layer block count:" << editingLayer->aggregate->blockCount() << std::endl;
	//changedBlocks.clear();
}

void VoxelScene::render(QOpenGLFunctions_3_3_Core &glf)
{
	// TODO: split updating from rendering
	for (auto &layer: layers)
	{
		if (layer->visible)
		{
			if (!layer->dirtyVolumes.empty())
			{
				blockSet_t dirtyBlocks;
				// convert dirty volumes to dirty blocks
				for (auto &volume: layer->dirtyVolumes)
				{
					VoxelAggregate::markDirtyBlocks(volume.second, dirtyBlocks);
				}
				layer->renderAg->update(glf, dirtyBlocks);
				layer->dirtyVolumes.clear();
			}
			layer->renderAg->render(glf);
		}
	}
	dirty = false;
	// TODO: probably should not be rendered here but after all opaque things
	glf.glEnable(GL_BLEND);
    glf.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (auto &layer: layers)
		if (layer->visible)
			layer->renderAg->renderTransparent(glf);

	glf.glDisable(GL_BLEND);
	//dirtyBlocks.clear();
}

bool VoxelScene::rayIntersect(const ray_t &ray, SceneRayHit &hit, int flags) const
{
	bool didHit = false;
	intersect_t hitInfo;
	if (flags & SceneRayHit::HIT_VOXEL)
	{
		didHit = editingLayer->aggregate->rayIntersect(ray, hit.voxelPos, hitInfo);
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
	/* // TODO: determine layer of memento
	// TODO: get the dirty blocks caused by changedBlocks
	editingLayer->aggregate->restoreState(state.getMemento(), changedBlocks);
	// convert changed blocks to dirty volumes, can't recover one (yet?) unfortunately
	for (auto &block: changedBlocks)
	{
		DirtyVolume vol;
		VoxelAggregate::blockPos(block, vol.low);
		for (int i = 0; i < 3; ++i)
			vol.high[i] = vol.low[i] + GRID_LEN - 1;
		vol.valid = true;
		dirtyVolumes[block] = vol;
	} */
	// temp fix! remove function...
	restoreAggregate(editingLayer, state.getMemento());
}

void VoxelScene::restoreAggregate(VoxelLayer *layer, AggregateMemento *memento)
{
	blockSet_t changed;
	layer->aggregate->restoreState(memento, changed);
	// convert changed blocks to dirty volumes, can't recover one (yet?) unfortunately
	for (auto &block: changed)
	{
		DirtyVolume vol;
		VoxelAggregate::blockPos(block, vol.low);
		for (int i = 0; i < 3; ++i)
			vol.high[i] = vol.low[i] + GRID_LEN - 1;
		vol.valid = true;
		layer->dirtyVolumes[block] = vol;
	}
	dirty = true;
}
