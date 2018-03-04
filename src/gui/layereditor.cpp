/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "layereditor.h"
#include "voxelscene.h"
#include "gui/ui_layereditor.h"

#include <QDataWidgetMapper>
#include <QBoxLayout>
#include <QSignalBlocker>

#include <iostream>
#include <cassert>

LayerManager::LayerManager()
{
	VoxelLayer* defLayer = new VoxelLayer;
	defLayer->name = "Default Layer";
	layers.push_back(defLayer);
}

bool LayerManager::createLayer(int layerN)
{
	if (layerN < -1 || layerN > int(layers.size()))
		return false;
	// TODO: implement
	if (layerN != -1)
		return false;

	VoxelLayer* newLayer = new VoxelLayer;
	newLayer->name = "New Layer";
	layers.push_back(newLayer);
	emit(layerCreated(layers.size()-1));
	return true;
}

bool LayerManager::deleteLayer(int layerN)
{
	if (layers.size() < 2 || layerN < 0 || layerN >= int(layers.size()))
		return false;

	// if we delete the topmost layer and it is active, the active layer needs to decrease
	if (layerN == m_activeLayer && layerN == int(layers.size() - 1))
	{
		setActiveLayer(layerN - 1);
	}
	delete layers[layerN];
	layers.erase(layers.begin() + layerN);
	emit(layerDeleted(layerN));
	return true;
}

bool LayerManager::setActiveLayer(int layerN)
{
	if (layerN < 0 || layerN >= (int)layers.size())
		return false;
	if (layerN != activeLayer())
	{
		int prev = m_activeLayer;
		m_activeLayer = layerN;
		emit(activeLayerChanged(layerN, prev));
	}
	return true;
}

bool LayerManager::setLayerBound(int layerN, const IBBox &bound)
{
	if (layerN < 0 || layerN >= (int)layers.size())
		return false;
	layers[layerN]->bound = bound;
	emit(layerSettingsChanged(layerN));
	return true;
}

bool LayerManager::setLayerBoundUse(int layerN, bool enabled)
{
	if (layerN < 0 || layerN >= (int)layers.size())
		return false;
	layers[layerN]->useBound = enabled;
	emit(layerSettingsChanged(layerN));
	return true;
}

bool LayerManager::setLayerVisibility(int layerN, bool visible)
{
	if (layerN < 0 || layerN >= (int)layers.size())
		return false;
	layers[layerN]->visible = visible;
	return true;
}

const VoxelLayer* LayerManager::getLayer(int layerN) const
{
	if (layerN >=0 && layerN < (int)layers.size())
		return layers[layerN];
	return 0;
}

/*=========================*/

LayerEditor::LayerEditor(QWidget *parent, LayerManager *manager):
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
	connect(manager, &LayerManager::layerCreated, this, &LayerEditor::layerCreated);
	connect(manager, &LayerManager::layerDeleted, this, &LayerEditor::layerDeleted);
	connect(manager, &LayerManager::activeLayerChanged, this, &LayerEditor::activeLayerChanged);
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
	/* // 1
	LayerWidget *testWidget = new LayerWidget(ui->layer_stack, this, icon);
	layerStackLayout->insertWidget(-1, testWidget);
	testWidget->setLayerName("testlayer");
	testWidget->setLayerNum(0);
	layerWidgets.push_back(testWidget);
	// 2
	testWidget = new LayerWidget(ui->layer_stack, this, icon);
	layerStackLayout->insertWidget(-1, testWidget);
	testWidget->setLayerName("testlayer 2");
	testWidget->setLayerNum(1);
	layerWidgets.push_back(testWidget);
	// 3
	testWidget = new LayerWidget(ui->layer_stack, this, icon);
	layerStackLayout->insertWidget(-1, testWidget);
	testWidget->setLayerName("testlayer 3");
	testWidget->setLayerNum(2);
	layerWidgets.push_back(testWidget); */
	// must be last, and stay last on widget additions
	layerStackLayout->addStretch();
}

LayerEditor::~LayerEditor()
{
	delete ui;
}

/* void LayerEditor::boundValueChanged(int val)
{
	std::cout << "new value: " << val << std::endl;
} */

void LayerEditor::adjustLowerBound(int val, int index)
{
	assert(index >= 0 && index < 3);
	std::cout << "set the lower bound[" << index << "] to " << val << std::endl;
	const VoxelLayer *layer = hub->getLayer(hub->activeLayer());
	IBBox newBound(layer->bound);
	newBound.pMin[index] = val;

	if (index == 0)
		ui->sb_max_x->setMinimum(val + 1);
	else if (index == 1)
		ui->sb_max_y->setMinimum(val + 1);
	else
		ui->sb_max_z->setMinimum(val + 1);

	hub->setLayerBound(hub->activeLayer(), newBound);
}

void LayerEditor::adjustUpperBound(int val, int index)
{
	assert(index >= 0 && index < 3);
	const VoxelLayer *layer = hub->getLayer(hub->activeLayer());
	IBBox newBound(layer->bound);
	newBound.pMax[index] = val;

	if (index == 0)
		ui->sb_min_x->setMaximum(val - 1);
	else if (index == 1)
		ui->sb_min_y->setMaximum(val - 1);
	else
		ui->sb_min_z->setMaximum(val - 1);

	hub->setLayerBound(hub->activeLayer(), newBound);
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
	// TODO: use signal/slot to sync visibility
	layerWidgets[layerNum]->setVisibilityStatus(visible);
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
	// if the layer count is now smaller than layerN,
	// the active layer **should** have been changed before deletion
	if (wasActive)
		layerWidgets[layerN]->setActiveStatus(true);
	// shift down layers there were above the deleted one
	for (int i = layerN; i < (int)layerWidgets.size(); ++i)
		layerWidgets[i]->setLayerNum(i);
}

void LayerEditor::activeLayerChanged(int layerN, int prev)
{
	layerWidgets[prev]->setActiveStatus(false);
	layerWidgets[layerN]->setActiveStatus(true);
	loadLayerDetails(layerN);
}

void LayerEditor::on_action_add_layer_triggered()
{
	std::cout << "on_action_add_layer_triggered()\n";
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
}

void LayerWidget::mousePressEvent(QMouseEvent *event)
{
	if (visibilityRect.contains(event->pos()))
		layerEditor->setVisibility(layerNum, !layerVisible);
	else if (!layerActive)
		layerEditor->activateLayer(layerNum);
	event->accept();
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
