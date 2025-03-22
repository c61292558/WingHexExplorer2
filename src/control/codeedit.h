/*==============================================================================
** Copyright (C) 2024-2027 WingSummer
**
** This program is free software: you can redistribute it and/or modify it under
** the terms of the GNU Affero General Public License as published by the Free
** Software Foundation, version 3.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
** details.
**
** You should have received a copy of the GNU Affero General Public License
** along with this program. If not, see <https://www.gnu.org/licenses/>.
** =============================================================================
*/

#ifndef CODEEDIT_H
#define CODEEDIT_H

#include "WingCodeEdit/wingcodeedit.h"
#include "dialog/searchdialog.h"

class CodeEdit : public WingCodeEdit {
    Q_OBJECT

public:
    explicit CodeEdit(QWidget *parent = nullptr);

public:
    void showSearchBar(bool show);

signals:
    void contentModified(bool b);

protected slots:
    virtual void onCompletion(const QModelIndex &index) override;

private:
    void addMoveLineShortCut();

    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    void applyEditorSetStyle();

private:
    SearchWidget *m_searchWidget;
};

#endif // CODEEDIT_H
