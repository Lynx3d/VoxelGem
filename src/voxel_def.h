/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *            Y
 *            ^
 *         v3 +-------------------+ v7
 *            |\                  .\
 *            | \                 . \                         +---------------+
 *            |  \                .  \                        |\               \
 *      .     |   \               .   \                  f4 > | \       f3      \
 *       .    |    \              .    \                      |  \               \
 *        .   |  v2 +-------------------+ v6                  |   +---------------+
 *         .  |     |             .     |                     |   |               |
 *          . |     |             .     |                     |   |               |
 *           .|     |             .     |                     |f0 |               | <- f1
 * ...........+ . . | . . . . . . + v4  |.......> X           +   |       f5      |
 *          v0 \    |              .    |                      \  |               |
 *              \   |               .   |                       \ |               |
 *               \  |                .  |                        \|               |
 *                \ |                 . |                         +---------------+
 *                 \|                  .|                                  ^
 *               v1 +-------------------+ v5                               f2
 *                   .
 *                    v
 *                     Z
 */
#ifndef VG_VOXELDEF_H
#define VG_VOXELDEF_H

static const float FACE_NORMALS[6][3] =
{
	{ -1,  0,  0 },
	{  1,  0,  0 },
	{  0, -1,  0 },
	{  0,  1,  0 },
	{  0,  0, -1 },
	{  0,  0,  1 }
};

static const int FACE_VERTICES[6][4] =
{
	{ 0, 1, 2, 3 },
	{ 7, 6, 5, 4 },
	{ 0, 4, 5, 1 },
	{ 2, 6, 7, 3 },
	{ 0, 3, 7, 4 },
	{ 5, 6, 2, 1 }
};

static const int VERTEX_POSITIONS[8][3] =
{
	{ 0, 0, 0 },
	{ 0, 0, 1 },
	{ 0, 1, 1 },
	{ 0, 1, 0 },
	{ 1, 0, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 1 },
	{ 1, 1, 0 },
};

enum VOXEL_NEIGHBOR_FLAG
{
	// z = -1 plane
	VN_xyz = 1,
	VN_nyz = 1 << 1,
	VN_Xyz = 1 << 2,
	VN_xnz = 1 << 3,
	VN_nnz = 1 << 4,
	VN_Xnz = 1 << 5,
	VN_xYz = 1 << 6,
	VN_nYz = 1 << 7,
	VN_XYz = 1 << 8,
	// z = 0 plane
	VN_xyn = 1 << 9,
	VN_nyn = 1 << 10,
	VN_Xyn = 1 << 11,
	VN_xnn = 1 << 12,
	// nnn is self
	VN_Xnn = 1 << 13,
	VN_xYn = 1 << 14,
	VN_nYn = 1 << 15,
	VN_XYn = 1 << 16,
	// z = 1 plane
	VN_xyZ = 1 << 17,
	VN_nyZ = 1 << 18,
	VN_XyZ = 1 << 19,
	VN_xnZ = 1 << 20,
	VN_nnZ = 1 << 21,
	VN_XnZ = 1 << 22,
	VN_xYZ = 1 << 23,
	VN_nYZ = 1 << 24,
	VN_XYZ = 1 << 25,
};

#endif // VG_VOXELDEF_H
