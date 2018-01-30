/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELGRID_H
#define VG_VOXELGRID_H

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

enum VoxelFlags
{
	VF_NON_EMPTY = 1
};

class GridEntry
{
	public:
		unsigned char col[4];
		unsigned int flags;
};

class VoxelGrid: public GLRenderable
{
	public:
		VoxelGrid(const int pos[3]);
		void setup(QOpenGLFunctions_3_3_Core &glf);
		void render(QOpenGLFunctions_3_3_Core &glf);
		inline int voxelIndex(int x, int y, int z) const
		{
			return x + y * GRID_LEN + z * GRID_LEN * GRID_LEN;
		}
		void setVoxel(int x, int y, int z, const GridEntry &voxel)
		{
			voxels[voxelIndex(x, y, z)] = voxel;
			dirty = true;
		}
		int posToVoxel(float pos, int axis) const
		{
			int val = pos - bound.pMin[axis];
			return val < 0 ? 0 : (val > GRID_LEN - 1 ? GRID_LEN - 1 : val);
		}
		float voxelEdge(int pos, int axis) const { return bound.pMin[axis] + (float)pos; }
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
	protected:
		int tesselate(GlVoxelVertex_t *vertices);
		BBox bound;
		int gridPos[3];
		int nTessTris;
		std::vector<GridEntry> voxels;
};

#endif // VG_VOXELGRID_H
