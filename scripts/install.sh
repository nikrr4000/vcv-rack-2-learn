#!/bin/bash
# Installs the Atlas plugin into VCV Rack 2 (macOS, Apple Silicon by default).
#
#   scripts/install.sh             install
#   scripts/install.sh --dry-run   print every command, change nothing
#
# Steps: the old AtlasTutor plugin and an older Atlas go to Rack2/atlas-backups/<name>-<date> —
# outside the plugins folder, because Rack loads every folder there and, for two plugins with one
# slug, the first one on disk (unsorted), so a backup next to the new Atlas could win; backups left
# there by earlier runs (*.bak-*) are moved out too; plugin/dist/Atlas → the plugins folder;
# Run `scripts/build.sh` first if plugin/dist/Atlas is missing.
set -euo pipefail
cd "$(dirname "$0")/.."
REPO="$(pwd)"
DRY=0
[ "${1:-}" = "--dry-run" ] && DRY=1

PLUGINS="${RACK_PLUGINS_DIR:-$HOME/Library/Application Support/Rack2/plugins-mac-arm64}"
BACKUPS="$(dirname "$PLUGINS")/atlas-backups"
SRC="$REPO/plugin/dist/Atlas"
STAMP="$(date +%Y%m%d-%H%M%S)"

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
	echo "Нет собранного плагина в $SRC. Сначала: scripts/build.sh" >&2
	exit 1
fi
if [ ! -d "$PLUGINS" ]; then
	echo "Не найдена папка плагинов Rack: $PLUGINS" >&2
	echo "Запусти VCV Rack 2 хотя бы раз, чтобы он её создал." >&2
	exit 1
fi

run mkdir -p "$BACKUPS"

# 0. Backups that earlier runs left inside the plugins folder: Rack would load them as plugins.
for old in "$PLUGINS"/Atlas.bak-* "$PLUGINS"/AtlasTutor.bak-*; do
	[ -e "$old" ] || continue
	echo "Резервная копия $(basename "$old") → atlas-backups/"
	run mv "$old" "$BACKUPS/$(basename "$old")"
done

# 1. The old tutor plugin goes aside (the new Atlas replaces it).
if [ -e "$PLUGINS/AtlasTutor" ]; then
	echo "Старый плагин AtlasTutor → atlas-backups/AtlasTutor-$STAMP"
	run mv "$PLUGINS/AtlasTutor" "$BACKUPS/AtlasTutor-$STAMP"
fi

# 2. An older Atlas is kept as a backup, then the new one is copied in.
if [ -e "$PLUGINS/Atlas" ]; then
	echo "Прежний Atlas → atlas-backups/Atlas-$STAMP"
	run mv "$PLUGINS/Atlas" "$BACKUPS/Atlas-$STAMP"
fi
echo "Новый Atlas → $PLUGINS/Atlas"
run cp -R "$SRC" "$PLUGINS/Atlas"

# 3. The tutor server (the tutor and progress live there).
echo "Сервер репетитора запускается отдельно, в своём терминале:"
echo "  cd $REPO && npm start"
echo
echo "Резервные копии: $BACKUPS"
echo "Закрой VCV Rack полностью (⌘Q) и запусти снова: плагин читается только при запуске."
echo "Проверка: правый клик по Навигатору → строка «Atlas … · сборка …»."
echo "Добавь модуль Atlas → Навигатор и нажми «Продолжить»."
echo "Что проверить: $REPO/docs/OWNER_CHECKLIST.md"
