/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QSurfaceFormat>
#include "mainwindow.h"

int main(int argc, char **argv)
{
	Q_INIT_RESOURCE(resources);

	// request proper OpenGL version
	QSurfaceFormat format;
	format.setDepthBufferSize(32);
	format.setVersion(3, 3);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
#if DEBUG_GL
	format.setOption(QSurfaceFormat::DebugContext);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	format.setColorSpace(QSurfaceFormat::sRGBColorSpace); // Qt 5.10+
#endif
	QSurfaceFormat::setDefaultFormat(format);

	// create application and load gui
	QApplication app(argc, argv);

	VGMainWindow window;
    window.show();

	return app.exec();
}
