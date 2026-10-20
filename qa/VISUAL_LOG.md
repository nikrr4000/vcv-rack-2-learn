# Visual QA log (W9)

The test Rack instance cannot start inside the session's sandbox (see `docs/rack/PROGRESS.md`), so
visual QA runs on the **render harness**: the plugin's own views drawn by the real VCV nanovg +
fontstash with a software back end (`rack/plugin/tests/render/`). Text measuring is the same code
as in Rack, so line breaks, pages and overflow match. What the harness cannot show is listed at
the end; those checks are in `docs/rack/OWNER_CHECKLIST.md` and `rack/scripts/rack-test.mjs`.

## Method

- `render previews <pack> <out> all [--dark]`: every item of every block of all 52 lessons in its
  module (pages, notes, figures, quizzes in four states, tasks with brief / result / hints, bench),
  the Navigator (home, lesson, map), the docked panel (open, collapsed, map), the Tutor module;
  plus one contact sheet per lesson with the Atlas modules of each row as laid out in the patch.
  2,270 images per theme.
- `report.json`: every text area (pages, overflow or scrolling, hyphenated words) and every figure
  with a label cut by «…» or a flow edge drawn through another box.
- **Build gate** (`rack/scripts/build.sh`, since W7): the build fails on any overflowing or
  scrolling text area, any cut figure label and any hidden flow edge.
- Curated sheets in `rack/qa/previews/course/` (one lesson per part, two in dark), figure gallery
  in `rack/qa/previews/figures/`, the pilots in `rack/qa/previews/p1-01/` and `p3-05/`.

## Final state (2026-10-20)

| | light | dark |
| --- | --- | --- |
| images | 2,270 | 2,270 |
| text areas | 1,956 | 1,956 |
| several pages | 0 | 0 |
| overflowing or scrolling | 0 | 0 |
| figures with cut labels or hidden edges | 0 | 0 |

## Findings and fixes

| # | Found | Fix |
| --- | --- | --- |
| 1 | 30 Task modules scrolled (27 «Покрути сам» lists, 3 practice briefs); the compiler fitted Page text only | `fitTask` in `textfit.mjs` models TaskView (title, steps 11.5 pt + «→ see» 10.5 pt italic, brief, «Проверить» bar); the compiler picks 14/16/18/20 HP. Three 6-step lists merged to 5 steps (4.6, 6.3, 7.5) |
| 2 | 17 figures with labels cut by «…» (tables with one long column, 5–7 boxes in one flow row) | Table columns sized by content; flow `two-rows` honours `row: 0|1` and splits by order when one row would be empty; labels and rows set in 4.3 (×2), 5.5, 6.4, 8.5 |
| 3 | 9.2 cover figure: 7 boxes in one row, every label cut, a branching diagram drawn as a chain | rows set explicitly (clock, sequencer, kick above; two Gates with their drums below) |
| 4 | 11 flow figures with an edge drawn straight through the boxes between its ends (fan-outs: one LFO to three targets, a mixer to Audio and Recorder, …) | the template routes such an edge over its row (up, across, down into the target); counted by the harness as «hidden edge» if it ever happens again |
| 5 | Figures from W5 (pilots): ADSR markers, harmonics title, clock label, mixer/stereo labels, cover width, vertical centring, «Ch 1» glued to its number | fixed in W5 (see PROGRESS) |

## Read-through (plan §10.7)

Four fresh subagents (sonnet, no part in writing) read all 52 lessons in order as a beginner and
returned 152 concrete edits (applied by `rack/scripts/apply-edits.mjs`, all matched): wrong or
ambiguous instructions (moving a cable while Scope watches a fixed output), factual slips (−60 %
is not «вдвое слабее»; beats between saws at a fifth; ±5 В vs the ±10 В Audio limit), hedges in
learner text («пока не проверено», «проверь в своей версии»), ⌘S instead of Ctrl+S, terms used
before they are taught. Follow-ups done in the main session: «агент» → «репетитор» everywhere,
VCMixer → «VCA Mix» (the panel name, 34 places), chat-only tasks in part 1 made optional («если
Atlas запущен…»), the old `vcv-N.vcv` file names replaced by the learner copy Atlas keeps (⌘S),
neutral wording about Clocked's run state. Points the readers raised that need Rack itself are in
OWNER_CHECKLIST.

## Not verifiable here (owner checklist / `rack-test.mjs`)

- The modules inside Rack: panel drawing at real zoom levels and on HiDPI, the resize handle,
  tooltips, the theme switch with Rack's «dark panels» setting.
- Pointer rings and leader lines over real module widgets, the pulse on block change.
- View glides between blocks with the docked panel open and collapsed.
- Lesson loading from the Navigator, the learner copy, the audio device carried over.
- `Rack -t 2` screenshots (`node rack/scripts/rack-test.mjs --shots`).
