#!/bin/sh
# Install FM Radio on the phone. Run from this directory, as the desktop user.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
sudo install -m 0755 "$HERE/sirius-fmd" /usr/local/sbin/sirius-fmd
sudo install -m 0644 "$HERE/sirius-fmd.service" /etc/systemd/system/sirius-fmd.service
sudo install -m 0755 "$HERE/sirius-fm" /usr/local/bin/sirius-fm
# The mono + low-pass filter is C: the pure-Python version cannot sustain
# 48 kHz on this SoC. Build it with whatever compiler is present.
CC=$(command -v clang || command -v cc || command -v gcc || true)
if [ -n "$CC" ]; then
	"$CC" -O2 -o /tmp/sirius-fm-downmix "$HERE/sirius-fm-downmix.c" &&
		sudo install -m 0755 /tmp/sirius-fm-downmix /usr/local/bin/sirius-fm-downmix
else
	echo "no C compiler found; FM audio will play in stereo without the low-pass"
fi
mkdir -p "$HOME/.local/share/applications"
install -m 0644 "$HERE/org.sirius.FmRadio.desktop" "$HOME/.local/share/applications/"
ICONS="$HOME/.local/share/icons/hicolor"
mkdir -p "$ICONS/scalable/apps"
install -m 0644 "$HERE/org.sirius.FmRadio.svg" "$ICONS/scalable/apps/"
[ -f "$ICONS/index.theme" ] || cp /usr/share/icons/hicolor/index.theme "$ICONS/" 2>/dev/null || true
gtk4-update-icon-cache -q -t -f "$ICONS" 2>/dev/null || true
sudo systemctl daemon-reload
sudo systemctl enable --now sirius-fmd.service
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
echo "installed; FM Radio is in the app grid"
