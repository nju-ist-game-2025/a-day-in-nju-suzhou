#ifndef ZHUHAOENEMY_H
#define ZHUHAOENEMY_H

#include "enemy.h"
#include <QTimer>
#include <QVector>

class Player;

class ZhuhaoProjectile;

/**
 * @brief 祝昊 - 第三关精英怪
 *
 * 特性：
 * - 远程攻击敌人，沿地图边缘移动
 * - 向内侧发射半圆形弹幕（角落时发射1/4圆）
 * - 一波弹幕有6发子弹，每3秒发射一波
 * - 三种子弹类型随机选取：
 *   1. 昏睡子弹 "zzz" - 无伤害，100%昏迷
 *   2. 听不懂子弹 "叽里咕噜" - 2点伤害，50%昏迷或50%惊吓
 *   3. CPU子弹 - 2点伤害，100%惊吓效果
 */
class ZhuhaoEnemy : public Enemy {
Q_OBJECT

public:
    explicit ZhuhaoEnemy(const QPixmap &pic, double scale = 1.0);

    ~ZhuhaoEnemy() override;

    // 重写移动方法，不跟随玩家
    void move() override;

    // 重写受伤方法
    void takeDamage(int damage) override;

    // 暂停/恢复定时器
    void pauseTimers() override;

    void resumeTimers() override;

    // 初始化到随机边缘位置（公开方法，供Level调用）
    void initializeAtRandomEdge();

    // 边缘位置枚举
    enum EdgePosition {
        EDGE_TOP,           // 上边缘
        EDGE_BOTTOM,        // 下边缘
        EDGE_LEFT,          // 左边缘
        EDGE_RIGHT,         // 右边缘
        CORNER_TOP_LEFT,    // 左上角
        CORNER_TOP_RIGHT,   // 右上角
        CORNER_BOTTOM_LEFT, // 左下角
        CORNER_BOTTOM_RIGHT // 右下角
    };

protected:
    void attackPlayer() override; // 不使用近战攻击

private slots:

    void onMoveTimer();  // 移动更新
    void onShootTimer(); // 射击定时器

private:
    void moveAlongEdge();                  // 沿边缘移动
    void updateZhuhaoFacing();             // 更新朝向
    EdgePosition getCurrentEdgePosition(); // 获取当前所在边缘位置
    bool isAtCorner();                     // 检查是否在角落
    void shootBarrage();                   // 发射弹幕
    double getInwardAngle();               // 获取向内的角度（用于计算弹幕方向）

    // 移动相关
    QTimer *m_moveTimer;
    double m_edgeSpeed;     // 沿边缘移动速度
    bool m_movingClockwise; // 是否顺时针移动
    int m_currentEdge;      // 当前所在边 0=上 1=右 2=下 3=左

    // 地图边界（内侧，考虑敌人大小）
    static constexpr double MAP_LEFT = 70.0;         // 左边界内侧
    static constexpr double MAP_RIGHT = 730.0;       // 右边界内侧
    static constexpr double MAP_TOP = 100.0;         // 上边界内侧
    static constexpr double MAP_BOTTOM = 500.0;      // 下边界内侧
    static constexpr double CORNER_THRESHOLD = 40.0; // 角落检测阈值

    // 射击相关
    QTimer *m_shootTimer;
    int m_bulletCount;    // 一波弹幕数量
    double m_bulletSpeed; // 子弹速度

    // 参数常量
    static constexpr int SHOOT_COOLDOWN = 3000;         // 3秒发射一波
    static constexpr int BULLETS_PER_WAVE = 18;         // 每波18发（360°均匀分布）
    static constexpr double DEFAULT_BULLET_SPEED = 3.0; // 子弹速度较慢
    static constexpr double EDGE_MOVE_SPEED = 1.5;      // 边缘移动速度
};

/**
 * @brief zhuhao特殊弹幕子弹基类
 */
class ZhuhaoProjectile : public QObject, public QGraphicsPixmapItem {
Q_OBJECT

public:
    enum BulletType {
        SLEEP_ZZZ, // 昏睡子弹 "zzz"
        CONFUSED,  // 听不懂子弹 "叽里咕噜"
        CPU        // CPU子弹
    };

    ZhuhaoProjectile(BulletType type, QPointF startPos, double angle, double speed, QGraphicsScene *scene);

    ~ZhuhaoProjectile() override;

    void setPaused(bool paused);

    bool isPaused() const { return m_isPaused; }

private slots:

    void onMoveTimer();

    void checkCollision();

private:
    void createVisual();              // 创建视觉效果
    void applyEffect(Player *player); // 应用效果到玩家

    BulletType m_type;
    double m_angle; // 移动角度（弧度）
    double m_speed;
    double m_dx, m_dy; // 移动方向
    QTimer *m_moveTimer;
    QTimer *m_collisionTimer;
    bool m_isPaused;
    bool m_isDestroying;
};

#endif // ZHUHAOENEMY_H
