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

class BBox
{
	public:
		BBox(const IVector3D &min, const IVector3D &max): pMin(min), pMax(max) {}
		BBox(const BBox &other): pMin(other.pMin), pMax(other.pMax) {}
		bool includes(const IVector3D &p) const
		{
			return ((p.x >= pMin.x) && (p.x <= pMax.x) &&
					(p.y >= pMin.y) && (p.y <= pMax.y) &&
					(p.z >= pMin.z) && (p.z <= pMax.z));
		}
		bool includes(const BBox &b) const
		{
			return ((b.pMin.x >= pMin.x) && (b.pMax.x <= pMax.x) &&
					(b.pMin.y >= pMin.y) && (b.pMax.y <= pMax.y) &&
					(b.pMin.z >= pMin.z) && (b.pMax.z <= pMax.z));
		}
		void join(const BBox &b)
		{
			for (int i = 0; i < 3; ++i)
			{
				pMin[i] = std::min(pMin[i], b.pMin[i]);
				pMax[i] = std::max(pMax[i], b.pMax[i]);
			}
		}
		bool rayIntersect(const ray_t &ray, intersect_t *hit = 0) const
		{
			intersect_t h(-FLT_MAX, FLT_MAX);
			for (int i = 0; i < 3; ++i)
			{
				int axisDirFlag = 0;
				float invDir = 1.f / ray.dir[i];
				float tNear = invDir * ((float)pMin[i] - ray.from[i]);
				float tFar = invDir * ((float)pMax[i] - ray.from[i]);
				if (tNear > tFar)
				{
					std::swap(tNear, tFar);
					axisDirFlag = intersect_t::AXIS_NEGATIVE;
				}
				if (tNear > h.tNear)
				{
					h.tNear = tNear;
					h.entryAxis = i | axisDirFlag;
				}
				if (tFar < h.tFar) h.tFar = tFar;
				if (h.tNear > h.tFar) return false;
			}
			if (hit) *hit = h;
			// valid hit when interval [tNear, tFar] overlaps ray interval [t_min, t_max]
			return ray.t_max >= h.tNear && h.tFar >= ray.t_min;
		}
		// temporary
		operator IBBox() const { return IBBox(pMin, pMax); }
		IVector3D pMin, pMax;
};

class GridMemento
{
	friend class VoxelGrid;
	std::vector<VoxelEntry> voxels;
};

class VoxelGrid
{
	public:
		VoxelGrid(const int pos[3]);
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
		const VoxelEntry* getVoxel(int pos[3]) const
		{
			return &voxels[voxelIndex(pos[0], pos[1], pos[2])];
		}
		int posToVoxel(float pos, int axis) const
		{
			int val = pos - bound.pMin[axis];
			return val < 0 ? 0 : (val > GRID_LEN - 1 ? GRID_LEN - 1 : val);
		}
		const int* getGridPos() const { return gridPos; }
		const BBox& getBound() const { return bound; }
		float voxelEdge(int pos, int axis) const { return bound.pMin[axis] + (float)pos; }
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
		void merge(const VoxelGrid &topLayer, VoxelGrid *targetGrid = 0);
		void applyChanges(const VoxelGrid &toolLayer, GridMemento *memento);
		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(GridMemento *memento);
		void tesselate(GlVoxelVertex_t *vertices, int nTris[2], const VoxelGrid* neighbourGrids[27]) const;
	protected:
		int writeFaces(const VoxelEntry &entry, uint8_t matIndex, int mask, int pos[3], GlVoxelVertex_t *vertices) const;
		std::vector<int> getNeighbourMasks(const VoxelGrid* neighbourGrids[27]) const;
		BBox bound;
		int gridPos[3];
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
