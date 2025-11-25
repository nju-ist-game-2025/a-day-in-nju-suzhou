#ifndef WASHMACHINEBOSS_H
#define WASHMACHINEBOSS_H

#include "boss.h"
#include <QPixmap>

/**
 * @brief WashMachine Boss - 洗衣机Boss（第二、三关）
 */
class WashMachineBoss : public Boss
{
    Q_OBJECT

public:
    explicit WashMachineBoss(const QPixmap &pic, double scale = 1.5);
    ~WashMachineBoss() override;

    // 可以添加洗衣机特有的行为
};

#endif // WASHMACHINEBOSS_H
