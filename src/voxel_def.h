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
	VN_nnn = 1 << 13, // self
	VN_Xnn = 1 << 14,
	VN_xYn = 1 << 15,
	VN_nYn = 1 << 16,
	VN_XYn = 1 << 17,
	// z = 1 plane
	VN_xyZ = 1 << 18,
	VN_nyZ = 1 << 19,
	VN_XyZ = 1 << 20,
	VN_xnZ = 1 << 21,
	VN_nnZ = 1 << 22,
	VN_XnZ = 1 << 23,
	VN_xYZ = 1 << 24,
	VN_nYZ = 1 << 25,
	VN_XYZ = 1 << 26,
};

static const int FACE_NEIGHBOUR_FLAGS[6] =
{
	VN_xnn,
	VN_Xnn,
	VN_nyn,
	VN_nYn,
	VN_nnz,
	VN_nnZ
};

static const int FACE_OCCLUSION_FLAGS[6][8] =
{	// 4 touching corners				4 touching edges
	//	v0		v1		v2		v3		0->1	1->2	2->3	3->0
	{ VN_xyz, VN_xyZ, VN_xYZ, VN_xYz, 	VN_xyn, VN_xnZ, VN_xYn, VN_xnz }, // f0 (-x)
	//	v7		v6		v5		v4		7->6	6->5	5->4	4->7
	{ VN_XYz, VN_XYZ, VN_XyZ, VN_Xyz, 	VN_Xnz, VN_XnZ, VN_Xyn, VN_XYn }, // f1 (+x)
	//	v0		v4		v5		v1		0->4	4->5	5->1	1->0
	{ VN_xyz, VN_Xyz, VN_XyZ, VN_xyZ, 	VN_nyz, VN_Xyn, VN_nyZ, VN_xyn }, // f2 (-y)
	//	v2		v6		v7		v3		2->6	6->7	7->3	3->2
	{ VN_xYZ, VN_XYZ, VN_XYz, VN_xYz, 	VN_nYZ, VN_XYn, VN_nYz, VN_xYn }, // f3 (+y)
	//	v0		v3		v7		v4		0->3	3->7	7->4	4->0
	{ VN_xyz, VN_xYz, VN_XYz, VN_Xyz, 	VN_xnz, VN_nYz, VN_Xnz, VN_nyz }, // f4 (-z)
	// v5		v6		v2		v1		5->6	6->2	2->1	1->5
	{ VN_XyZ, VN_XYZ, VN_xYZ, VN_xyZ, 	VN_XnZ, VN_nYZ, VN_xnZ, VN_nyZ }, // f5 (-z)
};


#endif // VG_VOXELDEF_H
