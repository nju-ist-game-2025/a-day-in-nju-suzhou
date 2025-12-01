#ifndef ENEMYFACTORY_H
#define ENEMYFACTORY_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <functional>
#include <unordered_map>

class Enemy;
class Player;
class QGraphicsScene;

/**
 * @brief 敌人工厂类 - 使用注册表模式创建敌人实例
 *
 * 将原来 Level::createEnemyByType() 中的巨大 if-else 链重构为可扩展的工厂模式。
 * 每种敌人类型可以通过静态注册的方式添加到工厂中，无需修改工厂代码。
 */
class EnemyFactory {
   public:
    // 敌人创建函数类型
    using CreatorFunc = std::function<Enemy*(const QPixmap&, double)>;

    /**
     * @brief 获取单例实例
     */
    static EnemyFactory& instance();

    /**
     * @brief 注册敌人类型
     * @param levelNumber 关卡号
     * @param enemyType 敌人类型标识符
     * @param creator 创建函数
     */
    void registerEnemy(int levelNumber, const QString& enemyType, CreatorFunc creator);

    /**
     * @brief 创建敌人实例
     * @param levelNumber 关卡号
     * @param enemyType 敌人类型标识符
     * @param pic 敌人图片
     * @param scale 缩放比例
     * @return 敌人实例指针，如果类型未注册则返回默认 Enemy
     */
    Enemy* createEnemy(int levelNumber, const QString& enemyType, const QPixmap& pic, double scale);

    /**
     * @brief 检查敌人类型是否已注册
     */
    bool isRegistered(int levelNumber, const QString& enemyType) const;

   private:
    EnemyFactory() = default;
    ~EnemyFactory() = default;

    // 禁止拷贝
    EnemyFactory(const EnemyFactory&) = delete;
    EnemyFactory& operator=(const EnemyFactory&) = delete;

    // 注册表：level_enemyType -> creator
    std::unordered_map<std::string, CreatorFunc> m_creators;

    // 生成注册表键
    std::string makeKey(int levelNumber, const QString& enemyType) const;
};

/**
 * @brief 敌人注册辅助类 - 用于静态初始化时自动注册敌人类型
 */
class EnemyRegistrar {
   public:
    EnemyRegistrar(int levelNumber, const QString& enemyType, EnemyFactory::CreatorFunc creator) {
        EnemyFactory::instance().registerEnemy(levelNumber, enemyType, creator);
    }
};

// 宏：简化敌人注册
#define REGISTER_ENEMY(level, type, className)                  \
    static EnemyRegistrar s_##className##Registrar(level, type, \
                                                   [](const QPixmap& pic, double scale) -> Enemy* { return new className(pic, scale); })

#endif  // ENEMYFACTORY_H
