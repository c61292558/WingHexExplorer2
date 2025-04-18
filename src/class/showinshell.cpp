#include "showinshell.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include "wingmessagebox.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <QAxObject>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextBrowser>
#include <QUuid>
#include <ShObjIdl.h>
#include <ShlDisp.h>
#include <Windows.h>

bool ShowInShell::showInWindowsShell(const QString &filePath, bool deselect) {
    QFileInfo appFI(filePath);
    auto matchPath = appFI.dir().path().toLower();
    auto matchName = appFI.fileName().toLower();

    QAxObject shellApp("Shell.Application");

    QAxObject *windows = shellApp.querySubObject("Windows()");
    windows->disableMetaObject();
    auto count = windows->dynamicCall("Count()").toInt();
    qDebug() << count;
    for (int i = 0; i < count; ++i) {
        QAxObject *win = windows->querySubObject("Item(QVariant)", {i});
        win->disableMetaObject();

        auto program = win->dynamicCall("FullName()").toString();
        QFileInfo programFI(program);
        if (programFI.baseName().toLower() != "explorer")
            continue;
        auto url = win->dynamicCall("LocationURL()").toUrl();
        if (!url.isLocalFile())
            continue;
        auto path = url.path().mid(1).toLower();
        if (path != matchPath)
            continue;

        QAxObject *doc = win->querySubObject("Document()");

        QAxObject *folder = doc->querySubObject("Folder()");
        folder->disableMetaObject();
        QAxObject *folderItems = folder->querySubObject("Items()");
        folderItems->disableMetaObject();

        QAxObject *ourEntry = {};
        int count = folderItems->dynamicCall("Count()").toInt();
        for (int j = 0; j < count; j++) {
            QAxObject *entry = folderItems->querySubObject("Item(QVariant)", j);
            entry->disableMetaObject();
            auto name = entry->dynamicCall("Name()").toString().toLower();
            if (name == matchName)
                ourEntry = entry;
        }
        if (ourEntry) {
            if (false)
                ourEntry->dynamicCall("InvokeVerb(QVariant)",
                                      QVariant()); // open etc.
            auto rc = doc->dynamicCall(
                "SelectItem(QVariant, int)", ourEntry->asVariant(),
                SVSI_SELECT | (deselect ? SVSI_DESELECTOTHERS : 0));
            auto hwnd = win->dynamicCall("HWND()").toLongLong();
            BringWindowToTop(HWND(hwnd));
            return true;
        }
    }
    return false;
}

#else
bool ShowInShell::showInWindowsShell(const QString &filePath, bool deselect) {
    Q_UNUSED(filePath);
    Q_UNUSED(deselect);
    return false;
}
#endif

bool ShowInShell::showInGraphicalShell(QWidget *parent, const QString &pathIn,
                                       bool deselect) {
    const QFileInfo fileInfo(pathIn);
    // Mac, Windows support folder or file.
    if (HostOsInfo::isWindowsHost()) {
        if (showInWindowsShell(pathIn, deselect))
            return true;
        const auto explorer =
            QStandardPaths::findExecutable(QLatin1String("explorer.exe"));
        if (explorer.isEmpty()) {
            WingMessageBox::warning(
                parent,
                QApplication::translate("ShowInShell",
                                        "Launching Windows Explorer Failed"),
                QApplication::translate(
                    "ShowInShell",
                    "Could not find explorer.exe in path to launch "
                    "Windows Explorer."));
            return false;
        }
        QStringList param;
        if (!fileInfo.isDir())
            param += QLatin1String("/select,");
        param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
        return QProcess::startDetached(explorer, param);
    } else if (HostOsInfo::isMacHost()) {
        QStringList openArgs;
        openArgs << QLatin1String("-R") << fileInfo.canonicalFilePath();
        int rc = QProcess::execute(QLatin1String("/usr/bin/open"), openArgs);
        return rc != -2 && rc != 1;
    } else {
        // we cannot select a file here, because no file browser really
        // supports it...
        const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath()
                                                : fileInfo.absolutePath();
        const QString app = QLatin1String("xdg-open");
        QProcess browserProc;
        bool success = browserProc.startDetached(app, {folder});
        const QString error =
            QString::fromLocal8Bit(browserProc.readAllStandardError());
        success = success && error.isEmpty();
        return success;
    }
}
