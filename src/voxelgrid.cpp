/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Implements a 16x16x16 voxel grid */

#include "voxelgrid.h"
#include "voxel_def.h"
#include <cstring>

#include <QOpenGLShaderProgram>

GLuint g_indexBuffer = 0;
GlVoxelVertex_t *g_vertexBuffer = 0;

const int neighbor_offset[] = {
	-273, -272, -271, // y = -1, z = -1
	-257, -256, -255, // y =  0, z = -1
	-241, -240, -239, // y =  1, z = -1
	 -17,  -16,  -15, // y = -1, z =  0
	  -1,    0,    1, // y =  0, z =  0
	  15,   16,   17, // y =  1, z =  0
	 239,  240,  241,
	 255,  256,  257,
	 271,  272,  273
};

VoxelGrid::VoxelGrid(const IVector3D &pos):
	bound(pos, pos + IVector3D(GRID_LEN, GRID_LEN, GRID_LEN)),
	voxels(GRID_LEN * GRID_LEN * GRID_LEN)
{
	memset(voxels.data(), 0, voxels.capacity()); // TODO: should be redundant with current VoxelEntry() constructor...
}

VoxelGrid::VoxelGrid(const VoxelGrid &other):
	bound(other.bound), voxels(other.voxels)
{
}

VoxelGrid::VoxelGrid(const IVector3D &pos, GridMemento *memento):
	bound(pos, pos + IVector3D(GRID_LEN, GRID_LEN, GRID_LEN))
{
	voxels.swap(memento->voxels);
}

VoxelGrid::~VoxelGrid()
{
}

bool VoxelGrid::rayIntersect(const ray_t &ray, SceneRayHit &hit) const
{
	float rayT;
	if (!bound.rayIntersect(ray, rayT, hit.flags))
		return false;
	// for interior ray origin, bound intersection returns negative tMin
	rayT = std::max(ray.t_min, rayT);
	hit.rayT = rayT;
	QVector3D gridIntersect = ray.from + rayT * ray.dir;
	float nextVoxelT[3], deltaT[3];

	int vPos[3], vOut[3], stepDir[3];
	// TODO: handle ray cast from inside voxel, currently causes paint tool to replace that voxel
	for (int axis = 0; axis < 3; ++axis)
	{
		vPos[axis] = posToVoxel(gridIntersect[axis], axis);
		if(ray.dir[axis] >= 0)
		{
			nextVoxelT[axis] = rayT + (voxelEdge(vPos[axis] + 1, axis) - gridIntersect[axis]) / ray.dir[axis];
			deltaT[axis] = 1.f / ray.dir[axis];
			stepDir[axis] = 1;
			vOut[axis] = GRID_LEN;
		}
		else
		{
			nextVoxelT[axis] = rayT + (voxelEdge(vPos[axis], axis) - gridIntersect[axis]) / ray.dir[axis];
			deltaT[axis] = -1.f / ray.dir[axis];
			stepDir[axis] = -1;
			vOut[axis] = -1;
		}
	}

	while (true)
	{
		const VoxelEntry &voxel = voxels[voxelIndex(vPos[0], vPos[1], vPos[2])];
		if ((voxel.flags & (Voxel::VF_NON_EMPTY | Voxel::VF_NO_COLLISION)) == Voxel::VF_NON_EMPTY)
		{
			hit.voxelPos = IVector3D(vPos) + bound.pMin;
			return true;
		}
		// Step to net voxel, find axis:
		int axis = nextVoxelT[0] < nextVoxelT[1] ?
				 ( nextVoxelT[0] < nextVoxelT[2] ? 0 : 2) :
				 ( nextVoxelT[1] < nextVoxelT[2] ? 1 : 2);
		vPos[axis] += stepDir[axis];
		hit.flags = axis | (stepDir[axis] < 0 ? SceneRayHit::AXIS_NEGATIVE : 0);
		hit.rayT = nextVoxelT[axis];
		if (vPos[axis] == vOut[axis] || hit.rayT > ray.t_max)
			break;
		nextVoxelT[axis] += deltaT[axis];
	}
	return false;
}

void VoxelGrid::merge(const VoxelGrid &topLayer, VoxelGrid *targetGrid)
{
	// TODO: combination modes/flags could be useful, maybe to use it in applyChanges()
	VoxelGrid *target = targetGrid ? targetGrid : this;
	for (int i = 0; i < GRID_LEN * GRID_LEN * GRID_LEN; ++i)
	{
		const VoxelEntry &entry = topLayer.voxels[i];
		if (entry.flags & Voxel::VF_ERASED)
			target->voxels[i] = VoxelEntry();
		else if(entry.flags & Voxel::VF_NON_EMPTY)
		{
			target->voxels[i] = entry;
		}
	}
}

int VoxelGrid::applyChanges(const VoxelGrid &toolLayer, GridMemento *memento)
{
	int nVoxels = 0;
	if (memento)
		memento->voxels = voxels;
	for (int i = 0; i < GRID_LEN * GRID_LEN * GRID_LEN; ++i)
	{
		const VoxelEntry &entry = toolLayer.voxels[i];
		if (entry.flags & Voxel::VF_ERASED)
			voxels[i] = VoxelEntry();
		else if(entry.flags & Voxel::VF_NON_EMPTY)
		{
			voxels[i] = entry;
			// (doesn't work) clear VF_NO_COLLISION flag, currently only valid for tool and rendering layer
			// voxels[i].flags &= ~Voxel::VF_NO_COLLISION;
		}
		if (voxels[i].flags & Voxel::VF_NON_EMPTY)
			++nVoxels;
	}
	return nVoxels;
}

void VoxelGrid::saveState(GridMemento *memento) const
{
	memento->voxels = voxels;
}

void VoxelGrid::restoreState(GridMemento *memento)
{
	voxels.swap(memento->voxels);
}

static void getOcclusionValues(int face, int mask, uint8_t occ[4])
{
	const int *faceFlags = FACE_OCCLUSION_FLAGS[face];
	for (int i = 0; i < 4; ++i)
	{
		if (mask & faceFlags[i]) // coners
		{
			++occ[i];
		}
		if (mask & faceFlags[i+4]) // edges
		{
			++occ[i];
			++occ[(i+1)&3];
		}
	}
}
static inline int getNormalMapIndex(int face, int mask)
{
	static const int face_edge_neighbours[6][4] =
	{
		{ VN_nyn, VN_nnZ, VN_nYn, VN_nnz },
		{ VN_nYn, VN_nnZ, VN_nyn, VN_nnz },
		{ VN_nnz, VN_Xnn, VN_nnZ, VN_xnn },
		{ VN_nnZ, VN_Xnn, VN_nnz, VN_xnn },
		{ VN_xnn, VN_nYn, VN_Xnn, VN_nyn },
		{ VN_Xnn, VN_nYn, VN_xnn, VN_nyn }
	};
	int index = 0;
	for (int edge = 0; edge < 4; ++edge)
		if (mask & face_edge_neighbours[face][edge])
			index |= 1 << edge;
	return index;
}

inline int VoxelGrid::writeFaces(const VoxelEntry &entry, uint8_t matIndex, int mask, IVector3D pos, GlVoxelVertex_t *vertices) const
{
	int nTriangles = 0;
	for (int face=0; face < 6; ++face)
	{
		if (mask & FACE_NEIGHBOUR_FLAGS[face])
			continue;

		uint8_t occlusion[4] = {};
		getOcclusionValues(face, mask, occlusion);
		for (int i=0; i < 4; ++i)
		{
			int v = 2 * nTriangles + i;
			GlVoxelVertex_t &vertex = vertices[v];
			const int *vpos = VERTEX_POSITIONS[FACE_VERTICES[face][i]];
			vertex.pos[0] = bound.pMin[0] + float(pos[0] + vpos[0]);
			vertex.pos[1] = bound.pMin[1] + float(pos[1] + vpos[1]);
			vertex.pos[2] = bound.pMin[2] + float(pos[2] + vpos[2]);
			vertex.col[0] = entry.col.r;
			vertex.col[1] = entry.col.g;
			vertex.col[2] = entry.col.b;
			vertex.col[3] = entry.col.a;
			vertex.index = 4*face + i;
			vertex.matIndex = matIndex;
			vertex.texIndex = matIndex == 8 ? 0 : getNormalMapIndex(face, mask);
			vertex.occlusion = occlusion[i];
		}
		nTriangles += 2;
	}
	return nTriangles;
}

void VoxelGrid::tesselate(GlVoxelVertex_t *vertices, int nTris[2], const VoxelGrid* neighbourGrids[27]) const
{
	nTris[0] = nTris[1] = 0;
	bool haveTransparent = false;
	std::vector<int> masks = getNeighbourMasks(neighbourGrids);

	for (int z = 0, index = 0; z < GRID_LEN; ++z)
		for (int y = 0; y < GRID_LEN; ++y)
			for (int x = 0; x < GRID_LEN; ++x, ++index)
	{
		const VoxelEntry &entry = voxels[index];
		if (!(entry.flags & Voxel::VF_NON_EMPTY))
			continue;
		if (entry.isTransparent())
		{
			haveTransparent = true;
			continue;
		}
		uint8_t matIndex = entry.getMaterialIndex();

		IVector3D pos(x, y, z);
		nTris[0] += writeFaces(entry, matIndex, masks[index], pos, vertices + 2 * nTris[0]);
	}
	// TODO: tesselating in two passes is probably not the fastest
	if (!haveTransparent)
		return;

	vertices += 2 * nTris[0];
	for (int z = 0, index = 0; z < GRID_LEN; ++z)
		for (int y = 0; y < GRID_LEN; ++y)
			for (int x = 0; x < GRID_LEN; ++x, ++index)
	{
		const VoxelEntry &entry = voxels[index];
		if (!(entry.flags & Voxel::VF_NON_EMPTY) || !entry.isTransparent())
			continue;

		uint8_t matIndex = entry.getMaterialIndex();
		IVector3D pos(x, y, z);
		nTris[1] += writeFaces(entry, matIndex, masks[index], pos, vertices + 2 * nTris[1]);
	}
}

void VoxelGrid::tesselateSlice(GlVoxelVertex_t *vertices, int nTris[2], const VoxelGrid* neighbourGrids[27],
								int axis, int level) const
{
	static const int slice_mask[3] =
	{
		(VN_nyz | VN_nnz | VN_nYz | VN_nyn | VN_nnn | VN_nYn | VN_nyZ | VN_nnZ | VN_nYZ),
		(VN_xnz | VN_nnz | VN_Xnz | VN_xnn | VN_nnn | VN_Xnn | VN_xnZ | VN_nnZ | VN_XnZ),
		(VN_xyn | VN_nyn | VN_Xyn | VN_xnn | VN_nnn | VN_Xnn | VN_xYn | VN_nYn | VN_XYn)
	};
	nTris[0] = nTris[1] = 0;

	int sxAxis = 1, syAxis = 2;
	if (axis == 1)
	{
		sxAxis = 0;
	}
	else if (axis == 2)
	{
		sxAxis = 0;
		syAxis = 1;
	}
	bool haveTransparent = false;
	// TODO: we don't need to compute all neighbour masks
	std::vector<int> masks = getNeighbourMasks(neighbourGrids);

	for (int sy = 0; sy < GRID_LEN; ++sy)
		for (int sx = 0; sx < GRID_LEN; ++sx)
	{
		IVector3D pos;
		pos[axis] = level;
		pos[sxAxis] = sx;
		pos[syAxis] = sy;
		int index = voxelIndex(pos.x, pos.y, pos.z);
		const VoxelEntry &entry = voxels[index];
		if (!(entry.flags & Voxel::VF_NON_EMPTY))
			continue;
		if (entry.isTransparent())
		{
			haveTransparent = true;
			continue;
		}
		uint8_t matIndex = entry.getMaterialIndex();
		nTris[0] += writeFaces(entry, matIndex, masks[index]&slice_mask[axis], pos, vertices + 2 * nTris[0]);
	}
	// TODO: tesselating in two passes is probably not the fastest
	if (!haveTransparent)
		return;

	vertices += 2 * nTris[0];
	for (int sy = 0; sy < GRID_LEN; ++sy)
		for (int sx = 0; sx < GRID_LEN; ++sx)
	{
		IVector3D pos;
		pos[axis] = level;
		pos[sxAxis] = sx;
		pos[syAxis] = sy;
		int index = voxelIndex(pos.x, pos.y, pos.z);
		const VoxelEntry &entry = voxels[index];
		if (!(entry.flags & Voxel::VF_NON_EMPTY) || !entry.isTransparent())
			continue;
		uint8_t matIndex = entry.getMaterialIndex();
		nTris[1] += writeFaces(entry, matIndex, masks[index]&slice_mask[axis], pos, vertices + 2 * nTris[1]);
	}
}

static inline bool isFaceHidden(const VoxelEntry &vox, const VoxelEntry &neighbour)
{
	return (neighbour.flags & Voxel::VF_NON_EMPTY) &&
			(vox.isTransparent() || !neighbour.isTransparent());
}

std::vector<int> VoxelGrid::getNeighbourMasks(const VoxelGrid* neighbourGrids[27]) const
{
	std::vector<int> masks(GRID_LEN * GRID_LEN * GRID_LEN);
	for (int z = 0; z < GRID_LEN; ++z)
		for (int y = 0; y < GRID_LEN; ++y)
			for (int x = 0; x < GRID_LEN; ++x)
	{
		int index = voxelIndex(x, y, z);
		const VoxelEntry &vox = voxels[index];
		if (!(vox.flags & Voxel::VF_NON_EMPTY))
			continue;

		int mask = 0;
		if (x == 0 || x ==  GRID_LEN - 1 || y == 0 || y == GRID_LEN - 1 || z == 0 || z == GRID_LEN - 1)
		{	// touching neighbour grids
			for (int nz = -1, i = 0; nz < 2; ++nz)
				for (int ny = -1; ny < 2; ++ny)
					for (int nx = -1; nx < 2; ++nx, ++i)
			{
				int block_x = (x + nx + GRID_LEN) >> LOG_GRID_LEN;
				int block_y = (y + ny + GRID_LEN) >> LOG_GRID_LEN; // could be moved to outer loop
				int block_z = (z + nz + GRID_LEN) >> LOG_GRID_LEN; // could be moved to outer loop
				int block_index = block_x + 3 * block_y + 9 * block_z;

				const VoxelGrid *nGrid = neighbourGrids[block_index];
				if (!nGrid)
					continue;
				int neighbourIndex = voxelIndex((x + nx) & (GRID_LEN - 1),
												(y + ny) & (GRID_LEN - 1),
												(z + nz) & (GRID_LEN - 1));
				const VoxelEntry &neighbour = nGrid->voxels[neighbourIndex];

				if (isFaceHidden(vox, neighbour))
					mask |= 1 << i;
			}
		}
		else // don't need neighbour grids
		{
			for (int nz = -1, i = 0; nz < 2; ++nz)
				for (int ny = -1; ny < 2; ++ny)
					for (int nx = -1; nx < 2; ++nx, ++i)
			{
				int neighbourIndex = index + voxelIndex(nx, ny, nz);
				const VoxelEntry &neighbour = voxels[neighbourIndex];

				if (isFaceHidden(vox, neighbour))
					mask |= 1 << i;
			}
		}
		masks[index] = mask;
	}
	return masks;
}

/*=================================
	RenderGrid
==================================*/

void initIndexBuffer(QOpenGLFunctions_3_3_Core &glf)
{
	const int faceIndices[6] = { 0, 1, 2,  2, 3, 0 }; // one quad => 2 triangles
	const int maxIndices = GRID_LEN * GRID_LEN * GRID_LEN * 6 * 2 * 3; // 6 faces * 2 triangles * 3 indices
	uint16_t *index_array = new uint16_t[maxIndices];
	for (int i = 0; i < maxIndices; ++i)
		index_array[i] = (i / 6) * 4 + faceIndices[i % 6];
	glf.glGenBuffers(1, &g_indexBuffer);
	glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBuffer);
	glf.glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint16_t), index_array, GL_STATIC_DRAW);
	delete[] index_array;
}

void RenderGrid::setup(QOpenGLFunctions_3_3_Core &glf)
{
	// Attribute 0: vertex position
	glf.glEnableVertexAttribArray(0);
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, pos));
	// Attribute 1: vertex color
	glf.glEnableVertexAttribArray(1);
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, col));
	// Attribute 2: vertex index
	glf.glEnableVertexAttribArray(2);
	glf.glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, index));
	// Attribute 3: vertex material index
	glf.glEnableVertexAttribArray(3);
	glf.glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, matIndex));
	// Attribute 4: vertex texture index (normal map/glow map)
	glf.glEnableVertexAttribArray(4);
	glf.glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, texIndex));
	// Attribute 5: vertex occlusion
	glf.glEnableVertexAttribArray(5);
	glf.glVertexAttribPointer(5, 1, GL_UNSIGNED_BYTE, false, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, occlusion));
}

void RenderGrid::clear(QOpenGLFunctions_3_3_Core &glf)
{
	cleanupGL(glf);
	nTessTris[0] = nTessTris[1] = 0;
}

void RenderGrid::update(QOpenGLFunctions_3_3_Core &glf, const VoxelGrid* neighbourGrids[27], const RenderOptions &opt)
{
	// TODO: move to a better place...
	if (!g_vertexBuffer)
	{
		initIndexBuffer(glf);
		g_vertexBuffer = new GlVoxelVertex_t[GRID_LEN * GRID_LEN * GRID_LEN * 6 * 4];
	}

	if (!glVAO.isCreated())
		glVAO.create();
	glVAO.bind();

	const VoxelGrid *tessGrid = neighbourGrids[13];
	if (opt.mode == RenderOptions::MODE_SLICE)
		tessGrid->tesselateSlice(g_vertexBuffer, nTessTris, neighbourGrids, opt.axis, opt.level & (GRID_LEN - 1));
	else
		tessGrid->tesselate(g_vertexBuffer, nTessTris, neighbourGrids);
	int totalTris = nTessTris[0] + nTessTris[1];
	if (totalTris > 0)
		uploadBuffer(glf, g_vertexBuffer, 2 * totalTris * sizeof(GlVoxelVertex_t));
	dirty = false;
}

void RenderGrid::render(QOpenGLFunctions_3_3_Core &glf)
{
	if (dirty || nTessTris[0] == 0)
		return;
	glVAO.bind();
	glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBuffer);
	glf.glDrawElements(GL_TRIANGLES, nTessTris[0] * 3, GL_UNSIGNED_SHORT, 0);
	glVAO.release();
}

void RenderGrid::renderTransparent(QOpenGLFunctions_3_3_Core &glf)
{
	if (dirty || nTessTris[1] == 0)
		return;
	glVAO.bind();
	glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBuffer);
	glf.glDrawElements(GL_TRIANGLES, nTessTris[1] * 3, GL_UNSIGNED_SHORT, (void*)(nTessTris[0] * 3 * sizeof(uint16_t)));
	glVAO.release();
}
