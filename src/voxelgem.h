/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELGEM_H
#define VG_VOXELGEM_H

#include <QVector3D>
#include <cstdint>
#include <cfloat>
#include <algorithm>

namespace Voxel
{
	enum VoxelFlags
	{
		VF_NON_EMPTY = 		1,
		VF_ERASED = 		1 << 1,
		VF_NO_COLLISION = 	1 << 2,
		VF_TOOL_SELECT = 	1 << 3,
	};
	enum Material
	{
		SOLID,			// 0
		GLOWING_SOLID,	// 1
		GLASS,			// 2
		TILED_GLASS,	// 3
		GLOWING_GLASS	// 4
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

union rgba_t
{
	uint32_t raw;
	struct
	{
		uint8_t r, g, b, a;
	};
	char bytes[4];
	rgba_t() {}
	rgba_t(uint32_t rgba): raw(rgba) {}
	rgba_t(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4):
		r(b1), g(b2), b(b3), a(b4) {}
};

static inline bool operator==(rgba_t c1, rgba_t c2) { return c1.raw == c2.raw; }
static inline bool operator!=(rgba_t c1, rgba_t c2) { return c1.raw != c2.raw; }

class VoxelEntry
{

	public:
		VoxelEntry(): col(0), flags(0) {}
		VoxelEntry(uint32_t rgba, uint32_t vflags): col(rgba), flags(vflags) {}
		VoxelEntry(uint8_t r, uint8_t g, uint8_t b, uint8_t a): col(r, g, b, a), flags(Voxel::VF_NON_EMPTY) {}
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
		bool isTransparent() const
		{
			return (flags & 0x600) != 0;
		}
		uint8_t getMaterialIndex() const
		{
			if (getMaterial() == 0)
				return getSpecular();
			else
				return getMaterial() + 5;
		}
		// data members
		rgba_t col;
		unsigned int flags;
};

class IVector3D
{
	public:
		IVector3D() {}
		IVector3D(const IVector3D &v): x(v.x), y(v.y), z(v.z) {}
		explicit IVector3D(const int v[3]): x(v[0]), y(v[1]), z(v[2]) {}
		IVector3D(int vx, int vy, int vz): x(vx), y(vy), z(vz) {}
		int operator[](int i) const { return (&x)[i]; }
		int& operator[](int i) { return (&x)[i]; }
		int x, y, z;
};

static inline IVector3D operator+(const IVector3D &v1, const IVector3D &v2)
{
	return IVector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

class DirtyVolume
{
	public:
		void addPosition(const IVector3D &pos)
		{
			if (valid)
				for (int i = 0; i < 3; ++i)
				{
					low[i] = std::min(low[i], pos[i]);
					high[i] = std::max(high[i], pos[i]);
				}
			else
			{
				for (int i = 0; i < 3; ++i)
					low[i] = high[i] = pos[i];
				valid = true;
			}
		}
		IVector3D low;
		IVector3D high;
		bool valid = false;
};

struct ray_t
{
	QVector3D dir;
	QVector3D from;
	float t_min;
	float t_max;
};


class SceneRayHit
{
	public:
		enum
		{
			AXIS_MASK = 0x3,
			AXIS_NEGATIVE = 0x4,
			HIT_VOXEL = 0x100,
			HIT_LINEGRID = 0x200,
			HIT_HANDLE = 0x400,
			HIT_MASK = 0x700
		};
		bool didHit() const { return (flags & HIT_MASK) != 0; }
		int hitFaceAxis() const { return flags & SceneRayHit::AXIS_MASK; }
		int hitFaceOrientation() const { return (flags & SceneRayHit::AXIS_NEGATIVE) ? 1 : -1; }
		bool getAdjacentVoxel(IVector3D &pos) const
		{
			if ((flags & SceneRayHit::HIT_MASK) == 0)
				return false;
			pos = IVector3D(voxelPos);
			// TODO: invert meaning of SceneRayHit::AXIS_NEGATIVE
			pos[flags & SceneRayHit::AXIS_MASK] += (flags & SceneRayHit::AXIS_NEGATIVE) ? 1 : -1;
			return true;
		}
		IVector3D voxelPos;
		int flags { 0 };
		float rayT;
};

// TODO: think about 'valid' member and replace DirtyVolume, or derive DirtyVolume from IBBox

class IBBox
{
	public:
		IBBox(const IVector3D &min, const IVector3D &max): pMin(min), pMax(max) {}
		IBBox(const IBBox &other): pMin(other.pMin), pMax(other.pMax) {}
		bool includes(const IVector3D &p) const
		{
			return ((p.x >= pMin.x) && (p.x <= pMax.x) &&
					(p.y >= pMin.y) && (p.y <= pMax.y) &&
					(p.z >= pMin.z) && (p.z <= pMax.z));
		}
		bool includes(const IBBox &b) const
		{
			return ((b.pMin.x >= pMin.x) && (b.pMax.x <= pMax.x) &&
					(b.pMin.y >= pMin.y) && (b.pMax.y <= pMax.y) &&
					(b.pMin.z >= pMin.z) && (b.pMax.z <= pMax.z));
		}
		void join(const IBBox &b)
		{
			for (int i = 0; i < 3; ++i)
			{
				pMin[i] = std::min(pMin[i], b.pMin[i]);
				pMax[i] = std::max(pMax[i], b.pMax[i]);
			}
		}
		bool rayIntersect(const ray_t &ray, float &tHit, int &entryAxis) const
		{
			float tMin = -FLT_MAX, tMax = FLT_MAX;
			for (int i = 0; i < 3; ++i)
			{
				int axisDirFlag = 0;
				float invDir = 1.f / ray.dir[i];
				float tNear = invDir * ((float)pMin[i] - ray.from[i]);
				float tFar = invDir * ((float)pMax[i] - ray.from[i]);
				if (tNear > tFar)
				{
					std::swap(tNear, tFar);
					axisDirFlag = SceneRayHit::AXIS_NEGATIVE;
				}
				if (tNear > tMin)
				{
					tMin = tNear;
					entryAxis = i | axisDirFlag;
				}
				if (tFar < tMax) tMax = tFar;
				if (tMin > tMax) return false;
			}
			// valid hit when interval [tMin, tMax] overlaps ray interval [t_min, t_max]
			tHit = tMin;
			return ray.t_max >= tMin && tMax >= ray.t_min;
		}
		IVector3D pMin, pMax;
};


class RenderOptions
{
	public:
		enum Modes
		{
			MODE_FULL,
			MODE_SLICE
		};

		Modes mode = MODE_FULL;
		int axis = 1;
		int level = 0;
};

#endif // VG_VOXELGEM_H

