#include "mletrap.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QtMath>
#include "player.h"

MleTrap::MleTrap(QPointF position, Player* player)
    : QObject(),
      QGraphicsItem(),
      m_player(player),
      m_updateTimer(nullptr),
      m_lifetimeTimer(nullptr),
      m_position(position),
      m_radius(40.0),        // 陷阱半径40像素
      m_rootDuration(1500),  // 定身1.5秒
      m_lifetime(8000),      // 存在8秒
      m_spiralAngle(0.0),
      m_triggered(false),
      m_isDestroying(false) {
    // 设置位置
    setPos(m_position);
    setZValue(50);  // 在地面之上

    // 启动更新定时器（检测碰撞和动画）
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MleTrap::onUpdateTimer);
    m_updateTimer->start(30);  // 约33fps

    // 启动生命周期定时器
    m_lifetimeTimer = new QTimer(this);
    m_lifetimeTimer->setSingleShot(true);
    connect(m_lifetimeTimer, &QTimer::timeout, this, &MleTrap::onLifetimeTimeout);
    m_lifetimeTimer->start(m_lifetime);

    qDebug() << "[MleTrap] 创建极大似然估计陷阱，位置:" << m_position;
}

MleTrap::~MleTrap() {
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    if (m_lifetimeTimer) {
        m_lifetimeTimer->stop();
        delete m_lifetimeTimer;
        m_lifetimeTimer = nullptr;
    }
    qDebug() << "[MleTrap] 陷阱销毁";
}

QRectF MleTrap::boundingRect() const {
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

void MleTrap::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制外圈（红色）
    QPen redPen(Qt::red, 3);
    painter->setPen(redPen);
    painter->setBrush(QBrush(QColor(255, 0, 0, 30)));  // 半透明红色填充
    painter->drawEllipse(boundingRect());

    // 绘制螺旋图案
    drawSpiral(painter);

    // 如果已触发，显示不同效果
    if (m_triggered) {
        painter->setBrush(QBrush(QColor(255, 0, 0, 100)));
        painter->drawEllipse(boundingRect());
    }
}

void MleTrap::drawSpiral(QPainter* painter) {
    // 绘制螺旋图案（类似陷阱纹理）
    painter->setPen(QPen(QColor(200, 0, 0, 180), 2));

    double centerX = 0;
    double centerY = 0;

    // 绘制多条螺旋线
    for (int arm = 0; arm < 3; ++arm) {
        double armOffset = arm * (2 * M_PI / 3);
        QPointF prevPoint;
        bool firstPoint = true;

        for (double t = 0; t < 3 * M_PI; t += 0.2) {
            double r = (m_radius - 5) * t / (3 * M_PI);
            double angle = t + m_spiralAngle + armOffset;

            QPointF point(centerX + r * qCos(angle), centerY + r * qSin(angle));

            if (!firstPoint) {
                painter->drawLine(prevPoint, point);
            }
            prevPoint = point;
            firstPoint = false;
        }
    }
}

void MleTrap::onUpdateTimer() {
    if (m_isDestroying)
        return;

    // 更新螺旋动画
    m_spiralAngle += 0.05;
    if (m_spiralAngle >= 2 * M_PI) {
        m_spiralAngle -= 2 * M_PI;
    }

    // 检测玩家碰撞
    checkPlayerCollision();

    // 触发重绘
    update();
}

void MleTrap::checkPlayerCollision() {
    if (!m_player || m_triggered || m_isDestroying)
        return;

    // 计算玩家中心与陷阱中心的距离
    QPointF playerCenter = m_player->pos() +
                           QPointF(m_player->boundingRect().width() / 2, m_player->boundingRect().height() / 2);
    QPointF trapCenter = pos();

    double dx = playerCenter.x() - trapCenter.x();
    double dy = playerCenter.y() - trapCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    // 如果玩家在陷阱范围内
    if (distance < m_radius) {
        m_triggered = true;
        applyRootEffect();

        // 触发后延迟销毁
        QTimer::singleShot(500, this, &MleTrap::destroy);
    }
}

void MleTrap::applyRootEffect() {
    if (!m_player || !scene())
        return;

    // 检查玩家是否已经处于定身状态
    if (!m_player->canMove())
        return;

    // 检查效果冷却
    if (m_player->isEffectOnCooldown())
        return;

    qDebug() << "[MleTrap] 玩家踩中陷阱！定身" << m_rootDuration << "ms";

    // 设置效果冷却
    m_player->setEffectCooldown(true);

    // 显示"定身"文字提示
    QGraphicsTextItem* rootText = new QGraphicsTextItem("定身!");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    rootText->setFont(font);
    rootText->setDefaultTextColor(QColor(255, 0, 0));  // 红色
    rootText->setPos(m_player->pos().x(), m_player->pos().y() - 40);
    rootText->setZValue(200);
    scene()->addItem(rootText);

    // 禁用玩家移动
    m_player->setCanMove(false);

    // 保存玩家指针
    QPointer<Player> playerPtr = m_player;
    QGraphicsScene* currentScene = scene();

    // 定身结束后恢复移动
    QTimer::singleShot(m_rootDuration, [playerPtr, rootText, currentScene]() {
        if (playerPtr) {
            playerPtr->setCanMove(true);
            qDebug() << "[MleTrap] 定身效果结束，玩家恢复移动";

            // 3秒后解除冷却
            QTimer::singleShot(3000, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }

        // 删除文字
        if (rootText && currentScene) {
            currentScene->removeItem(rootText);
            delete rootText;
        }
    });
}

void MleTrap::onLifetimeTimeout() {
    qDebug() << "[MleTrap] 陷阱超时，消失";
    destroy();
}

void MleTrap::destroy() {
    if (m_isDestroying)
        return;
    m_isDestroying = true;

    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_lifetimeTimer) {
        m_lifetimeTimer->stop();
    }

    if (scene()) {
        scene()->removeItem(this);
    }
    deleteLater();
}
