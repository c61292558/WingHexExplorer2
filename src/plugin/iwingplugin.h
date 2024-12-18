/*==============================================================================
** Copyright (C) 2024-2027 WingSummer
**
** You can redistribute this file and/or modify it under the terms of the
** BSD 3-Clause.
**
** THIS FILE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** =============================================================================
*/

#ifndef IWINGPLUGIN_H
#define IWINGPLUGIN_H

#include "settingpage.h"

#include <QCryptographicHash>
#include <QDockWidget>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QObject>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QtCore>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

/**
 * Don't try to modify this file, unless you are the dev
 * 不要尝试来修改该文件，除非你是开发者
 */

namespace WingHex {

Q_DECL_UNUSED constexpr auto SDKVERSION = 13;

Q_DECL_UNUSED static QString PLUGINDIR() {
    return QCoreApplication::applicationDirPath() + QStringLiteral("/plugin");
}

Q_DECL_UNUSED static QString HOSTRESPIMG(const QString &name) {
    return QStringLiteral(":/com.wingsummer.winghex/images/") + name +
           QStringLiteral(".png");
}

Q_NAMESPACE
enum ErrFile : int {
    Success = 0,
    Error = -1,
    UnSaved = -2,
    Permission = -3,
    NotExist = -4,
    AlreadyOpened = -5,
    IsNewFile = -6,
    IsDirver = -7,
    WorkSpaceUnSaved = -8,
    SourceFileChanged = -9,
    ClonedFile = -10,
    InvalidFormat = -11
};
Q_ENUM_NS(ErrFile)

struct FindResult {
    qsizetype offset = -1;
    qsizetype line = -1;
    qsizetype col = -1;
};

struct BookMark {
    qsizetype pos = -1;
    QString comment;
};

class IWingPlugin;

struct HexPosition {
    qsizetype line;
    int column;
    quint8 lineWidth;
    int nibbleindex;

    inline qsizetype offset() const {
        return static_cast<qsizetype>(line * lineWidth) + column;
    }
    inline qsizetype operator-(const HexPosition &rhs) const {
        return qsizetype(this->offset() - rhs.offset());
    }
    inline bool operator==(const HexPosition &rhs) const {
        return (line == rhs.line) && (column == rhs.column) &&
               (nibbleindex == rhs.nibbleindex);
    }
    inline bool operator!=(const HexPosition &rhs) const {
        return (line != rhs.line) || (column != rhs.column) ||
               (nibbleindex != rhs.nibbleindex);
    }
};

struct HexMetadataItem {
    qsizetype begin;
    qsizetype end;
    QColor foreground, background;
    QString comment;

    // added by wingsummer
    bool operator==(const HexMetadataItem &item) {
        return begin == item.begin && end == item.end &&
               foreground == item.foreground && background == item.background &&
               comment == item.comment;
    }

    HexMetadataItem() = default;

    HexMetadataItem(qsizetype begin, qsizetype end, QColor foreground,
                    QColor background, QString comment) {
        this->begin = begin;
        this->end = end;
        this->foreground = foreground;
        this->background = background;
        this->comment = comment;
    }
};

namespace WingPlugin {

class Reader : public QObject {
    Q_OBJECT
signals:
    bool isCurrentDocEditing();
    QString currentDocFilename();

    // document
    bool isReadOnly();
    bool isKeepSize();
    bool isLocked();
    qsizetype documentLines();
    qsizetype documentBytes();
    HexPosition currentPos();
    HexPosition selectionPos();
    qsizetype currentRow();
    qsizetype currentColumn();
    qsizetype currentOffset();
    qsizetype selectedLength();
    QByteArray selectedBytes();

    bool stringVisible();
    bool addressVisible();
    bool headerVisible();
    quintptr addressBase();
    bool isModified();

    bool isEmpty();
    bool atEnd();
    bool canUndo();
    bool canRedo();

    bool copy(bool hex = false);

    qint8 readInt8(qsizetype offset);
    qint16 readInt16(qsizetype offset);
    qint32 readInt32(qsizetype offset);
    qint64 readInt64(qsizetype offset);
    float readFloat(qsizetype offset);
    double readDouble(qsizetype offset);
    QString readString(qsizetype offset, const QString &encoding = QString());
    QByteArray readBytes(qsizetype offset, qsizetype count);

    // an extension for AngelScript
    // void read(? &in);    // this function can read bytes to input container

    qsizetype searchForward(qsizetype begin, const QByteArray &ba);
    qsizetype searchBackward(qsizetype begin, const QByteArray &ba);
    QList<qsizetype> findAllBytes(qsizetype begin, qsizetype end,
                                  const QByteArray &b);

    // render
    qsizetype documentLastLine();
    qsizetype documentLastColumn();

    // metadata
    bool lineHasMetadata(qsizetype line);
    QList<HexMetadataItem> getMetadatas(qsizetype offset);

    // bookmark
    bool lineHasBookMark(qsizetype line);
    QList<qsizetype> getsBookmarkPos(qsizetype line);
    BookMark bookMark(qsizetype pos);
    QString bookMarkComment(qsizetype pos);
    QList<BookMark> getBookMarks();
    bool existBookMark(qsizetype pos);

    // extension
    QStringList getSupportedEncodings();
    QString currentEncoding();

    // not available for AngelScript
    // only for plugin UI extenstion
    QDialog *createDialog(QWidget *content);
};

class Controller : public QObject {
    Q_OBJECT
signals:
    // document
    bool switchDocument(int handle);
    bool raiseDocument(int handle);
    bool setLockedFile(bool b);
    bool setKeepSize(bool b);
    bool setStringVisible(bool b);
    bool setAddressVisible(bool b);
    bool setHeaderVisible(bool b);
    bool setAddressBase(quintptr base);

    bool undo();
    bool redo();
    bool cut(bool hex = false);
    bool paste(bool hex = false);

    bool writeInt8(qsizetype offset, qint8 value);
    bool writeInt16(qsizetype offset, qint16 value);
    bool writeInt32(qsizetype offset, qint32 value);
    bool writeInt64(qsizetype offset, qint64 value);
    bool writeFloat(qsizetype offset, float value);
    bool writeDouble(qsizetype offset, double value);
    bool writeString(qsizetype offset, const QString &value,
                     const QString &encoding = QString());
    bool writeBytes(qsizetype offset, const QByteArray &data);

    bool insertInt8(qsizetype offset, qint8 value);
    bool insertInt16(qsizetype offset, qint16 value);
    bool insertInt32(qsizetype offset, qint32 value);
    bool insertInt64(qsizetype offset, qint64 value);
    bool insertFloat(qsizetype offset, float value);
    bool insertDouble(qsizetype offset, double value);
    bool insertString(qsizetype offset, const QString &value,
                      const QString &encoding = QString());
    bool insertBytes(qsizetype offset, const QByteArray &data);

    bool appendInt8(qint8 value);
    bool appendInt16(qint16 value);
    bool appendInt32(qint32 value);
    bool appendInt64(qint64 value);
    bool appendFloat(float value);
    bool appendDouble(double value);
    bool appendString(const QString &value,
                      const QString &encoding = QString());
    bool appendBytes(const QByteArray &data);

    bool remove(qsizetype offset, qsizetype len);
    bool removeAll(); // extension

    // cursor
    bool moveTo(qsizetype line, qsizetype column, int nibbleindex = 1);
    bool moveTo(qsizetype offset);
    bool select(qsizetype offset, qsizetype length);
    bool setInsertionMode(bool isinsert);

    // metadata
    bool foreground(qsizetype begin, qsizetype end, const QColor &fgcolor);
    bool background(qsizetype begin, qsizetype end, const QColor &bgcolor);
    bool comment(qsizetype begin, qsizetype end, const QString &comment);

    bool metadata(qsizetype begin, qsizetype end, const QColor &fgcolor,
                  const QColor &bgcolor, const QString &comment);

    bool removeMetadata(qsizetype offset);
    bool clearMetadata();
    bool setMetaVisible(bool b);
    bool setMetafgVisible(bool b);
    bool setMetabgVisible(bool b);
    bool setMetaCommentVisible(bool b);

    // mainwindow
    ErrFile newFile();
    ErrFile openFile(const QString &filename);
    ErrFile openRegionFile(const QString &filename, qsizetype start = 0,
                           qsizetype length = 1024);
    ErrFile openDriver(const QString &driver);
    ErrFile closeFile(int handle, bool force = false);
    ErrFile saveFile(int handle, bool ignoreMd5 = false);
    ErrFile exportFile(int handle, const QString &savename,
                       bool ignoreMd5 = false);
    bool exportFileGUI();
    ErrFile saveAsFile(int handle, const QString &savename,
                       bool ignoreMd5 = false);
    bool saveAsFileGUI();
    ErrFile closeCurrentFile(bool force = false);
    ErrFile saveCurrentFile(bool ignoreMd5 = false);
    bool openFileGUI();
    bool openRegionFileGUI();
    bool openDriverGUI();
    bool findGUI();
    bool gotoGUI();
    bool fillGUI();
    bool fillZeroGUI();

    // bookmark
    bool addBookMark(qsizetype pos, const QString &comment);
    bool modBookMark(qsizetype pos, const QString &comment);
    bool removeBookMark(qsizetype pos);
    bool clearBookMark();

    // workspace
    ErrFile openWorkSpace(const QString &filename);
    bool setCurrentEncoding(const QString &encoding);
};

class MessageBox : public QObject {
    Q_OBJECT
signals:
    void aboutQt(QWidget *parent = nullptr, const QString &title = QString());

    QMessageBox::StandardButton information(
        QWidget *parent, const QString &title, const QString &text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    QMessageBox::StandardButton question(
        QWidget *parent, const QString &title, const QString &text,
        QMessageBox::StandardButtons buttons =
            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    QMessageBox::StandardButton
    warning(QWidget *parent, const QString &title, const QString &text,
            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
            QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    QMessageBox::StandardButton
    critical(QWidget *parent, const QString &title, const QString &text,
             QMessageBox::StandardButtons buttons = QMessageBox::Ok,
             QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    void about(QWidget *parent, const QString &title, const QString &text);

    QMessageBox::StandardButton
    msgbox(QWidget *parent, QMessageBox::Icon icon, const QString &title,
           const QString &text,
           QMessageBox::StandardButtons buttons = QMessageBox::NoButton,
           QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
};

class InputBox : public QObject {
    Q_OBJECT
signals:
    QString getText(QWidget *parent, const QString &title, const QString &label,
                    QLineEdit::EchoMode echo = QLineEdit::Normal,
                    const QString &text = QString(), bool *ok = nullptr,
                    Qt::InputMethodHints inputMethodHints = Qt::ImhNone);
    QString
    getMultiLineText(QWidget *parent, const QString &title,
                     const QString &label, const QString &text = QString(),
                     bool *ok = nullptr,
                     Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

    QString getItem(QWidget *parent, const QString &title, const QString &label,
                    const QStringList &items, int current = 0,
                    bool editable = true, bool *ok = nullptr,
                    Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

    int getInt(QWidget *parent, const QString &title, const QString &label,
               int value = 0, int minValue = -2147483647,
               int maxValue = 2147483647, int step = 1, bool *ok = nullptr);

    double getDouble(QWidget *parent, const QString &title,
                     const QString &label, double value = 0,
                     double minValue = -2147483647,
                     double maxValue = 2147483647, int decimals = 1,
                     bool *ok = nullptr, double step = 1);
};

class FileDialog : public QObject {
    Q_OBJECT
signals:
    QString getExistingDirectory(
        QWidget *parent = nullptr, const QString &caption = QString(),
        const QString &dir = QString(),
        QFileDialog::Options options = QFileDialog::ShowDirsOnly);

    QString getOpenFileName(
        QWidget *parent = nullptr, const QString &caption = QString(),
        const QString &dir = QString(), const QString &filter = QString(),
        QString *selectedFilter = nullptr,
        QFileDialog::Options options = QFileDialog::Options());

    QStringList getOpenFileNames(
        QWidget *parent = nullptr, const QString &caption = QString(),
        const QString &dir = QString(), const QString &filter = QString(),
        QString *selectedFilter = nullptr,
        QFileDialog::Options options = QFileDialog::Options());

    QString getSaveFileName(
        QWidget *parent = nullptr, const QString &caption = QString(),
        const QString &dir = QString(), const QString &filter = QString(),
        QString *selectedFilter = nullptr,
        QFileDialog::Options options = QFileDialog::Options());
};

class ColorDialog : public QObject {
    Q_OBJECT
signals:
    QColor getColor(const QString &caption, QWidget *parent = nullptr);
};

class DataVisual : public QObject {
    Q_OBJECT
signals:
    bool updateText(const QString &data);
    bool updateTextList(const QStringList &data);
    bool updateTextTree(const QString &json);
    bool updateTextTable(const QString &json, const QStringList &headers,
                         const QStringList &headerNames = {});
};

} // namespace WingPlugin

struct WingPluginInfo {
    QString pluginName;
    QString pluginAuthor;
    uint pluginVersion;
    QString puid;
    QString pluginComment;
};

const auto WINGSUMMER = QStringLiteral("wingsummer");

struct WingDockWidgetInfo {
    QString widgetName;
    QString displayName;
    QWidget *widget = nullptr;
    Qt::DockWidgetArea area = Qt::DockWidgetArea::NoDockWidgetArea;
};

struct WingRibbonToolBoxInfo {
    struct RibbonCatagories {
        const QString FILE = QStringLiteral("File");
        const QString EDIT = QStringLiteral("Edit");
        const QString VIEW = QStringLiteral("View");
        const QString SCRIPT = QStringLiteral("Script");
        const QString PLUGIN = QStringLiteral("Plugin");
        const QString SETTING = QStringLiteral("Setting");
        const QString ABOUT = QStringLiteral("About");
    };

    QString catagory;
    QString displayName;

    struct Toolbox {
        QString name;
        QList<QToolButton *> tools;
    };
    QList<Toolbox> toolboxs;
};

class WingEditorViewWidget : public QWidget {
    Q_OBJECT

public:
    virtual QIcon icon() const = 0;

    virtual QString name() const = 0;

    virtual QString id() const = 0;

signals:
    void docSaved(bool saved);

public slots:
    virtual void toggled(bool isVisible) = 0;

    virtual void loadState(const QByteArray &state) { Q_UNUSED(state); }

    virtual QByteArray saveState() { return {}; }

    virtual WingEditorViewWidget *clone() = 0;

signals:
    void raiseView();
};

class IWingPlugin : public QObject {
    Q_OBJECT
public:
    typedef std::function<QVariant(const QVariantList &)> ScriptFn;

    enum MetaType : uint {
        Void,

        Bool,
        Int,
        UInt,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int64,
        UInt64,

        Float,
        Double,

        String,
        Char,
        Color,

        MetaMax, // reserved
        MetaTypeMask = 0xFFFFF,
        Ref = 0x10000000,
        Array = 0x100000,
    };

    static_assert(MetaType::MetaMax < MetaType::Array);

    struct ScriptFnInfo {
        MetaType ret;
        QVector<QPair<MetaType, QString>> params;
        ScriptFn fn;
    };

public:
    virtual int sdkVersion() const = 0;
    virtual const QString signature() const = 0;
    virtual ~IWingPlugin() = default;

    virtual bool init(const QList<WingPluginInfo> &loadedplugin) = 0;
    virtual void unload() = 0;
    virtual const QString pluginName() const = 0;
    virtual const QString pluginAuthor() const = 0;
    virtual uint pluginVersion() const = 0;
    virtual const QString pluginComment() const = 0;

    virtual QList<WingDockWidgetInfo> registeredDockWidgets() const {
        return {};
    }
    virtual QMenu *registeredHexContextMenu() const { return nullptr; }
    virtual QList<WingRibbonToolBoxInfo> registeredRibbonTools() const {
        return {};
    }
    // QMap<page, whether is visible in setting tab>
    virtual QHash<SettingPage *, bool> registeredSettingPages() const {
        return {};
    }
    virtual QList<PluginPage *> registeredPages() const { return {}; }
    virtual QList<WingEditorViewWidget *> registeredEditorViewWidgets() const {
        return {};
    }

    // QHash<function-name, fn>
    virtual QHash<QString, ScriptFnInfo> registeredScriptFn() { return {}; }

signals:
    // extension and exposed to WingHexAngelScript
    void toast(const QPixmap &icon, const QString &message);
    void trace(const QString &message);
    void debug(const QString &message);
    void warn(const QString &message);
    void error(const QString &message);
    void info(const QString &message);

public:
    WingPlugin::Reader reader;
    WingPlugin::Controller controller;
    WingPlugin::MessageBox msgbox;
    WingPlugin::InputBox inputbox;
    WingPlugin::FileDialog filedlg;
    WingPlugin::ColorDialog colordlg;
    WingPlugin::DataVisual visual;
};

} // namespace WingHex

constexpr auto IWINGPLUGIN_INTERFACE_IID = "com.wingsummer.iwingplugin";

Q_DECLARE_INTERFACE(WingHex::IWingPlugin, IWINGPLUGIN_INTERFACE_IID)

#endif // IWINGPLUGIN_H
