/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_LAYEREDITOR_H
#define VG_LAYEREDITOR_H

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <vector>

namespace Ui {
	class Layers;
}
class IBBox;
class SceneProxy;
class VoxelLayer;
class VoxelScene;
class QBoxLayout;
class QDataWidgetMapper;
class QLabel;
class QPaintEvent;
class LayerWidget;

class LayerEditor: public QObject
{
	Q_OBJECT
	public:
		LayerEditor(QWidget *parent, SceneProxy *manager);
		virtual ~LayerEditor();
		void activateLayer(int layerNum);
		void setVisibility(int layerNum, bool visible);
		void editLayerName(int layerN);
	public Q_SLOTS:
		void layerCreated(int layerN);
		void layerDeleted(int layerN);
		void layerMoved(int layerN, int targetN);
		void layerSettingsChanged(int layerN, int change_flags);
		void activeLayerChanged(int layerN, int prev);
		void on_nameEdit_editingFinished();
		void on_layer_bound_toggled(bool enabled);
		void on_action_add_layer_triggered();
		void on_action_layer_up_triggered();
		void on_action_layer_down_triggered();
		void on_action_delete_layer_triggered();
	protected:
		void adjustLowerBound(int val, int index);
		void adjustUpperBound(int val, int index);
		void loadLayerDetails(int layerN);
		Ui::Layers *ui;
		QBoxLayout *layerStackLayout;
		QLineEdit *nameEdit;
		std::vector<LayerWidget*> layerWidgets;
		SceneProxy *hub;
		QIcon *eyeIcon;
};

class LayerWidget: public QWidget
{
	Q_OBJECT
	public:
		LayerWidget(QWidget *parent, LayerEditor *editor, const QIcon *stateIcon);
		bool activeStatus() { return layerActive; }
		void setActiveStatus(bool active);
		void setVisibilityStatus(bool visible);
		void setLayerName(const QString &name);
		void setLayerNum(int num) { layerNum = num; }
	protected:
		void mousePressEvent(QMouseEvent *event) override;
		void mouseDoubleClickEvent(QMouseEvent *event) override;
		void paintEvent(QPaintEvent *event) override;

		static QRect visibilityRect;

		LayerEditor *layerEditor;
		const QIcon *visibilityIcon;
		QLabel *layerName;
		bool layerActive;
		bool layerVisible;
		int layerNum;
};

#endif // VG_LAYEREDITOR_H

