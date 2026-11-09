#include "FlightFirmwareInstaller.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QtEndian>

FlightFirmwareInstaller::FlightFirmwareInstaller(QObject *parent)
    : QObject(parent)
{
    _process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&_process, &QProcess::stateChanged, this, &FlightFirmwareInstaller::stateChanged);
    connect(&_process, &QProcess::readyRead, this, &FlightFirmwareInstaller::_readOutput);
    connect(&_process, &QProcess::finished, this, &FlightFirmwareInstaller::_finish);
    connect(&_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            _output = tr("Unable to start STM32CubeProgrammer. %1").arg(_process.errorString());
            emit stateChanged();
        }
    });
}

QUrl FlightFirmwareInstaller::defaultProgrammer() const
{
    QString program = QStandardPaths::findExecutable(QStringLiteral("STM32_Programmer_CLI"));
#ifdef Q_OS_WIN
    if (program.isEmpty()) {
        const QString cubeClt = QStringLiteral("D:/STM32CubeCLT/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe");
        if (QFileInfo::exists(cubeClt)) {
            program = cubeClt;
        }
    }
#endif
    return program.isEmpty() ? QUrl() : QUrl::fromLocalFile(program);
}

void FlightFirmwareInstaller::invalidateTarget()
{
    _acceptProbe = false;
    _target.clear();
    emit stateChanged();
}

void FlightFirmwareInstaller::probe(const QUrl &programmer, const QString &serial)
{
    if (busy()) {
        return;
    }
    _target.clear();
    const QFileInfo cli(programmer.toLocalFile());
    static const QRegularExpression serialPattern(QStringLiteral("^[A-Fa-f0-9]{24}$"));
    if (!programmer.isLocalFile() || !cli.isFile() ||
        cli.completeBaseName() != QStringLiteral("STM32_Programmer_CLI") ||
        !serialPattern.match(serial.trimmed()).hasMatch()) {
        _output = tr("Select STM32_Programmer_CLI and enter the 24-digit hexadecimal ST-LINK serial number.");
        emit stateChanged();
        return;
    }
    _programmer = cli.absoluteFilePath();
    _serial = serial.trimmed();
    _acceptProbe = true;
    _start({}, true);
}

void FlightFirmwareInstaller::flash(const QUrl &image)
{
    if (busy() || !targetReady()) {
        return;
    }
    QFile file(image.toLocalFile());
    constexpr quint32 flashBase = 0x08000000u;
    constexpr qint64 flashSize = 1024 * 1024;
    if (!image.isLocalFile() || QFileInfo(file).suffix().compare(QStringLiteral("bin"), Qt::CaseInsensitive) ||
        !file.open(QIODevice::ReadOnly) || file.size() < 8 || file.size() > flashSize) {
        _output = tr("Select a valid Flight binary image of at most 1 MiB.");
        emit stateChanged();
        return;
    }
    const QByteArray vector = file.read(8);
    if (vector.size() != 8) {
        _output = tr("Unable to read the firmware vector table.");
        emit stateChanged();
        return;
    }
    const quint32 stack = qFromLittleEndian<quint32>(vector.constData());
    const quint32 reset = qFromLittleEndian<quint32>(vector.constData() + 4);
    const bool stackInRam = (stack > 0x20000000u && stack <= 0x20020000u) ||
                            (stack > 0x24000000u && stack <= 0x24050000u);
    if (!stackInRam || (stack & 7u) || !(reset & 1u) ||
        reset < flashBase || reset - flashBase >= static_cast<quint32>(file.size())) {
        _output = tr("The binary vector table does not match the Flight H723 flash layout.");
        emit stateChanged();
        return;
    }
    _target.clear();
    _start({QStringLiteral("-d"), QFileInfo(file).absoluteFilePath(),
            QStringLiteral("0x08000000"), QStringLiteral("-v")}, false);
}

void FlightFirmwareInstaller::_start(const QStringList &args, bool probe)
{
    _probing = probe;
    _output.clear();
    QStringList command = {QStringLiteral("-c"), QStringLiteral("port=SWD"),
                           QStringLiteral("sn=%1").arg(_serial)};
    command.append(args);
    _process.start(_programmer, command);
    emit stateChanged();
}

void FlightFirmwareInstaller::_readOutput()
{
    _output.append(QString::fromLocal8Bit(_process.readAll()));
    emit stateChanged();
}

void FlightFirmwareInstaller::_finish(int exitCode, QProcess::ExitStatus exitStatus)
{
    _readOutput();
    const bool ok = exitStatus == QProcess::NormalExit && exitCode == 0;
    if (_probing) {
        static const QRegularExpression deviceId(QStringLiteral(R"(Device\s+ID\s*:\s*0x483\b)"),
                                                  QRegularExpression::CaseInsensitiveOption);
        if (_acceptProbe && ok && deviceId.match(_output).hasMatch()) {
            _target = tr("STM32H723 family (0x483), ST-LINK %1").arg(_serial);
        } else {
            _output.append(tr("\nFlight target probe failed. Expected STM32 device ID 0x483."));
        }
    } else if (ok && _output.contains(QStringLiteral("Download verified successfully"), Qt::CaseInsensitive)) {
        _output.append(tr("\nFirmware written and verified."));
    } else {
        _output.append(tr("\nFirmware verification did not succeed. Check the programmer output."));
    }
    emit stateChanged();
}
