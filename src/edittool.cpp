/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "edittool.h"
#include "glviewport.h"
#include "voxelscene.h"
#include "sceneproxy.h"

#include <QMouseEvent>

ToolEvent::ToolEvent(GlViewportWidget *vp, QMouseEvent *qmEvent, ray_t cRay):
	viewport(vp), mouseEvent(qmEvent), cursorRay(cRay)
{}

const RenderOptions& ToolEvent::getRenderOptions() const
{
	return viewport->getRenderOptions();
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

QVector3D ToolEvent::cursorPos() const
{
	// TODO: decide if origin shall be OpenGL (bottom left) or window system (top left)
	return QVector3D(mouseEvent->localPos());
}

/*========= EditTool ==========*/

EditTool::~EditTool()
{}

void EditTool::initialize(VoxelScene *vs)
{
	scene = vs;
}

void EditTool::mouseMoved(const ToolEvent &event)
{}

void EditTool::mouseDown(const ToolEvent &event)
{}

void EditTool::mouseUp(const ToolEvent &event)
{}

void EditTool::completeAction()
{
	if (!sceneProxy)
		return;
	sceneProxy->completeToolAction();
}

bool EditTool::getCursorVoxelAdd(const ToolEvent &event, IVector3D &pos, SceneRayHit *hit) const
{
	bool sliceMode = event.getRenderOptions().mode == RenderOptions::MODE_SLICE;
	int rayFlags = SceneRayHit::HIT_LINEGRID;
	if (!sliceMode)
		rayFlags |= SceneRayHit::HIT_VOXEL;

	SceneRayHit tmpHit;
	SceneRayHit &rayHit = hit ? *hit : tmpHit;
	scene->rayIntersect(event.getCursorRay(), rayHit, rayFlags);
	if (!rayHit.didHit())
		return false;

	if (sliceMode)
	{
		// only one valid pos when we only test grid plane
		pos = rayHit.voxelPos;
		// TODO: invert meaning of SceneRayHit::AXIS_NEGATIVE
		if (rayHit.flags & SceneRayHit::AXIS_NEGATIVE)
			pos[rayHit.flags & SceneRayHit::AXIS_MASK] += 1;
	}
	else
	{
		rayHit.getAdjacentVoxel(pos);
	}
	return true;
}

const VoxelEntry* EditTool::getCursorVoxelEdit(const ToolEvent &event, IVector3D &pos, SceneRayHit *hit) const
{
	bool sliceMode = event.getRenderOptions().mode == RenderOptions::MODE_SLICE;
	int rayFlags = sliceMode ? SceneRayHit::HIT_LINEGRID : SceneRayHit::HIT_VOXEL;

	SceneRayHit tmpHit;
	SceneRayHit &rayHit = hit ? *hit : tmpHit;
	scene->rayIntersect(event.getCursorRay(), rayHit, rayFlags);
	if (!rayHit.didHit())
		return nullptr;

	pos = rayHit.voxelPos;
	if (sliceMode)
	{
		// only one valid pos when we only test grid plane
		// TODO: invert meaning of SceneRayHit::AXIS_NEGATIVE
		if (rayHit.flags & SceneRayHit::AXIS_NEGATIVE)
			pos[rayHit.flags & SceneRayHit::AXIS_MASK] += 1;
	}
	const VoxelEntry *entry = scene->getVoxel(pos);
	if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY))
		return nullptr;
	return entry;
}
