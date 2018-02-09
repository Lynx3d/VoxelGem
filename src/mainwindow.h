/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_MAINWINDOW_H
#define VG_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
	class MainWindow;
}
class VoxelScene;
class GlViewportWidget;
class ColorPaletteView;
class ColorSet;

class VGMainWindow : public QMainWindow
{
    Q_OBJECT
	public:
		VGMainWindow();
		virtual ~VGMainWindow();
	protected:
		void loadTools();
	private slots:
		void on_action_axis_grids_triggered(bool checked);
		void on_material_currentIndexChanged(int index);
		void on_specular_currentIndexChanged(int index);
	// TODO: tool list
	private:
		Ui::MainWindow *mainUi;
		GlViewportWidget *viewport;
		ColorPaletteView *paletteView;
		ColorSet *colorSet;
		VoxelScene *scene;
};

#endif // VG_MAINWINDOW_H
