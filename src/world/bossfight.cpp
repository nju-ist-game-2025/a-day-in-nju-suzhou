#include "bossfight.h"
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPropertyAnimation>
#include <QtMath>
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/level_1/nightmareboss.h"
#include "../entities/level_2/washmachineboss.h"
#include "../entities/level_3/teacherboss.h"
#include "../entities/player.h"
#include "../entities/projectile.h"

BossFight::BossFight(Player* player, QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_player(player), m_scene(scene) {
}

BossFight::~BossFight() {
    cleanup();
}

void BossFight::cleanup() {
    // 清理吸纳动画定时器
    if (m_absorbAnimationTimer) {
        m_absorbAnimationTimer->stop();
        disconnect(m_absorbAnimationTimer, nullptr, nullptr, nullptr);
        delete m_absorbAnimationTimer;
        m_absorbAnimationTimer = nullptr;
    }

    m_absorbingItems.clear();
    m_absorbStartPositions.clear();
    m_absorbAngles.clear();

    m_currentWashMachineBoss = nullptr;
    m_currentTeacherBoss = nullptr;
    m_bossDefeated = false;
    m_rewardSequenceActive = false;
    m_isAbsorbAnimationActive = false;
}

// Boss初始化

void BossFight::initNightmareBoss(NightmareBoss* boss) {
    if (!boss)
        return;

    // 连接Nightmare Boss的特殊信号
    connect(boss, &NightmareBoss::requestSpawnEnemies,
            this, &BossFight::requestSpawnEnemies);

    // 当一阶段亡语触发时，开始背景渐变
    connect(boss, &NightmareBoss::phase1DeathTriggered,
            this, [this]() {
                emit requestFadeBackground(QString("assets/background/nightmare2_map.png"), 3000);
            });

    qDebug() << "[BossFight] NightmareBoss信号已连接";
}

void BossFight::initWashMachineBoss(WashMachineBoss* boss) {
    if (!boss)
        return;

    m_currentWashMachineBoss = boss;
    connectWashMachineBossSignals(boss);
}

void BossFight::initTeacherBoss(TeacherBoss* boss) {
    if (!boss)
        return;

    m_currentTeacherBoss = boss;
    connectTeacherBossSignals(boss);
}

// WashMachineBoss信号连接

void BossFight::connectWashMachineBossSignals(WashMachineBoss* boss) {
    if (!boss)
        return;

    // 连接请求对话信号
    connect(boss, &WashMachineBoss::requestShowDialog,
            this, &BossFight::onWashMachineBossRequestDialog);

    // 连接请求背景切换信号
    connect(boss, &WashMachineBoss::requestChangeBackground,
            this, &BossFight::onWashMachineBossRequestChangeBackground);

    // 连接请求显示文字提示信号
    connect(boss, &WashMachineBoss::requestShowTransitionText,
            this, [this](const QString& text) { showPhaseTransitionText(text); });

    // 连接请求吸纳信号
    connect(boss, &WashMachineBoss::requestAbsorbAllEntities,
            this, &BossFight::onWashMachineBossRequestAbsorb);

    // 连接召唤敌人信号
    connect(boss, &WashMachineBoss::requestSpawnEnemies,
            this, &BossFight::requestSpawnEnemies);

    qDebug() << "[BossFight] WashMachineBoss信号已连接";
}

void BossFight::onWashMachineBossRequestDialog(const QStringList& dialogs, const QString& background) {
    qDebug() << "[BossFight] WashMachineBoss请求显示对话";
    emit requestShowDialog(dialogs, true, background);
}

void BossFight::onWashMachineBossRequestChangeBackground(const QString& backgroundPath) {
    qDebug() << "[BossFight] WashMachineBoss请求更换背景:" << backgroundPath;
    changeBackground(backgroundPath);
}

void BossFight::onWashMachineBossRequestAbsorb() {
    qDebug() << "[BossFight] WashMachineBoss请求执行吸纳动画";
    // 吸纳动画需要敌人列表，由Level层调用performAbsorbAnimation
}

// TeacherBoss信号连接

void BossFight::connectTeacherBossSignals(TeacherBoss* boss) {
    if (!boss)
        return;

    // 连接请求对话信号
    connect(boss, &TeacherBoss::requestShowDialog,
            this, &BossFight::onTeacherBossRequestDialog);

    // 连接请求背景切换信号
    connect(boss, &TeacherBoss::requestChangeBackground,
            this, &BossFight::onTeacherBossRequestChangeBackground);

    // 连接请求显示文字提示信号
    connect(boss, &TeacherBoss::requestShowTransitionText,
            this, &BossFight::onTeacherBossRequestTransitionText);

    // 连接请求渐变背景信号
    connect(boss, &TeacherBoss::requestFadeBackground,
            this, &BossFight::onTeacherBossRequestFadeBackground);

    // 连接请求渐变对话背景信号
    connect(boss, &TeacherBoss::requestFadeDialogBackground,
            this, &BossFight::onTeacherBossRequestFadeDialogBackground);

    // 连接请求对话中切换背景信号
    connect(boss, &TeacherBoss::requestDialogBackgroundChange,
            this, &BossFight::onTeacherBossRequestDialogBackgroundChange);

    // 连接召唤敌人信号
    connect(boss, &TeacherBoss::requestSpawnEnemies,
            this, &BossFight::requestSpawnEnemies);

    // 设置场景引用
    boss->setScene(m_scene);

    qDebug() << "[BossFight] TeacherBoss信号已连接";
}

void BossFight::onTeacherBossRequestDialog(const QStringList& dialogs, const QString& background) {
    qDebug() << "[BossFight] TeacherBoss请求显示对话";
    emit requestShowDialog(dialogs, true, background);
}

void BossFight::onTeacherBossRequestChangeBackground(const QString& backgroundPath) {
    qDebug() << "[BossFight] TeacherBoss请求切换背景:" << backgroundPath;
    changeBackground(backgroundPath);
}

void BossFight::onTeacherBossRequestTransitionText(const QString& text) {
    qDebug() << "[BossFight] TeacherBoss请求显示文字:" << text;
    showPhaseTransitionText(text);
}

void BossFight::onTeacherBossRequestFadeBackground(const QString& backgroundPath, int duration) {
    qDebug() << "[BossFight] TeacherBoss请求渐变背景:" << backgroundPath << "持续" << duration << "ms";
    emit requestFadeBackground(backgroundPath, duration);
}

void BossFight::onTeacherBossRequestFadeDialogBackground(const QString& backgroundPath, int duration) {
    qDebug() << "[BossFight] TeacherBoss请求渐变对话背景:" << backgroundPath << "持续" << duration << "ms";
    m_pendingFadeDialogBackground = backgroundPath;
    m_pendingFadeDialogDuration = duration;
}

void BossFight::onTeacherBossRequestDialogBackgroundChange(int dialogIndex, const QString& backgroundName) {
    qDebug() << "[BossFight] TeacherBoss请求在对话索引" << dialogIndex << "时切换背景到:" << backgroundName;
    m_pendingDialogBackgrounds[dialogIndex] = backgroundName;
    emit dialogBackgroundChangeRequested(dialogIndex, backgroundName);
}

// 背景控制

void BossFight::changeBackground(const QString& backgroundPath) {
    if (!m_scene || !m_backgroundItem)
        return;

    QPixmap bg(backgroundPath);
    if (!bg.isNull()) {
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        m_currentBackgroundPath = backgroundPath;
        qDebug() << "[BossFight] 背景已切换为:" << backgroundPath;
    } else {
        qWarning() << "[BossFight] 无法加载背景图片:" << backgroundPath;
    }
}

void BossFight::fadeBackgroundTo(const QString& imagePath, int duration) {
    emit requestFadeBackground(imagePath, duration);
}

void BossFight::fadeDialogBackgroundTo(const QString& imagePath, int duration) {
    // 存储待执行的渐变信息
    m_pendingFadeDialogBackground = imagePath;
    m_pendingFadeDialogDuration = duration;
}

// 阶段转换

void BossFight::showPhaseTransitionText(const QString& text, const QColor& color) {
    // 此方法在Level中实现，这里只是接口
    Q_UNUSED(text)
    Q_UNUSED(color)
    qDebug() << "[BossFight] 显示阶段转换文字:" << text;
}

void BossFight::pauseAllEnemyTimers(const QVector<QPointer<Enemy>>& enemies) {
    for (const QPointer<Enemy>& enemyPtr : enemies) {
        if (Enemy* enemy = enemyPtr.data()) {
            enemy->pauseTimers();
        }
    }
}

void BossFight::resumeAllEnemyTimers(const QVector<QPointer<Enemy>>& enemies) {
    for (const QPointer<Enemy>& enemyPtr : enemies) {
        if (Enemy* enemy = enemyPtr.data()) {
            enemy->resumeTimers();
        }
    }
}

// 吸纳动画

void BossFight::performAbsorbAnimation(WashMachineBoss* boss, const QVector<QPointer<Enemy>>& enemies) {
    if (!boss || !m_scene || !m_player)
        return;

    m_isAbsorbAnimationActive = true;

    // 计算Boss中心作为吸纳目标点
    m_absorbCenter = boss->pos() + QPointF(boss->pixmap().width() / 2.0, boss->pixmap().height() / 2.0);

    // 清理场景中所有子弹
    QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        if (Projectile* proj = dynamic_cast<Projectile*>(item)) {
            proj->destroy();
        }
    }

    // 收集需要被吸纳的实体
    m_absorbingItems.clear();
    m_absorbStartPositions.clear();
    m_absorbAngles.clear();

    // 添加玩家
    m_absorbingItems.append(m_player);
    m_absorbStartPositions.append(m_player->pos());
    m_absorbAngles.append(0);

    // 禁用玩家移动和攻击，设置无敌
    m_player->setCanMove(false);
    m_player->setCanShoot(false);
    m_player->setPermanentInvincible(true);
    qDebug() << "[BossFight] 玩家已设置为无敌状态";

    // 添加所有敌人（除了Boss）
    for (const QPointer<Enemy>& enemyPtr : enemies) {
        if (Enemy* enemy = enemyPtr.data()) {
            if (enemy != boss) {
                enemy->pauseTimers();
                m_absorbingItems.append(enemy);
                m_absorbStartPositions.append(enemy->pos());
                m_absorbAngles.append(0);
            }
        }
    }

    // 设置初始螺旋角度
    for (int i = 0; i < m_absorbingItems.size(); ++i) {
        QPointF itemPos = m_absorbStartPositions[i];
        double dx = itemPos.x() - m_absorbCenter.x();
        double dy = itemPos.y() - m_absorbCenter.y();
        m_absorbAngles[i] = qAtan2(dy, dx);
    }

    m_absorbAnimationStep = 0;

    // 创建动画定时器
    if (!m_absorbAnimationTimer) {
        m_absorbAnimationTimer = new QTimer(this);
        connect(m_absorbAnimationTimer, &QTimer::timeout, this, &BossFight::onAbsorbAnimationStep);
    }
    m_absorbAnimationTimer->start(16);  // ~60fps

    qDebug() << "[BossFight] 吸纳动画开始，共" << m_absorbingItems.size() << "个实体";
}

void BossFight::onAbsorbAnimationStep() {
    if (!m_isAbsorbAnimationActive) {
        m_absorbAnimationTimer->stop();
        return;
    }

    m_absorbAnimationStep++;

    const int totalSteps = 120;  // 约2秒完成
    double progress = static_cast<double>(m_absorbAnimationStep) / totalSteps;

    if (progress >= 1.0) {
        // 动画完成
        m_absorbAnimationTimer->stop();
        m_isAbsorbAnimationActive = false;

        // 清理子弹
        if (m_scene) {
            QList<QGraphicsItem*> allItems = m_scene->items();
            for (QGraphicsItem* item : allItems) {
                if (Projectile* proj = dynamic_cast<Projectile*>(item)) {
                    proj->destroy();
                }
            }
        }

        // 隐藏被吸纳的实体
        for (int i = 0; i < m_absorbingItems.size(); ++i) {
            QGraphicsItem* item = m_absorbingItems[i];
            if (!item)
                continue;

            item->setVisible(false);

            if (item != m_player) {
                Enemy* enemy = dynamic_cast<Enemy*>(item);
                if (enemy) {
                    enemy->pauseTimers();
                }
            }
        }

        qDebug() << "[BossFight] 吸纳动画完成";
        emit absorbAnimationCompleted();
        return;
    }

    // 缓动函数
    double easeProgress = 1.0 - qPow(1.0 - progress, 3);

    // 更新每个实体的位置（螺旋收缩）
    for (int i = 0; i < m_absorbingItems.size(); ++i) {
        QGraphicsItem* item = m_absorbingItems[i];
        if (!item)
            continue;

        QPointF startPos = m_absorbStartPositions[i];
        double startAngle = m_absorbAngles[i];

        // 计算当前位置
        double currentAngle = startAngle + progress * M_PI * 4;  // 旋转2圈
        double startRadius = QLineF(startPos, m_absorbCenter).length();
        double currentRadius = startRadius * (1.0 - easeProgress);

        QPointF newPos;
        newPos.setX(m_absorbCenter.x() + currentRadius * qCos(currentAngle));
        newPos.setY(m_absorbCenter.y() + currentRadius * qSin(currentAngle));

        item->setPos(newPos);

        // 缩小效果
        double scale = 1.0 - easeProgress * 0.5;
        item->setScale(scale);
    }
}
