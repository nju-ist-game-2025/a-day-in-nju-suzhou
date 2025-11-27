#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QMap>
#include <QMediaPlayer>
#include <QObject>
#include <QSoundEffect>
#include <QVector>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <QAudioOutput>

#else
#include <QMediaPlaylist>
#endif

// 音效池 - 每种音效预创建多个实例，支持同时播放
struct SoundPool {
    QVector<QSoundEffect *> effects;
    int nextIndex = 0;
    QString filePath;
};

class AudioManager : public QObject {
Q_OBJECT

public:
    static AudioManager &instance();

    // 音效控制
    void playSound(const QString &soundName);

    void preloadSound(const QString &soundName, const QString &filePath);

    // 背景音乐控制
    void playMusic(const QString &musicFile);

    void stopMusic();

    void setMusicVolume(int volume);

    void setSoundVolume(int volume);

    // 状态检查
    [[nodiscard]] bool isMusicPlaying() const;

private:
    explicit AudioManager(QObject *parent = nullptr);

    ~AudioManager() override;

    QMap<QString, SoundPool> m_soundPools;  // 音效池
    static const int POOL_SIZE = 8;         // 每种音效的池大小

    QMediaPlayer *m_musicPlayer;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput *m_audioOutput;
    QString m_currentMusicFile;
#else
    QMediaPlaylist* m_playlist;
#endif

    int m_soundVolume;
    int m_musicVolume;

private
    slots:

    void onMediaStatusChanged(QMediaPlayer::MediaStatus
                              status);
};

#endif  // AUDIOMANAGER_H
