#!/bin/bash
# Atlas build: theme tokens → validate → compile → plugin → tests → package.
#   rack/scripts/build.sh           shipped build: rack/plugin/res/course (MCP on 2600), make dist
#   rack/scripts/build.sh --test    test build: rack/build/test-res (MCP on 2610), a test copy of the
#                                   plugin in rack/.rack-test/plugins-mac-arm64/Atlas (never the owner's folder)
set -euo pipefail
cd "$(dirname "$0")/../.."
REPO="$(pwd)"
TEST=0
[ "${1:-}" = "--test" ] && TEST=1
quiet() { grep -v -e xcrun_db -e "no longer supported" -e "^\s*|" -e "^\s*^" -e "warning: In file included" || true; }

echo "== theme tokens"
node rack/compiler/gen-theme.mjs
echo "== validate"
node rack/compiler/validate.mjs | tail -1
echo "== compile"
if [ $TEST = 1 ]; then node rack/compiler/compile.mjs --test | tail -1; else node rack/compiler/compile.mjs | tail -1; fi
echo "== gates"
node rack/compiler/gates.mjs $([ $TEST = 1 ] && echo --test)
echo "== plugin"
make -C rack/plugin -j8 2>&1 | quiet | grep -E "error|warning:" && { echo "plugin build has errors or warnings"; exit 1; } || true
test -f rack/plugin/plugin.dylib
echo "== tests"
make -C rack/plugin/tests unit 2>&1 | quiet | tail -1
make -C rack/plugin/tests smoke PACK="$REPO/rack/plugin/res/course/pack.json" 2>&1 | quiet | tail -1
echo "== text areas (render harness, every module of every lesson)"
make -C rack/plugin/tests render 2>&1 | quiet | grep -E "error" && exit 1 || true
rm -rf rack/build/previews-qa && mkdir -p rack/build/previews-qa
rack/plugin/tests/bin/render previews rack/plugin/res/course/pack.json rack/build/previews-qa all | tail -1
node -e 'const r=require(process.argv[1]);const o=Object.keys(r).filter((k)=>r[k].overflow);if(o.length){console.error("overflowing or scrolling:",o.join(", "));process.exit(1)}' "$REPO/rack/build/previews-qa/report.json"
if [ $TEST = 1 ]; then
	DEST="$REPO/rack/.rack-test/plugins-mac-arm64/Atlas"
	rm -rf "$DEST" && mkdir -p "$DEST"
	cp -R rack/plugin/plugin.json rack/plugin/plugin.dylib rack/plugin/res "$DEST/"
	rm -rf "$DEST/res/course" && cp -R rack/build/test-res/course "$DEST/res/course"
	echo "test plugin → $DEST"
else
	echo "== package"
	make -C rack/plugin dist 2>&1 | quiet | grep -E "error" || true
	ls rack/plugin/dist/*.vcvplugin
fi
echo "build OK"
