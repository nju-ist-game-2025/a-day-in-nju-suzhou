#ifndef NIGHTMAREBOSS_H
#define NIGHTMAREBOSS_H

#include "boss.h"
#include <QPixmap>

/**
 * @brief Nightmare Boss - 第一关的特殊Boss
 * 暂时使用洗衣机的基础属性以便调试
 */
class NightmareBoss : public Boss
{
    Q_OBJECT

public:
    explicit NightmareBoss(const QPixmap &pic, double scale = 1.5);
    ~NightmareBoss() override;

    // 可以在这里添加Nightmare特有的行为和属性
    // 例如：特殊攻击模式、阶段变化等

private:
    // Nightmare特有的属性
    int m_phase; // 战斗阶段（预留用于多阶段boss）
};

#endif // NIGHTMAREBOSS_H
