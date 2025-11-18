#include "hud.h"
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QTimer>

HUD::HUD(QGraphicsItem* parent) 
    : QGraphicsItem(parent), currentHealth(3.0f), maxHealth(3.0f), 
      isFlashing(false), flashCount(0) {
    
    flashTimer = new QTimer(this);
    connect(flashTimer, &QTimer::timeout, this, &HUD::endDamageFlash);

    screenFlashTimer = new QTimer(this);
    screenFlashTimer->setSingleShot(true);
    connect(screenFlashTimer, &QTimer::timeout, this, [this]() {
        isScreenFlashing = false;
        update();
    });
    
    setPos(0, 0);
}

QRectF HUD::boundingRect() const {
    return QRectF(0, 0, 230, 60);
}

void HUD::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制屏幕边缘红光闪烁效果
    if (isScreenFlashing) {
        QLinearGradient gradient;
        
        gradient.setStart(0, 0);
        gradient.setFinalStop(0, 100);
        gradient.setColorAt(0, QColor(255, 0, 0, 120));
        gradient.setColorAt(1, QColor(255, 0, 0, 0));
        painter->setBrush(QBrush(gradient));
        painter->setPen(Qt::NoPen);
        painter->drawRect(0, 0, 800, 100);
        
        gradient.setStart(0, 600);
        gradient.setFinalStop(0, 500);
        gradient.setColorAt(0, QColor(255, 0, 0, 120));
        gradient.setColorAt(1, QColor(255, 0, 0, 0));
        painter->setBrush(QBrush(gradient));
        painter->drawRect(0, 500, 800, 100);
        
        gradient.setStart(0, 0);
        gradient.setFinalStop(100, 0);
        gradient.setColorAt(0, QColor(255, 0, 0, 120));
        gradient.setColorAt(1, QColor(255, 0, 0, 0));
        painter->setBrush(QBrush(gradient));
        painter->drawRect(0, 0, 100, 600);
        
        gradient.setStart(800, 0);
        gradient.setFinalStop(700, 0);
        gradient.setColorAt(0, QColor(255, 0, 0, 120));
        gradient.setColorAt(1, QColor(255, 0, 0, 0));
        painter->setBrush(QBrush(gradient));
        painter->drawRect(700, 0, 100, 600);
    }

    const int textAreaWidth = 80;      // 文字区域宽度
    const int healthBarX = textAreaWidth; // 血条起始X坐标
    const int healthBarY = 10;         // 血条Y坐标
    const int healthBarWidth = 150;    // 血条宽度
    const int healthBarHeight = 25;    // 血条高度
    
    // 绘制血条背景
    painter->setBrush(QColor(50, 50, 50, 200));
    painter->setPen(QPen(Qt::black, 2));
    painter->drawRect(healthBarX, healthBarY, healthBarWidth, healthBarHeight);

    // 绘制当前血量
    if (currentHealth > 0) {
        float healthWidth = (currentHealth / maxHealth) * healthBarWidth;

        // 留出边框
        int fillX = healthBarX + 1;
        int fillY = healthBarY + 1;
        int fillWidth = healthWidth - 2;
        int fillHeight = healthBarHeight - 2;

        if (fillWidth > 0) {
            if (isFlashing && flashCount % 2 == 0) {
                painter->setBrush(QColor(255, 100, 100));
            } else {
                painter->setBrush(Qt::red);
            }
            painter->setPen(QPen(Qt::darkRed, 1));
            painter->drawRect(fillX, fillY, fillWidth, fillHeight);
        }
    }
    
    // 绘制血量文字
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPointSize(10);
    font.setBold(true);
    painter->setFont(font);

    QString healthText = QString("%1/%2").arg(currentHealth).arg(maxHealth);
    painter->drawText(QRect(healthBarX, healthBarY, healthBarWidth, healthBarHeight), 
                     Qt::AlignCenter, healthText);
    
    // 绘制"生命值"文字
    painter->setPen(Qt::red);
    font.setPointSize(11);
    painter->setFont(font);
    painter->drawText(QRect(5, healthBarY, textAreaWidth - 5, healthBarHeight), 
                     Qt::AlignLeft | Qt::AlignVCenter, "生命值");
}

void HUD::updateHealth(float current, float max) {
    float oldHealth = currentHealth;
    currentHealth = qMax(0.0f, current);
    maxHealth = qMax(1.0f, max);
    
    qDebug() << "HUD更新: 当前血量" << currentHealth << "/" << maxHealth;
    
    update();
}

void HUD::triggerDamageFlash() {
    isFlashing = true;
    flashCount = 0;
    
    flashTimer->stop();
    
    isScreenFlashing = true;
    screenFlashTimer->start(300);

    QTimer::singleShot(0, this, [this]() { update(); flashCount++; });
    QTimer::singleShot(150, this, [this]() { update(); flashCount++; });
    QTimer::singleShot(300, this, [this]() { update(); flashCount++; });
    QTimer::singleShot(450, this, [this]() { 
        isFlashing = false; 
        update(); 
    });

    update();
}

void HUD::endDamageFlash() {
    isFlashing = false;
    update();
}
