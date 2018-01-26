/* Code based on:
   https://github.com/lighthouse3d/VSL/blob/master/VSL/source/vsGLInfoLib.cpp

 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "shaderinfo.h"
#include <map>
#include <cstdio>

// quick and dirty...
#define addMessage(...) do { printf( __VA_ARGS__ ); printf("\n"); } while (0)

static GLInfoLib *infoLib;
static std::map<int, int> spGLSLTypeSize;
static std::map<int, std::string> spGLSLType;

void GLInfoLib::getUniformsInfo(unsigned int program)
{
	if (!infoLib)
	{
		GLInfoLib::create();
	}
	infoLib->_getUniformsInfo(program);
}

void GLInfoLib::_getUniformsInfo(unsigned int program)
{
	int activeUnif, actualLen, index, uniType, 
		uniSize, uniMatStride, uniArrayStride, uniOffset;
	char name[256];
	
	// Get named blocks info
	int count, dataSize, info;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &count);

	for (int i = 0; i < count; ++i) {
		// Get blocks name
		glGetActiveUniformBlockName(program, i, 256, NULL, name);
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
		addMessage("%s\n  Size %d", name, dataSize);

		glGetActiveUniformBlockiv(program, i,  GL_UNIFORM_BLOCK_BINDING, &index);
		addMessage("  Block binding point: %d", index);
		glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, index, &info);
		addMessage("  Buffer bound to binding point: %d {", info);


		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &activeUnif);

		unsigned int *indices;
		indices = (unsigned int *)malloc(sizeof(unsigned int) * activeUnif);
		glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (int *)indices);
			
		for (int k = 0; k < activeUnif; ++k) {
		
			glGetActiveUniformName(program, indices[k], 256, &actualLen, name);
			glGetActiveUniformsiv(program, 1, &indices[k], GL_UNIFORM_TYPE, &uniType);
			addMessage("\t%s\n\t    %s", name, spGLSLType[uniType].c_str());

			glGetActiveUniformsiv(program, 1, &indices[k], GL_UNIFORM_OFFSET, &uniOffset);
			addMessage("\t    offset: %d", uniOffset);

			glGetActiveUniformsiv(program, 1, &indices[k], GL_UNIFORM_SIZE, &uniSize);

			glGetActiveUniformsiv(program, 1, &indices[k], GL_UNIFORM_ARRAY_STRIDE, &uniArrayStride);

			glGetActiveUniformsiv(program, 1, &indices[k], GL_UNIFORM_MATRIX_STRIDE, &uniMatStride);

			int auxSize;
			if (uniArrayStride > 0)
				auxSize = uniArrayStride * uniSize;
				
			else if (uniMatStride > 0) {

				switch(uniType) {
					case GL_FLOAT_MAT2:
					case GL_FLOAT_MAT2x3:
					case GL_FLOAT_MAT2x4:
					case GL_DOUBLE_MAT2:
					case GL_DOUBLE_MAT2x3:
					case GL_DOUBLE_MAT2x4:
						auxSize = 2 * uniMatStride;
						break;
					case GL_FLOAT_MAT3:
					case GL_FLOAT_MAT3x2:
					case GL_FLOAT_MAT3x4:
					case GL_DOUBLE_MAT3:
					case GL_DOUBLE_MAT3x2:
					case GL_DOUBLE_MAT3x4:
						auxSize = 3 * uniMatStride;
						break;
					case GL_FLOAT_MAT4:
					case GL_FLOAT_MAT4x2:
					case GL_FLOAT_MAT4x3:
					case GL_DOUBLE_MAT4:
					case GL_DOUBLE_MAT4x2:
					case GL_DOUBLE_MAT4x3:
						auxSize = 4 * uniMatStride;
						break;
				}
			}
			else
				auxSize = spGLSLTypeSize[uniType];

			auxSize = getUniformByteSize(uniSize, uniType, uniArrayStride, uniMatStride);
			addMessage("\t    size: %d", auxSize);
			if (uniArrayStride > 0)
				addMessage("\t    array stride: %d", uniArrayStride);
			if (uniMatStride > 0)
				addMessage("\t    mat stride: %d", uniMatStride);
		}
		addMessage("    }");
	}
}

void GLInfoLib::create()
{
	infoLib = new GLInfoLib();
	infoLib->init();
}

void GLInfoLib::init()
{
	
	initializeOpenGLFunctions();

	spGLSLTypeSize[GL_FLOAT] = sizeof(float);
	spGLSLTypeSize[GL_FLOAT_VEC2] = sizeof(float)*2;
	spGLSLTypeSize[GL_FLOAT_VEC3] = sizeof(float)*3;
	spGLSLTypeSize[GL_FLOAT_VEC4] = sizeof(float)*4;

	spGLSLTypeSize[GL_DOUBLE] = sizeof(double);
	spGLSLTypeSize[GL_DOUBLE_VEC2] = sizeof(double)*2;
	spGLSLTypeSize[GL_DOUBLE_VEC3] = sizeof(double)*3;
	spGLSLTypeSize[GL_DOUBLE_VEC4] = sizeof(double)*4;

	spGLSLTypeSize[GL_SAMPLER_1D] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_3D] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_CUBE] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_1D_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_1D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_1D_ARRAY_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_ARRAY_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_MULTISAMPLE] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_MULTISAMPLE_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_CUBE_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_BUFFER] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_RECT] = sizeof(int);
	spGLSLTypeSize[GL_SAMPLER_2D_RECT_SHADOW] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_1D] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_2D] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_3D] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_CUBE] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_1D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_2D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_2D_MULTISAMPLE] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_BUFFER] = sizeof(int);
	spGLSLTypeSize[GL_INT_SAMPLER_2D_RECT] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_1D] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_2D] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_3D] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_CUBE] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_1D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_2D_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_BUFFER] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_SAMPLER_2D_RECT] = sizeof(int);
	spGLSLTypeSize[GL_BOOL] = sizeof(int);
	spGLSLTypeSize[GL_INT] = sizeof(int);
	spGLSLTypeSize[GL_BOOL_VEC2] = sizeof(int)*2;
	spGLSLTypeSize[GL_INT_VEC2] = sizeof(int)*2;
	spGLSLTypeSize[GL_BOOL_VEC3] = sizeof(int)*3;
	spGLSLTypeSize[GL_INT_VEC3] = sizeof(int)*3;
	spGLSLTypeSize[GL_BOOL_VEC4] = sizeof(int)*4;
	spGLSLTypeSize[GL_INT_VEC4] = sizeof(int)*4;

	spGLSLTypeSize[GL_UNSIGNED_INT] = sizeof(int);
	spGLSLTypeSize[GL_UNSIGNED_INT_VEC2] = sizeof(int)*2;
	spGLSLTypeSize[GL_UNSIGNED_INT_VEC3] = sizeof(int)*2;
	spGLSLTypeSize[GL_UNSIGNED_INT_VEC4] = sizeof(int)*2;

	spGLSLTypeSize[GL_FLOAT_MAT2] = sizeof(float)*4;
	spGLSLTypeSize[GL_FLOAT_MAT3] = sizeof(float)*9;
	spGLSLTypeSize[GL_FLOAT_MAT4] = sizeof(float)*16;
	spGLSLTypeSize[GL_FLOAT_MAT2x3] = sizeof(float)*6;
	spGLSLTypeSize[GL_FLOAT_MAT2x4] = sizeof(float)*8;
	spGLSLTypeSize[GL_FLOAT_MAT3x2] = sizeof(float)*6;
	spGLSLTypeSize[GL_FLOAT_MAT3x4] = sizeof(float)*12;
	spGLSLTypeSize[GL_FLOAT_MAT4x2] = sizeof(float)*8;
	spGLSLTypeSize[GL_FLOAT_MAT4x3] = sizeof(float)*12;
	spGLSLTypeSize[GL_DOUBLE_MAT2] = sizeof(double)*4;
	spGLSLTypeSize[GL_DOUBLE_MAT3] = sizeof(double)*9;
	spGLSLTypeSize[GL_DOUBLE_MAT4] = sizeof(double)*16;
	spGLSLTypeSize[GL_DOUBLE_MAT2x3] = sizeof(double)*6;
	spGLSLTypeSize[GL_DOUBLE_MAT2x4] = sizeof(double)*8;
	spGLSLTypeSize[GL_DOUBLE_MAT3x2] = sizeof(double)*6;
	spGLSLTypeSize[GL_DOUBLE_MAT3x4] = sizeof(double)*12;
	spGLSLTypeSize[GL_DOUBLE_MAT4x2] = sizeof(double)*8;
	spGLSLTypeSize[GL_DOUBLE_MAT4x3] = sizeof(double)*12;
	
	
	spGLSLType[GL_FLOAT] = "GL_FLOAT"; 
	spGLSLType[GL_FLOAT_VEC2] = "GL_FLOAT_VEC2";  
	spGLSLType[GL_FLOAT_VEC3] = "GL_FLOAT_VEC3";  
	spGLSLType[GL_FLOAT_VEC4] = "GL_FLOAT_VEC4";  
	spGLSLType[GL_DOUBLE] = "GL_DOUBLE"; 
	spGLSLType[GL_DOUBLE_VEC2] = "GL_DOUBLE_VEC2";  
	spGLSLType[GL_DOUBLE_VEC3] = "GL_DOUBLE_VEC3";  
	spGLSLType[GL_DOUBLE_VEC4] = "GL_DOUBLE_VEC4";  
	spGLSLType[GL_SAMPLER_1D] = "GL_SAMPLER_1D";
	spGLSLType[GL_SAMPLER_2D] = "GL_SAMPLER_2D";
	spGLSLType[GL_SAMPLER_3D] = "GL_SAMPLER_3D";
	spGLSLType[GL_SAMPLER_CUBE] = "GL_SAMPLER_CUBE";
	spGLSLType[GL_SAMPLER_1D_SHADOW] = "GL_SAMPLER_1D_SHADOW";
	spGLSLType[GL_SAMPLER_2D_SHADOW] = "GL_SAMPLER_2D_SHADOW";
	spGLSLType[GL_SAMPLER_1D_ARRAY] = "GL_SAMPLER_1D_ARRAY";
	spGLSLType[GL_SAMPLER_2D_ARRAY] = "GL_SAMPLER_2D_ARRAY";
	spGLSLType[GL_SAMPLER_1D_ARRAY_SHADOW] = "GL_SAMPLER_1D_ARRAY_SHADOW";
	spGLSLType[GL_SAMPLER_2D_ARRAY_SHADOW] = "GL_SAMPLER_2D_ARRAY_SHADOW";
	spGLSLType[GL_SAMPLER_2D_MULTISAMPLE] = "GL_SAMPLER_2D_MULTISAMPLE";
	spGLSLType[GL_SAMPLER_2D_MULTISAMPLE_ARRAY] = "GL_SAMPLER_2D_MULTISAMPLE_ARRAY";
	spGLSLType[GL_SAMPLER_CUBE_SHADOW] = "GL_SAMPLER_CUBE_SHADOW";
	spGLSLType[GL_SAMPLER_BUFFER] = "GL_SAMPLER_BUFFER";
	spGLSLType[GL_SAMPLER_2D_RECT] = "GL_SAMPLER_2D_RECT";
	spGLSLType[GL_SAMPLER_2D_RECT_SHADOW] = "GL_SAMPLER_2D_RECT_SHADOW";
	spGLSLType[GL_INT_SAMPLER_1D] = "GL_INT_SAMPLER_1D";
	spGLSLType[GL_INT_SAMPLER_2D] = "GL_INT_SAMPLER_2D";
	spGLSLType[GL_INT_SAMPLER_3D] = "GL_INT_SAMPLER_3D";
	spGLSLType[GL_INT_SAMPLER_CUBE] = "GL_INT_SAMPLER_CUBE";
	spGLSLType[GL_INT_SAMPLER_1D_ARRAY] = "GL_INT_SAMPLER_1D_ARRAY";
	spGLSLType[GL_INT_SAMPLER_2D_ARRAY] = "GL_INT_SAMPLER_2D_ARRAY";
	spGLSLType[GL_INT_SAMPLER_2D_MULTISAMPLE] = "GL_INT_SAMPLER_2D_MULTISAMPLE";
	spGLSLType[GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = "GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY";
	spGLSLType[GL_INT_SAMPLER_BUFFER] = "GL_INT_SAMPLER_BUFFER";
	spGLSLType[GL_INT_SAMPLER_2D_RECT] = "GL_INT_SAMPLER_2D_RECT";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_1D] = "GL_UNSIGNED_INT_SAMPLER_1D";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_2D] = "GL_UNSIGNED_INT_SAMPLER_2D";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_3D] = "GL_UNSIGNED_INT_SAMPLER_3D";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_CUBE] = "GL_UNSIGNED_INT_SAMPLER_CUBE";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_1D_ARRAY] = "GL_UNSIGNED_INT_SAMPLER_1D_ARRAY";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_2D_ARRAY] = "GL_UNSIGNED_INT_SAMPLER_2D_ARRAY";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE] = "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_BUFFER] = "GL_UNSIGNED_INT_SAMPLER_BUFFER";
	spGLSLType[GL_UNSIGNED_INT_SAMPLER_2D_RECT] = "GL_UNSIGNED_INT_SAMPLER_2D_RECT";
	spGLSLType[GL_BOOL] = "GL_BOOL";  
	spGLSLType[GL_INT] = "GL_INT";  
	spGLSLType[GL_BOOL_VEC2] = "GL_BOOL_VEC2";
	spGLSLType[GL_INT_VEC2] = "GL_INT_VEC2";  
	spGLSLType[GL_BOOL_VEC3] = "GL_BOOL_VEC3";
	spGLSLType[GL_INT_VEC3] = "GL_INT_VEC3";  
	spGLSLType[GL_BOOL_VEC4] = "GL_BOOL_VEC4";
	spGLSLType[GL_INT_VEC4] = "GL_INT_VEC4";  
	spGLSLType[GL_UNSIGNED_INT] = "GL_UNSIGNED_INT"; 
	spGLSLType[GL_UNSIGNED_INT_VEC2] = "GL_UNSIGNED_INT_VEC2";  
	spGLSLType[GL_UNSIGNED_INT_VEC3] = "GL_UNSIGNED_INT_VEC3";  
	spGLSLType[GL_UNSIGNED_INT_VEC4] = "GL_UNSIGNED_INT_VEC4";  
	spGLSLType[GL_FLOAT_MAT2] = "GL_FLOAT_MAT2";
	spGLSLType[GL_FLOAT_MAT3] = "GL_FLOAT_MAT3";
	spGLSLType[GL_FLOAT_MAT4] = "GL_FLOAT_MAT4";
	spGLSLType[GL_FLOAT_MAT2x3] = "GL_FLOAT_MAT2x3";
	spGLSLType[GL_FLOAT_MAT2x4] = "GL_FLOAT_MAT2x4";
	spGLSLType[GL_FLOAT_MAT3x2] = "GL_FLOAT_MAT3x2";
	spGLSLType[GL_FLOAT_MAT3x4] = "GL_FLOAT_MAT3x4";
	spGLSLType[GL_FLOAT_MAT4x2] = "GL_FLOAT_MAT4x2";
	spGLSLType[GL_FLOAT_MAT4x3] = "GL_FLOAT_MAT4x3";
	spGLSLType[GL_DOUBLE_MAT2] = "GL_DOUBLE_MAT2";
	spGLSLType[GL_DOUBLE_MAT3] = "GL_DOUBLE_MAT3";
	spGLSLType[GL_DOUBLE_MAT4] = "GL_DOUBLE_MAT4";
	spGLSLType[GL_DOUBLE_MAT2x3] = "GL_DOUBLE_MAT2x3";
	spGLSLType[GL_DOUBLE_MAT2x4] = "GL_DOUBLE_MAT2x4";
	spGLSLType[GL_DOUBLE_MAT3x2] = "GL_DOUBLE_MAT3x2";
	spGLSLType[GL_DOUBLE_MAT3x4] = "GL_DOUBLE_MAT3x4";
	spGLSLType[GL_DOUBLE_MAT4x2] = "GL_DOUBLE_MAT4x2";
	spGLSLType[GL_DOUBLE_MAT4x3] = "GL_DOUBLE_MAT4x3";
}

// aux function to get the size in bytes of a uniform
// it takes the strides into account
int GLInfoLib::getUniformByteSize(int uniSize, 
				int uniType, 
				int uniArrayStride, 
				int uniMatStride) {

	int auxSize;
	if (uniArrayStride > 0)
		auxSize = uniArrayStride * uniSize;
				
	else if (uniMatStride > 0) {

		switch(uniType) {
			case GL_FLOAT_MAT2:
			case GL_FLOAT_MAT2x3:
			case GL_FLOAT_MAT2x4:
			case GL_DOUBLE_MAT2:
			case GL_DOUBLE_MAT2x3:
			case GL_DOUBLE_MAT2x4:
				auxSize = 2 * uniMatStride;
				break;
			case GL_FLOAT_MAT3:
			case GL_FLOAT_MAT3x2:
			case GL_FLOAT_MAT3x4:
			case GL_DOUBLE_MAT3:
			case GL_DOUBLE_MAT3x2:
			case GL_DOUBLE_MAT3x4:
				auxSize = 3 * uniMatStride;
				break;
			case GL_FLOAT_MAT4:
			case GL_FLOAT_MAT4x2:
			case GL_FLOAT_MAT4x3:
			case GL_DOUBLE_MAT4:
			case GL_DOUBLE_MAT4x2:
			case GL_DOUBLE_MAT4x3:
				auxSize = 4 * uniMatStride;
				break;
		}
	}
	else
		auxSize = spGLSLTypeSize[uniType];

	return auxSize;
}
