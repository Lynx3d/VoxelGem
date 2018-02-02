/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_PALETTE_H
#define VG_PALETTE_H

#include <QAbstractTableModel>
#include <QAbstractItemDelegate>
#include <QTableView>
#include <QColor>
#include <vector>

class ColorSetEntry
{
	public:
		QColor color;
		QString name;
};

// Set of colors to be displayed by Palette
class ColorSet
{
	public:
		const ColorSetEntry* get(int index) const
		{
			if (index >= colors.size()) return 0;
			return &colors[index];
		}
		int numEntries() const
		{
			return colors.size();
		}
		void append(const ColorSetEntry &entry)
		{
			colors.push_back(entry);
		}
	protected:
		std::vector<ColorSetEntry> colors;
};

ColorSet* getTestPalette();

class ColorPaletteModel : public QAbstractTableModel
{
    Q_OBJECT
	public:
		ColorPaletteModel(QObject* parent = 0);

	    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		int rowCount(const QModelIndex& parent = QModelIndex()) const override;
		int columnCount(const QModelIndex& parent = QModelIndex()) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;
//		QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
		void setColorSet(ColorSet* colorSet);

	protected:
		ColorSet *activeColorSet;
};

class ColorPaletteDelegate : public QAbstractItemDelegate
{
	public:
		ColorPaletteDelegate(QObject * parent = 0);
		void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
		QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const override;

	private:
};

class ColorPaletteView: public QTableView
{
	Q_OBJECT
	public:
		ColorPaletteView(QWidget *parent = 0);
		void resizeEvent(QResizeEvent *event) override;
		QSize sizeHint() const override;
		void updateView();
		void setPaletteModel(ColorPaletteModel *model);
		ColorPaletteModel* getPaletteModel() { return activeModel; }

	public Q_SLOTS:
		void paletteModelChanged();

	protected:
		ColorPaletteModel *activeModel;
};


#endif // VG_PALETTE_H
