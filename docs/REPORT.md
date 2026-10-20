# Atlas in Rack — run report

Plan: `docs/rack/VCV_RACK_NATIVE_PLAN.md`. Checkpoint with every task, commit and deviation:
`docs/rack/PROGRESS.md`. Branch `rack-native` (from `vcv-course`), 19 commits, nothing pushed.
Owner's next step: `rack/scripts/owner-install.sh`, then `docs/rack/OWNER_CHECKLIST.md`.

## What was built, per workstream

| WS | Result |
| --- | --- |
| W0 | Orientation; Rack app found in `~/Applications/VCV Rack 2 Free.app` (2.6.6); SDK 2.6.6 in `vcv/.sdk`; `PROGRESS.md`. |
| W1 | Design system «Атлас»: PT Serif / PT Sans / JetBrains Mono (OFL, sources pinned), `rack/course/design-tokens.json` → generated `ThemeTokens.hpp`; light and dark palettes, an accent per part. |
| W2 | Plugin `Atlas` 2.1.0 (8 modules: Навигатор, Страница, Подпись, Схема, Вопрос, Задание, Верстак, Репетитор). Rack-free text engine (markdown subset, Russian line-breaking rules, pagination) with unit tests; host, theme switch, resize handles, paper panels. |
| W3 | Every module view; 12 figure templates; C++ checker with exact Node parity on 102 fixture cases and the new `fixed` kind; Navigator with course map, lesson loading (learner copy, layout hash, audio device carried over, missing-plugin notice), docked panel, pointer overlay, glides; Tutor UI and Atlas client; dev bridge for tests. |
| W4 | Lesson schema, compiler (pack.json + plain-JSON lesson patches, layout presets, demo bus per block, param resolver from display values with live calibration tables), text-fit tool emulating fontstash (200/200 exact parity with C++), validator, build gates, `build.sh` (+ `--test`). |
| W5 | Pilots 1.1 «Звуковая волна» and 3.5 «Первый голос», written by hand; template checks without Rack; live verification of demo blocks; `rack-test.mjs` for the in-Rack QA run. |
| W6 | Tutor runtime: `vcv/tutor/TUTOR.md` (600 words), one warm `claude -p` stream-json process per open lesson with warm-up, 15-minute idle exit, lesson-switch replacement and probe-and-fallback for unknown flags; `/api/rack/*` (health, lesson/open, ask + stream polling, progress); context header; compact log in the lesson's vault note; tutor model/effort in Atlas settings; stub; tests. |
| W7 | The whole course: 50 lessons converted by 10 writer subagents (sonnet), one per part, from the old lessons, cards, snapshots, checks, glossary and quotes; fixes in the main session. |
| W8 | Atlas «Модульный синтез» shows Rack progress (block N of M, learned, last lesson), note links, the tutor's state; «Как заниматься в Rack» page. The old web lesson stays as a «веб-урок» link. |
| W9 | Whole-course QA in both themes with new build gates; read-through by 4 fresh subagents (152 edits) and follow-ups; `rack/qa/VISUAL_LOG.md`. |
| W10 | `rack/scripts/owner-install.sh`, final build, this report, `OWNER_CHECKLIST.md`. |

## Numbers

| | |
| --- | --- |
| Lessons / parts | 52 / 10 |
| Blocks | 576: cover 52, quiz 52, story 112, live 75, anatomy 79, try 52, practice 51, debug 51, recap 52 |
| Pages (Страница items) | 499, each fits one page (0 multi-page, 0 overflow) |
| Quizzes / questions | 104 (probe + recap) / 227, every option with an explanation |
| FAQ items | 1,423 (answered instantly, «из урока») |
| «Объясни проще» texts | 106 |
| Figures | 153 (12 templates) |
| Notes with pointers | 683 |
| Tasks | try 52, practice 51 (the old check files, 3-step hint ladders), debug 51 (`fixed` checks) |
| Package | `rack/plugin/dist/Atlas-2.1.0-mac-arm64.vcvplugin` (1.2 MB) |

## Verification results (final run, 2026-10-20)

| Check | Result |
| --- | --- |
| Validator (`rack/compiler/validate.mjs`) | 52 lessons, 0 errors, 22 warnings (book and site titles in Latin, old-anatomy coverage notes) |
| Compiler | 576 blocks, 0 errors, 0 warnings |
| Build gates (MCP Server on 2600 in every shipped patch, no test settings, one MCP Server per patch, …) | 52 patches, 0 problems |
| Text areas, render harness, light and dark | 1,956 each: 0 overflowing or scrolling, 0 multi-page |
| Figures, render harness | 0 cut labels, 0 flow edges hidden behind boxes |
| C++ unit tests (text engine, checker parity, `fixed` kind) | 14 tests, 0 failed checks |
| Plugin smoke test (app's libRack, every module and widget, data round-trip) | 8 models, 0 failures |
| Text-fit parity Node ↔ C++ | 200/200 exact |
| Compiler tests | 6/6 |
| Template checks (every practice check fails on the empty bench; every debug check fails with its break and passes once repaired) | 102/102 |
| Live verification of demo blocks on the owner's Rack (session row, every module deleted, owner's 9 modules untouched) | 190/256 pass; every param matched its target; all 66 failures are silent outputs, explained below |
| Node checker and VCV tests (`app/server/vcv/*.test.mjs`) | 65/65 |
| All server tests incl. tutor (`app/server/**/*.test.mjs`) | 87/87; tutor runtime 14/14 on test servers 5185/5186 |
| Plugin Atlas client end to end (real `Client.cpp` on the app's libRack against `/api/rack` with the stub) | 8/8 + the exchange in the note log |
| Web e2e of the old VCV flow (`app/scripts/e2e-vcv.mjs`) | no failures |
| `owner-install.sh` | dry run and real run on `HOME=rack/.fakehome` |
| Atlas web screen | checked in the browser on test server 5185 |

Silent outputs in the live verification (66 blocks): MIDI modules without a controller (part 6);
Push and Pulses buttons that need a press; debug blocks whose break silences the chain; SEQ 3's
1 ms triggers missed by 100 ms sampling; Octave/8vert at 0 V; Clocked built over REST did not tick
(see UNVERIFIED).

## Visual QA

Render harness: the plugin's views with the real VCV nanovg + fontstash and a software back end,
2,270 images per theme. Found and fixed in W9: 30 scrolling Task modules (the compiler now models
TaskView and picks 14–20 HP), 17 figures with cut labels (content-width table columns; explicit
flow rows), 11 flow figures with edges drawn through boxes (routed over the row), one diagram drawn
as a chain. Details: `rack/qa/VISUAL_LOG.md`; sheets: `rack/qa/previews/`.

## Tutor latency

Stub path (5 runs): process start → warm-up answered 26–27 ms; message → first chunk 0.3 ms, whole
reply 0.3–0.4 ms; the plugin polls every 250 ms, so a chunk reaches the screen ≤ 250 ms after the
server has it. With the real model, process start, MCP connection and the cached system prompt
happen at lesson open (warm-up), so a question pays only the model's time to first token; replies
are short by template (≤ 60–90 words) and stream as they come. Not measured with a real model (the
session may not run `claude`).

## UNVERIFIED and why

The test Rack instance cannot start inside the sandbox (it hangs at CoreAudio init), and the
session may not run `claude`. Everything below is on the owner's checklist; `rack-test.mjs` runs
the in-Rack QA in one command outside the sandbox.

- The plugin inside Rack: drawing at real zoom, docked panel, pointer rings, glides, lesson loading
  from the Navigator, learner copy and layout-hash handling, audio device carry-over, progress
  sync, the dev bridge itself.
- The real tutor process: CLI flags (fallback is built in and logged to
  `APP_STATE_DIR/rack-tutor.jsonl`), first-reply time, reply shape on the real model, MCP read
  tools. Memory files (CLAUDE.md) may be loaded by the CLI despite `--setting-sources local`.
- Clocked: whether a new Clocked runs on its own, and which side of `Clk ratio` divides (the clock
  did not tick when built over REST; lessons now tell the learner to press Run if it stands still).
- Rack facts the writers and readers could not confirm from the fact base: the modifier key for
  fine knob moves, whether Viz shows a number, MIDI to CV having an MPE mode, Mutes buttons
  latching, Recorder menu items and file naming.

## Deviations from the plan (with reasons)

- Rack app in `~/Applications`, not `/Applications`.
- No test Rack instance and no `Rack -t` screenshots (sandbox); replaced by the render harness and a
  smoke test with the app's libRack.
- Every practice row gets its own Audio module (checks need a free «To "device output 1"»); the demo
  bus feeds the row-0 Audio.
- Practice probes sampled on the UI thread in the host's `step()`, not in `process()` (no engine
  pointers to modules a patch load can delete).
- Learner copies saved with `APP->patch->save`; the layout hash is kept in a side file.
- Tutor `claude` path is a separate setting (`tutorClaudePath`, env `RACK_TUTOR_CLAUDE`) so the
  fixtures can point only the tutor at its stub.
- The tutor's patch line comes from the plugin (bench and given modules); without it the server
  summarises the whole patch from REST.
- 0.4 explains the MCP Server panel in a story block instead of an anatomy block with a second MCP
  Server (two servers in one patch could fight for port 2600).
- Task module width is chosen by the compiler (14–20 HP) instead of a fixed 14 HP.
- The read-through ran as 4 subagents by part (context size), not one.
- `app/dist` was rebuilt to check W8 in the browser; if the owner's running service serves this
  checkout, it serves the new UI (it degrades gracefully until the service restarts).
- npm installs used the public registry with a temporary cache (the owner's npm config points at an
  internal registry).

## Краткое резюме

Курс «Модульный синтез» целиком переехал в VCV Rack: плагин Atlas (8 модулей), 52 урока, 576 блоков,
репетитор с тёплым процессом на урок, прогресс в Rack и в заметках Obsidian. Все детерминированные
проверки зелёные. Поведение внутри Rack и ответы настоящей модели проверить в песочнице было
нельзя — это первые пункты `OWNER_CHECKLIST.md`.
