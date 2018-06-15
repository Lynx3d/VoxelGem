/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_TOOL_DRAW_H
#define VG_TOOL_DRAW_H

#include "edittool.h"

class DrawTool: public EditTool
{
	public:
		void mouseMoved(const ToolEvent &event) override;
		void mouseDown(const ToolEvent &event) override;
		void mouseUp(const ToolEvent &event) override;
		static ToolInstance* getInstance();
	private:
		void processDragEvent(const ToolEvent &event);
		bool deleting;
		bool haveLastPos;
		IVector3D lastPos;
};

#endif // VG_TOOL_DRAW_H
