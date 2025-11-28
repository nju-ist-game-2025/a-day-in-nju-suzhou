#include "teacherboss.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../ui/explosion.h"
#include "chalkbeam.h"
#include "exampaper.h"
#include "invigilator.h"
#include "mletrap.h"
#include "player.h"
#include "projectile.h"

TeacherBoss::TeacherBoss(const QPixmap& pic, double scale)
    : Boss(pic, scale),
      m_phase(1),
      m_isTransitioning(false),
      m_waitingForDialog(false),
      m_isDefeated(false),
      m_firstDialogShown(false),  // 初始化为未播放，等待Level通知
      m_isFlyingOut(false),
      m_isFlyingIn(false),
      m_flyTimer(nullptr),
      m_scene(nullptr),
      m_normalBarrageTimer(nullptr),
      m_rollCallTimer(nullptr),
      m_examPaperTimer(nullptr),
      m_mleTrapTimer(nullptr),
      m_summonInvigilatorTimer(nullptr),
      m_failWarningTimer(nullptr),
      m_formulaBombTimer(nullptr),
      m_splitBulletTimer(nullptr) {
    // 从配置文件读取奶牛张Boss的一阶段属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getBossInt("teacher", "phase1", "health", 500));
    setContactDamage(config.getBossInt("teacher", "phase1", "contact_damage", 3));
    setVisionRange(config.getBossDouble("teacher", "phase1", "vision_range", 1000));
    setAttackRange(config.getBossDouble("teacher", "phase1", "attack_range", 70));
    setAttackCooldown(config.getBossInt("teacher", "phase1", "attack_cooldown", 1200));
    setSpeed(config.getBossDouble("teacher", "phase1", "speed", 1.5));

    // 第一阶段使用保持距离模式
    setMovementPattern(MOVE_KEEP_DISTANCE);
    setPreferredDistance(config.getBossDouble("teacher", "phase1", "preferred_distance", 200.0));

    // 加载图片资源
    loadTextures();

    // 设置初始图片
    if (!m_normalPixmap.isNull()) {
        setPixmap(m_normalPixmap);
    }

    // 创建后延迟启动第一阶段技能
    QTimer::singleShot(500, this, [this]() {
        if (!m_isDefeated && m_phase == 1) {
            emit requestShowTransitionText("「随堂测验」开始！");
            startPhase1Skills();
        }
    });

    qDebug() << "[TeacherBoss] 奶牛张创建，血量:" << health << "阶段: 1";
}

TeacherBoss::~TeacherBoss() {
    // 停止所有定时器
    if (m_normalBarrageTimer) {
        m_normalBarrageTimer->stop();
        delete m_normalBarrageTimer;
    }
    if (m_rollCallTimer) {
        m_rollCallTimer->stop();
        delete m_rollCallTimer;
    }
    if (m_examPaperTimer) {
        m_examPaperTimer->stop();
        delete m_examPaperTimer;
    }
    if (m_mleTrapTimer) {
        m_mleTrapTimer->stop();
        delete m_mleTrapTimer;
    }
    if (m_summonInvigilatorTimer) {
        m_summonInvigilatorTimer->stop();
        delete m_summonInvigilatorTimer;
    }
    if (m_failWarningTimer) {
        m_failWarningTimer->stop();
        delete m_failWarningTimer;
    }
    if (m_formulaBombTimer) {
        m_formulaBombTimer->stop();
        delete m_formulaBombTimer;
    }
    if (m_splitBulletTimer) {
        m_splitBulletTimer->stop();
        delete m_splitBulletTimer;
    }
    if (m_flyTimer) {
        m_flyTimer->stop();
        delete m_flyTimer;
    }

    // 清理监考员
    cleanupInvigilators();

    qDebug() << "[TeacherBoss] 奶牛张被击败！";
}

void TeacherBoss::loadTextures() {
    int bossSize = ConfigManager::instance().getEntitySize("bosses", "teacher");
    if (bossSize <= 0)
        bossSize = 80;

    QStringList possiblePaths = {
        "assets/boss/Teacher/",
        "../assets/boss/Teacher/",
        "../../our_game/assets/boss/Teacher/"};

    for (const QString& basePath : possiblePaths) {
        if (QFile::exists(basePath + "cow.png")) {
            // 加载各阶段图片
            m_normalPixmap = QPixmap(basePath + "cow.png")
                                 .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_angryPixmap = QPixmap(basePath + "cowAngry.png")
                                .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_finalPixmap = QPixmap(basePath + "cowFinal.png")
                                .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // 加载弹幕图片（放大尺寸）
            m_formulaBulletPixmap = QPixmap(basePath + "formula_bullet.png");
            if (m_formulaBulletPixmap.isNull()) {
                // 创建占位符
                m_formulaBulletPixmap = QPixmap(50, 50);
                m_formulaBulletPixmap.fill(Qt::yellow);
            } else {
                m_formulaBulletPixmap = m_formulaBulletPixmap.scaled(50, 50, Qt::KeepAspectRatio,
                                                                     Qt::SmoothTransformation);
            }

            // 加载粉笔光束图片
            m_chalkBeamPixmap = QPixmap(basePath + "chalk_beam.png");
            if (m_chalkBeamPixmap.isNull()) {
                m_chalkBeamPixmap = QPixmap(30, 60);
                m_chalkBeamPixmap.fill(Qt::white);
            }

            // 加载考卷图片（放大尺寸）
            m_examPaperPixmap = QPixmap(basePath + "exam_paper.png");
            if (m_examPaperPixmap.isNull()) {
                m_examPaperPixmap = QPixmap(60, 80);
                m_examPaperPixmap.fill(Qt::lightGray);
            } else {
                m_examPaperPixmap = m_examPaperPixmap.scaled(60, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            // 加载分裂弹图片
            m_finalBulletPixmap = QPixmap(basePath + "final_bullet.png");
            if (m_finalBulletPixmap.isNull()) {
                m_finalBulletPixmap = QPixmap(25, 25);
                m_finalBulletPixmap.fill(Qt::red);
            }

            qDebug() << "[TeacherBoss] 图片资源加载完成";
            break;
        }
    }

    if (m_normalPixmap.isNull()) {
        qWarning() << "[TeacherBoss] 无法加载图片，使用占位符";
        m_normalPixmap = QPixmap(bossSize, bossSize);
        m_normalPixmap.fill(Qt::darkCyan);
        m_angryPixmap = QPixmap(bossSize, bossSize);
        m_angryPixmap.fill(Qt::darkYellow);
        m_finalPixmap = QPixmap(bossSize, bossSize);
        m_finalPixmap.fill(Qt::darkRed);
    }
}

void TeacherBoss::takeDamage(int damage) {
    // 转换期间无敌
    if (m_isTransitioning || m_waitingForDialog || m_isFlyingOut || m_isFlyingIn) {
        return;
    }

    // 触发闪烁效果
    flash();

    // 计算实际伤害（确保至少为1，防止低伤害被缩放为0）
    int realDamage = static_cast<int>(damage * damageScale);
    if (damage > 0 && realDamage < 1) {
        realDamage = 1;
    }

    // 计算阶段临界血量
    int phase2Threshold = static_cast<int>(maxHealth * 0.6);  // 60% - 进入期中考试阶段
    int phase3Threshold = static_cast<int>(maxHealth * 0.3);  // 30% - 进入调离阶段

    // 根据当前阶段限制扣血
    if (m_phase == 1) {
        if (health - realDamage < phase2Threshold) {
            health = phase2Threshold;
        } else {
            health -= realDamage;
        }
    } else if (m_phase == 2) {
        if (health - realDamage < phase3Threshold) {
            health = phase3Threshold;
        } else {
            health -= realDamage;
        }
    } else {
        health -= realDamage;
        if (health < 0) {
            health = 0;
        }
    }

    qDebug() << "[TeacherBoss] 受到伤害:" << realDamage << "，剩余血量:" << health
             << "(" << (health * 100 / maxHealth) << "%)";

    checkPhaseTransition();
}

void TeacherBoss::checkPhaseTransition() {
    double healthPercent = static_cast<double>(health) / maxHealth;

    if (health <= 0) {
        onBossDefeated();
        return;
    }

    if (m_phase == 1 && healthPercent <= 0.6) {
        m_isTransitioning = true;
        qDebug() << "[TeacherBoss] 血量降至60%，准备进入期中考试阶段...";
        QTimer::singleShot(200, this, &TeacherBoss::enterPhase2);
    } else if (m_phase == 2 && healthPercent <= 0.3) {
        m_isTransitioning = true;
        qDebug() << "[TeacherBoss] 血量降至30%，准备进入调离阶段...";
        QTimer::singleShot(200, this, &TeacherBoss::enterPhase3);
    }
}

void TeacherBoss::enterPhase2() {
    m_phase = 2;
    qDebug() << "[TeacherBoss] 进入期中考试阶段！";

    // 停止第一阶段技能
    stopPhase1Skills();

    // 切换到愤怒图片
    if (!m_angryPixmap.isNull()) {
        setPixmap(m_angryPixmap);
        flash();
    }

    // 从配置文件读取二阶段属性
    ConfigManager& config = ConfigManager::instance();
    setMovementPattern(MOVE_DASH);
    setVisionRange(config.getBossDouble("teacher", "phase2", "vision_range", 10000));
    setDashChargeTime(config.getBossInt("teacher", "phase2", "dash_charge_time", 800));
    setDashSpeed(config.getBossDouble("teacher", "phase2", "dash_speed", 6.0));
    setContactDamage(config.getBossInt("teacher", "phase2", "contact_damage", 4));

    // 暂停并显示对话，开始背景是teacher1_dia
    m_waitingForDialog = true;
    // 在第5句"友好的测验"时切换到teacher2_dia（索引4）
    emit requestDialogBackgroundChange(4, "teacher2_dia");
    emit requestShowDialog(getPhase2Dialog(), "assets/background/teacher1_dia.png");
}

void TeacherBoss::enterPhase3() {
    m_phase = 3;
    qDebug() << "[TeacherBoss] 进入调离阶段！";

    // 停止第二阶段技能
    stopPhase2Skills();

    // 清理监考员
    cleanupInvigilators();

    // 开始飞出动画
    startFlyOutAnimation();
}

void TeacherBoss::startFlyOutAnimation() {
    m_isFlyingOut = true;
    m_waitingForDialog = true;

    // 停止移动和AI
    pauseTimers();

    // 记录起始位置，目标是飞出屏幕右侧
    m_flyStartPos = pos();
    m_flyTargetPos = QPointF(900, pos().y());  // 飞出右侧

    if (!m_flyTimer) {
        m_flyTimer = new QTimer(this);
        connect(m_flyTimer, &QTimer::timeout, this, &TeacherBoss::onFlyAnimationStep);
    }
    m_flyTimer->start(16);  // 约60fps

    qDebug() << "[TeacherBoss] 开始飞出动画";
}

void TeacherBoss::onFlyAnimationStep() {
    QPointF currentPos = pos();

    if (m_isFlyingOut) {
        // 飞出动画
        double newX = currentPos.x() + 8;  // 飞行速度
        setPos(newX, currentPos.y());

        if (newX >= m_flyTargetPos.x()) {
            m_flyTimer->stop();
            m_isFlyingOut = false;
            onFlyOutComplete();
        }
    } else if (m_isFlyingIn) {
        // 飞入动画
        double newX = currentPos.x() + 6;
        setPos(newX, currentPos.y());

        if (newX >= m_flyTargetPos.x()) {
            m_flyTimer->stop();
            m_isFlyingIn = false;
            setPos(m_flyTargetPos);

            // 恢复战斗
            m_isTransitioning = false;
            resumeTimers();
            startPhase3Skills();

            emit phaseChanged(3);
            qDebug() << "[TeacherBoss] 飞入完成，调离阶段开始！";
        }
    }
}

void TeacherBoss::onFlyOutComplete() {
    qDebug() << "[TeacherBoss] 飞出完成，显示对话";

    // 切换到最终图片
    if (!m_finalPixmap.isNull()) {
        setPixmap(m_finalPixmap);
    }

    // 三阶段对话开始背景是teacher2_dia，在"真正的概率论"那句时切换到teacher3_dia（索引6）
    emit requestDialogBackgroundChange(6, "teacher3_dia");
    emit requestShowDialog(getPhase3Dialog(), "assets/background/teacher2_dia.png");
}

void TeacherBoss::startFlyInAnimation() {
    m_isFlyingIn = true;

    // 设置起始位置（屏幕左侧外）和目标位置（画面中央偏左）
    m_flyStartPos = QPointF(-100, 300);
    m_flyTargetPos = QPointF(350, 300);
    setPos(m_flyStartPos);

    if (!m_flyTimer) {
        m_flyTimer = new QTimer(this);
        connect(m_flyTimer, &QTimer::timeout, this, &TeacherBoss::onFlyAnimationStep);
    }
    m_flyTimer->start(16);

    qDebug() << "[TeacherBoss] 开始飞入动画";
}

void TeacherBoss::onDialogFinished() {
    m_waitingForDialog = false;

    // 如果是击败对话结束，则触发背景渐变并死亡
    if (m_isDefeated) {
        qDebug() << "[TeacherBoss] 击败对话结束，开始地图背景渐变";
        // 地图背景从teacher3_map渐变到classroom
        emit requestFadeBackground("assets/background/classRoom.png", 3000);
        emit dying(this);
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    // 如果是初始对话结束（阶段1开始战斗）
    if (m_phase == 1 && !m_firstDialogShown) {
        m_firstDialogShown = true;
        m_isTransitioning = false;
        qDebug() << "[TeacherBoss] 初始对话结束，开始授课阶段";
        // 切换到一阶段战斗背景
        emit requestChangeBackground("assets/background/teacher1_map.png");
        emit requestShowTransitionText("「随堂测验」开始！");
        startPhase1Skills();
        resumeTimers();
        return;
    }

    // 期中考试阶段对话结束
    if (m_phase == 2) {
        qDebug() << "[TeacherBoss] 期中考试对话结束，继续战斗";
        // 切换到二阶段战斗背景
        emit requestChangeBackground("assets/background/teacher2_map.png");
        emit requestShowTransitionText("「期中考试」开始！");
        m_isTransitioning = false;
        startPhase2Skills();
        resumeTimers();
        return;
    }

    // 调离阶段对话结束，开始飞入动画
    if (m_phase == 3 && !m_isFlyingIn) {
        qDebug() << "[TeacherBoss] 调离阶段对话结束，开始飞入";
        // 切换到三阶段战斗背景
        emit requestChangeBackground("assets/background/teacher3_map.png");
        emit requestShowTransitionText("「方差爆炸」！");
        startFlyInAnimation();
        return;
    }
}

void TeacherBoss::onBossDefeated() {
    qDebug() << "[TeacherBoss] 奶牛张被击败！显示胜利对话";

    m_isTransitioning = true;
    m_isDefeated = true;
    m_waitingForDialog = true;

    // 停止所有攻击
    stopPhase1Skills();
    stopPhase2Skills();
    stopPhase3Skills();

    if (aiTimer)
        aiTimer->stop();
    if (moveTimer)
        moveTimer->stop();
    if (attackTimer)
        attackTimer->stop();

    // 清理监考员
    cleanupInvigilators();

    // 清理场上的弹幕
    if (m_scene) {
        QList<QGraphicsItem*> allItems = m_scene->items();
        for (QGraphicsItem* item : allItems) {
            if (Projectile* proj = dynamic_cast<Projectile*>(item)) {
                m_scene->removeItem(proj);
                proj->deleteLater();
            }
        }
    }

    AudioManager::instance().playSound("enemy_death");

    // 击败对话：使用teacher3_dia背景，从对话开始渐变到teacher1_dia（使用渐变动画）
    emit requestFadeDialogBackground("assets/background/teacher1_dia.png", 2000);
    emit requestShowDialog(getDefeatedDialog(), "assets/background/teacher3_dia.png");
}

void TeacherBoss::move() {
    if (m_isTransitioning || m_waitingForDialog || m_isFlyingOut || m_isFlyingIn) {
        return;
    }
    Enemy::move();
}

void TeacherBoss::attackPlayer() {
    if (m_isTransitioning || m_waitingForDialog || m_isFlyingOut || m_isFlyingIn) {
        return;
    }
    Boss::attackPlayer();
}

// ==================== 第一阶段技能 ====================

void TeacherBoss::startPhase1Skills() {
    qDebug() << "[TeacherBoss] 启动第一阶段技能";

    // 正态分布弹幕 - 每4秒
    if (!m_normalBarrageTimer) {
        m_normalBarrageTimer = new QTimer(this);
        connect(m_normalBarrageTimer, &QTimer::timeout, this, &TeacherBoss::fireNormalDistributionBarrage);
    }
    m_normalBarrageTimer->start(4000);

    // 随机点名 - 每8秒
    if (!m_rollCallTimer) {
        m_rollCallTimer = new QTimer(this);
        connect(m_rollCallTimer, &QTimer::timeout, this, &TeacherBoss::performRollCall);
    }
    m_rollCallTimer->start(8000);
}

void TeacherBoss::stopPhase1Skills() {
    if (m_normalBarrageTimer)
        m_normalBarrageTimer->stop();
    if (m_rollCallTimer)
        m_rollCallTimer->stop();
}

double TeacherBoss::randomNormal(double mean, double stddev) {
    // Box-Muller变换生成正态分布随机数
    static bool hasSpare = false;
    static double spare;

    if (hasSpare) {
        hasSpare = false;
        return mean + stddev * spare;
    }

    hasSpare = true;
    double u, v, s;
    do {
        u = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
        v = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);

    s = qSqrt(-2.0 * qLn(s) / s);
    spare = v * s;
    return mean + stddev * u * s;
}

void TeacherBoss::fireNormalDistributionBarrage() {
    if (!m_scene || !player || m_isPaused)
        return;

    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2, pixmap().height() / 2);
    QPointF playerCenter = player->pos() + QPointF(player->boundingRect().width() / 2,
                                                   player->boundingRect().height() / 2);

    // 计算朝向玩家的基础角度
    double dx = playerCenter.x() - bossCenter.x();
    double dy = playerCenter.y() - bossCenter.y();
    double baseAngle = qAtan2(dy, dx);

    // 发射15发弹幕，角度服从正态分布 N(baseAngle, 15°)
    int bulletCount = ConfigManager::instance().getBossInt("teacher", "phase1", "normal_barrage_count", 15);
    double stddevDegrees = ConfigManager::instance().getBossDouble("teacher", "phase1", "normal_barrage_stddev", 15.0);
    double stddevRadians = qDegreesToRadians(stddevDegrees);

    int bulletDamage = ConfigManager::instance().getBossInt("teacher", "phase1", "normal_barrage_damage", 1);
    double bulletSpeed = ConfigManager::instance().getBossDouble("teacher", "phase1", "normal_barrage_speed", 0.8);

    for (int i = 0; i < bulletCount; ++i) {
        double angle = randomNormal(baseAngle, stddevRadians);

        // 创建弹幕 - 使用formula_bullet.png图片
        Projectile* bullet = new Projectile(
            1,  // 敌人发出
            bulletDamage,
            bossCenter,
            m_formulaBulletPixmap,
            1.0);

        // 设置方向 - 降低速度（参考毒气速度4.0）
        double vx = qCos(angle) * bulletSpeed;
        double vy = qSin(angle) * bulletSpeed;
        bullet->setDir(static_cast<int>(vx * 10), static_cast<int>(vy * 10));

        m_scene->addItem(bullet);
    }

    AudioManager::instance().playSound("enemy_attack");
    qDebug() << "[TeacherBoss] 发射正态分布弹幕，共" << bulletCount << "发";
}

void TeacherBoss::performRollCall() {
    if (!m_scene || !player || m_isPaused)
        return;

    // 同时生成3个红圈，增加难度
    QPointF playerPos = player->pos();
    int beamCount = 3;

    for (int i = 0; i < beamCount; ++i) {
        // 在玩家附近随机位置生成
        QPointF offset(
            QRandomGenerator::global()->bounded(-80, 80),
            QRandomGenerator::global()->bounded(-80, 80));
        QPointF beamPos = playerPos + offset;

        // 限制在场景内
        beamPos.setX(qBound(50.0, beamPos.x(), 750.0));
        beamPos.setY(qBound(50.0, beamPos.y(), 550.0));

        ChalkBeam* beam = new ChalkBeam(beamPos, m_chalkBeamPixmap, m_scene);
        m_scene->addItem(beam);
        beam->startWarning();
    }

    qDebug() << "[TeacherBoss] 随机点名！生成" << beamCount << "个红圈";
}

// ==================== 第二阶段技能 ====================

void TeacherBoss::startPhase2Skills() {
    qDebug() << "[TeacherBoss] 启动第二阶段技能";

    // 考卷攻击 - 每5秒
    if (!m_examPaperTimer) {
        m_examPaperTimer = new QTimer(this);
        connect(m_examPaperTimer, &QTimer::timeout, this, &TeacherBoss::throwExamPaper);
    }
    m_examPaperTimer->start(5000);

    // 极大似然估计陷阱 - 每10秒
    if (!m_mleTrapTimer) {
        m_mleTrapTimer = new QTimer(this);
        connect(m_mleTrapTimer, &QTimer::timeout, this, &TeacherBoss::placeMleTrap);
    }
    m_mleTrapTimer->start(10000);

    // 召唤监考员 - 每15秒
    if (!m_summonInvigilatorTimer) {
        m_summonInvigilatorTimer = new QTimer(this);
        connect(m_summonInvigilatorTimer, &QTimer::timeout, this, &TeacherBoss::summonInvigilator);
    }
    m_summonInvigilatorTimer->start(15000);
}

void TeacherBoss::stopPhase2Skills() {
    if (m_examPaperTimer)
        m_examPaperTimer->stop();
    if (m_mleTrapTimer)
        m_mleTrapTimer->stop();
    if (m_summonInvigilatorTimer)
        m_summonInvigilatorTimer->stop();
}

void TeacherBoss::throwExamPaper() {
    if (!m_scene || !player || m_isPaused)
        return;

    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2, pixmap().height() / 2);
    QPointF playerCenter = player->pos() + QPointF(player->boundingRect().width() / 2,
                                                   player->boundingRect().height() / 2);

    // 计算方向
    QPointF direction = playerCenter - bossCenter;
    double length = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (length > 0) {
        direction /= length;
    }

    // 创建考卷
    ExamPaper* paper = new ExamPaper(bossCenter, direction, m_examPaperPixmap, player);
    m_scene->addItem(paper);

    AudioManager::instance().playSound("enemy_attack");
    qDebug() << "[TeacherBoss] 抛出期中考卷！";
}

void TeacherBoss::placeMleTrap() {
    if (!m_scene || !player || m_isPaused)
        return;

    // 预判玩家移动方向，在前方放置陷阱
    QPointF playerPos = player->pos();
    QPointF playerVelocity = QPointF(0, 0);

    // 简单预判：根据玩家当前移动方向预测
    if (player->isKeyPressed(Qt::Key_W))
        playerVelocity.ry() -= 1;
    if (player->isKeyPressed(Qt::Key_S))
        playerVelocity.ry() += 1;
    if (player->isKeyPressed(Qt::Key_A))
        playerVelocity.rx() -= 1;
    if (player->isKeyPressed(Qt::Key_D))
        playerVelocity.rx() += 1;

    // 预测位置（2秒后）
    QPointF predictedPos = playerPos + playerVelocity * 100;

    // 限制在场景内
    predictedPos.setX(qBound(50.0, predictedPos.x(), 750.0));
    predictedPos.setY(qBound(50.0, predictedPos.y(), 550.0));

    // 创建陷阱
    MleTrap* trap = new MleTrap(predictedPos, player);
    m_scene->addItem(trap);

    qDebug() << "[TeacherBoss] 放置极大似然估计陷阱，位置:" << predictedPos;
}

void TeacherBoss::summonInvigilator() {
    if (!m_scene || !player || m_isPaused)
        return;

    // 尝试多个可能的路径加载监考员图片
    QStringList possiblePaths = {
        "assets/boss/Teacher/",
        "../assets/boss/Teacher/",
        "../../our_game/assets/boss/Teacher/"};

    QPixmap normalPix;
    QPixmap angryPix;

    for (const QString& basePath : possiblePaths) {
        if (QFile::exists(basePath + "invigilatorNormal.png")) {
            normalPix = QPixmap(basePath + "invigilatorNormal.png");
            angryPix = QPixmap(basePath + "invigilatorAngry.png");
            break;
        }
    }

    if (normalPix.isNull()) {
        normalPix = QPixmap(70, 70);
        normalPix.fill(Qt::gray);
    } else {
        normalPix = normalPix.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (angryPix.isNull()) {
        angryPix = QPixmap(70, 70);
        angryPix.fill(Qt::red);
    } else {
        angryPix = angryPix.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // 在Boss附近生成监考员
    QPointF spawnPos = pos() + QPointF(
                                   QRandomGenerator::global()->bounded(-50, 50),
                                   QRandomGenerator::global()->bounded(-50, 50));

    Invigilator* invigilator = new Invigilator(normalPix, angryPix, this, 1.0);
    invigilator->setPos(spawnPos);
    invigilator->setPlayer(player);
    m_scene->addItem(invigilator);

    m_invigilators.append(invigilator);

    qDebug() << "[TeacherBoss] 召唤监考员！当前监考员数量:" << m_invigilators.size();
}

void TeacherBoss::cleanupInvigilators() {
    for (QPointer<Invigilator>& inv : m_invigilators) {
        if (inv) {
            if (inv->scene()) {
                inv->scene()->removeItem(inv);
            }
            inv->deleteLater();
        }
    }
    m_invigilators.clear();
}

// ==================== 第三阶段技能 ====================

void TeacherBoss::startPhase3Skills() {
    qDebug() << "[TeacherBoss] 启动第三阶段技能";

    // 切换到高速保持距离模式
    setMovementPattern(MOVE_KEEP_DISTANCE);
    setPreferredDistance(180.0);
    setSpeed(3.0);
    setContactDamage(5);

    // 挂科警告 - 每6秒
    if (!m_failWarningTimer) {
        m_failWarningTimer = new QTimer(this);
        connect(m_failWarningTimer, &QTimer::timeout, this, &TeacherBoss::performFailWarning);
    }
    m_failWarningTimer->start(6000);

    // 公式轰炸 - 每4秒
    if (!m_formulaBombTimer) {
        m_formulaBombTimer = new QTimer(this);
        connect(m_formulaBombTimer, &QTimer::timeout, this, &TeacherBoss::fireFormulaBomb);
    }
    m_formulaBombTimer->start(4000);

    // 喜忧参半分裂弹 - 每8秒
    if (!m_splitBulletTimer) {
        m_splitBulletTimer = new QTimer(this);
        connect(m_splitBulletTimer, &QTimer::timeout, this, &TeacherBoss::fireSplitBullet);
    }
    m_splitBulletTimer->start(8000);
}

void TeacherBoss::stopPhase3Skills() {
    if (m_failWarningTimer)
        m_failWarningTimer->stop();
    if (m_formulaBombTimer)
        m_formulaBombTimer->stop();
    if (m_splitBulletTimer)
        m_splitBulletTimer->stop();
}

void TeacherBoss::performFailWarning() {
    if (!m_scene || !player || m_isPaused)
        return;

    // 复用随机点名机制，但缩短警告时间
    QPointF playerPos = player->pos();

    ChalkBeam* beam = new ChalkBeam(playerPos, m_chalkBeamPixmap, m_scene);
    beam->setWarningTime(1000);  // 1秒警告时间
    m_scene->addItem(beam);
    beam->startWarning();

    qDebug() << "[TeacherBoss] 挂科警告！";
}

void TeacherBoss::fireFormulaBomb() {
    if (!m_scene || !player || m_isPaused)
        return;

    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2, pixmap().height() / 2);

    // 12发弹幕均匀分布360度，随机起始角度
    int bulletCount = ConfigManager::instance().getBossInt("teacher", "phase3", "formula_bomb_count", 12);
    double startAngle = QRandomGenerator::global()->generateDouble() * 2 * M_PI;

    int bulletDamage = ConfigManager::instance().getBossInt("teacher", "phase3", "formula_bomb_damage", 1);
    double bulletSpeed = ConfigManager::instance().getBossDouble("teacher", "phase3", "formula_bomb_speed", 0.4);

    for (int i = 0; i < bulletCount; ++i) {
        double angle = startAngle + (2 * M_PI * i / bulletCount);

        Projectile* bullet = new Projectile(
            1,
            bulletDamage,
            bossCenter,
            m_formulaBulletPixmap,
            1.0);

        // 降低速度（原来4*10=40，现在2.5*10=25）
        double vx = qCos(angle) * bulletSpeed;
        double vy = qSin(angle) * bulletSpeed;
        bullet->setDir(static_cast<int>(vx * 10), static_cast<int>(vy * 10));

        m_scene->addItem(bullet);
    }

    AudioManager::instance().playSound("enemy_attack");
    qDebug() << "[TeacherBoss] 公式轰炸！发射" << bulletCount << "发环形弹幕";
}

void TeacherBoss::fireSplitBullet() {
    if (!m_scene || !player || m_isPaused)
        return;

    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2, pixmap().height() / 2);
    QPointF playerCenter = player->pos() + QPointF(player->boundingRect().width() / 2,
                                                   player->boundingRect().height() / 2);

    // 计算朝向玩家的方向和距离
    QPointF direction = playerCenter - bossCenter;
    double totalDistance = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (totalDistance > 0) {
        direction /= totalDistance;
    }

    // 分裂距离：发射时Boss与玩家距离的三分之二
    double splitDistance = totalDistance * 2.0 / 3.0;

    // 从配置读取子弹参数
    int mainDamage = ConfigManager::instance().getBossInt("teacher", "phase3", "split_bullet_main_damage", 2);
    double mainSpeed = ConfigManager::instance().getBossDouble("teacher", "phase3", "split_bullet_main_speed", 0.3);
    int smallDamage = ConfigManager::instance().getBossInt("teacher", "phase3", "split_bullet_small_damage", 1);
    double smallSpeed = ConfigManager::instance().getBossDouble("teacher", "phase3", "split_bullet_small_speed", 0.6);
    int splitCount = ConfigManager::instance().getBossInt("teacher", "phase3", "split_bullet_count", 5);
    double spreadAngleDeg = ConfigManager::instance().getBossDouble("teacher", "phase3", "split_bullet_spread_angle", 30.0);

    // 创建大弹幕（使用缩放的final_bullet.png）
    QPixmap bigBullet = m_finalBulletPixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    Projectile* mainBullet = new Projectile(
        1,
        mainDamage,
        bossCenter,
        bigBullet,
        1.0);

    // 设置速度（参考毒气速度4.0，setDir需乘10）
    double vx = direction.x() * mainSpeed;
    double vy = direction.y() * mainSpeed;
    mainBullet->setDir(static_cast<int>(vx * 10), static_cast<int>(vy * 10));

    m_scene->addItem(mainBullet);

    // 使用定时器检查距离，到达分裂距离时分裂
    QPointer<Projectile> bulletPtr = mainBullet;
    QPointer<QGraphicsScene> scenePtr = m_scene;
    QPointF startPos = bossCenter;
    QPointF dir = direction;
    QPixmap smallPix = m_finalBulletPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    double splitDist = splitDistance;

    // 创建检查定时器
    QTimer* checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout, this,
            [bulletPtr, scenePtr, startPos, dir, smallPix, splitDist, checkTimer, smallDamage, smallSpeed, splitCount, spreadAngleDeg]() {
                if (!bulletPtr || !scenePtr) {
                    checkTimer->stop();
                    checkTimer->deleteLater();
                    return;
                }

                // 计算当前飞行距离
                QPointF currentPos = bulletPtr->pos();
                double dx = currentPos.x() - startPos.x();
                double dy = currentPos.y() - startPos.y();
                double currentDistance = qSqrt(dx * dx + dy * dy);

                // 到达分裂距离
                if (currentDistance >= splitDist) {
                    checkTimer->stop();
                    checkTimer->deleteLater();

                    QPointF splitPos = currentPos;

                    // 删除主弹
                    scenePtr->removeItem(bulletPtr);
                    bulletPtr->deleteLater();

                    // 生成小弹幕，扇形散开
                    double baseAngle = qAtan2(dir.y(), dir.x());
                    double spreadAngle = qDegreesToRadians(spreadAngleDeg);

                    for (int i = 0; i < splitCount; ++i) {
                        double angle = baseAngle - spreadAngle + (spreadAngle * 2 * i / (splitCount - 1));

                        Projectile* smallBullet = new Projectile(
                            1,
                            smallDamage,
                            splitPos,
                            smallPix,
                            1.0);

                        // 分裂后速度
                        double svx = qCos(angle) * smallSpeed;
                        double svy = qSin(angle) * smallSpeed;
                        smallBullet->setDir(static_cast<int>(svx * 10), static_cast<int>(svy * 10));

                        scenePtr->addItem(smallBullet);
                    }
                    qDebug() << "[TeacherBoss] 分裂弹已分裂，距离:" << currentDistance;
                }
            });
    checkTimer->start(50);  // 每50毫秒检查一次

    AudioManager::instance().playSound("enemy_attack");
    qDebug() << "[TeacherBoss] 喜忧参半！发射分裂弹，分裂距离:" << splitDistance;
}

// ==================== 暂停控制 ====================

void TeacherBoss::pauseTimers() {
    Enemy::pauseTimers();

    if (m_normalBarrageTimer)
        m_normalBarrageTimer->stop();
    if (m_rollCallTimer)
        m_rollCallTimer->stop();
    if (m_examPaperTimer)
        m_examPaperTimer->stop();
    if (m_mleTrapTimer)
        m_mleTrapTimer->stop();
    if (m_summonInvigilatorTimer)
        m_summonInvigilatorTimer->stop();
    if (m_failWarningTimer)
        m_failWarningTimer->stop();
    if (m_formulaBombTimer)
        m_formulaBombTimer->stop();
    if (m_splitBulletTimer)
        m_splitBulletTimer->stop();
}

void TeacherBoss::resumeTimers() {
    Enemy::resumeTimers();

    // 根据阶段恢复对应的技能定时器
    if (m_phase == 1) {
        if (m_normalBarrageTimer && m_firstDialogShown)
            m_normalBarrageTimer->start();
        if (m_rollCallTimer && m_firstDialogShown)
            m_rollCallTimer->start();
    } else if (m_phase == 2) {
        if (m_examPaperTimer)
            m_examPaperTimer->start();
        if (m_mleTrapTimer)
            m_mleTrapTimer->start();
        if (m_summonInvigilatorTimer)
            m_summonInvigilatorTimer->start();
    } else if (m_phase == 3) {
        if (m_failWarningTimer)
            m_failWarningTimer->start();
        if (m_formulaBombTimer)
            m_formulaBombTimer->start();
        if (m_splitBulletTimer)
            m_splitBulletTimer->start();
    }
}

// ==================== 对话内容 ====================

QStringList TeacherBoss::getInitialDialog() {
    return {
        "【奶牛张】\n『哦？又来了一位勇敢的同学？』",
        "【奶牛张】\n『让我看看...根据极大似然估计，你通过这场考试的概率是...』",
        "【奶牛张】\n『嗯，不太乐观啊！』",
        "「智科er」 \n 老师，我只是想通过这一关...",
        "【奶牛张】\n『通过？哈哈，这让我想起了那次期中考试...』",
        "【奶牛张】\n『全班的成绩呈完美的正态分布，均值μ=62，方差σ²=144...』",
        "【奶牛张】\n『多么美丽的数据啊！』",
        "「智科er」 \n （那不就是大家都差点挂科吗...）",
        "【奶牛张】\n『好了，让我们开始今天的「随堂测验」吧！』"};
}

QStringList TeacherBoss::getPhase2Dialog() {
    return {
        "【奶牛张】\n『看来你还挺能扛的...』",
        "【奶牛张】\n『那么，是时候进行【期中考试】了！』",
        "「智科er」 \n 什么？！",
        "【奶牛张】\n『别紧张，这只是一场...』",
        "【奶牛张】\n『「友好」的测验』"};
}

QStringList TeacherBoss::getPhase3Dialog() {
    return {
        "【奶牛张】\n『哈...哈哈...』",
        "【奶牛张】\n『你知道吗，我要被调到北京去了。』",
        "「智科er」 \n 恭喜老师高升？",
        "【奶牛张】\n『高升？哈哈，喜忧参半吧！』",
        "【奶牛张】\n『喜的是，终于可以离开这里...』",
        "【奶牛张】\n『忧的是，我还没来得及...』",
        "【奶牛张】\n『让你们见识真正的概率论！』",
        "「智科er」 \n （不妙...）",
        "【奶牛张】\n『在我离开之前，让你体验一下什么叫做...』",
        "【奶牛张】\n『【方差爆炸】！』"};
}

QStringList TeacherBoss::getDefeatedDialog() {
    return {
        "【奶牛张】\n『咳咳...没想到，你真的通过了...』",
        "【奶牛张】\n『看来我的极大似然估计...出了点偏差。』",
        "「智科er」 \n 老师，您...",
        "【奶牛张】\n『没关系，这是一个很好的样本数据。』",
        "【奶牛张】\n『我会把它记录下来，用于改进我的模型。』",
        "【奶牛张】\n『去吧，勇敢的同学...』",
        "【奶牛张】\n『记住，生活就像概率论...』",
        "【奶牛张】\n『充满不确定性，但总有规律可循。』",
        "「智科er」 \n 老师，北京那边...",
        "【奶牛张】\n『哈哈，喜忧参半吧！再见了。』",
        "（奶牛张化作一道光消散）"};
}
