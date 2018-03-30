/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "edittool.h"
#include "glviewport.h"
#include "palette.h"
#include "gui/layereditor.h"
#include "voxelscene.h"
#include "sceneproxy.h"

#include <QFileDialog>
#include <QActionGroup>
#include <QIcon>

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
	// setup toolbar
	toolGroup = new QActionGroup(this);
	connect(toolGroup, SIGNAL(triggered(QAction *)), this, SLOT(on_toolActionTriggered(QAction *)));
	connect(this, SIGNAL(activeToolChanged(EditTool *)), viewport, SLOT(on_activeToolChanged(EditTool *)));
	// color palette
	ColorPaletteModel *paletteModel = new ColorPaletteModel;
	colorSet = getTestPalette();
	paletteModel->setColorSet(colorSet);
	paletteView->setPaletteModel(paletteModel);
	mainUi->gridLayout_3->addWidget(paletteView, 5, 0, 1, 1);
	connect(mainUi->colorswatch, SIGNAL(colorSelectionChanged(QColor)), this, SLOT(on_colorSelectionChanged(QColor)));
	connect(this, SIGNAL(colorSelectionChanged(QColor)), mainUi->colorswatch, SLOT(on_colorSelectionChanged(QColor)));
	connect(paletteView, SIGNAL(entrySelected(const ColorSetEntry &)), this, SLOT(on_colorSetEntrySelected(const ColorSetEntry &)));
	// layer editor
	sceneProxy = new SceneProxy(scene, this);
	LayerEditor *layer_ed = new LayerEditor(mainUi->layers, sceneProxy);
	layer_ed->setParent(this);
	// TODO: load tools in a better place...
	ToolInstance *testTool = PaintTool::getInstance();
	addTool(testTool);
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

void VGMainWindow::addTool(ToolInstance *tool)
{
	QAction *action = new QAction(tool->icon, tool->toolTip, toolGroup);
	action->setCheckable(true);
	action->setStatusTip(tool->statusTip);
	action->setAutoRepeat(false);
	mainUi->toolbar_edit->addAction(action);
	if (!toolGroup->checkedAction())
	{
		action->setChecked(true);
		// this apparently does not trigger QActionGroup::triggered()
		emit(activeToolChanged(tool->tool));
	}
	tool->tool->setSceneProxy(sceneProxy);
	toolMap[action] = tool->tool;
}

void VGMainWindow::on_action_axis_grids_triggered(bool checked)
{
}

void VGMainWindow::on_action_undo_triggered()
{
	sceneProxy->undo();
	if (scene->needsUpdate())
	{
		scene->update();
		viewport->update();
	}
}

void VGMainWindow::on_action_redo_triggered()
{
	sceneProxy->redo();
	if (scene->needsUpdate())
	{
		scene->update();
		viewport->update();
	}
}

// TODO: create header
void qubicle_import(const QString &filename, VoxelScene &scene);
void qubicle_export(const QString &filename, VoxelScene &scene);
void qubicle_export_layer(const QString &filename, SceneProxy *sceneP);

void VGMainWindow::on_action_open_triggered()
{
	QString browseDir;
	QString fileName = QFileDialog::getOpenFileName(this, "Open File",
			browseDir, "Qubicle (*.qb)");
	if (fileName.isEmpty())
		return;
	qubicle_import(fileName, *scene);
}

void VGMainWindow::on_action_save_triggered()
{
	QString browseDir;
	QString fileName = QFileDialog::getSaveFileName(this, "Open File",
			browseDir, "Qubicle (*.qb)");
	if (fileName.isEmpty())
		return;
	// TODO: add proper extension if not entered
	qubicle_export(fileName, *scene);
}

void VGMainWindow::on_action_export_layer_triggered()
{
	QString browseDir;
	QString fileName = QFileDialog::getSaveFileName(this, "Open File",
			browseDir, "Qubicle (*.qb)");
	if (fileName.isEmpty())
		return;
	// TODO: add proper extension if not entered
	qubicle_export_layer(fileName, sceneProxy);
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

void VGMainWindow::on_colorSetEntrySelected(const ColorSetEntry &entry)
{
	QColor col = entry.color;
	scene->setTemplateColor(col.red(), col.green(), col.blue(), col.alpha());
	emit(colorSelectionChanged(col));
}

void VGMainWindow::on_toolActionTriggered(QAction *action)
{
	// NOTE: clicking on an action will trigger even if it is already checked
	EditTool *tool = 0;
	auto entry = toolMap.find(action);
	if (entry != toolMap.end())
		tool = entry->second;
	emit(activeToolChanged(tool));
}
