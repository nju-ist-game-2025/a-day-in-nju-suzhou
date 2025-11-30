#ifndef PROBABILITYENEMY_H
#define PROBABILITYENEMY_H

#include <QTimer>
#include <QGraphicsTextItem>
#include <QPointer>
#include "../enemy.h"

class Player;

/**
 * @brief 跟随目标的治疗文字的控制器
 */
class HealTextController : public QObject
{
    Q_OBJECT
public:
    HealTextController(Enemy *target, QGraphicsTextItem *textItem, QObject *parent = nullptr);
    ~HealTextController() override;

private slots:
    void updatePosition();
    void cleanup();

private:
    QPointer<Enemy> m_target;
    QGraphicsTextItem *m_textItem;
    QTimer *m_updateTimer;
};

/**
 * @brief 概率论 - 第三关特殊敌人
 * 特性：
 * - 无法移动
 * - 初始时很小，随时间推移慢慢变大
 * - 60秒后变为最大（与全图高度相同）
 * - 然后进入闪烁动画并爆炸
 * - 爆炸对所有剩余的玩家和敌人造成30点伤害
 */
class ProbabilityEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit ProbabilityEnemy(const QPixmap &pic, double scale = 1.0);
    ~ProbabilityEnemy() override;

    void move() override;                 // 重写move，使其不移动
    void takeDamage(int damage) override; // 重写受伤处理

    // 检查是否有其他敌人在接触/内部
    bool hasContactingEnemies() const;

    // 暂停/恢复定时器
    void pauseTimers() override;
    void resumeTimers() override;

protected:
    void attackPlayer() override; // 重写攻击，概率论不主动攻击

private slots:
    void onGrowthUpdate();   // 成长更新
    void onBlinkTimeout();   // 闪烁超时
    void onExplodeTimeout(); // 爆炸超时
    void onContactCheck();   // 接触检测（给敌人回血）

private:
    void startBlinking();                 // 开始闪烁动画
    void toggleBlink();                   // 切换闪烁状态
    void explode(bool dealDamage = true); // 触发爆炸
    void damageAllEntities();             // 对所有实体造成伤害（爆炸效果）
    void showExplosionText();             // 显示爆炸文字
    void updateScale();                   // 更新缩放
    void healContactingEnemies();         // 给接触的敌人回血

    // 定时器
    QTimer *m_growthTimer;  // 成长定时器
    QTimer *m_blinkTimer;   // 闪烁定时器
    QTimer *m_explodeTimer; // 爆炸定时器
    QTimer *m_contactTimer; // 接触检测定时器

    // 状态
    bool m_isBlinking; // 是否正在闪烁
    bool m_exploded;   // 是否已爆炸
    bool m_isRed;      // 当前是否显示红色

    // 图片
    QPixmap m_originalPixmap; // 原始图片
    QPixmap m_normalPixmap;   // 当前普通图片
    QPixmap m_redPixmap;      // 当前红色图片

    // 成长参数
    double m_currentScale; // 当前缩放比例
    double m_initialScale; // 初始缩放比例
    double m_maxScale;     // 最大缩放比例
    int m_growthTime;      // 成长时间（毫秒）
    int m_elapsedTime;     // 已过时间（毫秒）

    // 常量参数
    static constexpr int GROWTH_DURATION_MS = 60000;    // 成长时间60秒
    static constexpr int GROWTH_UPDATE_INTERVAL = 100;  // 成长更新间隔100ms
    static constexpr int BLINK_INTERVAL = 300;          // 闪烁间隔300ms
    static constexpr int EXPLODE_DELAY = 2500;          // 闪烁后2.5秒爆炸
    static constexpr int EXPLOSION_DAMAGE = 30;         // 爆炸伤害30点
    static constexpr double INITIAL_SCALE_RATIO = 0.1;  // 初始大小为最大的10%
    static constexpr double SCENE_HEIGHT = 600.0;       // 场景高度
    static constexpr int CONTACT_CHECK_INTERVAL = 2000; // 接触检测间隔2秒（伤害频率的1/2）
    static constexpr int HEAL_AMOUNT = 2;               // 每次回血量（与碰撞伤害相同）
};

#endif // PROBABILITYENEMY_H
