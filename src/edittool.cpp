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

ToolEvent::ToolEvent(QMouseEvent *qmEvent, SceneRayHit *srHit):
	mouseEvent(qmEvent), rayHit(srHit)
{}

bool ToolEvent::getAdjacentVoxel(int pos[3]) const
{
	if ((rayHit->flags & SceneRayHit::HIT_MASK) == 0)
		return false;
	pos[0] = rayHit->voxelPos[0];
	pos[1] = rayHit->voxelPos[1];
	pos[2] = rayHit->voxelPos[2];
	// TODO: invert meaning of SceneRayHit::AXIS_NEGATIVE
	pos[rayHit->flags & SceneRayHit::AXIS_MASK] += (rayHit->flags & SceneRayHit::AXIS_NEGATIVE) ? 1 : -1;
	return true;
}

const SceneRayHit* ToolEvent::getCursorHit() const
{
	return rayHit;
}

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

void EditTool::mouseMoved(const ToolEvent &event)
{}

void EditTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::mouseUp(const ToolEvent &event, VoxelScene &scene)
{}

/*========= PaintTool TODO: move to proper file ==========*/
#include "voxelgrid.h" // for VoxelEntry...
void PaintTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
{
	const SceneRayHit *hit = event.getCursorHit();
	if ((hit->flags & SceneRayHit::HIT_MASK) == 0)
		return;

	if (event.isShiftPressed())
		scene.eraseVoxel(hit->voxelPos);
	else
	{
		VoxelEntry vox = { { 128, 128, 255, 255 }, VF_NON_EMPTY };
		int fillPos[3];
		event.getAdjacentVoxel(fillPos);
		scene.setVoxel(fillPos, vox);
	}
}
