#include <QApplication>
#include <QDebug>
#include "core/configmanager.h"
#include "core/configvalidator.h"
#include "core/gamewindow.h"
#include "core/logging.h"
#include "items/itemeffectconfig.h"

int main(int argc, char* argv[]) {

    QApplication a(argc, argv);
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