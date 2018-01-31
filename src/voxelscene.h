/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_VOXELSCENE_H
#define VG_VOXELSCENE_H

class VoxelEntry;
class VoxelAggregate;
class GlViewportWidget;

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
	protected:
		GlViewportWidget *viewport;
		VoxelAggregate *renderLayer;
};

#endif // VG_VOXELSCENE_H
