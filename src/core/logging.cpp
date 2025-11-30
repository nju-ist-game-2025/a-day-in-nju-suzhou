#include "core/logging.h"
#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>
#include <cstdio>

namespace {

    static std::atomic_bool g_debug_enabled{false};
    static QMutex g_print_mutex;

    void defaultMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        // 如果是 debug 消息并且当前关闭，则丢弃
        if (type == QtDebugMsg && !g_debug_enabled.load()) {
            return;
        }
        QByteArray localMsg = msg.toLocal8Bit();
        const char *file = context.file ? context.file : "";
        const char *function = context.function ? context.function : "";

        // 标记时间 & 类型
        const char *typeStr = "DEBUG";
        switch (type) {
            case QtDebugMsg:
                typeStr = "DEBUG";
                break;
            case QtInfoMsg:
                typeStr = "INFO";
                break;
            case QtWarningMsg:
                typeStr = "WARNING";
                break;
            case QtCriticalMsg:
                typeStr = "CRITICAL";
                break;
            case QtFatalMsg:
                typeStr = "FATAL";
                break;
            default:
                typeStr = "LOG";
                break;
        }

        QMutexLocker locker(&g_print_mutex);
        fprintf(stderr, "%s %s (%s:%u, %s)\n",
                QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8().constData(),
                typeStr,
                file,
                context.line,
                function);
        fprintf(stderr, "%s\n", localMsg.constData());
        fflush(stderr);

        if (type == QtFatalMsg) {
            abort();
        }
    }

}  // anonymous namespace

namespace Logging {

    void initializeLogging(bool debugEnabled) {
        g_debug_enabled.store(debugEnabled);
        // 安装自定义消息处理程序
        qInstallMessageHandler(defaultMessageHandler);
    }

    void setDebugEnabled(bool enabled) {
        g_debug_enabled.store(enabled);
    }

    bool isDebugEnabled() {
        return g_debug_enabled.load();
    }

}  // namespace Logging
