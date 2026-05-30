#!/usr/bin/env python3
"""BGM 分类脚本 - 按场景氛围将音乐拷贝到对应子目录，原文件保留在 originals/"""

import os
import shutil
import re

BGMDIR = os.path.join("assets", "bgm")
ORIG   = os.path.join(BGMDIR, "originals")

CATEGORIES = {
    "menu": [
        "Mice on Venus", "Haggstrom", "Cat - C418", "Dog - C418",
        "Candyland", "The Right Path", "Life - Tobu",
        "はじまり", "やわらかな光", "白日食堂",
        "钢琴", "烟袋斜街", "璃上有月",
        "無仙", "无仙", "银花", "村巷欢声",
        "沉浸感", "ひやむぎ",
    ],
    "game": [
        "MEGALOVANIA", "Hopes And Dreams", "Brainiac Maniac",
        "Astronomia", "Go Big Or Go Extinct", "Dragonflame",
        "Higher - Tobu", "Sunburst", "Ryukyuvania", "sans_",
        "WINDOWS开机", "color-X", "上上下下",
        "太空漫步", "睡蕉之歌", "Tom And Jerry",
    ],
    "failure": [
        "流浪者之歌", "二泉映月", "His Theme", "Your New Home",
        "Travelers' encore", "Torches", "Por Una Cabeza",
        "舌尖上的中国", "III (Find Yourself)", "Tom And Jerry",
    ],
    "success": [
        "やったー", "年夜饭", "Unwelcome School",
        "2：23AM", "世界尽头酒馆", "III (Find Yourself)",
    ],
}

def main():
    if not os.path.isdir(ORIG):
        print(f"ERROR: {ORIG} not found")
        return

    originals = os.listdir(ORIG)
    stats = {}

    for cat, patterns in CATEGORIES.items():
        dest = os.path.join(BGMDIR, cat)
        os.makedirs(dest, exist_ok=True)
        count = 0
        for filename in originals:
            for pat in patterns:
                if pat in filename:
                    src = os.path.join(ORIG, filename)
                    dst = os.path.join(dest, filename)
                    if not os.path.exists(dst):
                        shutil.copy2(src, dst)
                        print(f"  [{cat}] {filename}")
                        count += 1
                    break
        stats[cat] = count

    print("\n=== 统计 ===")
    for cat, cnt in stats.items():
        print(f"  {cat}: {cnt} 首")
    print(f"  sfx: {len(os.listdir(os.path.join(BGMDIR, 'sfx')))} 个")
    print(f"  originals: {len(originals)} 首 (备份)")
    print("=== 完成 ===")

if __name__ == "__main__":
    main()