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

#include "scriptingconsole.h"
#include "class/scriptconsolemachine.h"

#include <QApplication>
#include <QColor>
#include <QShortcut>

ScriptingConsole::ScriptingConsole(QWidget *parent) : QConsoleWidget(parent) {
    auto shortCut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
    connect(shortCut, &QShortcut::activated, this,
            &ScriptingConsole::clearConsole);
}

ScriptingConsole::~ScriptingConsole() {}

void ScriptingConsole::stdOut(const QString &str) { writeStdOut(str); }

void ScriptingConsole::stdErr(const QString &str) { writeStdErr(str); }

void ScriptingConsole::stdWarn(const QString &str) {
    write(str, QStringLiteral("stdwarn"));
}

void ScriptingConsole::newLine() { _s << Qt::endl; }

void ScriptingConsole::init() {
    _getInputFn = std::bind(&ScriptingConsole::getInput, this);

    _sp = new ScriptConsoleMachine(_getInputFn, this);
    connect(_sp, &ScriptConsoleMachine::onClearConsole, this,
            &ScriptingConsole::clear);

    connect(_sp, &ScriptConsoleMachine::onOutput, this,
            [=](ScriptConsoleMachine::MessageType type,
                const ScriptConsoleMachine::MessageInfo &message) {
                switch (type) {
                case ScriptMachine::MessageType::Info:
                    stdOut(tr("[Info]") + message.message);
                    newLine();
                    break;
                case ScriptMachine::MessageType::Warn:
                    stdWarn(tr("[Warn]") + message.message);
                    newLine();
                    break;
                case ScriptMachine::MessageType::Error:
                    stdErr(tr("[Error]") + message.message);
                    newLine();
                    break;
                case ScriptMachine::MessageType::Print:
                    stdOut(message.message);
                    break;
                }
            });

    connect(this, &QConsoleWidget::consoleCommand, this,
            &ScriptingConsole::consoleCommand);
}

void ScriptingConsole::initOutput() {
    _s.setDevice(this->device());
    stdWarn(tr("Scripting console for WingHexExplorer"));

    _s << Qt::endl;
    stdWarn(tr(">>>> Powered by AngelScript <<<<"));
    _s << Qt::endl << Qt::endl;
    appendCommandPrompt();
    setMode(Input);
}

void ScriptingConsole::clearConsole() {
    setMode(Output);
    clear();
    appendCommandPrompt(_lastCommandPrompt);
    setMode(Input);
}

void ScriptingConsole::pushInputCmd(const QString &cmd) {
    QMutexLocker locker(&_queueLocker);
    _cmdQueue.append(cmd);
}

void ScriptingConsole::consoleCommand(const QString &code) {
    if (_waitforRead) {
        _waitforRead = false;
        return;
    }

    setMode(Output);
    if (!_sp->executeCode(code)) {
    }
    appendCommandPrompt();
    setMode(Input);
}

QString ScriptingConsole::getInput() {
    appendCommandPrompt(true);
    setMode(Input);
    _waitforRead = true;
    QString instr;

    auto d = _s.device();
    d->skip(d->bytesAvailable());

    do {
        {
            QMutexLocker locker(&_queueLocker);
            if (!_cmdQueue.isEmpty()) {
                instr = _cmdQueue.takeFirst();
                setMode(Output);
                QConsoleWidget::write(instr);
                setMode(Input);
                break;
            }
        }
        qApp->processEvents();
    } while (!d->waitForReadyRead(100));

    instr = _s.readAll();

    setMode(Output);

    return instr;
}

void ScriptingConsole::appendCommandPrompt(bool storeOnly) {
    QString commandPrompt;

    if (storeOnly) {
        commandPrompt += QStringLiteral("... > ");
    } else {
        auto cursor = this->cursor();
        if (!cursor.atBlockStart()) {
            commandPrompt = QStringLiteral("\n");
        }
        if (_sp && _sp->isInDebugMode()) {
            commandPrompt += QStringLiteral("[dbg] > ");
        } else {
            commandPrompt += QStringLiteral("as > ");
        }
    }

    _lastCommandPrompt = storeOnly;

    QConsoleWidget::write(commandPrompt);
}

ScriptMachine *ScriptingConsole::machine() const { return _sp; }

ScriptConsoleMachine *ScriptingConsole::consoleMachine() const { return _sp; }
