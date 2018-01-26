/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_LIB_SHADERINFO_H
#define VG_LIB_SHADERINFO_H

#include <QOpenGLFunctions_3_3_Core>

class GLInfoLib : protected QOpenGLFunctions_3_3_Core
{
	public:
		
		static void getUniformsInfo(unsigned int program);
	protected:
		static void create();
		void init();
		void _getUniformsInfo(unsigned int program);
		int getUniformByteSize(int uniSize, int uniType, int uniArrayStride, int uniMatStride);
};

#endif // VG_LIB_SHADERINFO_H
