# 生成游戏音频文件（不修改 C++ 代码）

## 概述

用纯 Python stdlib 程序化生成 6 个 WAV 音频文件到 `sounds/` 目录。后续可择机集成到 Qt 程序中。

---

## 步骤

### 1. 编写 generate_sounds.py

**文件**: `D:\code\project\0.0.3\ball_game_qt\generate_sounds.py`

### 2. 运行脚本

```bash
cd D:\code\project\0.0.3\ball_game_qt
python generate_sounds.py
```

### 3. 输出

```
sounds/
├── split.wav      # 0.15s — 分裂音效（噪声爆破）
├── eject.wav      # 0.10s — 吐孢音效（频率下滑）
├── eat.wav        # 0.15s — 吞噬音效（上滑音）
├── skill.wav      # 0.40s — 技能获得（琶音）
├── hurt.wav       # 0.30s — 受伤音效（嗡鸣）
└── bgm.wav        # ~30s  — 背景音乐（8-bit 风格循环）
```

## 音频规格

| 属性 | 值 |
|------|-----|
| 格式 | WAV (PCM) |
| 采样率 | 44100 Hz |
| 位深 | 16-bit |
| 声道 | 单声道 |
| 兼容性 | QSoundEffect / QMediaPlayer 直接支持 |
