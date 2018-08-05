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
#include <QMouseEvent>
#include <QColorDialog>
#include <QTextStream>

#include <iostream>

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

static bool isComment(const QString &line)
{
	for (int i = 0; i < line.size(); ++i)
	{
		if (line[i].isSpace())
			continue;
		if (line[i] != '#')
			return false;
		break;
	}
	return true;
}

ColorSet* loadGimpPalette(const QString &filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return nullptr;
	QString line;
	QTextStream palStream(&file), lineStream;
	if (!palStream.readLineInto(&line))
		return nullptr;
	if (!line.startsWith("GIMP Palette"))
	{
		std::cout << "File does not appear to be a GIMP Palette.\n";
		return nullptr;
	}
	int lNum = 2;
	bool haveLine = palStream.readLineInto(&line);
	if (!haveLine)
	{
		std::cout << "Palette file ended unexpectedly.\n";
		return nullptr;
	}
	if (line.startsWith("Name:"))
	{
		// don't support palette names yet...skip
		haveLine = palStream.readLineInto(&line);
		++lNum;
	}
	if (!haveLine)
	{
		std::cout << "Palette file ended unexpectedly.\n";
		return nullptr;
	}
	if (line.startsWith("Columns:"))
	{
		// don't support column hints yet...skip
		haveLine = palStream.readLineInto(&line);
		++lNum;
	}
	ColorSet* colorSet = new ColorSet();
	while (haveLine)
	{
		if (!isComment(line))
		{
			lineStream.setString(&line);
			int col[3];

			for (int i = 0; i < 3; ++i)
			{
				lineStream >> col[i];
				if (lineStream.status() != QTextStream::Ok || col[i] < 0 || col[i] > 255)
				{
					std::cout << "Invalid token at line " << lNum << " token " << i << ", skipping entry.\n";
					break;
				}
			}
			if (lineStream.status() == QTextStream::Ok)
			{
				ColorSetEntry entry;
				entry.color = QColor(col[0], col[1], col[2]);
				entry.name = lineStream.readLine().trimmed();
				colorSet->append(entry);
				// TODO: handle empty color name?
			}
		}

		++lNum;
		haveLine = palStream.readLineInto(&line);
	}

	return colorSet;
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

QModelIndex ColorPaletteModel::index(int row, int column, const QModelIndex& parent) const
{
	unsigned int setIndex = column + row * columnCount();
	if (activeColorSet && setIndex < activeColorSet->numEntries())
		return QAbstractTableModel::index(row, column, parent);
	return QModelIndex();
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

const ColorSetEntry* ColorPaletteModel::colorSetEntryFromIndex(const QModelIndex &index) const
{
	if (!activeColorSet)
		return 0;
	unsigned int setIndex = index.column() + index.row() * columnCount();
	return activeColorSet->get(setIndex);
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
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Horizontal scrollbar is never needed
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Avoid that removing bar increases row size so it's needed again -> infinite redraw
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

void ColorPaletteView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		QModelIndexList indices = selectedIndexes();
		if (indices.empty() || !indices.first().isValid())
			return;
		emit(entrySelected(*activeModel->colorSetEntryFromIndex(indices.first())));
	}
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
