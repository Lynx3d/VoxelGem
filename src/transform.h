/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELTRANSFORM_H
#define VG_VOXELTRANSFORM_H

#include "voxelgem.h"

class VoxelAggregate;

class VoxelTransform
{
	public:
		virtual void operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target) = 0;
};

class VTTranslate: public VoxelTransform
{
	public:
		VTTranslate(const IVector3D &_offset): offset(_offset) {}
		virtual void operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target) override;
	protected:
		IVector3D offset;
};

VoxelAggregate* transformAggregate(const VoxelAggregate *ag, VoxelTransform &xform);

#endif // VG_VOXELTRANSFORM_H
