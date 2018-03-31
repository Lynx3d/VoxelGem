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

typedef std::unordered_set<uint64_t> blockSet_t;

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
		uint64_t setVoxel(int x, int y, int z, const VoxelEntry &voxel); // deprecated!
		uint64_t setVoxel(const IVector3D &pos, const VoxelEntry &voxel);
		const VoxelEntry* getVoxel(const int pos[3]) const; // deprecated!
		const VoxelEntry* getVoxel(const IVector3D &pos) const;
		bool rayIntersect(const ray_t &ray, IVector3D &hitPos, intersect_t &hit) const;
		void clear();
		void clearBlocks(const std::unordered_set<uint64_t> &blocks);
		void clearBlocks(const std::unordered_map<uint64_t, DirtyVolume> &blocks);
		void clone(const VoxelAggregate &source);
		void merge(const VoxelAggregate &topLayer, const std::unordered_set<uint64_t> &blocks);
		void merge(const VoxelAggregate &topLayer, const std::unordered_map<uint64_t, DirtyVolume> &blocks);
		void applyChanges(const VoxelAggregate &toolLayer, AggregateMemento *memento);

		// the memento shall be altered to allow reversing the restore (i.e. "redo" operation)
		void restoreState(AggregateMemento *memento, std::unordered_set<uint64_t> &changed);
		int blockCount() { return blockMap.size(); }
		const VoxelGrid* getBlock(uint64_t blockId) const;
		const blockMap_t& getBlockMap() const { return blockMap; }
		static void markDirtyBlocks(const DirtyVolume &vol, std::unordered_set<uint64_t> &blocks);
		void getNeighbours(const int gridPos[3], const VoxelGrid* neighbours[27]);
		bool getBound(IBBox &bound) const;
	protected:
		blockMap_t blockMap;
};

class RenderAggregate
{
	public:
		RenderAggregate(VoxelAggregate *va = 0): aggregate(va) {};
		void clear(QOpenGLFunctions_3_3_Core &glf);
		void update(QOpenGLFunctions_3_3_Core &glf, const blockSet_t &dirtyBlocks);
		void rebuild(QOpenGLFunctions_3_3_Core &glf);
		void render(QOpenGLFunctions_3_3_Core &glf);
		void renderTransparent(QOpenGLFunctions_3_3_Core &glf);
		void setAggregate(VoxelAggregate *va) { aggregate = va; }
	protected:
		std::unordered_map<uint64_t, RenderGrid*> renderBlocks;
		VoxelAggregate *aggregate;
};

#endif // VG_VOXELAGGREGATE_H
