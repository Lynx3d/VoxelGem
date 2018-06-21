/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dialog.h"
#include "gui/ui_dialog_translate.h"

VGTranslateDialog::VGTranslateDialog(QWidget *parent, Qt::WindowFlags f):
	QDialog(parent, f), ui(new Ui::TranslateDialog)
{
	ui->setupUi(this);
}

VGTranslateDialog::~VGTranslateDialog()
{
	delete ui;
}

IVector3D VGTranslateDialog::getOffset() const
{
	return IVector3D(ui->move_x->value(), ui->move_y->value(), ui->move_z->value());
}
