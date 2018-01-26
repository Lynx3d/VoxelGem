/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include "ui_mainwindow.h"
#include "glviewport.h"
#include "palette.h"

int main(int argc, char **argv)
{
	Q_INIT_RESOURCE(resources);

	// request proper OpenGL version
	QSurfaceFormat format;
	format.setDepthBufferSize(32);
	format.setVersion(3, 3);
	format.setRenderableType(QSurfaceFormat::OpenGL);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	format.setColorSpace(QSurfaceFormat::sRGBColorSpace); // Qt 5.10+
#endif
	QSurfaceFormat::setDefaultFormat(format);

	// create application and load gui
	QApplication app(argc, argv);
	QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
	// create our specialized QOpenGlWidget
	GlViewportWidget glvp(NULL);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	glvp.setTextureFormat(GL_SRGB8_ALPHA8); /* Qt 5.10+ */
#endif
	ui.glparent->layout()->addWidget(&glvp);
	
	// color palette test
	ColorSet* testPalette = getTestPalette();
	ColorPaletteModel paletteModel;
	ColorPaletteView paletteView;
	paletteView.setPaletteModel(&paletteModel);
	paletteModel.setColorSet(testPalette);
	ui.gridLayout_3->addWidget(&paletteView, 1, 0, 1, 1);
	
    window.show();

	return app.exec();
}
