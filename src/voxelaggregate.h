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
		#define POS_MASK (0x1FFFFF * GRID_LEN)
		static inline uint64_t blockID(int x, int y, int z)
		{
			return  uint64_t(x & POS_MASK) >> LOG_GRID_LEN |
					uint64_t(y & POS_MASK) << (21 - LOG_GRID_LEN) |
					uint64_t(z & POS_MASK) << (42 - LOG_GRID_LEN);
		}
		static inline void blockPos(uint64_t id, int pos[3])
		{
			pos[0] = (id << LOG_GRID_LEN) & POS_MASK;
			pos[1] = (id >> (21 - LOG_GRID_LEN)) & POS_MASK;
			pos[2] = (id >> (42 - LOG_GRID_LEN)) & POS_MASK;
		}
		#undef POS_MASK
		uint64_t setVoxel(int x, int y, int z, const VoxelEntry &voxel);
		const VoxelEntry* getVoxel(const int pos[3]) const;
		bool rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const;
		void clear();
		void clearBlocks(const std::unordered_set<uint64_t> &blocks);
		void merge(const VoxelAggregate &topLayer, const std::unordered_set<uint64_t> &blocks);
		void applyChanges(const VoxelAggregate &toolLayer, AggregateMemento *memento);
		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(AggregateMemento *memento, std::unordered_set<uint64_t> &changed);
		int blockCount() { return blockMap.size(); }
		const VoxelGrid* getBlock(uint64_t blockId) const;
		static void markDirtyBlocks(const DirtyVolume &vol, std::unordered_set<uint64_t> &blocks);
		void getNeighbours(const int gridPos[3], const VoxelGrid* neighbours[27]);
	protected:
		blockMap_t blockMap;
};

#endif // VG_VOXELAGGREGATE_H
