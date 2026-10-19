# Atlas in Rack — progress checkpoint

Read this first after a compaction or restart; continue from the first task that is not `DONE`. Plan: `docs/rack/VCV_RACK_NATIVE_PLAN.md`.

- Branch: `rack-native`, created from `vcv-course` (head `080fe40`).
- Rack app: **`~/Applications/VCV Rack 2 Free.app`** (not `/Applications`; version 2.6.6). Binary: `"$HOME/Applications/VCV Rack 2 Free.app/Contents/MacOS/Rack"`.
- Toolchain: Xcode CLT `/Library/Developer/CommandLineTools`, Apple clang 21, GNU Make 3.81, node 24.11.1, Rack SDK 2.6.6 in `vcv/.sdk/Rack-SDK`. `xcrun_db` cache warnings are harmless (sandbox blocks `/var/folders/.../T`).
- Live Rack (:2600): MCP Bridge 1.3.0, 48 kHz. The owner's agent patch now has 9 modules (Clocked, MCP Server, AtlasTutor/Tutor, 2× VCO, Octave, Mixer, Scope, Core/AudioInterface) and cables between them — **never touch**. Session row = below y=38760 (use `new_row` from `/rack/layout`).
- Atlas service: port 5175 (owner's, never touched). Session test servers: 5185, 5186. Test Rack MCP: 2610, dev bridge: 2700.

## Tasks

| WS | Task | Status | Commit | Notes |
| --- | --- | --- | --- | --- |
| W0 | orientation, inventory, PROGRESS.md, branch | DONE | | Rack app in ~/Applications (deviation recorded) |
| W1 | 1.1 fonts (PT Serif, PT Sans, JetBrains Mono) + SOURCES.md | TODO | | |
| W1 | 1.2 Theme.hpp + design-tokens.json | TODO | | |
| W2 | 2.1 plugin skeleton (plugin.json, Makefile, plugin.cpp/hpp) | TODO | | |
| W2 | 2.2 text engine + unit tests | TODO | | |
| W2 | 2.3 host, theme switch, resize, panel drawing, previews | TODO | | |
| W3 | 3.1 checker fixtures | TODO | | |
| W3 | 3.2 Page | TODO | | |
| W3 | 3.3 Note + pointer overlay | TODO | | |
| W3 | 3.4 Figure + templates | TODO | | |
| W3 | 3.5 Quiz | TODO | | |
| W3 | 3.6 Task + Bench | TODO | | |
| W3 | 3.7 C++ checker (+ `fixed` kind in Node) | TODO | | |
| W3 | 3.8 Navigator, course map, lesson loading, docked panel | TODO | | |
| W3 | 3.9 Tutor module + panel UI + Atlas client | TODO | | |
| W3 | 3.10 dev bridge | TODO | | |
| W4 | 4.1 schema, compiler, presets, param resolver | TODO | | |
| W4 | 4.2 text-fit tool + parity test | TODO | | |
| W4 | 4.3 validator | TODO | | |
| W4 | 4.4 build.sh (+ --test), gallery.vcv | TODO | | |
| W5 | 5.1 pilot p1-01 + full QA | TODO | | |
| W5 | 5.2 pilot p3-05 | TODO | | |
| W6 | tutor runtime, /api/rack/*, stub, tests | TODO | | |
| W7 | parts 0–9 | TODO | | |
| W8 | Atlas companion | TODO | | |
| W9 | whole-course QA, read-through | TODO | | |
| W10 | owner-install.sh, final build, REPORT, OWNER_CHECKLIST | TODO | | |

## Deviations

- The Rack app bundle lives in `~/Applications`, not `/Applications`; it is read (and run for the test instance) from there.

## Open issues

## Background processes

None.

## Session-created module ids (live Rack)

None alive.
