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
#include <unordered_set>
#include <memory>

typedef std::shared_ptr<VoxelGrid> voxelGridPtr_t;
typedef std::unordered_map<uint64_t, std::shared_ptr<VoxelGrid>> blockMap_t;

typedef std::unique_ptr<GridMemento> gridMementoPtr_t;
typedef std::unordered_map<uint64_t, gridMementoPtr_t> mementoMap_t;

class AggregateMemento
{
	friend class VoxelAggregate;
	mementoMap_t blockMap;
};

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
		uint64_t setVoxel(int x, int y, int z, const VoxelEntry &voxel);
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
		void render(QOpenGLFunctions_3_3_Core &glf);
		void clear();
		void clearBlocks(const std::unordered_set<uint64_t> &blocks);
		void merge(const VoxelAggregate &topLayer, const std::unordered_set<uint64_t> &blocks);
		void applyChanges(const VoxelAggregate &toolLayer, AggregateMemento *memento);
		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(AggregateMemento *memento, std::unordered_set<uint64_t> &changed);
		int blockCount() { return blockMap.size(); }
	protected:
		void getNeighbours(const int gridPos[3], const VoxelGrid* neighbours[27]);
		blockMap_t blockMap;
};

#endif // VG_VOXELAGGREGATE_H
