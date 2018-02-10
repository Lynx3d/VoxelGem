/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "glviewport.h"
#include "palette.h"
#include "voxelscene.h"

VGMainWindow::VGMainWindow():
	mainUi(new Ui::MainWindow),
	paletteView(new ColorPaletteView),
	scene(new VoxelScene)
{
	mainUi->setupUi(this);
	viewport = new GlViewportWidget(scene),
	// takes care of "gamma correction" (sRGB transfer curve is not strictly a gamma function)
	#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	viewport->setTextureFormat(GL_SRGB8_ALPHA8); /* Qt 5.10+ */
	#endif
	mainUi->glparent->layout()->addWidget(viewport);
	// color palette
	ColorPaletteModel *paletteModel = new ColorPaletteModel;
	colorSet = getTestPalette();
	paletteModel->setColorSet(colorSet);
	paletteView->setPaletteModel(paletteModel);
	mainUi->gridLayout_3->addWidget(paletteView, 5, 0, 1, 1);
	connect(mainUi->colorswatch, SIGNAL(colorSelectionChanged(QColor)), this, SLOT(on_colorSelectionChanged(QColor)));
	connect(this, SIGNAL(colorSelectionChanged(QColor)), mainUi->colorswatch, SLOT(on_colorSelectionChanged(QColor)));
}

VGMainWindow::~VGMainWindow()
{
	// the widgets of mainUi aswell as viewport etc. are deleted through ~QObject
	// because the mainwindow takes ownership.
	// keeping mainUi is purely for easy access.
	delete mainUi;
	ColorPaletteModel *paletteModel = paletteView->getPaletteModel();
	delete paletteView;
	delete paletteModel;
	// scene and palette are currently tied to mainwindow instance
	delete scene;
	delete colorSet;
}

void VGMainWindow::on_action_axis_grids_triggered(bool checked)
{
}

void VGMainWindow::on_material_currentIndexChanged(int index)
{
	if (index >= 0 && index <= Voxel::GLOWING_GLASS)
	scene->setTemplateMaterial(static_cast<Voxel::Material>(index));
}

void VGMainWindow::on_specular_currentIndexChanged(int index)
{
	if (index >= 0 && index <= Voxel::WAXY)
	scene->setTemplateSpecular(static_cast<Voxel::Specular>(index));
}

void VGMainWindow::on_colorSelectionChanged(QColor col)
{
	scene->setTemplateColor(col.red(), col.green(), col.blue(), col.alpha());
	emit(colorSelectionChanged(col));
}
