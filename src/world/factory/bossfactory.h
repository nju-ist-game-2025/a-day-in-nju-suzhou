#ifndef BOSSFACTORY_H
#define BOSSFACTORY_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <functional>
#include <unordered_map>

class Boss;
class QGraphicsScene;

/**
 * @brief Boss工厂类 - 使用注册表模式创建Boss实例
 *
 * 将原来 Level::createBossByLevel() 中的 switch-case 重构为可扩展的工厂模式。
 * 每种Boss类型可以通过静态注册的方式添加到工厂中。
 */
class BossFactory {
   public:
    // Boss创建函数类型（包含场景参数，因为某些Boss需要场景引用）
    using CreatorFunc = std::function<Boss*(const QPixmap&, double, QGraphicsScene*)>;

    /**
     * @brief 获取单例实例
     */
    static BossFactory& instance();

    /**
     * @brief 注册Boss类型
     * @param levelNumber 关卡号
     * @param creator 创建函数
     */
    void registerBoss(int levelNumber, CreatorFunc creator);

    /**
     * @brief 创建Boss实例
     * @param levelNumber 关卡号
     * @param pic Boss图片
     * @param scale 缩放比例
     * @param scene 场景指针（某些Boss需要）
     * @return Boss实例指针，如果关卡未注册则返回默认Boss
     */
    Boss* createBoss(int levelNumber, const QPixmap& pic, double scale, QGraphicsScene* scene = nullptr);

    /**
     * @brief 检查关卡是否已注册Boss
     */
    bool isRegistered(int levelNumber) const;

   private:
    BossFactory() = default;
    ~BossFactory() = default;

    // 禁止拷贝
    BossFactory(const BossFactory&) = delete;
    BossFactory& operator=(const BossFactory&) = delete;

    // 注册表：levelNumber -> creator
    std::unordered_map<int, CreatorFunc> m_creators;
};

/**
 * @brief Boss注册辅助类 - 用于静态初始化时自动注册Boss类型
 */
class BossRegistrar {
   public:
    BossRegistrar(int levelNumber, BossFactory::CreatorFunc creator) {
        BossFactory::instance().registerBoss(levelNumber, creator);
    }
};

// 宏：简化Boss注册（无场景依赖）
#define REGISTER_BOSS(level, className)                  \
    static BossRegistrar s_##className##Registrar(level, \
                                                  [](const QPixmap& pic, double scale, QGraphicsScene*) -> Boss* { return new className(pic, scale); })

// 宏：简化Boss注册（需要场景）
#define REGISTER_BOSS_WITH_SCENE(level, className)       \
    static BossRegistrar s_##className##Registrar(level, \
                                                  [](const QPixmap& pic, double scale, QGraphicsScene* scene) -> Boss* { return new className(pic, scale, scene); })

#endif  // BOSSFACTORY_H
