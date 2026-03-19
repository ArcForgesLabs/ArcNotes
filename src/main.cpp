/*********************************************************************************************
 * Mozilla License
 * Just a meantime project to see the ability of qt, the framework that my OS might be based on
 * And for those linux users that believe in the power of notes
 *********************************************************************************************/

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QTextStream>
#include <QWindow>

#include "appbackend.h"
#include "singleinstance.h"

namespace {
QMutex g_log_mutex;

QString notes_log_file_path() {
    const QString log_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!log_dir.isEmpty()) {
        QDir().mkpath(log_dir);
        return log_dir + QStringLiteral("/notes-runtime-debug.log");
    }

    return QCoreApplication::applicationDirPath() + QStringLiteral("/notes-runtime-debug.log");
}

void notes_message_handler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QMutexLocker locker(&g_log_mutex);
    QFile log_file(notes_log_file_path());
    if (!log_file.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&log_file);
    stream << qFormatLogMessage(type, context, message) << Qt::endl;
}
}  // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    qSetMessagePattern(QStringLiteral("[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{type}] [%{category}] %{message}"));
    qInstallMessageHandler(notes_message_handler);

    // Set application information
    QGuiApplication::setApplicationName("Notes");
    QGuiApplication::setApplicationVersion(APP_VERSION);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QGuiApplication::setDesktopFileName(APP_ID);
#endif

    if (QFontDatabase::addApplicationFont(":/fonts/fontawesome/fa-solid-900.ttf") < 0)
        qWarning() << "FontAwesome Solid cannot be loaded !";

    if (QFontDatabase::addApplicationFont(":/fonts/fontawesome/fa-regular-400.ttf") < 0)
        qWarning() << "FontAwesome Regular cannot be loaded !";

    if (QFontDatabase::addApplicationFont(":/fonts/fontawesome/fa-brands-400.ttf") < 0)
        qWarning() << "FontAwesome Brands cannot be loaded !";

    if (QFontDatabase::addApplicationFont(":/fonts/material/material-symbols-outlined.ttf") < 0)
        qWarning() << "Material Symbols cannot be loaded !";

    // Load fonts from resources
    // Roboto
    QFontDatabase::addApplicationFont(":/fonts/roboto-hinted/Roboto-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/roboto-hinted/Roboto-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/roboto-hinted/Roboto-Regular.ttf");

    // Source Sans Pro
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-Black.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-BlackItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-BoldItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-ExtraLight.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-ExtraLightItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-LightItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/sourcesanspro/SourceSansPro-SemiBoldItalic.ttf");

    // Trykker
    QFontDatabase::addApplicationFont(":/fonts/trykker/Trykker-Regular.ttf");

    // Mate
    QFontDatabase::addApplicationFont(":/fonts/mate/Mate-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/mate/Mate-Italic.ttf");

    // PT Serif
    QFontDatabase::addApplicationFont(":/fonts/ptserif/PTSerif-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/ptserif/PTSerif-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/ptserif/PTSerif-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/ptserif/PTSerif-BoldItalic.ttf");

    // iA Mono
    QFontDatabase::addApplicationFont(":/fonts/iamono/iAWriterMonoS-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iamono/iAWriterMonoS-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iamono/iAWriterMonoS-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iamono/iAWriterMonoS-BoldItalic.ttf");

    // iA Duo
    QFontDatabase::addApplicationFont(":/fonts/iaduo/iAWriterDuoS-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaduo/iAWriterDuoS-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaduo/iAWriterDuoS-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaduo/iAWriterDuoS-BoldItalic.ttf");

    // iA Quattro
    QFontDatabase::addApplicationFont(":/fonts/iaquattro/iAWriterQuattroS-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaquattro/iAWriterQuattroS-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaquattro/iAWriterQuattroS-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/iaquattro/iAWriterQuattroS-BoldItalic.ttf");

    // Prevent many instances of the app to be launched
    QString name = "com.awsomeness.notes";
    SingleInstance instance;
    if (SingleInstance::hasPrevious(name)) {
        return EXIT_SUCCESS;
    }

    instance.listen(name);

    QQmlApplicationEngine engine;
    engine.loadFromModule("nuttyartist.notes", "Main");
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    auto* rootWindow = qobject_cast<QWindow*>(engine.rootObjects().constFirst());
    QObject::connect(&instance, &SingleInstance::newInstance, &app, [rootWindow]() {
        if (rootWindow == nullptr) {
            return;
        }
        rootWindow->show();
        rootWindow->raise();
        rootWindow->requestActivate();
    });

    return QGuiApplication::exec();
}
