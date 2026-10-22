#!/bin/bash
# Atlas build: theme tokens → validate → compile → plugin → tests → package.
#   scripts/build.sh           shipped build: plugin/res/course (MCP on 2600), make dist
#   scripts/build.sh --test    test build: build/test-res (MCP on 2610), a test copy of the
#                                   plugin in .rack-test/plugins-mac-arm64/Atlas (never the owner's folder)
set -euo pipefail
cd "$(dirname "$0")/.."
REPO="$(pwd)"
TEST=0
[ "${1:-}" = "--test" ] && TEST=1
quiet() { grep -v -e xcrun_db -e "no longer supported" -e "^\s*|" -e "^\s*^" -e "warning: In file included" || true; }

# First run: the Rack SDK and the compiler's packages.
[ -f .sdk/Rack-SDK/plugin.mk ] || scripts/get-sdk.sh
[ -d compiler/node_modules ] || (cd compiler && npm ci --silent)

echo "== theme tokens"
node compiler/gen-theme.mjs
echo "== validate"
node compiler/validate.mjs | tail -1
echo "== compile"
if [ $TEST = 1 ]; then node compiler/compile.mjs --test | tail -1; else node compiler/compile.mjs | tail -1; fi
echo "== gates"
node compiler/gates.mjs $([ $TEST = 1 ] && echo --test)
echo "== plugin"
# Build marker shown in the Navigator's context menu (src/BuildInfo.hpp, not in git).
BUILD_ID="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)$(git diff --quiet HEAD -- plugin/src 2>/dev/null || echo +)  $(date '+%Y-%m-%d %H:%M')"
printf '#pragma once\n#define ATLAS_BUILD "%s"\n' "$BUILD_ID" > plugin/src/BuildInfo.hpp
make -C plugin -j8 2>&1 | quiet | grep -E "error|warning:" && { echo "plugin build has errors or warnings"; exit 1; } || true
test -f plugin/plugin.dylib
echo "== tests"
make -C plugin/tests unit 2>&1 | quiet | tail -1
make -C plugin/tests smoke PACK="$REPO/plugin/res/course/pack.json" 2>&1 | quiet | tail -1
echo "== text areas (render harness, every module of every lesson)"
make -C plugin/tests render 2>&1 | quiet | grep -E "error" && exit 1 || true
rm -rf build/previews-qa && mkdir -p build/previews-qa
plugin/tests/bin/render previews plugin/res/course/pack.json build/previews-qa all | tail -1
node -e 'const r=require(process.argv[1]);const o=Object.keys(r).filter((k)=>r[k].overflow||r[k].truncated);if(o.length){console.error("overflowing text or cut figure labels:",o.join(", "));process.exit(1)}' "$REPO/build/previews-qa/report.json"
if [ $TEST = 1 ]; then
	DEST="$REPO/.rack-test/plugins-mac-arm64/Atlas"
	rm -rf "$DEST" && mkdir -p "$DEST"
	cp -R plugin/plugin.json plugin/plugin.dylib plugin/res "$DEST/"
	rm -rf "$DEST/res/course" && cp -R build/test-res/course "$DEST/res/course"
	echo "test plugin → $DEST"
else
	echo "== package"
	make -C plugin dist 2>&1 | quiet | grep -E "error" || true
	ls plugin/dist/*.vcvplugin
fi
echo "build OK"
