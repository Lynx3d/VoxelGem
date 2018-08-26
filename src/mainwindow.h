/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_MAINWINDOW_H
#define VG_MAINWINDOW_H

#include <voxelgem.h>
#include <map>
#include <QMainWindow>

namespace Ui {
	class MainWindow;
}
class VoxelScene;
class SceneProxy;
class GlViewportWidget;
class ColorPaletteView;
class ColorSet;
class ColorSetEntry;
class EditTool;
class ToolInstance;
class QAction;
class QActionGroup;

class VGMainWindow : public QMainWindow
{
    Q_OBJECT
	public:
		VGMainWindow();
		virtual ~VGMainWindow();
		void addTool(ToolInstance *tool);
	protected:
		void loadTools();
	private Q_SLOTS:
		void on_action_axis_grids_triggered(bool checked);
		void on_action_open_triggered();
		void on_action_save_triggered();
		void on_action_export_trove_triggered();
		void on_action_export_layer_triggered();
		void on_action_undo_triggered();
		void on_action_redo_triggered();
		void on_action_rotate_x_triggered();
		void on_action_rotate_y_triggered();
		void on_action_rotate_z_triggered();
		void on_action_mirror_x_triggered();
		void on_action_mirror_y_triggered();
		void on_action_mirror_z_triggered();
		void on_action_translate_dialog_triggered();
		void on_action_merge_down_triggered();
		void on_material_currentIndexChanged(int index);
		void on_specular_currentIndexChanged(int index);
		void on_colorSelectionChanged(QColor col);
		void on_templateColorChanged(rgba_t col);
		void on_colorSetEntrySelected(const ColorSetEntry &entry);
		void on_toolActionTriggered(QAction *action);
		void on_viewModeActionTriggered(QAction *action);
	Q_SIGNALS:
		void colorSelectionChanged(QColor col);
		void activeToolChanged(EditTool *tool);
	// TODO: tool list
	private:
		Ui::MainWindow *mainUi;
		QActionGroup *toolGroup;
		QActionGroup *viewModeGroup;
		std::map<QAction *, EditTool *> toolMap;
		GlViewportWidget *viewport;
		ColorPaletteView *paletteView;
		ColorSet *colorSet;
		VoxelScene *scene;
		SceneProxy *sceneProxy;
};

#endif // VG_MAINWINDOW_H
