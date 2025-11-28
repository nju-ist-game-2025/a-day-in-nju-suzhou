#include <QApplication>
#include <QDebug>
#include "core/configmanager.h"
#include "core/configvalidator.h"
#include "core/gamewindow.h"
#include "items/itemeffectconfig.h"

int main(int argc, char* argv[]) {
    // 禁用缩放，确保窗口大小在所有电脑上一致
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#endif

    QApplication a(argc, argv);
    // 加载配置文件
    if (!ConfigManager::instance().loadConfig("assets/config.json")) {
        qCritical() << "无法加载配置文件，程序退出";
        return 1;
    }

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