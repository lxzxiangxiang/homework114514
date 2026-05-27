# 非 8-bit 风格纯音乐生成方案

## 可行性分析

纯 Python stdlib (`wave` + `math`) 可以合成多种风格，但受限于**基本波形合成**（无真实采样乐器）：

| 风格 | 可行性 | 核心技术 |
|------|--------|----------|
| **Ambient / 氛围** | ✅ 完全可行 | sine 波叠加 + 慢起音/慢衰减 + 混响模拟 |
| **Orchestral 弦乐/铜管** | ⚠️ 近似 | sawtooth 叠 sine 模拟铜管；多层 detune sine 模拟弦乐 |
| **Electronic / Synthwave** | ✅ 完全可行 | saw/square 波 + 滤波器扫频 + 鼓机 |
| **Piano-like / 音乐盒** | ⚠️ 近似 | 短促攻击 + 指数衰减 sine + 泛音列 |
| **Jazz / Swing** | ❌ 困难 | 节奏可做，但音色无法模拟真实乐器 |
| **8-bit / Chiptune** | ✅ 已实现 | square + triangle + noise（现有 bgm_play.wav） |

## 推荐：为三场景生成 Ambient 版本

Ambient 风格最适合这个游戏（Agar.io 类），理由：
- 不抢戏，做背景氛围
- 可以无缝循环
- 合成质量高（sine 波先天干净）

### 生成文件

| 文件 | 场景 | 风格 |
|------|------|------|
| `bgm_menu_a.wav` | 菜单 | 空灵 pad，Dm→Am 慢速漂移 |
| `bgm_play_a.wav` | 游戏过程 | 脉冲低音 + 高音旋律线，渐强渐弱 |
| `bgm_win_a.wav` | 胜利 | 明亮大调和弦铺底 + 钟琴感上行音阶 |

### 技术要点

```
Ambient 特征:
  - 慢起音 (attack 0.3~0.5s)
  - 长延音 + 慢释放
  - 多声部 sine 叠层 (基频 + 3/5/8度泛音)
  - 少量 detune (相差 1~3Hz 产生自然摇曳感)
  - 模拟混响: 叠加延迟 0.05~0.15s 的低音量回声
  - 低通感: 高频泛音比例随时间递减
  
8-bit 特征 (对比):
  - 快起音 (attack < 0.01s)
  - 方波/三角波基础
  - 无混响
  - 鼓组明确分离
```

## 实现

### 修改文件

**`D:\code\project\0.0.3\ball_game_qt\generate_sounds.py`** — 新增 3 个 ambient 函数：

```python
def gen_bgm_menu_ambient():   → bgm_menu_a.wav
def gen_bgm_play_ambient():   → bgm_play_a.wav
def gen_bgm_win_ambient():    → bgm_win_a.wav
```

### 运行

```bash
python generate_sounds.py
```

### 输出

```
D:\code\project\1.0.0\sound\
├── bgm_menu.wav       # 8-bit 菜单 (保留)
├── bgm_menu_a.wav     # Ambient 菜单 (新增)
├── bgm_play.wav       # 8-bit 游戏 (保留)
├── bgm_play_a.wav     # Ambient 游戏 (新增)
├── bgm_win.wav        # 8-bit 胜利 (保留)
├── bgm_win_a.wav      # Ambient 胜利 (新增)
├── (保留原有 SFX)
```

## 总结

可以生成。推荐先做 Ambient 风格作为 8-bit 的替代选项，后续如果需要其他风格（Synthwave、钢琴等）可以继续扩展。