#include "audiomanager.h"
#include <QDebug>
#include <QDir>

AudioManager& AudioManager::instance() {
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager(QObject* parent)
    : QObject(parent),
      m_soundVolume(100),
      m_musicVolume(80) {
    // 初始化音乐播放器
    m_musicPlayer = new QMediaPlayer(this);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6: 使用 QAudioOutput
    m_audioOutput = new QAudioOutput(this);
    m_musicPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(m_musicVolume / 100.0);
#else
    // Qt 5: 使用 QMediaPlaylist
    m_playlist = new QMediaPlaylist(this);
    m_musicPlayer->setPlaylist(m_playlist);
    m_musicPlayer->setVolume(m_musicVolume);
#endif

    // 实现循环播放
    connect(m_musicPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &AudioManager::onMediaStatusChanged);
}

AudioManager::~AudioManager() {
    // 清理所有音效池
    for (auto& pool : m_soundPools) {
        qDeleteAll(pool.effects);
        pool.effects.clear();
    }
    m_soundPools.clear();
}

void AudioManager::preloadSound(const QString& soundName, const QString& filePath) {
    if (m_soundPools.contains(soundName)) {
        qDebug() << "Sound already preloaded:" << soundName;
        return;
    }

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        qWarning() << "Sound file not found:" << filePath;
        return;
    }

    // 创建音效池
    SoundPool pool;
    pool.filePath = filePath;
    pool.nextIndex = 0;
    pool.effects.reserve(POOL_SIZE);

    // 预创建多个QSoundEffect实例
    for (int i = 0; i < POOL_SIZE; ++i) {
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl::fromLocalFile(filePath));
        effect->setVolume(m_soundVolume / 100.0);
        pool.effects.append(effect);
    }

    m_soundPools[soundName] = pool;
    qDebug() << "Preloaded sound pool:" << soundName << "with" << POOL_SIZE << "instances";
}

void AudioManager::playSound(const QString& soundName) {
    if (!m_soundPools.contains(soundName)) {
        qWarning() << "Sound effect not found:" << soundName;
        return;
    }

    SoundPool& pool = m_soundPools[soundName];

    // 使用轮询方式选择下一个可用的音效实例
    // 这样可以避免同一个实例被重复调用play()导致阻塞
    QSoundEffect* effect = pool.effects[pool.nextIndex];
    pool.nextIndex = (pool.nextIndex + 1) % pool.effects.size();

    // 播放音效（非阻塞，因为使用不同实例）
    effect->play();
}

void AudioManager::playMusic(const QString& musicFile) {
    if (!QFile::exists(musicFile)) {
        qWarning() << "Music file not found:" << musicFile;
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6: 使用 setSource
    m_currentMusicFile = musicFile;
    m_musicPlayer->setSource(QUrl::fromLocalFile(musicFile));
#else
    // Qt 5: 使用 QMediaPlaylist
    m_playlist->clear();
    m_playlist->addMedia(QUrl::fromLocalFile(musicFile));
    m_playlist->setPlaybackMode(QMediaPlaylist::Loop);
#endif

    m_musicPlayer->play();
    qDebug() << "Playing music:" << musicFile;
}

void AudioManager::stopMusic() {
    m_musicPlayer->stop();
}

void AudioManager::setMusicVolume(int volume) {
    m_musicVolume = qBound(0, volume, 100);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_audioOutput->setVolume(m_musicVolume / 100.0);
#else
    m_musicPlayer->setVolume(m_musicVolume);
#endif
}

void AudioManager::setSoundVolume(int volume) {
    m_soundVolume = qBound(0, volume, 100);
    // 更新所有音效池中所有实例的音量
    for (auto& pool : m_soundPools) {
        for (QSoundEffect* effect : pool.effects) {
            effect->setVolume(m_soundVolume / 100.0);
        }
    }
}

bool AudioManager::isMusicPlaying() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return m_musicPlayer->playbackState() == QMediaPlayer::PlayingState;
#else
    return m_musicPlayer->state() == QMediaPlayer::PlayingState;
#endif
}

void AudioManager::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Qt 6: 手动实现循环播放
        qDebug() << "Music playback finished, looping...";
        m_musicPlayer->setPosition(0);
        m_musicPlayer->play();
#else
        // Qt 5: QMediaPlaylist 自动处理循环
        qDebug() << "Music playback finished";
#endif
    }
}
