/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELGRID_H
#define VG_VOXELGRID_H

#include "voxelgem.h"
#include "renderobject.h"

#include <cfloat>
#include <vector>

#define GRID_LEN 16 // must be power of two
#define LOG_GRID_LEN 4 // must be ld(GRID_LEN)

class GridMemento
{
	friend class VoxelGrid;
	std::vector<VoxelEntry> voxels;
};

class VoxelGrid
{
	public:
		VoxelGrid(const IVector3D &pos);
		VoxelGrid(const VoxelGrid &other);
		virtual ~VoxelGrid();
		inline int voxelIndex(int x, int y, int z) const
		{
			return x + y * GRID_LEN + z * GRID_LEN * GRID_LEN;
		}
		void setVoxel(int x, int y, int z, const VoxelEntry &voxel)
		{
			voxels[voxelIndex(x, y, z)] = voxel;
		}
		const VoxelEntry* getVoxel(const IVector3D &pos) const
		{
			return &voxels[voxelIndex(pos.x, pos.y, pos.z)];
		}
		int posToVoxel(float pos, int axis) const
		{
			int val = pos - bound.pMin[axis];
			return val < 0 ? 0 : (val > GRID_LEN - 1 ? GRID_LEN - 1 : val);
		}
		const IVector3D& getGridPos() const { return gridPos; }
		const IBBox& getBound() const { return bound; }
		float voxelEdge(int pos, int axis) const { return bound.pMin[axis] + (float)pos; }
		bool rayIntersect(const ray_t &ray, SceneRayHit &hit) const;
		void merge(const VoxelGrid &topLayer, VoxelGrid *targetGrid = 0);
		void applyChanges(const VoxelGrid &toolLayer, GridMemento *memento);
		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(GridMemento *memento);
		void tesselate(GlVoxelVertex_t *vertices, int nTris[2], const VoxelGrid* neighbourGrids[27]) const;
	protected:
		int writeFaces(const VoxelEntry &entry, uint8_t matIndex, int mask, int pos[3], GlVoxelVertex_t *vertices) const;
		std::vector<int> getNeighbourMasks(const VoxelGrid* neighbourGrids[27]) const;
		IBBox bound;
		IVector3D gridPos; // TODO: redundant right now, use bound.pMin?
		std::vector<VoxelEntry> voxels;
};


class RenderGrid: public GLRenderable
{
	public:
		void setup(QOpenGLFunctions_3_3_Core &glf) override;
		void cleanupGL(QOpenGLFunctions_3_3_Core &glf);
		/*! @param neighbourGrids: neighbourGrids[13] is the center to generate the mesh from */
		void update(QOpenGLFunctions_3_3_Core &glf, const VoxelGrid* neighbourGrids[27]);
		void render(QOpenGLFunctions_3_3_Core &glf) override;
		void renderTransparent(QOpenGLFunctions_3_3_Core &glf);
	protected:
		int nTessTris[2];
};

#endif // VG_VOXELGRID_H
