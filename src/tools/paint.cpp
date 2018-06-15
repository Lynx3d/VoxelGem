/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "paint.h"
#include "voxelscene.h"
#include "sceneproxy.h"

void PaintTool::mouseDown(const ToolEvent &event)
{
	picking = event.isShiftPressed();
	haveLastPos = false;
	processDragEvent(event);
}

void PaintTool::mouseUp(const ToolEvent &event)
{
	if (haveLastPos)
		completeAction();
	haveLastPos = false;
}

void PaintTool::mouseMoved(const ToolEvent &event)
{
	processDragEvent(event);
}

void PaintTool::processDragEvent(const ToolEvent &event)
{
	IVector3D curPos;
	const VoxelEntry *voxel = getCursorVoxelEdit(event, curPos);
	if (!voxel)
		return;

	if (haveLastPos && lastPos[0] == curPos[0] && lastPos[1] == curPos[1] && lastPos[2] == curPos[2])
		return;

	if (picking)
	{
		sceneProxy->setTemplateColor(voxel->col);
	}
	else
	{
		scene->setVoxel(curPos, *scene->getVoxelTemplate());
	}
	lastPos = curPos;
	haveLastPos = true;
}

ToolInstance* PaintTool::getInstance()
{
	ToolInstance* instance = new ToolInstance;
	instance->tool = new PaintTool();
	instance->icon.addFile(QStringLiteral(":/images/gfx/icons/paintbrush.svg"));
	instance->toolTip = "Paint Voxels";
	instance->statusTip = "Press Shift to pick color";
	return instance;
}
