/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "draw.h"
#include "voxelscene.h"

void DrawTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
{
//	static bool test = false;
	SceneRayHit hit;
	scene.rayIntersect(event.getCursorRay(), hit);
	if (!hit.didHit())
		return;

	if (event.isShiftPressed())
	{
		deleting = true;
		scene.eraseVoxel(hit.voxelPos);
		lastPos = hit.voxelPos;
		haveLastPos = true;
	}
	else
	{
		deleting = false;
		IVector3D fillPos;
		hit.getAdjacentVoxel(fillPos);
		scene.setVoxel(fillPos, *scene.getVoxelTemplate());
		lastPos = fillPos;
		haveLastPos = true;
	}
}

void DrawTool::mouseUp(const ToolEvent &event, VoxelScene &scene)
{
	haveLastPos = false;
	completeAction();
}

void DrawTool::mouseMoved(const ToolEvent &event, VoxelScene &scene)
{
	//const SceneRayHit *hit = event.getCursorHit();
	SceneRayHit hit;
	scene.rayIntersect(event.getCursorRay(), hit);
	if (!hit.didHit())
		return;

	if (deleting)
	{
		if (haveLastPos)
		{
			IVector3D fillPos;
			hit.getAdjacentVoxel(fillPos);
			if (lastPos[0] == fillPos[0] && lastPos[1] == fillPos[1] && lastPos[2] == fillPos[2])
				return;
		}
		lastPos = hit.voxelPos;
		haveLastPos = true;
		scene.eraseVoxel(hit.voxelPos);
	}
	else
	{
		IVector3D fillPos;
		hit.getAdjacentVoxel(fillPos);
		if (haveLastPos && lastPos[0] == fillPos[0] && lastPos[1] == fillPos[1] && lastPos[2] == fillPos[2])
			return;
		scene.setVoxel(fillPos, *scene.getVoxelTemplate());
		lastPos = fillPos;
		haveLastPos = true;
	}
}

ToolInstance* DrawTool::getInstance()
{
	ToolInstance* instance = new ToolInstance;
	instance->tool = new DrawTool();
	instance->icon.addFile(QStringLiteral(":/images/gfx/icons/pencil.svg"));
	instance->toolTip = "Draw Voxels";
	instance->statusTip = "Press Shift to remove voxels";
	return instance;
}
