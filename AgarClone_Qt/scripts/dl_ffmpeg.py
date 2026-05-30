"""Download + extract ffmpeg (run standalone, may take minutes)"""
import urllib.request
import zipfile
import os
import shutil
import sys

URL = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
TMP = os.path.join("tools", "_ffmpeg_tmp.zip")
DEST = os.path.join("tools", "ffmpeg")
LOG = os.path.join("tools", "_ffmpeg_status.txt")

def log(msg):
    print(msg)
    with open(LOG, "w") as f:
        f.write(msg)

try:
    # Cleanup
    if os.path.exists(TMP):
        os.remove(TMP)
    for f in [LOG]:
        try: os.remove(f)
        except: pass

    # Download with progress
    log("downloading")
    def report(blocknum, blocksize, totalsize):
        pct = min(100, int(blocknum * blocksize * 100 / totalsize)) if totalsize > 0 else 0
        if blocknum % 50 == 0:
            print(f"  {pct}%", end="\r")

    urllib.request.urlretrieve(URL, TMP, report)
    sz = os.path.getsize(TMP) / (1024 * 1024)
    log(f"downloaded_{sz:.0f}mb")

    if sz < 10:
        log("error_too_small")
        sys.exit(1)

    # Extract
    log("extracting")
    if os.path.exists(DEST):
        shutil.rmtree(DEST)
    os.makedirs(DEST, exist_ok=True)

    with zipfile.ZipFile(TMP) as z:
        z.extractall(DEST)

    # Find ffmpeg.exe
    ffmpeg_path = None
    for root, dirs, files in os.walk(DEST):
        for f in files:
            if f == "ffmpeg.exe":
                ffmpeg_path = os.path.join(root, f)
                break
        if ffmpeg_path:
            break

    if ffmpeg_path:
        log(f"done_{ffmpeg_path}")
    else:
        log("error_no_exe")

    os.remove(TMP)
except Exception as e:
    log(f"error_{e}")