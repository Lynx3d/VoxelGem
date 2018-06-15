/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "draw.h"
#include "voxelscene.h"

void DrawTool::mouseDown(const ToolEvent &event)
{
	haveLastPos = false;
	deleting = event.isShiftPressed();
	processDragEvent(event);
}

void DrawTool::mouseUp(const ToolEvent &event)
{
	if (haveLastPos)
		completeAction();
	haveLastPos = false;
}

void DrawTool::mouseMoved(const ToolEvent &event)
{
	processDragEvent(event);
}

void DrawTool::processDragEvent(const ToolEvent &event)
{
	IVector3D curPos;
	if (deleting)
	{
		const VoxelEntry *voxel = getCursorVoxelEdit(event, curPos);
		if (!voxel)
			return;
		if (haveLastPos && lastPos == curPos)
			return;

		scene->eraseVoxel(curPos);
	}
	else
	{
		if(!getCursorVoxelAdd(event, curPos))
			return;
		if (haveLastPos && lastPos == curPos)
			return;
		scene->setVoxel(curPos, *scene->getVoxelTemplate());
	}
	lastPos = curPos;
	haveLastPos = true;
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
