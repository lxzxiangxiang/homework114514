# 生成三首不同场景背景音乐

## 概述

在 `generate_sounds.py` 中新增 3 首 BGM，分别对应菜单、游戏过程、胜利三个场景。保存到 `D:\code\project\1.0.0\sound\`。

---

## 设计方案

| 文件 | 时长 | 风格 | 音乐特征 |
|------|------|------|----------|
| `bgm_menu.wav` | ~20s | 舒缓期待感 | 慢节奏、琶音分解和弦、轻柔 sine 波 |
| `bgm_play.wav` | ~30s | 活泼紧张感 | 快速节奏、方波旋律 + 三角波低音 + 底鼓（现有 bgm.wav 风格） |
| `bgm_win.wav` | ~12s | 胜利庆典感 | 大调上行音阶、铜管感 sawtooth 波、镲片噪声 |

---

## 音频参数

| 属性 | 值 |
|------|-----|
| 格式 | WAV (PCM) |
| 采样率 | 44100 Hz |
| 位深 | 16-bit |
| 声道 | 单声道 |

---

## 三首 BGM 详细定义

### 1. 菜单音乐 `bgm_menu.wav` (~20s)

**音乐设计**:
- 氛围感强，不抢戏
- sine 波 + 少量 triangle 波
- 慢速分解和弦: Am → F → C → G 循环
- 轻柔的弦乐感 pad
- BPM ≈ 60，每拍约 1 秒

**和弦进行** (每个和弦 4 秒):
```
Am (A3 C4 E4) → F (F3 A3 C4) → C (C3 E4 G4) → G (G3 B3 D4) → 循环
```

### 2. 游戏过程音乐 `bgm_play.wav` (~30s)

**音乐设计**:
- 复用现有 `gen_bgm()` 的 C 大调旋律 + 低音 + 底鼓结构
- 升级为更快节奏: BPM ≈ 150，每拍 0.4 秒
- 旋律线: square 波 (8-bit 风格)
- 低音线: triangle 波
- 打击乐: 每 0.25 秒一个 hi-hat (短噪声) + 每 0.5 秒一个 kick
- 加入简单的鼓点填充

**旋律** (约 16 小节):
```
小节 1-4:  C大调上行主题
小节 5-8:  a小调对比段
小节 9-12: F→G→C 解决
小节 13-16: 重复主题变奏
```

### 3. 胜利音乐 `bgm_win.wav` (~12s)

**音乐设计**:
- 大调上行音阶: C5 → D5 → E5 → F5 → G5 → C6
- sawtooth 波模拟铜管/号角
- 开头: 快速上行琶音 (0.5s)
- 主体: 嘹亮的主旋律 (8s)
- 结尾: 下行音阶收尾 + 长 C 大调和弦延音 (3s)
- 每半小节 (0.5s) 一个镲片 crash 强调重音

---

## 实现

### 修改文件

**`D:\code\project\0.0.3\ball_game_qt\generate_sounds.py`** — 新增 3 个函数：

```
gen_bgm_menu()   → 生成 bgm_menu.wav
gen_bgm_play()   → 生成 bgm_play.wav  
gen_bgm_win()    → 生成 bgm_win.wav
```

在 `__main__` 中添加调用，同时移除旧的 `gen_bgm()`（或保留为 `bgm.wav` 作为备用）。

### 运行

```bash
cd D:\code\project\0.0.3\ball_game_qt
python generate_sounds.py
```

### 输出

```
D:\code\project\1.0.0\sound\
├── bgm_menu.wav    # 菜单界面 (~1.6 MB)
├── bgm_play.wav    # 游戏过程 (~2.4 MB)
├── bgm_win.wav     # 胜利界面 (~1.0 MB)
├── (保留原有 5 个 SFX)
```

---

## 修改文件清单

| 文件 | 修改 |
|------|------|
| `generate_sounds.py` | 新增 `gen_bgm_menu()`, `gen_bgm_play()`, `gen_bgm_win()` + `__main__` 调用 |