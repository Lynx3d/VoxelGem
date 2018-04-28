/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_SHADING_H
#define VG_SHADING_H

#include "voxelgem.h"

#include <QOpenGLFunctions_3_3_Core>

class QOpenGLShaderProgram;

enum ShaderId
{
	VT_SHADER_VOXEL,
	VT_SHADER_SOLID_COLOR,
	FR_SHADER_VOXEL,
	FR_SHADER_SOLID_COLOR,
	SHADER_MAX_ID
};

enum ShaderProgId
{
	SHADER_VOXEL,
	SHADERPROG_MAX_ID
};

GLuint genVertexUBO(QOpenGLFunctions_3_3_Core &glf);
GLuint genNormalTex(QOpenGLFunctions_3_3_Core &glf);
void initShaders(QOpenGLFunctions_3_3_Core &glf);
QOpenGLShaderProgram* getShaderProgram(ShaderProgId program);

#endif // VG_SHADING_H
