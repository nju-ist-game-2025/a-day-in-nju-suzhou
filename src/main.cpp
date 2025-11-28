#include <QApplication>
#include <QDebug>
#include "core/configmanager.h"
#include "core/configvalidator.h"
#include "core/gamewindow.h"
#include "core/logging.h"
#include "items/itemeffectconfig.h"

int main(int argc, char* argv[]) {
    // 禁用缩放，确保窗口大小在所有电脑上一致
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#endif

    QApplication a(argc, argv);
    // 可选：使用自定义消息处理程序过滤消息（可以更细粒度地控制，或在不同情况下切换）
    // qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg){
    //     if (type == QtDebugMsg) {
    //         return; // 丢弃 debug 日志
    //     }
    //     QByteArray localMsg = msg.toLocal8Bit();
    //     const char *file = context.file ? context.file : "";
    //     const char *function = context.function ? context.function : "";
    //     fprintf(stderr, "%s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
    // });
    // 加载配置文件
    if (!ConfigManager::instance().loadConfig("assets/config.json")) {
        qCritical() << "无法加载配置文件，程序退出";
        return 1;
    }

    // 从配置读取 logging.debug 并初始化日志系统
    bool enableQDebug = ConfigManager::instance().getLoggingDebug(false);
    Logging::initializeLogging(enableQDebug);

    // 加载道具效果配置
    if (!ItemEffectConfig::instance().loadConfig("assets/item_effects.json")) {
        qWarning() << "无法加载道具效果配置文件，将使用默认值";
    }

    // 验证配置（仅在启用配置验证时）
    if (ConfigManager::instance().isConfigValidationEnabled()) {
        qDebug() << "\n"
                 << ConfigValidator::getValidationReport();
        if (!ConfigValidator::validateAllConfigs()) {
            qWarning() << "配置验证存在错误，请检查config.json";
        }
    }

    GameWindow w;
    w.show();
    return QApplication::exec();
}