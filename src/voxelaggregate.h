/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELAGGREGATE_H
#define VG_VOXELAGGREGATE_H

#include "voxelgrid.h"
#include <unordered_map>
#include <memory>

typedef std::shared_ptr<VoxelGrid> voxelGridPtr_t;
typedef std::unordered_map<uint64_t, std::shared_ptr<VoxelGrid>> blockMap_t;

class VoxelAggregate
{
	public:
		static inline uint64_t blockID(int x, int y, int z)
		{
			#define POS_MASK (0x1FFFFF * GRID_LEN)
			return  uint64_t(x & POS_MASK) >> LOG_GRID_LEN |
					uint64_t(y & POS_MASK) << (21 - LOG_GRID_LEN) |
					uint64_t(z & POS_MASK) << (42 - LOG_GRID_LEN);
			#undef POS_MASK
		}
		void setVoxel(int x, int y, int z, const GridEntry &voxel);
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
		void render(QOpenGLFunctions_3_3_Core &glf);
	protected:
		blockMap_t blockMap;
};

#endif // VG_VOXELAGGREGATE_H
