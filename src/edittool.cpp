/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "edittool.h"
#include "voxelscene.h"
#include "sceneproxy.h"

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

QVector3D ToolEvent::cursorPos() const
{
	// TODO: decide if origin shall be OpenGL (bottom left) or window system (top left)
	return QVector3D(mouseEvent->localPos());
}

/*========= EditTool ==========*/

void EditTool::mouseMoved(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::mouseDown(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::mouseUp(const ToolEvent &event, VoxelScene &scene)
{}

void EditTool::completeAction()
{
	if (!sceneProxy)
		return;
	sceneProxy->completeToolAction();
}
