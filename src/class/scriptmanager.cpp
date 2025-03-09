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

#include "scriptmanager.h"

#include "dbghelper.h"

#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>

#include "settingmanager.h"
#include "utilities.h"
#include "wingmessagebox.h"

ScriptManager &ScriptManager::instance() {
    static ScriptManager ins;
    return ins;
}

QString ScriptManager::userScriptPath() const { return m_usrScriptsPath; }

QString ScriptManager::systemScriptPath() const { return m_sysScriptsPath; }

ScriptManager::ScriptManager() : QObject() {
    ASSERT_SINGLETON;

    // init script directory
    m_sysScriptsPath = qApp->applicationDirPath() + QDir::separator() +
                       QStringLiteral("scripts");
    m_usrScriptsPath = Utilities::getAppDataPath() + QDir::separator() +
                       QStringLiteral("scripts");

    refresh();
}

ScriptManager::~ScriptManager() { detach(); }

QStringList ScriptManager::getScriptFileNames(const QDir &dir) const {
    if (!dir.exists()) {
        return {};
    }
    QStringList ret;
    for (auto &info : dir.entryInfoList({"*.as"}, QDir::Files)) {
        ret << info.absoluteFilePath();
    }
    return ret;
}

QString ScriptManager::readJsonObjString(const QJsonObject &jobj,
                                         const QString &key) {
    auto t = jobj.value(key);
    if (!t.isUndefined() && t.isString()) {
        return t.toString();
    }
    return {};
}

QMenu *ScriptManager::buildUpScriptDirMenu(QWidget *parent,
                                           const QStringList &files,
                                           bool isSys) {
    auto menu = new QMenu(parent);
    for (auto &file : files) {
        menu->addAction(
            ICONRES(QStringLiteral("script")), QFileInfo(file).fileName(),
            parent, [=] {
                if (Utilities::isRoot() && !isSys &&
                    !SettingManager::instance().allowUsrScriptInRoot()) {
                    WingMessageBox::critical(nullptr, tr("RunScript"),
                                             tr("CanNotRunUsrScriptForPolicy"));
                    return;
                }
                ScriptManager::instance().runScript(file);
            });
    }
    return menu;
}

QStringList ScriptManager::sysScriptsDbCats() const {
    return m_sysScriptsDbCats;
}

QStringList ScriptManager::getUsrScriptFileNames(const QString &cat) const {
    QDir scriptDir(m_usrScriptsPath);
    if (!scriptDir.exists()) {
        return {};
    }
    scriptDir.cd(cat);
    return getScriptFileNames(scriptDir);
}

QStringList ScriptManager::getSysScriptFileNames(const QString &cat) const {
    QDir scriptDir(m_sysScriptsPath);
    if (!scriptDir.exists()) {
        return {};
    }
    scriptDir.cd(cat);
    return getScriptFileNames(scriptDir);
}

void ScriptManager::refresh() {
    refreshSysScriptsDbCats();
    refreshUsrScriptsDbCats();
}

void ScriptManager::refreshUsrScriptsDbCats() {
    m_usrScriptsDbCats.clear();

    QDir scriptDir(m_usrScriptsPath);
    if (!scriptDir.exists()) {
        QDir().mkpath(m_usrScriptsPath);
    } else {
        for (auto &info :
             scriptDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            m_usrScriptsDbCats << info.fileName();
            auto meta = ensureDirMeta(info);
            meta.isSys = false;
            _usrDirMetas.insert(info.fileName(), meta);
        }
    }
}

void ScriptManager::refreshSysScriptsDbCats() {
    m_sysScriptsDbCats.clear();

    QDir sysScriptDir(m_sysScriptsPath);
    if (sysScriptDir.exists()) {
        for (auto &info :
             sysScriptDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            m_sysScriptsDbCats << info.fileName();
            auto meta = ensureDirMeta(info);
            meta.isSys = true;
            _sysDirMetas.insert(info.fileName(), meta);
        }
    }
}

ScriptManager::ScriptDirMeta
ScriptManager::ensureDirMeta(const QFileInfo &info) {
    ScriptDirMeta meta;

    if (!info.isDir()) {
        return meta;
    }
    QDir base(info.absoluteFilePath());
    if (!base.exists(QStringLiteral(".wingasmeta"))) {
        QJsonObject jobj;
        jobj.insert(QStringLiteral("name"), info.fileName());
        jobj.insert(QStringLiteral("author"), QLatin1String());
        jobj.insert(QStringLiteral("license"), QLatin1String());
        jobj.insert(QStringLiteral("homepage"), QLatin1String());
        jobj.insert(QStringLiteral("comment"), QLatin1String());
        QFile f(base.absoluteFilePath(QStringLiteral(".wingasmeta")));
        if (f.open(QFile::WriteOnly | QFile::Text)) {
            QJsonDocument jdoc(jobj);
            if (f.write(jdoc.toJson(QJsonDocument::JsonFormat::Indented)) >=
                0) {
                f.close();
            }
        }
        meta.name = info.fileName();
    } else {
        QFile f(base.absoluteFilePath(QStringLiteral(".wingasmeta")));
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            if (err.error == QJsonParseError::NoError) {
                auto jobj = doc.object();
                auto name = readJsonObjString(jobj, QStringLiteral("name"));
                if (name.trimmed().isEmpty()) {
                    name = info.fileName();
                }
                meta.name = name;
                meta.author = readJsonObjString(jobj, QStringLiteral("author"));
                meta.license =
                    readJsonObjString(jobj, QStringLiteral("license"));
                meta.homepage =
                    readJsonObjString(jobj, QStringLiteral("homepage"));
                meta.comment =
                    readJsonObjString(jobj, QStringLiteral("comment"));
            } else {
                meta.name = info.fileName();
            }
        } else {
            meta.name = info.fileName();
        }
    }

    meta.rawName = info.fileName();
    return meta;
}

void ScriptManager::attach(ScriptingConsole *console) {
    if (console) {
        _console = console;
    }
}

void ScriptManager::detach() { _console = nullptr; }

ScriptManager::ScriptDirMeta
ScriptManager::usrDirMeta(const QString &cat) const {
    return _usrDirMetas.value(cat);
}

ScriptManager::ScriptDirMeta
ScriptManager::sysDirMeta(const QString &cat) const {
    return _sysDirMetas.value(cat);
}

ScriptManager::ScriptActionMaps
ScriptManager::buildUpRibbonScriptRunner(RibbonButtonGroup *group) {
    ScriptActionMaps maps;

    auto &sm = ScriptManager::instance();
    auto &stm = SettingManager::instance();

    auto hideCats = stm.sysHideCats();
    for (auto &cat : sm.sysScriptsDbCats()) {
        if (hideCats.contains(cat)) {
            continue;
        }
        maps.sysList << addPannelAction(
            group, ICONRES(QStringLiteral("scriptfolder")),
            sm.sysDirMeta(cat).name,
            buildUpScriptDirMenu(group, sm.getSysScriptFileNames(cat), true));
    }

    hideCats = stm.usrHideCats();
    for (auto &cat : sm.usrScriptsDbCats()) {
        if (hideCats.contains(cat)) {
            continue;
        }
        maps.usrList << addPannelAction(
            group, ICONRES(QStringLiteral("scriptfolderusr")),
            sm.usrDirMeta(cat).name,
            buildUpScriptDirMenu(group, sm.getUsrScriptFileNames(cat), false));
    }

    return maps;
}

void ScriptManager::runScript(const QString &filename) {
    Q_ASSERT(_console);
    _console->setMode(ScriptingConsole::Output);
    _console->write(tr("Excuting:") + filename);
    _console->newLine();
    _console->machine()->executeScript(filename);
    _console->appendCommandPrompt();
    _console->setMode(ScriptingConsole::Input);
}

QStringList ScriptManager::usrScriptsDbCats() const {
    return m_usrScriptsDbCats;
}
