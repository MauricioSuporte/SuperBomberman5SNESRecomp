#!/usr/bin/env bash
# Smoke test of the Android APK on an emulator (CI, android.yml). No ROM is
# involved: it checks that the app starts, extracts its data, shows the
# launcher, and that "Browse For ROM" opens the system picker and imports a
# placeholder file into the game folder. Output (screenshots, logcat excerpt,
# summary.txt) goes to $OUT.
#
#   tools/android-emulator-test.sh <apk> <out dir>
set -u
APK="$1"
OUT="$2"
PKG=io.github.mauriciosuporte.sb5recomp
mkdir -p "$OUT"
summary() { echo "$*" | tee -a "$OUT/summary.txt"; }
shot() { adb exec-out screencap -p > "$OUT/$1.png"; }

adb wait-for-device
adb shell settings put global window_animation_scale 0
adb shell settings put global transition_animation_scale 0
adb shell settings put global animator_duration_scale 0
summary "device: $(adb shell getprop ro.product.model) api $(adb shell getprop ro.build.version.sdk) abi $(adb shell getprop ro.product.cpu.abi)"
summary "screen: $(adb shell wm size | tr -d '\r') $(adb shell wm density | tr -d '\r')"

adb install -r "$APK" > "$OUT/install.txt" 2>&1
summary "install: $(tail -1 "$OUT/install.txt")"
adb logcat -c
adb shell am start -W -n "$PKG/.SB5Activity" > "$OUT/start.txt" 2>&1
sleep 25
shot 01-launcher
PID=$(adb shell pidof "$PKG" | tr -d '\r')
summary "running after start: ${PID:-no}"

# Placeholder file to pick (not a ROM).
adb shell 'echo sb5-picker-test > /sdcard/Download/picker-test.sfc'
adb shell am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE \
    -d file:///sdcard/Download/picker-test.sfc > /dev/null 2>&1

# "Browse For ROM": position measured on the launcher at 2400x1080 and scale
# 1.8 (left panel, below the ROM details). Override with BROWSE_XY="x y".
read -r BX BY <<< "${BROWSE_XY:-390 516}"
adb shell input tap "$BX" "$BY"
sleep 6
shot 02-after-browse
adb shell uiautomator dump /sdcard/ui.xml > /dev/null 2>&1
adb pull /sdcard/ui.xml "$OUT/02-ui.xml" > /dev/null 2>&1
FOCUS=$(adb shell dumpsys window | grep -m1 mCurrentFocus | tr -d '\r')
summary "focus after browse: $FOCUS"

# Pick the placeholder in the system picker, if it is listed (open Downloads
# first through the drawer when the picker starts elsewhere).
tap_text() {
    adb shell uiautomator dump /sdcard/ui.xml > /dev/null 2>&1
    adb pull /sdcard/ui.xml "$OUT/ui-$1.xml" > /dev/null 2>&1
    python3 - "$OUT/ui-$1.xml" "$2" <<'PY'
import re, sys
xml = open(sys.argv[1], encoding="utf-8", errors="replace").read()
for m in re.finditer(r'<node [^>]*>', xml):
    node = m.group(0)
    if f'text="{sys.argv[2]}"' in node:
        b = re.search(r'bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', node)
        if b:
            x1, y1, x2, y2 = map(int, b.groups())
            print((x1 + x2) // 2, (y1 + y2) // 2)
            break
PY
}
XY=$(tap_text pick picker-test.sfc)
if [ -z "$XY" ]; then
    # Chooser or another folder: try "Files"/"Downloads" once.
    for label in Files Downloads Download; do
        XY2=$(tap_text "nav-$label" "$label")
        if [ -n "$XY2" ]; then adb shell input tap $XY2; sleep 3; fi
    done
    XY=$(tap_text pick2 picker-test.sfc)
fi
summary "placeholder in picker: ${XY:-not found}"
if [ -n "$XY" ]; then
    adb shell input tap $XY
    sleep 6
fi
shot 03-after-pick

adb logcat -d > "$OUT/logcat-full.txt" 2>&1
grep -E " SB5 | SDL | libc |DEBUG|FATAL|AndroidRuntime|recomp|launcher" "$OUT/logcat-full.txt" | tail -300 > "$OUT/logcat.txt"
rm -f "$OUT/logcat-full.txt"
summary "imported: $(grep -c 'imported .*picker-test.sfc' "$OUT/logcat.txt")"
summary "crash lines: $(grep -cE 'FATAL|AndroidRuntime: FATAL|signal 11|SIGSEGV|SIGABRT' "$OUT/logcat.txt")"
PID=$(adb shell pidof "$PKG" | tr -d '\r')
summary "running at end: ${PID:-no}"
adb shell run-as "$PKG" ls -la files 2>/dev/null | head -20 > "$OUT/internal-files.txt"
adb shell ls -la "/sdcard/Android/data/$PKG/files" "/sdcard/Android/data/$PKG/files/roms" > "$OUT/external-files.txt" 2>&1
exit 0
