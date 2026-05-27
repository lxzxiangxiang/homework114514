# 分裂/吐孢音效 + 背景音乐方案

## 概述

当前项目只链接了 `Qt6::Core` 和 `Qt6::Widgets`，无音视频支持。需要添加 **Qt Multimedia** 模块来实现音效（SFX）和背景音乐（BGM）。

---

## 技术选型：`QSoundEffect` vs `QMediaPlayer`

| 特性 | QSoundEffect | QMediaPlayer |
|------|-------------|-------------|
| 适用场景 | 短音效（<5秒）、低延迟、可叠加 | 音乐、长音频 |
| 格式支持 | WAV（原生）、部分平台支持 OGG | MP3/WAV/OGG/FLAC 等 |
| 同时播放 | 支持多实例同时播放 | 单实例 |
| 延迟 | 极低（预加载到内存） | 有缓冲延迟 |
| Qt 模块 | `Qt6::Multimedia` | `Qt6::Multimedia` |

**结论**: 音效用 `QSoundEffect`，背景音乐用 `QMediaPlayer` + `QAudioOutput`。

---

## 实现步骤

### 步骤 1: CMakeLists.txt — 添加 Multimedia 依赖

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

target_link_libraries(ball_game_qt PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
)
```

### 步骤 2: 用 Python 脚本程序化生成所有音频文件（无需外部素材）

在项目根目录运行一次 Python 脚本，自动生成 `sounds/` 目录和全部 WAV 文件：

**`generate_sounds.py`** (新建，位于 `ball_game_qt/` 目录下):

```python
"""生成球球大乱斗全部音效和背景音乐 (纯 Python stdlib, 无需第三方库)"""
import wave
import math
import struct
import os
import random

SAMPLE_RATE = 44100

def write_wav(filename, samples):
    """将 float 采样列表 [-1,1] 写入 16-bit 单声道 WAV"""
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with wave.open(filename, 'w') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        packed = b''.join(
            struct.pack('<h', max(-32767, min(32767, int(s * 32767))))
            for s in samples
        )
        wav.writeframes(packed)

def envelope(total, attack_ratio, decay_start):
    """生成 A-D-S 包络曲线 [0,1]，在 decay_start 处开始线性衰减到0"""
    attack = int(total * attack_ratio)
    decay_begin = int(total * decay_start)
    env = []
    for i in range(total):
        if i < attack:
            env.append(i / attack)
        elif i < decay_begin:
            env.append(1.0)
        else:
            t = (i - decay_begin) / (total - decay_begin)
            env.append(1.0 - t)
    return env

# ==================== 音效生成 ====================

def gen_split():
    """分裂音效: 短爆破 + 低频底音"""
    dur = 0.15
    n = int(SAMPLE_RATE * dur)
    env = envelope(n, 0.02, 0.05)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        noise = (random.random() * 2 - 1)
        low = math.sin(2 * math.pi * 80 * t)
        s = (noise * 0.7 + low * 0.3) * env[i]
        samples.append(s)
    write_wav('sounds/split.wav', samples)

def gen_eject():
    """吐孢音效: 高频短促弹出声"""
    dur = 0.1
    n = int(SAMPLE_RATE * dur)
    env = envelope(n, 0.02, 0.05)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 1200 - t * 6000
        s = math.sin(2 * math.pi * freq * t) * 0.6
        noise = (random.random() * 2 - 1) * 0.15
        samples.append((s + noise) * env[i])
    write_wav('sounds/eject.wav', samples)

def gen_eat():
    """吞噬音效: 短促上滑音"""
    dur = 0.15
    n = int(SAMPLE_RATE * dur)
    env = envelope(n, 0.03, 0.2)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 300 + t * 2000
        s = math.sin(2 * math.pi * freq * t) * 0.5
        s += math.sin(2 * math.pi * freq * 1.5 * t) * 0.3
        samples.append(s * env[i])
    write_wav('sounds/eat.wav', samples)

def gen_skill():
    """技能获得: 三音上行琶音 (叮-叮-叮↑)"""
    dur = 0.4
    n = int(SAMPLE_RATE * dur)
    notes = [523, 659, 784]  # C5 E5 G5
    note_len = n // 3
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        idx = min(i // note_len, 2)
        freq = notes[idx]
        local_t = t - idx * (note_len / SAMPLE_RATE)
        env = max(0, 1.0 - local_t * 6)
        s = math.sin(2 * math.pi * freq * t) * 0.4
        s += math.sin(2 * math.pi * freq * 2 * t) * 0.15
        samples.append(s * env)
    write_wav('sounds/skill.wav', samples)

def gen_hurt():
    """受伤音效: 低频嗡鸣 + 噪声"""
    dur = 0.3
    n = int(SAMPLE_RATE * dur)
    env = envelope(n, 0.03, 0.15)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 150 - t * 100
        s = math.sin(2 * math.pi * freq * t) * 0.5
        noise = (random.random() * 2 - 1) * 0.4
        samples.append((s + noise) * env[i])
    write_wav('sounds/hurt.wav', samples)

# ==================== 背景音乐 ====================

def gen_bgm():
    """8-bit 风格循环背景音乐 (约 30 秒循环)"""
    # 简单的 C 大调旋律
    C4, D4, E4, F4, G4, A4, B4, C5 = 262, 294, 330, 349, 392, 440, 494, 523
    G3, A3 = 196, 220

    melody = [
        (C4, 0.25), (E4, 0.25), (G4, 0.25), (C5, 0.25),
        (A4, 0.25), (G4, 0.25), (E4, 0.25), (C4, 0.25),
        (D4, 0.25), (F4, 0.25), (A4, 0.25), (C5, 0.25),
        (B4, 0.25), (A4, 0.25), (G4, 0.25), (E4, 0.25),
        (C4, 0.25), (D4, 0.25), (E4, 0.25), (G4, 0.25),
        (F4, 0.25), (E4, 0.25), (D4, 0.25), (C4, 0.25),
        (A3, 0.25), (C4, 0.25), (E4, 0.25), (A4, 0.25),
        (G4, 0.25), (E4, 0.25), (C4, 0.25), (A3, 0.5),
        # 第二段变奏
        (E4, 0.125), (G4, 0.125), (C5, 0.125), (E5, 0.125),
        (D5, 0.25), (C5, 0.25), (A4, 0.25), (G4, 0.25),
        (A4, 0.125), (C5, 0.125), (E5, 0.125), (G5, 0.125),
        (F5, 0.25), (E5, 0.25), (D5, 0.25), (C5, 0.25),
        (E4, 0.125), (D4, 0.125), (E4, 0.125), (G4, 0.125),
        (C5, 0.25), (A4, 0.25), (G4, 0.5),
        (F4, 0.25), (E4, 0.25), (D4, 0.25), (C4, 0.5),
    ]

    bass = [
        (C3:=131, 1.0), (G3, 1.0), (A3, 1.0), (E3:=165, 1.0),
        (F3:=175, 1.0), (C3, 1.0), (G3, 1.0), (C3, 1.0),
        (C3, 1.0), (A3, 1.0), (F3, 1.0), (G3, 1.0),
        (C3, 0.5), (E3, 0.5), (F3, 0.5), (G3, 0.5),
    ]

    total_dur = sum(d for _, d in melody)
    n = int(SAMPLE_RATE * total_dur)
    samples = []

    mel_pos = 0.0
    bass_pos = 0.0
    mel_idx = 0
    bass_idx = 0

    for i in range(n):
        t = i / SAMPLE_RATE

        # 旋律线 (方波)
        if mel_idx < len(melody):
            note, note_dur = melody[mel_idx]
            if t >= mel_pos + note_dur:
                mel_pos += note_dur
                mel_idx += 1
        if mel_idx < len(melody):
            note, _ = melody[mel_idx]
            local_t = t - mel_pos
            # 方波合成
            duty = 0.5
            phase = (local_t * note) % 1.0
            square = 1.0 if phase < duty else -1.0
            mel_sample = square * 0.15
        else:
            mel_sample = 0

        # 低音线 (三角波)
        if bass_idx < len(bass):
            bnote, bdur = bass[bass_idx]
            if t >= bass_pos + bdur:
                bass_pos += bdur
                bass_idx += 1
        if bass_idx < len(bass):
            bnote, _ = bass[bass_idx]
            local_t = t - bass_pos
            phase = (local_t * bnote) % 1.0
            tri = 4.0 * abs(phase - 0.5) - 1.0
            bass_sample = tri * 0.2
        else:
            bass_sample = 0

        # 简单打击乐 (每 0.5 秒一个 kick)
        beat_t = t % 0.5
        if beat_t < 0.05:
            kick = math.sin(2 * math.pi * (150 - beat_t * 2500) * beat_t)
            kick *= max(0, 1.0 - beat_t / 0.05)
            drum = kick * 0.3
        else:
            drum = 0

        samples.append(mel_sample + bass_sample + drum)

    write_wav('sounds/bgm.wav', samples)

if __name__ == '__main__':
    print("正在生成 split.wav ...");   gen_split()
    print("正在生成 eject.wav ...");   gen_eject()
    print("正在生成 eat.wav ...");     gen_eat()
    print("正在生成 skill.wav ...");   gen_skill()
    print("正在生成 hurt.wav ...");    gen_hurt()
    print("正在生成 bgm.wav ...");     gen_bgm()
    print("全部生成完成! 文件位于 sounds/ 目录")
```

运行命令：
```bash
python generate_sounds.py
```

生成的 WAV 文件均为 44100Hz 16-bit 单声道，可以直接被 `QSoundEffect` 和 `QMediaPlayer` 使用。

同时创建 `sounds.qrc` 资源文件：

```xml
<RCC>
    <qresource prefix="/sounds">
        <file>sounds/split.wav</file>
        <file>sounds/eject.wav</file>
        <file>sounds/eat.wav</file>
        <file>sounds/skill.wav</file>
        <file>sounds/hurt.wav</file>
        <file>sounds/bgm.wav</file>
    </qresource>
</RCC>
```

### 步骤 3: 创建 SoundManager 类

**SoundManager.h**：

```cpp
#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>

class SoundManager : public QObject {
    Q_OBJECT
public:
    explicit SoundManager(QObject* parent = nullptr);

    void playSplit();
    void playEject();
    void playBGM();
    void stopBGM();
    void setSFXVolume(float vol);    // 0.0 ~ 1.0
    void setBGMVolume(float vol);    // 0.0 ~ 1.0

private:
    QSoundEffect* m_splitSound;
    QSoundEffect* m_ejectSound;
    QMediaPlayer* m_bgmPlayer;
    QAudioOutput* m_audioOutput;
};

#endif
```

**SoundManager.cpp**：

```cpp
#include "SoundManager.h"

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
{
    m_splitSound = new QSoundEffect(this);
    m_splitSound->setSource(QUrl("qrc:/sounds/split.wav"));
    m_splitSound->setVolume(0.5);

    m_ejectSound = new QSoundEffect(this);
    m_ejectSound->setSource(QUrl("qrc:/sounds/eject.wav"));
    m_ejectSound->setVolume(0.5);

    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.3);

    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmPlayer->setAudioOutput(m_audioOutput);
    m_bgmPlayer->setSource(QUrl("qrc:/sounds/bgm.wav"));
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);
}

void SoundManager::playSplit() { m_splitSound->play(); }
void SoundManager::playEject() { m_ejectSound->play(); }

void SoundManager::playBGM()   { m_bgmPlayer->play(); }
void SoundManager::stopBGM()   { m_bgmPlayer->stop(); }

void SoundManager::setSFXVolume(float vol) {
    m_splitSound->setVolume(vol);
    m_ejectSound->setVolume(vol);
}

void SoundManager::setBGMVolume(float vol) {
    m_audioOutput->setVolume(vol);
}
```

### 步骤 4: 在 GameWindow 中集成 SoundManager

**GameWindow.h** — 添加成员：
```cpp
#include "SoundManager.h"

class GameWindow : public QMainWindow {
    // ...
private:
    SoundManager* m_soundManager;
};
```

**GameWindow.cpp** — 构造函数中创建：
```cpp
GameWindow::GameWindow(QWidget* parent)
    : /* ... */
{
    // ...
    m_soundManager = new SoundManager(this);
    m_soundManager->playBGM();  // 启动时播放 BGM
}
```

**GameWindow.cpp** — `keyPressEvent` 中触发音效：
```cpp
// Playing 状态下
} else if (key == Qt::Key_Space) {
    m_world->handleSplit();
    m_soundManager->playSplit();       // ← 分裂音效
} else if (key == Qt::Key_E) {
    m_world->handleEject();
    m_soundManager->playEject();       // ← 吐孢音效
}
```

### 步骤 5: CMakeLists.txt 注册 .qrc 文件

```cmake
set(RESOURCES
    sounds.qrc
)

qt_add_executable(ball_game_qt
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)
```

同时把 SoundManager 加入编译：
```cmake
set(SOURCES
    # ... 原有文件 ...
    src/SoundManager.cpp
)

set(HEADERS
    # ... 原有文件 ...
    src/SoundManager.h
)
```

---

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `CMakeLists.txt` | 添加 `Multimedia` 依赖 + `SoundManager.cpp/h` + `sounds.qrc` |
| `sounds.qrc` (新建) | Qt 资源文件，引用 sounds/ 目录下的音频 |
| `src/SoundManager.h` (新建) | 音效管理器类声明 |
| `src/SoundManager.cpp` (新建) | 音效管理器实现 |
| `src/GameWindow.h` | 添加 `SoundManager*` 成员 |
| `src/GameWindow.cpp` | 构造中创建 SoundManager + 按键触发音效 |
| `sounds/` 目录 + 6 个 WAV (新建) | 程序化生成，见 `generate_sounds.py` |

---

## 音频素材：全部由脚本自动生成

无需外部下载。运行 `python generate_sounds.py` 一次性生成 6 个 WAV：

| 文件 | 时长 | 合成方式 | 触发时机 |
|------|------|----------|----------|
| `split.wav` | 0.15s | 噪声爆破 + 80Hz 底音 | 空格分裂 |
| `eject.wav` | 0.10s | 1200→200Hz 频率下滑 | E 键吐孢 |
| `eat.wav` | 0.15s | 300→2300Hz 上滑音 | 吞噬实体时 |
| `skill.wav` | 0.40s | C5-E5-G5 琶音 | 获得技能球时 |
| `hurt.wav` | 0.30s | 150→50Hz 嗡鸣 + 噪声 | 触碰陷阱/debuff |
| `bgm.wav` | ~30s | 方波旋律 + 三角波低音 + 底鼓 | 游戏进程中循环 |

不满足于合成音效时，可替换为从 [sfxr.me](https://sfxr.me) 生成的 8-bit 音效或 [freesound.org](https://freesound.org) 下载的 WAV。

---

## 可选扩展

| 功能 | 实现方式 |
|------|----------|
| 吞噬音效 | `SoundManager::playEat()` + `GameWorld::checkCollisions()` 吃球时调用 |
| 技能获得音效 | `SoundManager::playSkill()` + `GameWorld::checkCollisions()` 碰技能球时 |
| 受伤音效 | `SoundManager::playHurt()` + `Ball::applyDebuff()` 减益触发时 |
| 音效音量设置 | UI 菜单中加滑块，调用 `setSFXVolume()` / `setBGMVolume()` |
| BGM 切换 | 读取多个 mp3，按游戏状态（菜单/游戏中/结束）切换 `setSource()` |
