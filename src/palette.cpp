/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "palette.h"

#include <QPainter>
#include <QHeaderView>
#include <QAbstractItemDelegate>
#include <QColorDialog>

ColorSet* getTestPalette()
{
	ColorSet* testSet = new ColorSet();
	for (int i = 0; i < 11; ++i)
	{
		ColorSetEntry entry;
		int val = (i * 255) / 10;
		entry.color = QColor(val, val, val);
		entry.name = QString::asprintf("%i%% black", 100 - 10*i);
		testSet->append(entry);
	}
	return testSet;
}

/*============================
// Model
=============================*/

ColorPaletteModel::ColorPaletteModel(QObject* parent):
	QAbstractTableModel(parent), activeColorSet(0)
{
}

QVariant ColorPaletteModel::data(const QModelIndex& index, int role) const
{
	unsigned int setIndex = index.column() + index.row() * columnCount();
	if (activeColorSet && setIndex < activeColorSet->numEntries())
	{
		const ColorSetEntry *setEntry = activeColorSet->get(setIndex);
		switch (role)
		{
			case Qt::ToolTipRole:
			case Qt::DisplayRole:
			{
				return setEntry->name;
			}
			case Qt::BackgroundRole:
			{
				return QBrush(setEntry->color);
			}
		}
	}
	return QVariant();
}

int ColorPaletteModel::rowCount(const QModelIndex& /* parent */) const
{
	if (!activeColorSet)
        return 0;

	return activeColorSet->numEntries()/columnCount() + 1;
}

int ColorPaletteModel::columnCount(const QModelIndex& /*parent*/) const
{
	//if (!activeColorSet)
	//	return 0;
	return 11;
}

Qt::ItemFlags ColorPaletteModel::flags(const QModelIndex& index) const
{
	if (index.isValid())
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }
    return Qt::NoItemFlags;
}

void ColorPaletteModel::setColorSet(ColorSet* colorSet)
{
	activeColorSet = colorSet;
    beginResetModel();
    endResetModel();
}

/*============================
// Delegate
=============================*/

ColorPaletteDelegate::ColorPaletteDelegate(QObject* parent): QAbstractItemDelegate(parent)
{
}

void ColorPaletteDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	if (! index.isValid())
        return;

    painter->save();
	bool isSelected = option.state & QStyle::State_Selected;
	int width = isSelected ? 2 : 1;

	if (isSelected)
	{
		painter->fillRect(option.rect, option.palette.highlight());
	}
	QRect paintRect = option.rect.adjusted(width, width, -width, -width);
	QBrush brush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));
	painter->fillRect(paintRect, brush);

    painter->restore();
}

QSize ColorPaletteDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const
{
    return option.decorationSize;
}

/*============================
// View
=============================*/

ColorPaletteView::ColorPaletteView(QWidget *parent)	: QTableView(parent), activeModel(0)
{
	setSelectionMode(QAbstractItemView::SingleSelection);
	verticalHeader()->hide();
	horizontalHeader()->hide();
	verticalHeader()->setDefaultSectionSize(20);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	//setViewMode(FIXED_COLUMNS);
	setShowGrid(false);
	setItemDelegate(new ColorPaletteDelegate());
}

void ColorPaletteView::resizeEvent(QResizeEvent *event)
{
	QTableView::resizeEvent(event);
	updateView();
}

QSize ColorPaletteView::sizeHint() const
{
	// 11 columns at 13px width
	return QSize(13 * 11, 13 * 11);
}

void ColorPaletteView::updateView()
{
	int columnCount = model()->columnCount(QModelIndex());
	int rowCount = model()->rowCount(QModelIndex());

	int columnWidth = viewport()->size().width() / columnCount;

	for (int i = 0; i < columnCount; ++i)
		setColumnWidth(i, columnWidth);

	for (int i = 0; i < rowCount; ++i)
		setRowHeight(i, columnWidth);
}

void ColorPaletteView::paletteModelChanged()
{
    updateView();
}

void ColorPaletteView::setPaletteModel(ColorPaletteModel *model)
{
	if (activeModel)
	    disconnect(activeModel, 0, this, 0);

	activeModel = model;
	setModel(model);
	paletteModelChanged();
	connect(activeModel, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)), this, SLOT(paletteModelChanged()));
	connect(activeModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(paletteModelChanged()));
	connect(activeModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(paletteModelChanged()));
	connect(activeModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(paletteModelChanged()));
	connect(activeModel, SIGNAL(modelReset()), this, SLOT(paletteModelChanged()));

}

/*============================
// ColorSwatch
=============================*/

ColorSwatch::ColorSwatch(QWidget *parent):
	QWidget(parent), color(128, 128, 255)
{}

void ColorSwatch::on_colorSelectionChanged(QColor col)
{
	color = col;
	update();
}

void ColorSwatch::mouseReleaseEvent(QMouseEvent *event)
{
	 QColor new_color = QColorDialog::getColor(color, 0, QString(), QColorDialog::ShowAlphaChannel);
	if (new_color.isValid())
	{
		color = new_color;
		emit(colorSelectionChanged(color));
	}
}

void ColorSwatch::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(rect(), color);
}
