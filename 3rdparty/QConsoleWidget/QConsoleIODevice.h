#ifndef QCONSOLEIODEVICE_H
#define QCONSOLEIODEVICE_H

#include <QByteArray>
#include <QIODevice>

class QConsoleWidget;

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

class QConsoleIODevice : public QIODevice {
    Q_OBJECT

public:
    explicit QConsoleIODevice(QConsoleWidget *w, QObject *parent = nullptr);
    ~QConsoleIODevice();
    qint64 bytesAvailable() const override;
    bool waitForReadyRead(int msecs) override;
    QConsoleWidget *widget() const { return widget_; }

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    friend class QConsoleWidget;
    QConsoleWidget *widget_;
    QByteArray readbuff_;
    int readpos_;
    qint64 writtenSinceLastEmit_, readSinceLastEmit_;
    bool readyReadEmmited_;
    void consoleWidgetInput(const QString &in);
};

#endif
