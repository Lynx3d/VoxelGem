/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELGEM_H
#define VG_VOXELGEM_H

#include <cstdint>

namespace Voxel
{
	enum VoxelFlags
	{
		VF_NON_EMPTY = 		1,
		VF_ERASED = 		1 << 1,
		VF_NO_COLLISION = 	1 << 2
	};
	enum Material
	{
		SOLID,
		GLASS,
		TILED_GLASS,
		GLOWING_SOLID,
		GLOWING_GLASS
	};
	enum Specular
	{
		ROUGH,
		METAL,
		WATER,
		IRIDESCENT,
		WAVE,
		WAXY
	};
}

class VoxelEntry
{

	public:
		VoxelEntry(): raw_rgba(0), flags(0) {}
		VoxelEntry(uint32_t rgba, uint32_t vflags): raw_rgba(rgba), flags(vflags) {}
		VoxelEntry(uint8_t r, uint8_t g, uint8_t b, uint8_t a): flags(Voxel::VF_NON_EMPTY)
		{
			col[0] = r, col[1] = g, col[2] = b, col[3] = a;
		}
		void setMaterial(Voxel::Material mat)
		{
			flags &= ~0xF00; // clear bits 9-12
			flags |= mat << 8;
		}
		void setSpecular(Voxel::Specular spec)
		{
			flags &= ~0xF000; // clear bits 13-16
			flags |= spec << 12;
		}
		Voxel::Material getMaterial() const
		{
			return static_cast<Voxel::Material>((flags & 0xF00) >> 8);
		}
		Voxel::Specular getSpecular() const
		{
			return static_cast<Voxel::Specular>((flags & 0xF000) >> 12);
		}
		// data members
		union
		{
			unsigned int raw_rgba;
			unsigned char col[4];
		};
		unsigned int flags;
};

#endif // VG_VOXELGEM_H

