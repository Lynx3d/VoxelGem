/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "voxelscene.h"
#include "voxelaggregate.h"
#include "glviewport.h"

VoxelScene::VoxelScene(): viewport(0), voxelTemplate(128, 128, 255, 255)
{
	renderLayer = new VoxelAggregate();
}

VoxelScene::~VoxelScene()
{
	delete renderLayer;
}

/* TODO:
	So far we only have a render layer!
	Editing layer and multiple scene layers	are yet to be done.
*/

void VoxelScene::setVoxel(const int pos[3], const VoxelEntry &voxel)
{
	renderLayer->setVoxel(pos[0], pos[1], pos[2], voxel);
}

void VoxelScene::eraseVoxel(const int pos[3])
{
	renderLayer->setVoxel(pos[0], pos[1], pos[2], VoxelEntry());
}

// TODO!
const VoxelEntry* VoxelScene::getVoxel(const int pos[3])
{
	return 0;
}
