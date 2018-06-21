/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VG_DIALOG_H
#define VG_DIALOG_H

#include <QDialog>

#include "voxelgem.h"

namespace Ui {
	class TranslateDialog;
}

class VGTranslateDialog: public QDialog
{
	public:
		VGTranslateDialog(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
		~VGTranslateDialog();
		IVector3D getOffset() const;
	protected:
		Ui::TranslateDialog *ui;
};

#endif // VG_DIALOG_H
