/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sceneproxy.h"
#include "voxelscene.h"
#include "voxelaggregate.h"

#include <iostream>

SceneMemento::~SceneMemento()
{
	std::cout << "deleting SceneMemento...\n";
	for (auto layer: sourceLayers)
		delete layer;
	delete memento;
}

SceneProxy::SceneProxy(VoxelScene *scene_, QObject *parent):
	QObject(parent), scene(scene_)
{
	undoState = editHistory.end();
}

void SceneProxy::dropRedoHistory()
{
	if (undoState != editHistory.end())
	{
		std::cout << "deleting outdated redo history\n";
		editHistory.erase(undoState, editHistory.end());
	}
}

void SceneProxy::moveActiveLayer(int layerN)
{
	if (layerN == activeLayer())
	{
		int activeLayerNew = (layerN == layerCount() - 1) ? layerN - 1 : layerN + 1;
		scene->setActiveLayer(activeLayerNew);
		emit(activeLayerChanged(activeLayerNew, layerN));
	}
}

int SceneProxy::activeLayer() const
{
	return scene->activeLayerN;
}

int SceneProxy::layerCount() const
{
	return scene->layers.size();
}

const VoxelLayer* SceneProxy::getLayer(int layerN) const
{
	if (layerN >=0 && layerN < (int)scene->layers.size())
		return scene->layers[layerN];
	return 0;
}

void SceneProxy::completeToolAction()
{
	AggregateMemento *memento = new AggregateMemento;
	scene->applyToolChanges(memento);
	dropRedoHistory();
	editHistory.emplace_back(memento, scene->activeLayerN);
	undoState = editHistory.end();
}

bool SceneProxy::createLayer(int layerN)
{
	if (layerN < -1 || layerN > int(scene->layers.size()))
		return false;

	if (layerN == -1)
		layerN = scene->layers.size();
	//else
		// [TODO: implement] should work now
		//return false;

	VoxelLayer* newLayer = new VoxelLayer;
	newLayer->name = "New Layer";
	newLayer->aggregate = new VoxelAggregate();
	scene->insertLayer(newLayer, layerN);
	dropRedoHistory();
	editHistory.emplace_back(SceneMemento());
	SceneMemento &memento = editHistory.back();
	memento.action = SceneMemento::ADD_LAYER;
	//memento.sourceLayers.push_back(newLayer); // ??
	memento.targetLayerIndex = layerN;
	undoState = editHistory.end();

	emit(layerCreated(layerN));
	return true;
}

bool SceneProxy::insertLayer(VoxelLayer *layer, int layerN)
{
	if (layerN < -1 || layerN > layerCount() || !layer->aggregate)
		return false;

	if (layerN == -1)
		layerN = scene->layers.size();

	scene->insertLayer(layer, layerN);
	dropRedoHistory();
	editHistory.emplace_back(SceneMemento());
	SceneMemento &memento = editHistory.back();
	memento.action = SceneMemento::ADD_LAYER;
	memento.targetLayerIndex = layerN;
	undoState = editHistory.end();

	emit(layerCreated(layerN));
	emit(renderDataChanged());
	return true;
}

bool SceneProxy::deleteLayer(int layerN)
{
	if (scene->layers.size() < 2 || layerN < 0 || layerN >= int(scene->layers.size()))
		return false;

	moveActiveLayer(layerN);
	VoxelLayer *removed = scene->removeLayer(layerN);
	dropRedoHistory();
	editHistory.emplace_back(SceneMemento());
	SceneMemento &memento = editHistory.back();
	memento.action = SceneMemento::DELETE_LAYER;
	memento.sourceLayers.push_back(removed);
	memento.targetLayerIndex = layerN;
	undoState = editHistory.end();

	emit(layerDeleted(layerN));
	emit(renderDataChanged());
	return true;
}

bool SceneProxy::setActiveLayer(int layerN)
{
	if (layerN < 0 || layerN >= (int)scene->layers.size())
		return false;
	if (layerN != activeLayer())
	{
		int prev = activeLayer();
		scene->setActiveLayer(layerN);
		emit(activeLayerChanged(layerN, prev));
	}
	return true;
}

bool SceneProxy::setLayerBound(int layerN, const IBBox &bound)
{
	if (layerN < 0 || layerN >= (int)scene->layers.size())
		return false;
	scene->layers[layerN]->bound = bound;
	emit(layerSettingsChanged(layerN, VoxelLayer::BOUND_CHANGED));
	return true;
}

bool SceneProxy::setLayerBoundUse(int layerN, bool enabled)
{
	if (layerN < 0 || layerN >= (int)scene->layers.size())
		return false;
	scene->layers[layerN]->useBound = enabled;
	emit(layerSettingsChanged(layerN, VoxelLayer::USE_BOUND_CHANGED));
	return true;
}

bool SceneProxy::setLayerVisibility(int layerN, bool visible)
{
	if (layerN < 0 || layerN >= (int)scene->layers.size())
		return false;
	scene->layers[layerN]->visible = visible;
	emit(layerSettingsChanged(layerN, VoxelLayer::VISIBILITY_CHANGED));
	return true;
}

bool SceneProxy::renameLayer(int layerN, const std::string &name)
{
	if (layerN < 0 || layerN >= (int)scene->layers.size())
		return false;
	scene->layers[layerN]->name = name;
	emit(layerSettingsChanged(layerN, VoxelLayer::NAME_CHANGED));
	return true;
}

void SceneProxy::setTemplateColor(rgba_t col)
{
	if (col != scene->voxelTemplate.col)
	{
		scene->voxelTemplate.col = col;
		emit(templateColorChanged(scene->voxelTemplate.col));
	}
}

void SceneProxy::undo()
{
	// on empty list, begin() == end()
	if (undoState == editHistory.begin())
		return;
	// undoState points one beyond current state, i.e. the first redo state
	--undoState;
	SceneMemento &state = *undoState;
	switch (state.action)
	{
		case SceneMemento::EDIT_VOXELS:
			scene->restoreAggregate(scene->layers[state.targetLayerIndex], state.memento);
			break;
		case SceneMemento::ADD_LAYER:
			state.sourceLayers.push_back(scene->layers[state.targetLayerIndex]);
			moveActiveLayer(state.targetLayerIndex);
			scene->removeLayer(state.targetLayerIndex);
			emit(layerDeleted(state.targetLayerIndex));
			break;
		case SceneMemento::DELETE_LAYER:
			scene->insertLayer(state.sourceLayers.back(), state.targetLayerIndex);
			state.sourceLayers.pop_back();
			emit(layerCreated(state.targetLayerIndex));
			break;
		case SceneMemento::EDIT_LAYER:
			// TODO
			break;
		case SceneMemento::INVALID_ACTION:
			std::cout << "Invalid undo state!\n";
	}
	emit(renderDataChanged());
}

void SceneProxy::redo()
{
	if (undoState == editHistory.end())
		return;
	SceneMemento &state = *undoState;
	switch (state.action)
	{
		case SceneMemento::EDIT_VOXELS:
			scene->restoreAggregate(scene->layers[state.targetLayerIndex], state.memento);
			break;
		case SceneMemento::ADD_LAYER:
			scene->insertLayer(state.sourceLayers.back(), state.targetLayerIndex);
			state.sourceLayers.pop_back();
			emit(layerCreated(state.targetLayerIndex));
			break;
		case SceneMemento::DELETE_LAYER:
			state.sourceLayers.push_back(scene->layers[state.targetLayerIndex]);
			moveActiveLayer(state.targetLayerIndex);
			scene->removeLayer(state.targetLayerIndex);
			emit(layerDeleted(state.targetLayerIndex));
			break;
		case SceneMemento::EDIT_LAYER:
			// TODO
			break;
		case SceneMemento::INVALID_ACTION:
			std::cout << "Invalid redo state!\n";
	}
	// undoState points one beyond current state, i.e. the first redo state
	++undoState;
	emit(renderDataChanged());
}
