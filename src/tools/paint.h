/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_TOOL_PAINT_H
#define VG_TOOL_PAINT_H

#include "edittool.h"

class PaintTool: public EditTool
{
	public:
		void mouseMoved(const ToolEvent &event) override;
		void mouseDown(const ToolEvent &event) override;
		void mouseUp(const ToolEvent &event) override;
		static ToolInstance* getInstance();
	protected:
		void processDragEvent(const ToolEvent &event);
		bool painting = false;
		//bool deleting;
		bool haveLastPos;
		bool picking;
		IVector3D lastPos;
};

#endif // VG_TOOL_PAINT_H
