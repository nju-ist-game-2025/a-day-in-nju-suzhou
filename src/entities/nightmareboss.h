#ifndef NIGHTMAREBOSS_H
#define NIGHTMAREBOSS_H

#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QPixmap>
#include <QTimer>
#include "boss.h"

/**
 * @brief Nightmare Boss - 第一关的特殊Boss
 * 一阶段：单段dash
 * 死亡时：亡语触发进入二阶段
 * 二阶段：单段dash（更快） + 两个技能
 */
class NightmareBoss : public Boss {
    Q_OBJECT

   public:
    explicit NightmareBoss(const QPixmap& pic, double scale = 1.5, QGraphicsScene* scene = nullptr);
    ~NightmareBoss() override;

    void takeDamage(int damage) override;  // 重写伤害处理，实现亡语
    void move() override;                  // 重写移动

    // 重写暂停/恢复方法
    void pauseTimers() override;
    void resumeTimers() override;

   signals:
    void phase1DeathTriggered();                                            // 一阶段死亡信号
    void requestSpawnEnemies(const QVector<QPair<QString, int>>& enemies);  // 请求召唤敌人

   private:
    // 阶段管理
    int m_phase;             // 当前阶段（1或2）
    bool m_isTransitioning;  // 是否正在阶段转换中
    QPixmap m_phase2Pixmap;  // 二阶段图片

    // 技能1：噩梦缠绕（每20秒）
    QTimer* m_nightmareWrapTimer;

    // 技能2：噩梦降临（每60秒）
    QTimer* m_nightmareDescentTimer;
    bool m_firstDescentTriggered;  // 首次技能2是否已触发

    // 遮罩效果（由NightmareBoss自己管理）
    QGraphicsPixmapItem* m_shadowOverlay;
    QGraphicsTextItem* m_shadowText;
    QTimer* m_shadowTimer;

    // 私有方法
    void enterPhase2();              // 进入二阶段
    void setupPhase2Skills();        // 设置二阶段技能
    void performNightmareWrap();     // 执行噩梦缠绕
    void performNightmareDescent();  // 执行噩梦降临
    void killAllEnemies();           // 击杀场上所有小怪

    // 遮罩效果方法（内部使用）
    void showShadowOverlay(const QString& text, int duration);
    void hideShadowOverlay();

   private slots:
    void onNightmareWrapTimeout();     // 噩梦缠绕定时触发
    void onNightmareDescentTimeout();  // 噩梦降临定时触发
};

#endif  // NIGHTMAREBOSS_H
