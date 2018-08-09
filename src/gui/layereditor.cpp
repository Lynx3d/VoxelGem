/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "layereditor.h"
#include "voxelscene.h"
#include "sceneproxy.h"
#include "gui/ui_layereditor.h"

#include <QDataWidgetMapper>
#include <QBoxLayout>
#include <QSignalBlocker>

#include <iostream>
#include <cmath>
#include <cassert>

LayerEditor::LayerEditor(QWidget *parent, SceneProxy *manager):
	hub(manager)
{
	ui = new Ui::Layers;
	ui->setupUi(parent);
	ui->btn_add_layer->setDefaultAction(ui->action_add_layer);
	ui->btn_delete->setDefaultAction(ui->action_delete_layer);
	/* connect(ui->layer_list->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
			this, SLOT(selectionChanged(const QModelIndex &, const QModelIndex &))); */
	// test
	// meh, QOverload requires Qt 5.7... // connect(ui->sb_min_x, QOverload<int>::of(&QSpinBox::valueChanged),
	connect(ui->sb_min_x, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustLowerBound(val, 0); });
	connect(ui->sb_min_y, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustLowerBound(val, 1); });
	connect(ui->sb_min_z, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustLowerBound(val, 2); });
	connect(ui->sb_max_x, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustUpperBound(val, 0); });
	connect(ui->sb_max_y, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustUpperBound(val, 1); });
	connect(ui->sb_max_z, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int val){ this->adjustUpperBound(val, 2); });
	connect(ui->group_bound, &QGroupBox::clicked, this, &LayerEditor::on_layer_bound_toggled);
	connect(ui->action_add_layer, &QAction::triggered, this, &LayerEditor::on_action_add_layer_triggered);
	connect(ui->action_delete_layer, &QAction::triggered, this, &LayerEditor::on_action_delete_layer_triggered);
	connect(manager, &SceneProxy::layerCreated, this, &LayerEditor::layerCreated);
	connect(manager, &SceneProxy::layerDeleted, this, &LayerEditor::layerDeleted);
	connect(manager, &SceneProxy::activeLayerChanged, this, &LayerEditor::activeLayerChanged);
	connect(manager, &SceneProxy::layerSettingsChanged, this, &LayerEditor::layerSettingsChanged);
	// TODO: handle layerSettingsChanged() signal to catch outside changes
	layerStackLayout = new QBoxLayout(QBoxLayout::BottomToTop, ui->layer_stack);
	eyeIcon = new QIcon(":/images/gfx/icons/visible_off.svg");//(ui->action_add_layer->icon());
	eyeIcon->addFile(":/images/gfx/icons/visible.svg", QSize(), QIcon::Normal, QIcon::On);
	//
	for (int i = 0; i < hub->layerCount(); ++i)
	{
		const VoxelLayer *layer = hub->getLayer(i);
		LayerWidget *widget = new LayerWidget(ui->layer_stack, this, eyeIcon);
		layerStackLayout->insertWidget(-1, widget);
		widget->setLayerName(QString::fromStdString(layer->name));
		widget->setLayerNum(i);
		layerWidgets.push_back(widget);
	}
	layerWidgets[hub->activeLayer()]->setActiveStatus(true);
	loadLayerDetails(hub->activeLayer());
	// must be last, and stay last on widget additions
	layerStackLayout->addStretch();
	// edit widget for renames
	nameEdit = new QLineEdit(ui->layer_stack);
	nameEdit->hide();
	connect(nameEdit, &QLineEdit::editingFinished, this, &LayerEditor::on_nameEdit_editingFinished);
}

LayerEditor::~LayerEditor()
{
	delete ui;
}

void LayerEditor::adjustLowerBound(int val, int index)
{
	assert(index >= 0 && index < 3);
	const VoxelLayer *layer = hub->getLayer(hub->activeLayer());
	IBBox newBound(layer->bound);
	newBound.pMin[index] = val;

	/* if (index == 0)
		ui->sb_max_x->setMinimum(val + 1);
	else if (index == 1)
		ui->sb_max_y->setMinimum(val + 1);
	else
		ui->sb_max_z->setMinimum(val + 1); */

	hub->setLayerBound(hub->activeLayer(), newBound);
}

void LayerEditor::adjustUpperBound(int val, int index)
{
	assert(index >= 0 && index < 3);
	const VoxelLayer *layer = hub->getLayer(hub->activeLayer());
	IBBox newBound(layer->bound);
	newBound.pMax[index] = val;

	/* if (index == 0)
		ui->sb_min_x->setMaximum(val - 1);
	else if (index == 1)
		ui->sb_min_y->setMaximum(val - 1);
	else
		ui->sb_min_z->setMaximum(val - 1); */

	hub->setLayerBound(hub->activeLayer(), newBound);
}

void LayerEditor::editLayerName(int layerN)
{
	QRect geometry = layerWidgets[layerN]->geometry();
	nameEdit->move(geometry.topLeft() + QPoint(30, 2));
	nameEdit->setFixedWidth(geometry.width() - 30);
	nameEdit->setText(QString::fromStdString(hub->getLayer(layerN)->name));
	nameEdit->raise();
	nameEdit->show();
	nameEdit->setFocus();
}

void LayerEditor::loadLayerDetails(int layerN)
{
	// we don't want signals from modifying the UI elements here:
	QSignalBlocker b1(ui->sb_min_x), b2(ui->sb_min_y), b3(ui->sb_min_z),
					b4(ui->sb_max_x), b5(ui->sb_max_y), b6(ui->sb_max_z);
	const VoxelLayer *layer = hub->getLayer(layerN);
	ui->sb_min_x->setValue(layer->bound.pMin.x);
	ui->sb_min_x->setMaximum(layer->bound.pMax.x - 1);
	ui->sb_min_y->setValue(layer->bound.pMin.y);
	ui->sb_min_y->setMaximum(layer->bound.pMax.y - 1);
	ui->sb_min_z->setValue(layer->bound.pMin.z);
	ui->sb_min_z->setMaximum(layer->bound.pMax.z - 1);
	ui->sb_max_x->setValue(layer->bound.pMax.x);
	ui->sb_max_x->setMinimum(layer->bound.pMin.x + 1);
	ui->sb_max_y->setValue(layer->bound.pMax.y);
	ui->sb_max_y->setMinimum(layer->bound.pMin.y + 1);
	ui->sb_max_z->setValue(layer->bound.pMax.z);
	ui->sb_max_z->setMinimum(layer->bound.pMin.z + 1);
	ui->group_bound->setChecked(layer->useBound);
}

void LayerEditor::activateLayer(int layerNum)
{
	if (layerNum >= (int)layerWidgets.size() || layerNum < 0)
		return;
	// TODO: actually sync with scene and update current active widget
	//layerWidgets[layerNum]->setActiveStatus(true);
	hub->setActiveLayer(layerNum);
}

void LayerEditor::setVisibility(int layerNum, bool visible)
{
	if (layerNum >= (int)layerWidgets.size() || layerNum < 0)
		return;
	hub->setLayerVisibility(layerNum, visible);
}

void LayerEditor::layerCreated(int layerN)
{
	const VoxelLayer *layer = hub->getLayer(layerN);
	LayerWidget *widget = new LayerWidget(ui->layer_stack, this, eyeIcon);
	layerStackLayout->insertWidget(layerN, widget);
	widget->setLayerName(QString::fromStdString(layer->name));
	widget->setLayerNum(layerN);
	// TODO: shift layers up if we support inserting layer before end
	layerWidgets.insert(layerWidgets.begin() + layerN, widget);
}

void LayerEditor::layerDeleted(int layerN)
{
	LayerWidget *widget = layerWidgets[layerN];
	bool wasActive = widget->activeStatus();
	layerStackLayout->removeWidget(widget);

	widget->hide();
	widget->deleteLater();
	layerWidgets.erase(layerWidgets.begin() + layerN);
	// if the layer was active, determine new active layer
	if (wasActive)
	{
		int activeLayer = hub->activeLayer();
		layerWidgets[activeLayer]->setActiveStatus(true);
	}
	// shift down layers that were above the deleted one
	for (int i = layerN; i < (int)layerWidgets.size(); ++i)
		layerWidgets[i]->setLayerNum(i);
}

void LayerEditor::layerSettingsChanged(int layerN, int change_flags)
{
	const VoxelLayer *layer = hub->getLayer(layerN);
	if (change_flags & VoxelLayer::VISIBILITY_CHANGED)
		layerWidgets[layerN]->setVisibilityStatus(layer->visible);
	if (change_flags & VoxelLayer::NAME_CHANGED)
		layerWidgets[layerN]->setLayerName(QString::fromStdString(layer->name));
	if (layerN == hub->activeLayer() &&	(change_flags & (VoxelLayer::BOUND_CHANGED | VoxelLayer::USE_BOUND_CHANGED)))
		loadLayerDetails(layerN);
}

void LayerEditor::activeLayerChanged(int layerN, int prev)
{
	layerWidgets[prev]->setActiveStatus(false);
	layerWidgets[layerN]->setActiveStatus(true);
	loadLayerDetails(layerN);
}

void LayerEditor::on_nameEdit_editingFinished()
{
	QString newName = nameEdit->text();
	hub->renameLayer(hub->activeLayer(), newName.toStdString());
	nameEdit->hide();
}

void LayerEditor::on_action_add_layer_triggered()
{
	hub->createLayer();
}

void LayerEditor::on_layer_bound_toggled(bool enabled)
{
	hub->setLayerBoundUse(hub->activeLayer(), enabled);
}

void LayerEditor::on_action_delete_layer_triggered()
{
	hub->deleteLayer(hub->activeLayer());
}

/* ==========================
	Layer Widget
=============================*/
// TODO: move up if we keep it
#include <QPalette>
#include <QPainter>
#include <QMouseEvent>

QRect LayerWidget::visibilityRect(2, 2, 22, 22);

LayerWidget::LayerWidget(QWidget *parent, LayerEditor *editor, const QIcon *stateIcon):
	QWidget(parent),
	layerEditor(editor),
	visibilityIcon(stateIcon),
	layerName(new QLabel(this)),
	layerActive(false),
	layerVisible(true),
	layerNum(-1) // invalid, must call setLayerNum before usage!
{
	setMinimumSize(100, 24);
	layerName->move(30, 2);
}

void LayerWidget::setActiveStatus(bool active)
{
	layerActive = active;
	update();
}

void LayerWidget::setVisibilityStatus(bool visible)
{
	layerVisible = visible;
	update();
}
void LayerWidget::setLayerName(const QString &name)
{
	layerName->setText(name);
	layerName->adjustSize();
}

void LayerWidget::mousePressEvent(QMouseEvent *event)
{
	if (visibilityRect.contains(event->pos()))
		layerEditor->setVisibility(layerNum, !layerVisible);
	else if (!layerActive)
		layerEditor->activateLayer(layerNum);
	event->accept();
}

void LayerWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	layerEditor->editLayerName(layerNum);
}

void LayerWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	if (layerActive)
		painter.setBrush(palette().brush(QPalette::Highlight));
	else
		painter.setBrush(palette().brush(QPalette::Base	));
	//
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(rect(), 4, 4);
	QIcon::State state = layerVisible ? QIcon::On : QIcon::Off;
	//visibilityIcon->paint(&painter, 2, 2, 16, 16, Qt::AlignCenter, mode);
	visibilityIcon->paint(&painter, visibilityRect, Qt::AlignCenter, QIcon::Normal, state);
}
