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

class VTMirror: public VoxelTransform
{
	public:
		VTMirror(int _axis, int _center = 0): axis(_axis), center(_center) {}
		virtual void operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target) override;
	protected:
		int axis;
		int center;
};

class VTRotate: public VoxelTransform
{
	public:
		enum Rotation
		{
			Rot90,
			Rot180,
			Rot270
		};
		VTRotate(int axis, Rotation rotation);
		virtual void operator()(const IVector3D &pos, const VoxelEntry *voxel, VoxelAggregate *target) override;
	protected:
		IVector3D axisMap;
		IVector3D axisScale;
};

VoxelAggregate* transformAggregate(const VoxelAggregate *ag, VoxelTransform &xform);

#endif // VG_VOXELTRANSFORM_H
