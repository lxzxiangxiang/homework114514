"""Trim audio segments using ffmpeg - copies from originals/, generates _clip.mp3"""
import os
import subprocess
import sys

FFMPEG = os.path.join("tools", "ffmpeg", "ffmpeg-8.1.1-essentials_build", "bin", "ffmpeg.exe")
ORIG  = os.path.join("assets", "bgm", "originals")
BGM   = os.path.join("assets", "bgm")

TRIMS = [
    # (match_keyword, start_time, duration, category, clip_suffix)
    # === FAILURE ===
    ("流浪者之歌",       "0",    "45",  "failure", "clip"),
    ("二泉映月",          "0",    "60",  "failure", "clip"),
    ("His Theme",         "0",    "50",  "failure", "clip"),
    ("Your New Home",     "0",    "45",  "failure", "clip"),
    ("Travelers' encore", "8",    "42",  "failure", "clip"),
    ("Torches",           "60",   "60",  "failure", "clip"),
    ("Por Una Cabeza",    "30",   "45",  "failure", "clip"),
    ("舌尖上的中国",      "20",   "50",  "failure", "clip"),
    ("Tom And Jerry",     "40",   "40",  "failure", "clip"),
    # === SUCCESS ===
    ("年夜饭",             "0",    "50",  "success", "clip"),
    ("Unwelcome School",  "0",    "45",  "success", "clip"),
    ("III (Find Yourself)","270",  "60",  "success", "climax"),
    ("2：23AM",           "10",   "45",  "success", "clip"),
    ("世界尽头酒馆",      "15",   "45",  "success", "clip"),
    # === GAME ===
    ("MEGALOVANIA",       "16",   "60",  "game",    "clip"),
    ("Astronomia",        "14",   "40",  "game",    "clip"),
    ("Hopes And Dreams",  "0",    "70",  "game",    "clip"),
    ("Brainiac Maniac",   "5",    "45",  "game",    "clip"),
    ("Go Big Or Go Extinct","30", "60",  "game",    "clip"),
    ("Dragonflame",       "15",   "45",  "game",    "clip"),
    ("sans_",             "10",   "60",  "game",    "clip"),
    ("color-X",           "10",   "50",  "game",    "clip"),
    ("太空漫步",          "5",    "45",  "game",    "clip"),
    # === MENU ===
    ("白日食堂",          "0",    "90",  "menu",    "clip"),
    ("村巷欢声",          "0",    "80",  "menu",    "clip"),
]

def find_file(keyword):
    for f in os.listdir(ORIG):
        if keyword in f:
            return os.path.join(ORIG, f)
    return None

if not os.path.exists(FFMPEG):
    print(f"ERROR: ffmpeg not found at {FFMPEG}")
    sys.exit(1)

ok = fail = skip = 0

for keyword, start, duration, category, suffix in TRIMS:
    src = find_file(keyword)
    if not src:
        print(f"  SKIP: {keyword} - file not found in originals/")
        skip += 1
        continue

    base = os.path.splitext(os.path.basename(src))[0]
    dst_dir = os.path.join(BGM, category)
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, f"{base}_{suffix}.mp3")

    if os.path.exists(dst):
        print(f"  SKIP: {os.path.basename(dst)} - already exists")
        skip += 1
        continue

    cmd = [FFMPEG, "-y", "-i", src, "-ss", start, "-t", duration,
           "-acodec", "libmp3lame", "-q:a", "2", dst]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode == 0 and os.path.exists(dst):
            sz_kb = os.path.getsize(dst) / 1024
            print(f"  OK: [{category}] {os.path.basename(dst)} ({sz_kb:.0f} KB)")
            ok += 1
        else:
            print(f"  FAIL: {keyword} - ffmpeg error")
            fail += 1
    except subprocess.TimeoutExpired:
        print(f"  FAIL: {keyword} - timeout")
        fail += 1
    except Exception as e:
        print(f"  FAIL: {keyword} - {e}")
        fail += 1

print(f"\n=== Done: {ok} ok, {fail} failed, {skip} skipped ===")