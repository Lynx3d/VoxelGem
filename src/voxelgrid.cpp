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
#include <algorithm>
#include <cstdint>
#include <cstring>

#include <QOpenGLShaderProgram>

extern QOpenGLShaderProgram *m_program, *m_voxel_program;

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

VoxelGrid::VoxelGrid(const int pos[3]):
	bound(QVector3D(pos[0], pos[1], pos[2]), QVector3D(pos[0] + GRID_LEN, pos[1] + GRID_LEN, pos[2] + GRID_LEN)),
	voxels(GRID_LEN * GRID_LEN * GRID_LEN)
{
	gridPos[0] = pos[0];
	gridPos[1] = pos[1];
	gridPos[2] = pos[2];
	memset(voxels.data(), 0, voxels.capacity());
	/* TEST! TODO*/
	GridEntry &testVoxel = voxels[voxelIndex(3,2,5)];
	testVoxel = { { 255, 220, 200, 255 }, VF_NON_EMPTY };
	//testVoxel.flags = VF_NON_EMPTY;
}

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

void VoxelGrid::setup(QOpenGLFunctions_3_3_Core &glf)
{
	m_voxel_program->bind();
	m_voxel_program->enableAttributeArray("v_position");
	glf.glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, pos));
	m_voxel_program->enableAttributeArray("v_color");
	glf.glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, col));
	m_voxel_program->enableAttributeArray("v_normal");
	glf.glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(GlVoxelVertex_t),
								(const GLvoid*)offsetof(GlVoxelVertex_t, normal));
}

void VoxelGrid::render(QOpenGLFunctions_3_3_Core &glf)
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

	if (dirty)
	{
		glf.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBuffer);
		nTessTris = tesselate(g_vertexBuffer);
		uploadBuffer(glf, g_vertexBuffer, 2 * nTessTris * sizeof(GlVoxelVertex_t));
		dirty = false;
	}
	glf.glDrawElements(GL_TRIANGLES, nTessTris * 3, GL_UNSIGNED_SHORT, 0);
	//glf.glDrawArrays(GL_TRIANGLES, 0, 3);
	glVAO.release();
}

bool VoxelGrid::rayIntersect(const ray_t &ray, int hitPos[3], intersect_t &hit) const
{
	//intersect_t hit;
	if (!bound.rayIntersect(ray, &hit))
		return false;
	QVector3D gridIntersect = ray.from + hit.tNear * ray.dir;
	float nextVoxelT[3], deltaT[3];
	float rayT = hit.tNear;
	int vPos[3], vOut[3], stepDir[3];
	// TODO: calc vPos from gridIntersect
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
		const GridEntry &voxel = voxels[voxelIndex(vPos[0], vPos[1], vPos[2])];
		if (voxel.flags & VF_NON_EMPTY)
		{
			hitPos[0] = vPos[0] + gridPos[0];
			hitPos[1] = vPos[1] + gridPos[1];
			hitPos[2] = vPos[2] + gridPos[2];
			return true;
		}
		// Step to net voxel, find axis:
		int axis = nextVoxelT[0] < nextVoxelT[1] ?
				 ( nextVoxelT[0] < nextVoxelT[2] ? 0 : 2) :
				 ( nextVoxelT[1] < nextVoxelT[2] ? 1 : 2);
		vPos[axis] += stepDir[axis];
		hit.entryAxis = axis | (stepDir[axis] < 0 ? intersect_t::AXIS_NEGATIVE : 0);
		hit.tNear = nextVoxelT[axis];
		if (vPos[axis] == vOut[axis] || hit.tNear > ray.t_max)
			break;
		nextVoxelT[axis] += deltaT[axis];
	}
	return false;
}

int VoxelGrid::tesselate(GlVoxelVertex_t *vertices)
{
	int index = 0, nTriangles = 0;
	for (int z = 0; z < GRID_LEN; ++z)
		for (int y = 0; y < GRID_LEN; ++y)
			for (int x = 0; x < GRID_LEN; ++x, ++index)
	{
		GridEntry &entry = voxels[index];
		if (!entry.flags & VF_NON_EMPTY) continue;
		for (int face=0; face < 6; ++face)
		{
			// TODO: determine if face is visible
			for (int i=0; i < 4; ++i)
			{
				int v = 2 * nTriangles + i;
				const int *vpos = VERTEX_POSITIONS[FACE_VERTICES[face][i]];
				vertices[v].pos[0] = bound.pMin[0] + float(x + vpos[0]);
				vertices[v].pos[1] = bound.pMin[1] + float(y + vpos[1]);
				vertices[v].pos[2] = bound.pMin[2] + float(z + vpos[2]);
				// TODO implement acutal voxel coloring
				vertices[v].col[0] = entry.col[0];
				vertices[v].col[1] = entry.col[1];
				vertices[v].col[2] = entry.col[2];
				vertices[v].col[3] = entry.col[3];
				vertices[v].normal[0] = FACE_NORMALS[face][0];
				vertices[v].normal[1] = FACE_NORMALS[face][1];
				vertices[v].normal[2] = FACE_NORMALS[face][2];
			}
			nTriangles += 2;
		}
	}
	return nTriangles;
}
