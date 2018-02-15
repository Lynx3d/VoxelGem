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
class RenderGrid;
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

/*! This class holds all the "scene" data for a file edit session.
	Includes viewport and various UI information required for
	editing tools to work on the scene.
*/

class VoxelScene
{
	friend class GlViewportWidget;
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
		bool needsUpdate() const { return !changedBlocks.empty(); }
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
		void restoreState(UndoItem &state);
		GlViewportWidget *viewport;
		VoxelAggregate *renderLayer;
		VoxelAggregate *editingLayer;
		VoxelAggregate *toolLayer;
		VoxelEntry voxelTemplate;
		std::unordered_map<uint64_t, RenderGrid*> renderBlocks;
		std::unordered_set<uint64_t> changedBlocks;
		std::unordered_set<uint64_t> dirtyBlocks;
		dirtyMap_t dirtyVolumes;
		std::list<UndoItem> undoList;
		std::list<UndoItem>::iterator undoState;
};

#endif // VG_VOXELSCENE_H
