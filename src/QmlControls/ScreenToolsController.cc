#include "ScreenToolsController.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QStringList>

#include <QtGui/QCursor>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QInputDevice>

#if defined(Q_OS_IOS)
#include <sys/utsname.h>
#endif

QGC_LOGGING_CATEGORY(ScreenToolsControllerLog, "QMLControls.ScreenToolsController")

ScreenToolsController::ScreenToolsController(QObject *parent)
    : QObject(parent)
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

ScreenToolsController::~ScreenToolsController()
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

int ScreenToolsController::mouseX()
{
    return QCursor::pos().x();
}

int ScreenToolsController::mouseY()
{
    return QCursor::pos().y();
}

bool ScreenToolsController::hasTouch()
{
    for (const auto &inputDevice: QInputDevice::devices()) {
        if (inputDevice->type() == QInputDevice::DeviceType::TouchScreen) {
            return true;
        }
    }
    return false;
}

QString ScreenToolsController::iOSDevice()
{
#if defined(Q_OS_IOS)
    struct utsname systemInfo;
    uname(&systemInfo);
    return QString(systemInfo.machine);
#else
    return QString();
#endif
}

QString ScreenToolsController::fixedFontFamily()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString ScreenToolsController::normalFontFamily()
{
    //-- See App.SettingsGroup.json for the language setting index.
    QLocale::Language languageId = static_cast<QLocale::Language>(
        SettingsManager::instance()->appSettings()->qLocaleLanguage()->rawValue().toInt());
    if (languageId == QLocale::AnyLanguage) {
        languageId = QLocale::system().language();
    }

    if (languageId == QLocale::Korean) {
        return QStringLiteral("NanumGothic");
    }

    if (languageId == QLocale::Chinese) {
        QStringList preferredCjkFamilies;
#if defined(Q_OS_WIN)
        preferredCjkFamilies = {
            QStringLiteral("Microsoft YaHei UI"),
            QStringLiteral("Microsoft YaHei"),
            QStringLiteral("SimHei"),
            QStringLiteral("SimSun")
        };
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        preferredCjkFamilies = {
            QStringLiteral("PingFang SC"),
            QStringLiteral("Heiti SC"),
            QStringLiteral("Songti SC")
        };
#elif defined(Q_OS_ANDROID) || defined(Q_OS_LINUX)
        preferredCjkFamilies = {
            QStringLiteral("Noto Sans CJK SC"),
            QStringLiteral("Noto Sans SC"),
            QStringLiteral("Source Han Sans SC"),
            QStringLiteral("WenQuanYi Micro Hei")
        };
#endif
        const QFontDatabase fontDatabase;
        const QStringList installedFamilies = fontDatabase.families();
        for (const QString &family : preferredCjkFamilies) {
            if (installedFamilies.contains(family, Qt::CaseInsensitive)) {
                return family;
            }
        }
    }

    // Chinese UI uses one CJK-capable family for consistent Latin and CJK rendering.
    // Other locales use Open Sans; Qt handles missing glyphs for mixed-language text.
    return QStringLiteral("Open Sans");
}

double ScreenToolsController::defaultFontDescent(int pointSize)
{
    return QFontMetrics(QFont(normalFontFamily(), pointSize)).descent();
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
bool ScreenToolsController::fakeMobile()
{
    return qgcApp()->fakeMobile();
}
#endif
