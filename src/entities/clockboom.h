#ifndef CLOCKBOOM_H
#define CLOCKBOOM_H

#include "enemy.h"
#include <QTimer>

class Player;

/**
 * @brief 爆炸时钟怪物 - 碰撞后倒计时爆炸
 * 特性：
 * - 不移动、不巡逻
 * - 碰撞不造成伤害
 * - 首次碰撞后进入2.5秒倒计时
 * - 倒计时期间每0.5秒切换闪烁（普通/深红色）
 * - 倒计时结束后爆炸，对范围内玩家造成1点伤害，敌人造成3点伤害
 */
class ClockBoom : public Enemy
{
    Q_OBJECT

public:
    explicit ClockBoom(const QPixmap &normalPic, const QPixmap &redPic, double scale = 1.0);
    ~ClockBoom() override;

    void move() override; // 重写move，使其不移动

protected:
    void attackPlayer() override; // 重写攻击，首次碰撞触发倒计时

private:
    bool m_triggered;         // 是否已触发倒计时
    bool m_exploded;          // 是否已爆炸
    QTimer *m_collisionTimer; // 碰撞检测定时器
    QTimer *m_blinkTimer;     // 闪烁定时器
    QTimer *m_explodeTimer;   // 爆炸定时器
    QPixmap m_normalPixmap;   // 普通图片
    QPixmap m_redPixmap;      // 深红色图片
    bool m_isRed;             // 当前是否显示红色

    void checkCollisionWithPlayer(); // 检测与玩家的碰撞
    void startCountdown();           // 开始倒计时
    void toggleBlink();              // 切换闪烁状态
    void explode();                  // 触发爆炸
    void damageNearbyEntities();     // 对范围内实体造成伤害

private slots:
    void onCollisionCheck();
    void onBlinkTimeout();
    void onExplodeTimeout();
};

#endif // CLOCKBOOM_H
