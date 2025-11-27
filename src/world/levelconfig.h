#ifndef LEVELCONFIG_H
#define LEVELCONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * @brief 敌人配置结构
 */
struct EnemySpawnConfig {
    QString type; // 敌人类型（如 "clock_normal", "clock_boom", "sock_normal"）
    int count;    // 生成数量

    EnemySpawnConfig() : count(0) {}

    EnemySpawnConfig(const QString &t, int c) : type(t), count(c) {}
};

/**
 * @brief Boss奖励道具配置结构
 */
struct BossRewardItem {
    QString type; // 道具类型：damage_up, speed_up, shoot_speed_up, bullet_speed_up, red_heart, bomb, key
    double value; // 道具数值（倒率或数量）

    BossRewardItem() : value(1.0) {}

    BossRewardItem(const QString &t, double v) : type(t), value(v) {}
};

/**
 * @brief 房间配置结构
 */
struct RoomConfig {
    QString backgroundImage;                 // 房间背景图片路径（相对 assets/）
    int enemyCount;                          // 房间内敌人数量（保留用于兼容，实际由enemies决定）
    QVector<EnemySpawnConfig> enemies;       // 敌人生成配置列表
    bool hasChest;                           // 是否有宝箱
    bool isChestLocked;                      // 宝箱是否锁定
    bool hasBoss;                            // 是否有 Boss
    QStringList bossDialog;                  // Boss房间的对话文案
    QString bossDialogBackground;            // Boss对话背景图片路径（可选，为空则使用默认图片）
    QVector<BossRewardItem> bossRewardItems; // Boss奖励道具配置

    // 精英房间配置
    bool isEliteRoom;                    // 是否是精英房间
    QStringList eliteDialog;             // 精英房间开场对话
    QString eliteDialogBackground;       // 精英房间开场对话背景
    QStringList elitePhase2Dialog;       // 精英房间第二阶段对话（第一只杨林死亡后）
    QString elitePhase2DialogBackground; // 精英房间第二阶段对话背景

    // 门的连接信息：-1 表示无门，>= 0 表示连接到的房间索引
    int doorUp;    // 上门连接到的房间索引
    int doorDown;  // 下门连接到的房间索引
    int doorLeft;  // 左门连接到的房间索引
    int doorRight; // 右门连接到的房间索引

    RoomConfig()
            : enemyCount(0),
              hasChest(false),
              isChestLocked(false),
              hasBoss(false),
              isEliteRoom(false),
              doorUp(-1),
              doorDown(-1),
              doorLeft(-1),
              doorRight(-1) {}
};

/**
 * @brief 关卡配置类
 * 用于从 JSON 文件加载关卡的房间布局和配置
 */
class LevelConfig {
public:
    LevelConfig();

    /**
     * @brief 从 JSON 文件加载关卡配置
     * @param levelNumber 关卡号
     * @return 是否加载成功
     */
    bool loadFromFile(int levelNumber);

    /**
     * @brief 从 JSON 对象加载配置
     */
    bool loadFromJson(const QJsonObject &levelObj);

    /**
     * @brief 获取房间数量
     */
    int getRoomCount() const { return m_rooms.size(); }

    /**
     * @brief 获取指定房间的配置
     */
    const RoomConfig &getRoom(int index) const;

    /**
     * @brief 获取起始房间索引
     */
    int getStartRoomIndex() const { return m_startRoomIndex; }

    /**
     * @brief 获取关卡名称
     */
    QString getLevelName() const { return m_levelName; }

    static QStringList readDescriptionsFromJson(const QString &filePath);

    QStringList &getDescription() { return m_description; };

private:
    QString m_levelName;         // 关卡名称
    int m_startRoomIndex;        // 起始房间索引
    QVector<RoomConfig> m_rooms; // 房间配置列表
    QStringList m_description;

    /**
     * @brief 解析单个房间配置
     */
    RoomConfig parseRoomConfig(const QJsonObject &roomObj);
};

#endif // LEVELCONFIG_H
