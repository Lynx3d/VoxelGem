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

typedef std::unordered_map<uint64_t, DirtyVolume> dirtyMap_t;

class VoxelLayer
{
	public:
		enum ChangeFlags
		{
			NAME_CHANGED 		= 1,
			BOUND_CHANGED 		= 1 << 1,
			USE_BOUND_CHANGED 	= 1 << 2,
			VISIBILITY_CHANGED 	= 1 << 3,
			ALL_CHANGED 		= 15
		};
		VoxelLayer(): aggregate(0), visible(true), useBound(false), bound(IVector3D(0,0,0), IVector3D(16,16,16)) {}
		~VoxelLayer();
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
		void setVoxel(const IVector3D &pos, const VoxelEntry &voxel);
		/* mark a voxel as erased in the editing layer */
		void eraseVoxel(const IVector3D &pos);
		/* read a voxel from the scene (exclude current edit changes) */
		const VoxelEntry* getVoxel(const IVector3D &pos);
		const VoxelEntry* getVoxelTemplate() const { return &voxelTemplate; }
		/* probably an iterator/accessor class would be better */
		const VoxelAggregate* getAggregate(int layer);
		bool needsUpdate() const { return dirty; }
		void setTemplateColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		{
			voxelTemplate.col = rgba_t(r, g, b, a);
		}
		void setTemplateMaterial(Voxel::Material mat) { voxelTemplate.setMaterial(mat); }
		void setTemplateSpecular(Voxel::Specular spec) { voxelTemplate.setSpecular(spec); }
		void update();
		void render(QOpenGLFunctions_3_3_Core &glf);
		bool rayIntersect(const ray_t &ray, SceneRayHit &hit, int flags = SceneRayHit::HIT_MASK) const;
	protected:
		void applyToolChanges(AggregateMemento *memento);
		void insertLayer(VoxelLayer *layer, int layerN);
		VoxelLayer* removeLayer(int layerN);
		void restoreAggregate(VoxelLayer *layer, AggregateMemento *memento);
		VoxelAggregate* replaceAggregate(int layerN, VoxelAggregate *aggregate);
		void setActiveLayer(int layerN);
		GlViewportWidget *viewport;
		VoxelLayer *renderLayer;
		VoxelLayer *editingLayer;
		VoxelAggregate *toolLayer;
		std::vector<VoxelLayer*> layers;
		std::vector<RenderAggregate*> removedRAg;
		VoxelEntry voxelTemplate;
		int activeLayerN;
		bool dirty;
};

#endif // VG_VOXELSCENE_H
