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

#include "finddialog.h"
#include "utilities.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

#include "control/toast.h"

FindDialog::FindDialog(const FindInfo &info, QWidget *parent)
    : FramelessDialogBase(parent) {

    auto widget = new QWidget(this);
    auto layout = new QVBoxLayout(widget);

    layout->addWidget(new QLabel(tr("Mode:"), this));
    m_findMode = new QComboBox(this);
    m_findMode->addItem(QStringLiteral("HEX"));
    m_findMode->addItems(Utilities::getEncodings());
    m_findMode->setCurrentIndex(1);

    layout->addWidget(m_findMode);
    layout->addSpacing(3);

    layout->addWidget(new QLabel(tr("Content:"), this));
    m_lineeditor = new QHexTextEdit(this);
    layout->addWidget(m_lineeditor);
    layout->addSpacing(3);

    layout->addWidget(new QLabel(tr("EncBytes:"), this));
    m_preview = new QTextEdit(this);
    m_preview->setFocusPolicy(Qt::NoFocus);
    m_preview->setReadOnly(true);
    m_preview->setUndoRedoEnabled(false);
    m_preview->setWordWrapMode(QTextOption::WordWrap);
    m_preview->setAcceptRichText(false);
    m_preview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    layout->addWidget(m_preview);

    connect(m_lineeditor, &QHexTextEdit::textChanged, this, [this]() {
        auto text = m_lineeditor->toPlainText();
        if (m_findMode->currentIndex()) {
            auto encoding = m_findMode->currentText();
            auto dbytes = Utilities::encodingString(text, encoding);
            m_preview->setText(dbytes.toHex(' '));
        } else {
            m_preview->setText(text);
        }
    });
    connect(m_findMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                auto oldva = m_lineeditor->isHexMode();
                auto newva = index == 0;
                m_lineeditor->setIsHexMode(newva);
                if (oldva != newva) {
                    m_lineeditor->clear();
                } else {
                    // force update
                    emit m_lineeditor->textChanged();
                }
            });

    if (info.isStringFind) {
        if (!info.encoding.isEmpty()) {
            m_findMode->setCurrentText(info.encoding);
        }
    } else {
        m_findMode->setCurrentIndex(0);
    }

    m_lineeditor->setText(info.str);

    auto regionw = new QWidget(this);
    auto regionLayout = new QHBoxLayout(regionw);

    regionLayout->addWidget(new QLabel(tr("Region:"), regionw));

    m_regionStart = new QtLongLongSpinBox(regionw);
    Q_ASSERT(info.stop >= info.start);
    m_regionStart->setRange(info.start, info.stop);
    m_regionStart->setEnabled(false);
    m_regionStart->setValue(info.start);
    m_regionStart->setDisplayIntegerBase(16);
    m_regionStart->setPrefix(QStringLiteral("0x"));
    regionLayout->addWidget(m_regionStart, 1);

    regionLayout->addWidget(new QLabel(QStringLiteral(" - "), regionw));

    m_regionStop = new QtLongLongSpinBox(regionw);
    m_regionStop->setRange(info.start, info.stop);
    connect(m_regionStart, &QtLongLongSpinBox::valueChanged, m_regionStop,
            &QtLongLongSpinBox::setMinimum);
    m_regionStop->setEnabled(false);
    m_regionStop->setValue(qMin(info.start + 1024 * 1024, info.stop));
    m_regionStop->setDisplayIntegerBase(16);
    m_regionStop->setPrefix(QStringLiteral("0x"));
    regionLayout->addWidget(m_regionStop, 1);

    auto group = new QButtonGroup(this);
    group->setExclusive(true);

    auto btnBox = new QWidget(this);
    auto buttonLayout = new QHBoxLayout(btnBox);
    buttonLayout->setSpacing(0);

    int id = 0;

    auto b = new QPushButton(tr("None"), this);
    b->setCheckable(true);
    connect(b, &QPushButton::toggled, this, [=](bool b) {
        if (b) {
            _result.dir = SearchDirection::None;
        }
    });
    group->addButton(b, id++);
    b->setEnabled(!info.isBigFile);
    buttonLayout->addWidget(b);

    b = new QPushButton(tr("Region"), this);
    b->setCheckable(true);
    connect(b, &QPushButton::toggled, this, [=](bool b) {
        if (b) {
            _result.dir = SearchDirection::Region;
        }
        regionw->setVisible(b);
        regionw->setEnabled(b);
    });
    group->addButton(b, id++);
    buttonLayout->addWidget(b);

    b = new QPushButton(tr("BeforeCursor"), this);
    b->setCheckable(true);
    connect(b, &QPushButton::toggled, this, [=](bool b) {
        if (b) {
            _result.dir = SearchDirection::Foreword;
        }
    });
    group->addButton(b, id++);
    buttonLayout->addWidget(b);

    b = new QPushButton(tr("AfterCursor"), this);
    b->setCheckable(true);
    connect(b, &QPushButton::toggled, this, [=](bool b) {
        if (b) {
            _result.dir = SearchDirection::Backword;
        }
    });
    group->addButton(b, id++);
    buttonLayout->addWidget(b);

    b = new QPushButton(tr("Selection"), this);
    b->setCheckable(true);
    if (info.isSel) {
        connect(b, &QPushButton::toggled, this, [=](bool b) {
            if (b) {
                _result.dir = SearchDirection::Selection;
            }
        });
    } else {
        b->setEnabled(false);
    }

    group->addButton(b, id++);
    buttonLayout->addWidget(b);
    group->button(info.isBigFile ? 1 : 0)->setChecked(true);

    layout->addWidget(btnBox);

    auto dbbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(dbbox, &QDialogButtonBox::accepted, this, &FindDialog::on_accept);
    connect(dbbox, &QDialogButtonBox::rejected, this, &FindDialog::on_reject);
    auto key = QKeySequence(Qt::Key_Return);
    auto s = new QShortcut(key, this);
    connect(s, &QShortcut::activated, this, &FindDialog::on_accept);

    layout->addWidget(regionw);
    regionw->hide();

    layout->addSpacing(20);
    layout->addWidget(dbbox);

    buildUpContent(widget);
    this->setWindowTitle(tr("find"));

    m_lineeditor->setFocus();
}

FindDialog::Result FindDialog::getResult() const { return _result; }

void FindDialog::on_accept() {
    _result.isStringFind = m_findMode->currentIndex() > 0;
    _result.str = m_lineeditor->toPlainText().trimmed();

    if (!_result.isStringFind) {
        // check the last byte nibbles
        if (std::next(_result.str.rbegin())->isSpace()) {
            Toast::toast(this, NAMEICONRES("find"), tr("InvalidHexSeq"));
            return;
        }
    }

    if (m_regionStart->isEnabled()) {
        _result.start = m_regionStart->value();
        _result.stop = m_regionStop->value();
    } else {
        _result.start = 0;
        _result.stop = 0;
    }

    _result.encoding = m_findMode->currentText();
    done(1);
}

void FindDialog::on_reject() { done(0); }
