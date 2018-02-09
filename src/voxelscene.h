/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELSCENE_H
#define VG_VOXELSCENE_H

#include "voxelgem.h"

class VoxelEntry;
class VoxelAggregate;
class GlViewportWidget;

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
		int voxelPos[3];
		int flags { 0 };
		float rayT;
};

/*! This class holds all the "scene" data for a file edit session.
	Includes viewport and various UI information required for
	editing tools to work on the scene.
	Maybe another class holding application-wide state information
	such as tool settings might be useful though...
*/

class VoxelScene
{
	friend class GlViewportWidget;
	public:
		VoxelScene();
		~VoxelScene();
		/* set a voxel of the editing layer */
		void setVoxel(const int pos[3], const VoxelEntry &voxel);
		/* mark a voxel as erased in the editing layer */
		void eraseVoxel(const int pos[3]);
		/* read a voxel from the scene (exclude current edit changes) */
		const VoxelEntry* getVoxel(const int pos[3]);
		const VoxelEntry* getVoxelTemplate() const { return &voxelTemplate; }
		void setTemplateMaterial(Voxel::Material mat) { voxelTemplate.setMaterial(mat); }
		void setTemplateSpecular(Voxel::Specular spec) { voxelTemplate.setSpecular(spec); }
	protected:
		GlViewportWidget *viewport;
		VoxelAggregate *renderLayer;
		VoxelEntry voxelTemplate;
};

#endif // VG_VOXELSCENE_H
