#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QStringList>

class SoundManager : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SoundManager)

public:
    explicit SoundManager(const QString& bgmDir, QObject* parent = nullptr);
    ~SoundManager();

    void playMenuMusic();
    void playGameMusic();
    void playGameOverMusic();
    void playVictoryMusic();
    void playSplitSound();
    void playStartGameSound();
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

private:
    static constexpr float BGM_VOLUME = 0.8f;
    static constexpr float SFX_VOLUME = 0.6f;

    QMediaPlayer* m_bgmPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QStringList m_menuBgmFiles;
    QStringList m_gameBgmFiles;
    QStringList m_failureBgmFiles;
    QStringList m_successBgmFiles;

    QMediaPlayer* m_eventPlayer = nullptr;
    QAudioOutput* m_eventOutput = nullptr;

    QSoundEffect* m_startGameSound = nullptr;
    QMediaPlayer* m_splitPlayer = nullptr;
    QAudioOutput* m_splitOutput = nullptr;

    bool m_isPaused = false;
    qint64 m_currentPosition = 0;

    QString m_bgmDir;
    QString randomBgm(const QStringList& list) const;
    void playRandomBgm(const QStringList& list);
    void playEventMusic(const QStringList& list);
};
