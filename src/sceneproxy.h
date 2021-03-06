/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_SCENEPROXY_H
#define VG_SCENEPROXY_H

#include <QObject>

#include "voxelgem.h"

#include <list>

class IBBox;
class VoxelScene;
class VoxelLayer;
class VoxelAggregate;
class AggregateMemento;

class SceneMemento
{
	public:
		enum Action
		{
			EDIT_VOXELS,
			ADD_LAYER,
			DELETE_LAYER,
			MOVE_LAYER,
			EDIT_LAYER,
			REPLACE_AGGREGATE,
			INVALID_ACTION
		};
		SceneMemento(): action(INVALID_ACTION), memento(0) {}
		SceneMemento(AggregateMemento *mem, unsigned int layer):
			action(EDIT_VOXELS), targetLayerIndex(layer), memento(mem) {}
		~SceneMemento();
	//protected:
		Action action;
		//bool redoAction = false; // redundant?
		std::vector<VoxelLayer*> sourceLayers; // for merge operations
		unsigned int sourceLayerIndex;
		unsigned int targetLayerIndex;
		//VoxelLayer *targetLayer; // redundant?
		AggregateMemento *memento;
};

/* This class handles all the Qt signals and slots associated with scene editing.
   The scene should always be manipulated through the proxy when it affects any UI element. */

class SceneProxy: public QObject
{
	Q_OBJECT
	public:
		SceneProxy(VoxelScene *scene, QObject *parent = 0);
		int activeLayer() const;
		int layerCount() const;
		const VoxelLayer* getLayer(int layerN) const;
		/*! use with care! */
		VoxelScene* getScene() { return scene; }
		void completeToolAction();
		bool createLayer(int layerN = -1);
		bool insertLayer(VoxelLayer *layer, int layerN = -1);
		bool deleteLayer(int layerN);
		bool moveLayer(int layerN, int targetN);
		bool mergeLayers(int source, int target);
		bool setActiveLayer(int layerN);
		bool setLayerBound(int layerN, const IBBox &bound);
		bool setLayerBoundUse(int layerN, bool enabled);
		bool setLayerVisibility(int layerN, bool visible);
		bool renameLayer(int layerN, const std::string &name);
		bool replaceAggregate(int layerN, VoxelAggregate *aggregate);
		void setTemplateColor(rgba_t col);
		void undo();
		void redo();
	Q_SIGNALS:
		void layerDeleted(int layerN);
		void layerCreated(int layerN);
		void layerMoved(int layerN, int targetN);
		void layerSettingsChanged(int layerN, int change_flags);
		void activeLayerChanged(int layerN, int prev);
		void templateColorChanged(rgba_t col);
		void renderDataChanged();
	protected:
		void dropRedoHistory();
		void moveActiveLayer(int layerN);
		VoxelScene *scene;
		std::list<SceneMemento> editHistory;
		std::list<SceneMemento>::iterator undoState;
};

#endif // VG_SCENEPROXY_H
