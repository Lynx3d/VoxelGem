/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELSCENE_H
#define VG_VOXELSCENE_H

#include "voxelgem.h"

#include <unordered_map>
#include <unordered_set>
#include <list>

class VoxelEntry;
class VoxelAggregate;
class AggregateMemento;
class RenderAggregate;
class RenderGrid;
class SceneProxy;
class GlViewportWidget;
class QOpenGLFunctions_3_3_Core;

class SceneRayHit
{
	public:
		enum
		{
			AXIS_MASK = 0x3,
			AXIS_NEGATIVE = 0x4,
			HIT_VOXEL = 0x100,
			HIT_LINEGRID = 0x200,
			HIT_HANDLE = 0x400,
			HIT_MASK = 0x700
		};
		bool didHit() const { return (flags & HIT_MASK) != 0; }
		bool getAdjacentVoxel(int pos[3]) const
		{
			if ((flags & SceneRayHit::HIT_MASK) == 0)
				return false;
			pos[0] = voxelPos[0];
			pos[1] = voxelPos[1];
			pos[2] = voxelPos[2];
			// TODO: invert meaning of SceneRayHit::AXIS_NEGATIVE
			pos[flags & SceneRayHit::AXIS_MASK] += (flags & SceneRayHit::AXIS_NEGATIVE) ? 1 : -1;
			return true;
		}
		int voxelPos[3];
		int flags { 0 };
		float rayT;
};

class UndoItem
{
	public:
		UndoItem(AggregateMemento *mem): memento(mem) {}
		UndoItem(UndoItem &&other): memento(other.memento) { other.memento = 0; }
		~UndoItem();
		AggregateMemento* getMemento() { return memento; }
	protected:
		AggregateMemento *memento {0};
};

typedef std::unordered_map<uint64_t, DirtyVolume> dirtyMap_t;

class VoxelLayer
{
	public:
		VoxelLayer(): aggregate(0), visible(true), useBound(false), bound(IVector3D(0,0,0), IVector3D(16,16,16)) {}
		VoxelAggregate *aggregate;
		bool visible;
		bool useBound;
		IBBox bound;
		std::string name;
		// rendering
		RenderAggregate *renderAg = 0;
		bool renderInitialized = false;
		dirtyMap_t dirtyVolumes;
};

class SceneMemento
{
	public:
		enum Action
		{
			EDIT_VOXELS,
			ADD_LAYER,
			DELETE_LAYER,
			EDIT_LAYER,
			INVALID_ACTION
		};
		SceneMemento(): action(INVALID_ACTION), memento(0) {}
		SceneMemento(AggregateMemento *mem, unsigned int layer):
			action(EDIT_VOXELS), targetLayerIndex(layer), memento(mem) {}
	//protected:
		Action action;
		bool redoAction = false; // redundant?
		std::vector<VoxelLayer*> sourceLayers; // for merge operations
		unsigned int targetLayerIndex;
		VoxelLayer *targetLayer; // redundant?
		AggregateMemento *memento;
};

/*! This class holds all the "scene" data for a file edit session.
	Includes viewport and various UI information required for
	editing tools to work on the scene.
*/

class VoxelScene
{
	friend class GlViewportWidget;
	friend class SceneProxy;
	public:
		VoxelScene();
		~VoxelScene();
		/* set a voxel of the editing layer */
		void setVoxel(const int pos[3], const VoxelEntry &voxel);
		/* mark a voxel as erased in the editing layer */
		void eraseVoxel(const int pos[3]);
		/* read a voxel from the scene (exclude current edit changes) */
		const VoxelEntry* getVoxel(const int pos[3]);
		const VoxelEntry* getVoxelTemplate() const { return &voxelTemplate; }
		/* probably an iterator/accessor class would be better */
		const VoxelAggregate* getAggregate(int layer);
		bool needsUpdate() const { return dirty; }
		void setTemplateColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		{
			voxelTemplate.col[0] = r;
			voxelTemplate.col[1] = g;
			voxelTemplate.col[2] = b;
			voxelTemplate.col[3] = a;
		}
		void setTemplateMaterial(Voxel::Material mat) { voxelTemplate.setMaterial(mat); }
		void setTemplateSpecular(Voxel::Specular spec) { voxelTemplate.setSpecular(spec); }
		void completeToolAction();
		void update();
		void render(QOpenGLFunctions_3_3_Core &glf);
		void undo();
		void redo();
		bool rayIntersect(const ray_t &ray, SceneRayHit &hit, int flags = SceneRayHit::HIT_MASK) const;
	protected:
		void applyToolChanges(AggregateMemento *memento);
		void insertLayer(VoxelLayer *layer, int layerN);
		VoxelLayer* removeLayer(int layerN);
		void restoreState(UndoItem &state);
		void restoreAggregate(VoxelLayer *layer, AggregateMemento *memento);
		void setActiveLayer(int layerN);
		GlViewportWidget *viewport;
		VoxelLayer *renderLayer;
		VoxelLayer *editingLayer;
		VoxelAggregate *toolLayer;
		std::vector<VoxelLayer*> layers;
		std::vector<RenderAggregate*> removedRAg;
		VoxelEntry voxelTemplate;
		std::list<UndoItem> undoList;
		std::list<UndoItem>::iterator undoState;
		int activeLayerN;
		bool dirty;
};

#endif // VG_VOXELSCENE_H
