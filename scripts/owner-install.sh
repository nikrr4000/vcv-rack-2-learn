#!/bin/bash
# Installs the Atlas plugin into the owner's VCV Rack and restarts the Atlas service (plan §13).
#
#   rack/scripts/owner-install.sh             install
#   rack/scripts/owner-install.sh --dry-run   print every command, change nothing
#
# Steps: the old AtlasTutor plugin → AtlasTutor.bak-<date>; an older Atlas → Atlas.bak-<date>;
# rack/plugin/dist/Atlas → the Rack plugins folder; the Atlas service restarted through launchctl
# when its LaunchAgent is installed (otherwise: how to start it by hand).
# Run `rack/scripts/build.sh` first if rack/plugin/dist/Atlas is missing.
set -euo pipefail
cd "$(dirname "$0")/../.."
REPO="$(pwd)"
DRY=0
[ "${1:-}" = "--dry-run" ] && DRY=1

PLUGINS="${RACK_PLUGINS_DIR:-$HOME/Library/Application Support/Rack2/plugins-mac-arm64}"
SRC="$REPO/rack/plugin/dist/Atlas"
STAMP="$(date +%Y%m%d-%H%M%S)"
LABEL="com.nikrr.atlas"
AGENT="$HOME/Library/LaunchAgents/$LABEL.plist"

run() {
	if [ $DRY = 1 ]; then
		printf '[dry-run]'
		printf ' %q' "$@"
		printf '\n'
	else
		"$@"
	fi
}

[ $DRY = 1 ] && echo "Сухой прогон: ничего не меняется, команды только печатаются."

if [ ! -f "$SRC/plugin.dylib" ] || [ ! -f "$SRC/plugin.json" ]; then
	echo "Нет собранного плагина в $SRC. Сначала: rack/scripts/build.sh" >&2
	exit 1
fi
if [ ! -d "$PLUGINS" ]; then
	echo "Не найдена папка плагинов Rack: $PLUGINS" >&2
	echo "Запусти VCV Rack 2 хотя бы раз, чтобы он её создал." >&2
	exit 1
fi

# 1. The old tutor plugin goes aside (the new Atlas replaces it).
if [ -e "$PLUGINS/AtlasTutor" ]; then
	echo "Старый плагин AtlasTutor → AtlasTutor.bak-$STAMP"
	run mv "$PLUGINS/AtlasTutor" "$PLUGINS/AtlasTutor.bak-$STAMP"
fi

# 2. An older Atlas is kept as a backup, then the new one is copied in.
if [ -e "$PLUGINS/Atlas" ]; then
	echo "Прежний Atlas → Atlas.bak-$STAMP"
	run mv "$PLUGINS/Atlas" "$PLUGINS/Atlas.bak-$STAMP"
fi
echo "Новый Atlas → $PLUGINS/Atlas"
run cp -R "$SRC" "$PLUGINS/Atlas"

# 3. The Atlas service (the tutor and progress live there).
if [ -f "$AGENT" ]; then
	echo "Перезапускаю сервис Atlas ($LABEL)"
	run launchctl kickstart -k "gui/$(id -u)/$LABEL" || echo "launchctl не смог перезапустить сервис: запусти Atlas вручную (cd $REPO/app && npm start)."
else
	echo "Сервис Atlas не установлен как LaunchAgent. Запусти его вручную:"
	echo "  cd $REPO/app && npm start"
fi

echo
echo "Перезапусти VCV Rack, добавь модуль Atlas → Навигатор и нажми «Продолжить»."
echo "Что проверить: $REPO/docs/rack/OWNER_CHECKLIST.md"
