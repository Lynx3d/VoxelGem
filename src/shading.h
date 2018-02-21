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

GLuint genVertexUBO(QOpenGLFunctions_3_3_Core &glf);
GLuint genNormalTex(QOpenGLFunctions_3_3_Core &glf);

#endif // VG_SHADING_H
