#include "QConsoleWidget.h"
#include "QConsoleIODevice.h"

#include "class/langservice.h"
#include "qdocumentline.h"
#include "qformatscheme.h"
#include "qlanguagefactory.h"
#include "utilities.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>

QConsoleWidget::QConsoleWidget(QWidget *parent)
    : QEditor(false, parent), mode_(Output), completer_(nullptr) {
    iodevice_ = new QConsoleIODevice(this, this);

    // QTextCharFormat fmt = currentCharFormat();
    // for (int i = 0; i < nConsoleChannels; i++)
    //     chanFormat_[i] = fmt;

    chanFormat_[StandardOutput].setForeground(Qt::darkBlue);
    chanFormat_[StandardError].setForeground(Qt::red);

    // setTextInteractionFlags();

    setUndoRedoEnabled(false);
}

QConsoleWidget::~QConsoleWidget() {}

void QConsoleWidget::setMode(ConsoleMode m) {
    if (m == mode_)
        return;

    if (m == Input) {
        auto cursor = this->cursor();
        cursor.movePosition(QDocumentCursor::End);
        setCursor(cursor);
        // setCurrentCharFormat(chanFormat_[StandardInput]);
        inpos_ = cursor;
        mode_ = Input;
    }

    if (m == Output) {
        mode_ = Output;
    }
}

QString QConsoleWidget::getCommandLine() {
    if (mode_ == Output)
        return QString();
    auto textCursor = this->cursor();
    auto ltxt = textCursor.line().text();
    QString code = ltxt.mid(inpos_.columnNumber());
    return code.replace(QChar::ParagraphSeparator, QChar::LineFeed);
}

void QConsoleWidget::clear() { document()->clear(); }

void QConsoleWidget::handleReturnKey() {
    QString code = getCommandLine();

    // start new block
    auto textCursor = this->cursor();
    auto line = textCursor.line();
    textCursor.moveTo(line.lineNumber(), line.length());
    textCursor.insertLine();

    setMode(Output);
    setCursor(textCursor);

    // Update the history
    if (!code.isEmpty())
        history_.add(code);

    // send signal / update iodevice
    if (iodevice_->isOpen())
        iodevice_->consoleWidgetInput(code);

    emit consoleCommand(code);
}

void QConsoleWidget::setCompleter(QCodeCompletionWidget *c) {
    if (completer_) {
        // completer_->setWidget(0);
        // QObject::disconnect(completer_, SIGNAL(activated(const QString &)),
        //                     this, SLOT(insertCompletion(const QString &)));
    }
    completer_ = c;
    if (completer_) {
        // completer_->setWidget(this);
        // QObject::connect(completer_, SIGNAL(activated(const QString &)),
        // this,
        //                  SLOT(insertCompletion(const QString &)));
    }
}

void QConsoleWidget::keyPressEvent(QKeyEvent *e) {
    if (completer_ && completer_->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    auto textCursor = this->cursor();
    bool selectionInEditZone = isSelectionInEditZone();

    // check for user abort request
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Q) // Ctrl-Q aborts
        {
            emit abortEvaluation();
            e->accept();
            return;
        }
    }

    // Allow copying anywhere in the console ...
    if (e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier) {
        if (textCursor.hasSelection())
            copy();
        e->accept();
        return;
    }

    // the rest of key events are ignored during output mode
    if (mode() != Input) {
        e->ignore();
        return;
    }

    // Allow cut only if the selection is limited to the interactive area ...
    if (e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier) {
        if (selectionInEditZone)
            cut();
        e->accept();
        return;
    }

    // Allow paste only if the selection is in the interactive area ...
    if (e->key() == Qt::Key_V && e->modifiers() == Qt::ControlModifier) {
        if (selectionInEditZone || isCursorInEditZone()) {
            const QMimeData *const clipboard =
                QApplication::clipboard()->mimeData();
            const QString text = clipboard->text();
            if (!text.isNull()) {
                textCursor.insertText(text/*,
                channelCharFormat(StandardInput)*/);
            }
        }

        e->accept();
        return;
    }

    int key = e->key();
    // int shiftMod = e->modifiers() == Qt::ShiftModifier;

    if (history_.isActive() && key != Qt::Key_Up && key != Qt::Key_Down)
        history_.deactivate();

    // Force the cursor back to the interactive area
    // for all keys except modifiers
    if (!isCursorInEditZone() && key != Qt::Key_Control &&
        key != Qt::Key_Shift && key != Qt::Key_Alt) {
        auto line = textCursor.line();
        textCursor.moveTo(line.lineNumber(), line.length());
        setCursor(textCursor);
    }

    switch (key) {
    case Qt::Key_Up:
        // Activate the history and move to the 1st matching history item
        if (!history_.isActive())
            history_.activate(getCommandLine());
        if (history_.move(true))
            replaceCommandLine(history_.currentValue());
        else
            QApplication::beep();
        e->accept();
        break;

    case Qt::Key_Down:
        if (history_.move(false))
            replaceCommandLine(history_.currentValue());
        else
            QApplication::beep();
        e->accept();

    case Qt::Key_Left:
        if (textCursor > inpos_)
            QEditor::keyPressEvent(e);
        else {
            QApplication::beep();
            e->accept();
        }
        break;

    case Qt::Key_Delete:
        e->accept();
        if (selectionInEditZone)
            cut();
        else {
            // cursor must be in edit zone
            if (textCursor < inpos_)
                QApplication::beep();
            else
                QEditor::keyPressEvent(e);
        }
        break;

    case Qt::Key_Backspace:
        e->accept();
        if (selectionInEditZone)
            cut();
        else {
            // cursor must be in edit zone
            if (textCursor <= inpos_)
                QApplication::beep();
            else
                QEditor::keyPressEvent(e);
        }
        break;

    case Qt::Key_Home:
        e->accept();
        setCursor(inpos_);
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        e->accept();
        handleReturnKey();
        break;

    case Qt::Key_Escape:
        e->accept();
        replaceCommandLine(QString());
        break;

    default:
        // setCurrentCharFormat(chanFormat_[StandardInput]);
        if (isCursorInEditZone()) {
            QEditor::keyPressEvent(e);
        } else {
            e->ignore();
        }
        break;
    }
}

void QConsoleWidget::contextMenuEvent(QContextMenuEvent *event) {
    // QMenu *menu = createStandardContextMenu();

    // QAction *a;
    // if ((a = menu->findChild<QAction *>("edit-cut")))
    //     a->setEnabled(canCut());
    // if ((a = menu->findChild<QAction *>("edit-delete")))
    //     a->setEnabled(canCut());
    // if ((a = menu->findChild<QAction *>("edit-paste")))
    //     a->setEnabled(canPaste());

    // menu->exec(event->globalPos());
    // delete menu;
}

bool QConsoleWidget::isSelectionInEditZone() const {
    auto textCursor = this->cursor();
    if (!textCursor.hasSelection())
        return false;

    auto selectionStart = textCursor.selectionStart();
    auto selectionEnd = textCursor.selectionEnd();

    return selectionStart > inpos_ && selectionEnd >= inpos_;
}

bool QConsoleWidget::isCursorInEditZone() const { return cursor() >= inpos_; }

bool QConsoleWidget::canPaste() const {
    auto textCursor = this->cursor();
    if (textCursor < inpos_)
        return false;
    return true;
}

void QConsoleWidget::replaceCommandLine(const QString &str) {

    // Select the text after the last command prompt ...
    auto textCursor = this->cursor();
    auto line = textCursor.line();
    textCursor.moveTo(line.lineNumber(), line.length());

    auto bcursor = inpos_;
    bcursor.setSelectionBoundary(textCursor);
    bcursor.replaceSelectedText(str);
    bcursor.movePosition(str.length());

    setCursor(bcursor);
}

QString QConsoleWidget::getHistoryPath() {
    QDir dir(Utilities::getAppDataPath());
    return dir.absoluteFilePath(QStringLiteral(".command_history.lst"));
}

void QConsoleWidget::write(const QString &message, const QTextCharFormat &fmt) {
    // QTextCharFormat currfmt = currentCharFormat();
    auto tc = cursor();

    if (mode() == Input) {
        // in Input mode output messages are inserted
        // before the edit block

        // get offset of current pos from the end
        auto editpos = tc;
        auto line = editpos.line();
        tc.moveTo(line, 0);
        tc.insertLine();
        tc.insertText(message /*, fmt*/);

        setCursor(editpos);
        // setCurrentCharFormat(currfmt);
    } else {
        // in output mode messages are appended
        auto tc1 = tc;
        tc1.movePosition(QDocumentCursor::End);

        // check is cursor was not at the end
        // (e.g. had been moved per mouse action)
        bool needsRestore = tc1.position() != tc.position();

        // insert text
        setCursor(tc1);
        tc.insertText(message /*, fmt*/);
        ensureCursorVisible();

        // restore cursor if needed
        if (needsRestore)
            setCursor(tc);
    }
}

void QConsoleWidget::writeStdOut(const QString &s) {
    write(s, chanFormat_[StandardOutput]);
}

void QConsoleWidget::writeStdErr(const QString &s) {
    write(s, chanFormat_[StandardError]);
}

/////////////////// QConsoleWidget::History /////////////////////

QConsoleWidget::History QConsoleWidget::history_;

QConsoleWidget::History::History(void)
    : pos_(0), active_(false), maxsize_(10000) {
    QFile f(QConsoleWidget::getHistoryPath());
    if (f.open(QFile::ReadOnly)) {
        QTextStream is(&f);
        while (!is.atEnd())
            add(is.readLine());
    }
}
QConsoleWidget::History::~History(void) {
    QFile f(QConsoleWidget::getHistoryPath());
    if (f.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream os(&f);
        int n = strings_.size();
        while (n > 0)
            os << strings_.at(--n) << Qt::endl;
    }
}

void QConsoleWidget::History::add(const QString &str) {
    active_ = false;
    // if (strings_.contains(str)) return;
    if (strings_.size() == maxsize_)
        strings_.pop_back();
    strings_.push_front(str);
}

void QConsoleWidget::History::activate(const QString &tk) {
    active_ = true;
    token_ = tk;
    pos_ = -1;
}

bool QConsoleWidget::History::move(bool dir) {
    if (active_) {
        int next = indexOf(dir, pos_);
        if (pos_ != next) {
            pos_ = next;
            return true;
        } else
            return false;
    } else
        return false;
}

int QConsoleWidget::History::indexOf(bool dir, int from) const {
    int i = from, to = from;
    if (dir) {
        while (i < strings_.size() - 1) {
            const QString &si = strings_.at(++i);
            if (si.startsWith(token_)) {
                return i;
            }
        }
    } else {
        while (i > 0) {
            const QString &si = strings_.at(--i);
            if (si.startsWith(token_)) {
                return i;
            }
        }
        return -1;
    }
    return to;
}

/////////////////// Stream manipulators /////////////////////

QTextStream &waitForInput(QTextStream &s) {
    QConsoleIODevice *d = qobject_cast<QConsoleIODevice *>(s.device());
    if (d)
        d->waitForReadyRead(-1);
    return s;
}

QTextStream &inputMode(QTextStream &s) {
    QConsoleIODevice *d = qobject_cast<QConsoleIODevice *>(s.device());
    if (d && d->widget())
        d->widget()->setMode(QConsoleWidget::Input);
    return s;
}
QTextStream &outChannel(QTextStream &s) {
    QConsoleIODevice *d = qobject_cast<QConsoleIODevice *>(s.device());
    if (d)
        d->setCurrentWriteChannel(QConsoleWidget::StandardOutput);
    return s;
}
QTextStream &errChannel(QTextStream &s) {
    QConsoleIODevice *d = qobject_cast<QConsoleIODevice *>(s.device());
    if (d)
        d->setCurrentWriteChannel(QConsoleWidget::StandardError);
    return s;
}