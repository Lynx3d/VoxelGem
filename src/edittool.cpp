/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "edittool.h"
#include "voxelscene.h"

#include <QMouseEvent>

ToolEvent::ToolEvent(QMouseEvent *qmEvent, ray_t cRay):
	mouseEvent(qmEvent), cursorRay(cRay)
{}

bool ToolEvent::isShiftPressed() const
{
	return mouseEvent->modifiers() & Qt::ShiftModifier;
}

bool ToolEvent::isAltPressed() const
{
	return mouseEvent->modifiers() & Qt::AltModifier;
}

bool ToolEvent::isControlPressed() const
{
	return mouseEvent->modifiers() & Qt::ControlModifier;
}

/*========= EditTool ==========*/

void EditTool::mouseMoved(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::mouseUp(const ToolEvent &event, VoxelScene &scene)
{}

/*========= PaintTool TODO: move to proper file ==========*/
#include "voxelgrid.h" // for VoxelEntry...
void PaintTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
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
		lastPos[0] = hit.voxelPos[0];
		lastPos[1] = hit.voxelPos[1];
		lastPos[2] = hit.voxelPos[2];
		haveLastPos = true;
	}
	else
	{
		deleting = false;
		int fillPos[3];
		hit.getAdjacentVoxel(fillPos);
		scene.setVoxel(fillPos, *scene.getVoxelTemplate());
		lastPos[0] = fillPos[0];
		lastPos[1] = fillPos[1];
		lastPos[2] = fillPos[2];
		haveLastPos = true;
	}
}

void PaintTool::mouseUp(const ToolEvent &event, VoxelScene &scene)
{
	haveLastPos = false;
	scene.completeToolAction();
}

void PaintTool::mouseMoved(const ToolEvent &event, VoxelScene &scene)
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
			int fillPos[3];
			hit.getAdjacentVoxel(fillPos);
			if (lastPos[0] == fillPos[0] && lastPos[1] == fillPos[1] && lastPos[2] == fillPos[2])
				return;
		}
		lastPos[0] = hit.voxelPos[0];
		lastPos[1] = hit.voxelPos[1];
		lastPos[2] = hit.voxelPos[2];
		haveLastPos = true;
		scene.eraseVoxel(hit.voxelPos);
	}
	else
	{
		int fillPos[3];
		hit.getAdjacentVoxel(fillPos);
		if (haveLastPos && lastPos[0] == fillPos[0] && lastPos[1] == fillPos[1] && lastPos[2] == fillPos[2])
			return;
		scene.setVoxel(fillPos, *scene.getVoxelTemplate());
		lastPos[0] = fillPos[0];
		lastPos[1] = fillPos[1];
		lastPos[2] = fillPos[2];
		haveLastPos = true;
	}
}

ToolInstance* PaintTool::getInstance()
{
	ToolInstance* instance = new ToolInstance;
	instance->tool = new PaintTool();
	instance->icon.addFile(QStringLiteral(":/images/gfx/icons/pencil.svg"));
	instance->toolTip = "Draw Voxels";
	instance->statusTip = "Press Shift to remove voxels";
	return instance;
}
