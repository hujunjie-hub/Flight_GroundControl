#pragma once

#include <QtCore/QProcess>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

class FlightFirmwareInstaller : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool targetReady READ targetReady NOTIFY stateChanged)
    Q_PROPERTY(QString output READ output NOTIFY stateChanged)
    Q_PROPERTY(QString target READ target NOTIFY stateChanged)
    Q_PROPERTY(QUrl defaultProgrammer READ defaultProgrammer CONSTANT)

public:
    explicit FlightFirmwareInstaller(QObject *parent = nullptr);

    bool busy() const { return _process.state() != QProcess::NotRunning; }
    bool targetReady() const { return !_target.isEmpty(); }
    QString output() const { return _output; }
    QString target() const { return _target; }
    QUrl defaultProgrammer() const;

    Q_INVOKABLE void probe(const QUrl &programmer, const QString &serial);
    Q_INVOKABLE void flash(const QUrl &image);
    Q_INVOKABLE void invalidateTarget();

signals:
    void stateChanged();

private:
    void _start(const QStringList &args, bool probe);
    void _readOutput();
    void _finish(int exitCode, QProcess::ExitStatus exitStatus);

    QProcess _process;
    QString _programmer;
    QString _serial;
    QString _target;
    QString _output;
    bool _probing = false;
    bool _acceptProbe = false;
};
