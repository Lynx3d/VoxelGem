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

#include <QVector3D>

#include <cfloat>
#include <vector>

#define GRID_LEN 16 // must be power of two
#define LOG_GRID_LEN 4 // must be ld(GRID_LEN)

class BBox
{
	public:
		BBox(const QVector3D &min, const QVector3D &max): pMin(min), pMax(max) {}
		BBox(const BBox &other): pMin(other.pMin), pMax(other.pMax) {}
		bool includes(const QVector3D &p) const
		{
			return ( (p.x() >= pMin.x()) && (p.x() <= pMax.x()) &&
					(p.y() >= pMin.y()) && (p.y() <= pMax.y()) &&
					(p.z() >= pMin.z()) && (p.z() <= pMax.z()) );
		}
		bool rayIntersect(const ray_t &ray, intersect_t *hit = 0) const
		{
			intersect_t h(-FLT_MAX, FLT_MAX);
			for (int i = 0; i < 3; ++i)
			{
				int axisDirFlag = 0;
				float invDir = 1.f / ray.dir[i];
				float tNear = invDir * (pMin[i] - ray.from[i]);
				float tFar = invDir * (pMax[i] - ray.from[i]);
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
		QVector3D pMin, pMax;
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
		int posToVoxel(float pos, int axis) const
		{
			int val = pos - bound.pMin[axis];
			return val < 0 ? 0 : (val > GRID_LEN - 1 ? GRID_LEN - 1 : val);
		}
		const int* getGridPos() const { return gridPos; }
		float voxelEdge(int pos, int axis) const { return bound.pMin[axis] + (float)pos; }
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
		void merge(const VoxelGrid &topLayer, VoxelGrid *targetGrid = 0);
		void applyChanges(const VoxelGrid &toolLayer, GridMemento *memento);
		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(GridMemento *memento);
		int tesselate(GlVoxelVertex_t *vertices, const VoxelGrid* neighbourGrids[27]) const;
	protected:
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
	protected:
		int nTessTris;
};

#endif // VG_VOXELGRID_H
