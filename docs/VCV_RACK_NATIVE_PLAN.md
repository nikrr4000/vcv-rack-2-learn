# Atlas in Rack — the course moves into VCV Rack

A self-contained task for one long autonomous Claude Code session. Read it fully before starting. Everything you need is in this file, in `docs/vcv/VCV_COURSE_RESEARCH.md` (sources), and in what the previous run built (`docs/vcv/REPORT.md`, `facts/`).

## 0. What the owner wants

The owner learns modular synthesis in VCV Rack 2 Free from zero. The previous run built a course of 52 lessons inside the Atlas web app plus a small «Tutor» module in Rack. The owner tried it and found:

- the lesson reads better on the web page than in Rack, so he keeps switching windows;
- the Tutor module cuts text off and has no way to give his own answer;
- the agent takes too long to give an ordinary answer;
- he does not like how the product looks.

**The goal of this run: the owner learns the whole course without leaving VCV Rack.** Theory and practice both live in the rack. A lesson is a narrative laid out in rows. Each row is one block: a text page, the live modules it talks about (a Scope showing the wave, a VCO to turn), captions pointing at the knobs, a figure. The owner moves from block to block with one button. Questions are answered instantly from prepared material; the model is called only for free-form questions and answers, and it answers fast and short.

The owner gives full freedom over the look and depth. He will come back after the run and check that it works. Make it complete, consistent and beautiful.

## 1. Language

- **How you work:** think in English. Your chat messages to the owner are in Russian. Work files (this plan, code, comments, commit messages, reports, schemas, templates' instructions) are in English.
- **What you build:** everything the learner sees is in Russian — lesson text, captions, quizzes, hints, FAQ answers, tutor replies, UI labels in Rack and in Atlas. Module, knob and port names stay exactly as Rack prints them (English), with a Russian gloss on first use in a lesson.
- Russian text inside this plan (labels, examples, templates) is product text: use it as written.

## 2. Run mode

- **One autonomous session, no owner involvement.** Do not ask questions; do not wait for approval. On a failure: record it in `docs/PROGRESS.md`, take the conservative option, continue.
- **No stage gates.** Work through the workstreams in order (section 9), but do not stop between them for verification by the owner. Self-verify continuously with the deterministic tools this plan asks you to build (section 10). The only full stop is the end of the plan.
- **Checkpoint.** Keep `docs/PROGRESS.md` current after every task: workstream, task, status, commit hash, open issues, known follow-ups. If the context is compacted or the session restarts, read `PROGRESS.md` first and continue from the first unfinished task. Never redo a finished task.
- **Save context with subagents.** Content conversion (W7) runs one subagent per course part; module code can be split by module. Each subagent gets exactly the files it needs and returns a summary of at most 15 lines. Re-run validators in the main session after every subagent; send failures to a new fix-up subagent.
- **Long-running processes** (Atlas test server, test Rack instance, tutor stub) are started as background tasks of this session or with a self-exit timer (`--idle-exit 20m`), so they can always be stopped. The previous run could not `kill` a process started by an earlier command; do not rely on `kill` across commands.
- **Git.** Branch `rack-native` from `vcv-course`. One commit per task. Never `git push`, never touch remotes.
- **No nested `claude` runs by the session itself.** The tutor runtime is tested with a stub `claude` (section 7.6). A real lesson with the model is part of the owner's final check.

## 3. Safety boundary (unchanged in spirit)

The session runs in the Claude Code Bash sandbox configured in `.claude/settings.local.json` (the owner updates it before the run, section 13). Rules:

1. **Stay inside `REPO`.** All writes go to `REPO` (`~/Documents/personal/learn-with-claude`). No writes to the vault, `~/Library`, `~/.claude`, or anywhere else. Nothing under any `.claude/` directory is written by you — not even inside `REPO` (the auto-mode classifier refused that last time). Configuration you need goes to normal folders (`tutor/`, `rack/…`) and is passed by flags.
2. **Read-only exceptions:** the owner's Rack plugins folder `~/Library/Application Support/Rack2/plugins-mac-arm64/` may be read (to copy plugins into the test instance). Nothing else outside `REPO` is read except the Rack app bundle in `/Applications`.
3. **Network:** only the hosts in the sandbox allowlist. Package downloads come from npm and GitHub; GitHub sources come as tarballs from `codeload.github.com` (nested `git clone` fails in the sandbox).
4. **The owner's live Rack (port 2600)** is used only to verify standard-module behaviour (section 10.5), exactly as the previous run did: modules go into a separate row, every module and cable you create is deleted, nothing else is touched, no patch is saved, loaded or cleared.
5. **The test Rack instance** (section 8) runs from the app bundle with a user folder inside `REPO`; you may start, restart and stop it freely.
6. **Never work around the sandbox or a deny rule.** A refusal is a result: record it, use the fallback, list it in the final report.
7. **No GUI automation** (no scripted clicks or keystrokes in other apps).
8. **Copyright.** Lesson text is written in your own words. At most one quote per source per lesson, under 15 words, only quotes already verified in `facts/quotes.json`. Figures are drawn from templates (section 6.5), not copied from manuals.

## 4. What exists (read before designing)

- `docs/vcv/REPORT.md` — the previous run: what was built, the manual follow-ups, live findings.
- `facts/` — 52 lessons (`lessons/*.md`, written as notes for a teaching agent, ~77 000 words), 51 module cards (`modules/*.md`), 51 live snapshots (`modules/_snapshot/*.json`: params with names, ranges, `display` at min/mid/max; inputs; outputs), 51 checks (`checks/*.json`, port and param names verified live), glossary (186 terms), `sources.md`, `quotes.json`, `course.json`.
- `scripts/` — `snapshot-modules.mjs`, `validate-course.mjs`, `live-check.mjs` (builds reference patches on the live Rack and runs checks), `seed-notes.mjs`, `merge-glossary.mjs`, `lib/`.
- `app/server/vcv/` — `rack.mjs`, `course.mjs`, `checker.mjs`, `notes.mjs`, `tutor.mjs`; the lesson runner `app/server/lessons.mjs` (one `claude -p` per turn, `--resume`, relay MCP for questions).
- `vcv/tutor-plugin/` — the old `AtlasTutor` plugin (one 30 HP module). It is **replaced** by the new plugin.
- `vcv/.sdk/Rack-SDK` — Rack SDK 2.6.6 for mac arm64 (it has `rack::network::requestJson`).
- Live findings worth keeping: Fundamental modules report output voltage only when the output is connected; a new SEQ 3 and a new Clocked start running; 8vert shows volts with an empty input and percent with a cable; see the report for the rest.

Everything content-related is **reused**: the old lessons are the raw material for the new ones (mapping in Appendix D), cards and snapshots stay the source of truth for module facts, checks are reused by the in-Rack checker.

## 5. The product

### 5.1 The owner's experience (acceptance story)

1. The owner installs the new plugin with one command and restarts Rack (section 13). In the module browser, the brand **Atlas** has a module **Навигатор**. He adds it to an empty patch.
2. The Navigator shows the course: «Модульный синтез с нуля», parts and lessons with progress. He presses «Продолжить» (or picks a lesson on the course map).
3. The lesson patch opens. The view glides to row 1: a wide page with the lesson title, what he will learn and what he will build. A docked panel on the right edge of the Rack window shows the lesson outline, the block counter «Блок 1 из 7» and the tutor.
4. He presses «Дальше →». The view glides to row 2: a page explains the idea in four short paragraphs, next to it a VCO already patched into a Scope and an Audio module, captions point at the knobs that matter, a figure shows the harmonics of each waveform. He turns the knob the caption suggests and sees the wave change.
5. Row 3 is a quiz inside the rack. He picks an answer, the module shows the verdict and why. «Свой ответ» lets him type his own answer; the tutor grades it in a few seconds.
6. Row 5 is practice: a task card with steps, an empty bench between two markers, and «Проверить». He patches on the bench; the check runs instantly inside Rack and shows which points pass and a hint for the first failing one.
7. Any time, the panel offers ready questions for the current block («Почему пила звучит ярче?»). They are answered instantly from prepared text. He can also type any question; the tutor answers in Russian within seconds, streaming, knowing which block he is on and what is patched.
8. At the end of the lesson he presses «Урок пройден». Progress is saved in Rack and synced to Atlas, which updates the Obsidian note (`status: learned`). The Navigator offers the next lesson; his audio device carries over; his copy of the lesson patch (with his practice) is kept and reopens where he left it.

Everything happens inside Rack. The Atlas web app keeps working as a companion (course overview, notes), but it is never required to learn.

### 5.2 Product principles

- **On rails, with a living tutor.** The narrative, demos, quizzes, hints and FAQ are prepared in advance and work offline. The model answers only what preparation cannot: free questions, free answers, reviewing a practice patch in words.
- **Real modules are the illustrations.** A wave on a real Scope beats a picture of a wave. Figures are used only where no module can show the idea (harmonic spectrum, ADSR shape, signal-flow map).
- **One idea per block, one screen per block.** A block fits one view at a comfortable zoom. Text never overflows: it paginates.
- **Nothing is cut off.** Every text area in the plugin wraps, scrolls or paginates; nothing is clipped silently.
- **Fast by design.** Instant local answers first; model answers short, streamed, on a fast model.

## 6. Design

### 6.1 Visual identity «Атлас»

A calm editorial look that stands apart from busy hardware-style panels: printed atlas pages placed into a rack.

- **Paper and ink.** Light theme: panel `#F3EEE4` (warm paper), ink `#1E1C19`, secondary ink `#6B655C`, hairlines `#D9D1C3`. Dark theme (when Rack's «Prefer dark panels» is on, `settings::preferDarkPanels`): panel `#1C1D1F`, ink `#ECE6DA`, secondary `#9A938A`, hairlines `#34363A`.
- **One accent per course part** (muted, readable on both themes): part 0 `#8A8F98` slate, 1 `#C0583B` vermilion, 2 `#D08C2F` ochre, 3 `#5E8C61` sage, 4 `#3F7CAC` cobalt, 5 `#8A5FA8` violet, 6 `#B5476F` raspberry, 7 `#2F8F8A` teal, 8 `#A86B3C` umber, 9 `#4A4E9C` indigo. Used for the top rule of each panel, the block number, the active state of buttons, quiz verdict accents — never for body text.
- **Typography (all OFL, all with Cyrillic):** headings **PT Serif Bold**, body **PT Sans** (Regular, Bold, Italic), numbers and values **JetBrains Mono**. Bundle the TTF files and their licences in `res/fonts/`. Download them from GitHub (`google/fonts` repository on GitHub for PT Serif and PT Sans; `JetBrains/JetBrainsMono` releases or tree for JetBrains Mono): list the folders via `api.github.com`, then fetch with `raw.githubusercontent.com`; record exact paths and commit hashes in `res/fonts/SOURCES.md`.
- **Type scale (at zoom 1, in Rack pixels):** page title 17, block heading 14.5, body 12, small/caption 10.5, mono values 11. Body line height 1.42. Paragraph spacing 7. Panel padding 12 (left/right), 14 (top), 12 (bottom).
- **Panel anatomy.** Every Atlas module draws its own panel in code (no SVG text): paper fill, a 3 px accent rule at the top, a small header line («3.5 · Блок 2» in mono, secondary ink), content, a footer line (page dots, controls). No screws, no faux hardware. Corners square (rack modules are rectangles).
- **Controls.** Flat text buttons with a hairline border; hover darkens the border; active fills with the accent at 15 % and accent text. Minimum hit area 18 × 18 px at zoom 1.
- **Icons.** None from icon fonts; simple vector glyphs drawn with nanovg (arrow, check, cross, question, dot).
- **Motion.** View transitions between blocks glide over 350 ms with ease-in-out; nothing else animates except the tutor's typing indicator and streaming text.

Write the tokens once in `plugin/src/ui/Theme.hpp` and `course/design-tokens.json` (the compiler's text-fit tool reads the same numbers).

### 6.2 The plugin `Atlas`

A new plugin, slug `Atlas`, brand «Atlas», licence `proprietary` (personal use; do not copy code from GPL plugins — learn techniques only), source in `plugin/`. It supersedes `AtlasTutor` (its source stays in the repo for reference; the owner's install script moves the old folder to a backup).

Modules (Russian names in the module browser; slugs in English):

| Slug | Name | Width | Purpose |
| --- | --- | --- | --- |
| `Navigator` | Навигатор | 10 HP | Course home and lesson controls: course map, «Продолжить», block counter, «← Назад» / «Дальше →», «Урок пройден», Atlas connection light. Owns the docked panel and the host (6.3). |
| `Page` | Страница | resizable 8–40 HP (default from the lesson) | Narrative text: heading, paragraphs, lists, bold/italic, inline values, glossary terms with hover tooltips. Paginates with dots and arrows when text does not fit. |
| `Note` | Подпись | resizable 4–12 HP | A short caption (≤ 40 words) with optional pointer rings/leader lines to targets on other modules (knobs, ports, lights). |
| `Figure` | Схема | resizable 8–24 HP | A procedural figure from a template (6.5) plus a caption. |
| `Quiz` | Вопрос | 14 HP | One question, 2–4 options + «Не знаю», local grading with per-option explanation, «Свой ответ» → tutor grading. Several questions in one module page through with dots. |
| `Task` | Задание | 14 HP | Practice brief (numbered steps), «Проверить», per-item results with ladder hints, a status light. Also used for «Покрути сам» (steps without a check). |
| `Bench` | Верстак | 2 HP (left and right marker) | Marks the practice area in a row. The check looks only at modules between the two markers of the same row (plus the modules the task names as given). |
| `Tutor` | Репетитор | resizable 14–30 HP | The tutor as a module, for owners who prefer it in the rack. Same engine and UI as the docked panel. |

Common rules for all modules:

- Content is **not** stored in the patch. A module stores only references in `dataToJson`: `{"lesson": "p3-05-first-voice", "block": "b2", "item": "page-1"}` and per-learner state (current page, quiz answers, check result). Text comes from the course pack (6.4). Fixing a typo in the pack fixes every patch.
- Width is set from the lesson but the learner can resize (`Page`, `Note`, `Figure`, `Tutor`): implement a resize handle like the one Core «Blank» uses.
- Every text area uses one text engine (6.3.3) — wrap, paginate or scroll; never clip.
- Every module renders a meaningful **preview** when `module == nullptr` (module browser and `Rack -t` screenshots): a sample of real course text chosen by the env var `ATLAS_PREVIEW` if set (section 10.4), otherwise a fixed sample. Previews never touch `APP->scene`, the host or the network; they read the pack through the plugin-level singleton (6.3.1).
- Page layout happens synchronously inside `draw()` (the text engine is fast); nothing depends on a later frame.
- Modules process no audio. `process()` does only what is needed (lights).

### 6.3 Inside the plugin

#### 6.3.1 The host

Two levels of shared state:

- **Plugin-level singletons** in `plugin.cpp`, created lazily on first use and usable everywhere, previews included: the course pack (loaded from `res/course/pack.json`), Atlas settings (`asset::user("Atlas/settings.json")`: `atlasUrl`, default `http://127.0.0.1:5175`, overridden by the env var `ATLAS_URL`), and the Atlas client.
- **`AtlasHost`**, a widget added to `APP->scene` by the first Atlas module widget created with `module != nullptr` (any type, not only the Navigator). Add it with `APP->scene->addChildBelow(host, APP->scene->menuBar)` so it stays under the menu bar and the module browser. Scene children survive patch loads; module widgets do not. The host owns everything that must outlive a patch:

- the action queue: patch loads, view glides, saves — never performed from inside a module's event handler (loading a patch destroys that module); the host runs queued actions in its `step()`;
- the course pack (loaded once from `res/course/pack.json`);
- learner progress (`asset::user("Atlas/progress.json")`) and saved practice patches (`asset::user("Atlas/patches/")`);
- the Atlas client (HTTP, background thread, 6.3.5);
- the docked panel (6.3.4);
- the pointer overlay (6.3.6).

Lifetime rules: the host and the overlay never cache `ModuleWidget*`, `ParamWidget*` or `PortWidget*` across frames. They store module ids and names and resolve them every frame (`APP->scene->rack->getModule(id)` → `getParam(id)` / `getOutput(id)` / `getInput(id)`); a missing target is skipped. Background threads (Atlas client, dev bridge) never touch widgets, nanovg, the engine or the patch manager: they push requests into the host's queue and read snapshots the host publishes in `step()`.

Rack APIs to use (verify every signature against the SDK headers in `vcv/.sdk/Rack-SDK/include` before relying on it): `APP->patch->load(path)`, `APP->patch->loadAction(path)`, `APP->patch->save(path)`, `APP->patch->path`; `APP->scene->rack` (`RackWidget`: `getModule(id)`, `getModules()`, module boxes); `APP->scene->rackScroll` (`RackScrollWidget`: `zoomToBound(math::Rect)`, `getZoom()`, `setZoom()`, `getGridOffset()`, `setGridOffset()`); `APP->window->loadFont(path)`; `rack::plugin::getPlugin(slug)`; `settings::preferDarkPanels`; `asset::plugin(pluginInstance, …)`, `asset::user(…)`. Methods marked `PRIVATE` in the headers are not available to plugins.

#### 6.3.2 Lesson loading

- Lesson patches ship in `res/course/patches/<lesson>.vcv`. The compiler writes plain JSON (Rack 2 loads a file without the zstd header as a legacy JSON patch) with `"version"` set to the Rack version found in W0; if a test load shows any problem with plain JSON, write a real `.vcv` (tar + zstd) from Node instead.
- Every patch carries `atlas: {lesson, layoutHash}` in the Navigator's data. A saved learner copy whose `layoutHash` differs from the shipped template is moved to `Atlas/patches/old/` and replaced by the new template (the learner's progress in `progress.json` is kept).
- Opening a lesson, all inside the host's `step()`:
  1. If the current patch is a course patch, save it with `APP->patch->save(path)` to `Atlas/patches/<lesson>.vcv`. If it is **not** a course patch (for example the owner's agent patch), save it first to `Atlas/patches/_previous-<timestamp>.vcv` and tell the learner where it went.
  2. Read the current `Core/AudioInterface2` module's data (`dataToJson`) — the audio device.
  3. Pick the source: the learner's saved copy if present and current, otherwise the template from `res/`. Read it as JSON, put the saved audio data into its `Core/AudioInterface2` module's `data`, write the result to `Atlas/patches/<lesson>.vcv`.
  4. Check that every module in the patch exists: `rack::plugin::getModel(pluginSlug, modelSlug)` for each one. If any is missing, do not load (Rack would show a blocking dialog); show «Нужен плагин …: подпишись в VCV Library и перезапусти Rack».
  5. Load with `APP->patch->load(path)` inside `try/catch` (never `loadAction`, which shows a dialog on failure), then set `APP->patch->path = path` so Ctrl+S saves the learner's copy, never the template.
  6. Glide to the saved block or block 1.
- A patch without a Navigator still works: each module reads its own references and creates the host; the docked panel appears only when a Navigator exists, and the `Tutor` module provides the tutor without it.

#### 6.3.3 Text engine

One engine for all text, in pure C++ with no Rack dependency except a thin nanovg adapter, so it can be unit-tested (10.3):

- Markdown subset: `#` heading, paragraphs, `-` lists, `1.` lists, `**bold**`, `*italic*`, `` `value` `` (mono), `[[термин]]` (glossary term: dotted underline, tooltip with the definition), `→` arrows, line breaks. Nothing else.
- Layout: greedy line breaking by measured glyph widths (`nvgTextBounds`/`nvgTextGlyphPositions` through the adapter), Russian hyphenation off, no word splits except words longer than a line (break with a hyphen).
- Pagination: blocks never split across pages unless a single paragraph is taller than a page; then split at a line.
- Scroll mode (tutor): smooth wheel scrolling, scrollbar, «вниз ↓» chip when new text arrives out of view.
- **The runtime is the authority:** `pack.json` stores raw block text, and the plugin paginates it for the current width and zoom. The compiler's text-fit tool (10.2) implements the same algorithm in Node with the same fonts (`opentype.js`, kerning off — fontstash uses only the old `kern` table) and tokens, with a 3 % width margin and one spare line, and reports warnings and page counts. Cross-check both implementations on a corpus (10.3), allowing a difference of ±1 line per paragraph.

#### 6.3.4 The docked panel

A panel docked to the right edge of the Rack window (a scene child, above the rack, below menus). It holds: lesson title and part accent, the outline of blocks (click to glide), the block counter with «← Назад» / «Дальше →», and the tutor (6.3.5). Width 360 px, collapsible to a 28 px strip with one click; state remembered. When the panel is open, view glides leave it out of the visible rect (`zoomToBound` with the panel width subtracted). If the panel misbehaves on some Rack version, the `Tutor` module and the Navigator's own buttons still provide every function.

#### 6.3.5 Tutor UI and client

- Conversation: bubbles (learner right, tutor left, plain text with the markdown subset), streaming text with a typing indicator, scroll mode from 6.3.3, copy-on-right-click.
- Input: multi-line field (wraps; Enter sends; Shift+Enter new line; grows to 5 lines, then scrolls). Placeholder «Спроси про этот блок…».
- Quick actions under the input: «Объясни проще», «Пример», «Подсказка», «Проверь мой патч». Above them, **FAQ chips** for the current block (from the pack): clicking one shows the prepared answer instantly as a tutor bubble marked «из урока» — no model call.
- Quiz free answers («Свой ответ» in `Quiz`) go through the same client with kind `grade`.
- Offline: if Atlas is not reachable, chips and local content work; free input shows «Репетитор недоступен: Atlas не запущен» and offers FAQ.
- Client: HTTP to `atlasUrl` from the plugin settings (6.3.1; editable in the context menu of the Navigator and the Tutor, stored in `Atlas/settings.json`; env `ATLAS_URL` wins), always on a background thread, never in `process()` or the UI thread. Use `rack::network::requestJson`. Streaming by short polling (`GET …/stream/:id?after=n` every 250 ms while a reply is in progress).

#### 6.3.6 Pointers

`Note` targets (`{module: <ref>, param|input|output|light: <name>}`; the compiler resolves refs to module ids and names to indices and stores both in the pack) are drawn by an overlay widget added as a child of `APP->scene->rack`, so it scrolls and zooms with the rack: a 2 px accent ring around the target widget's box and an optional leader line from the note. Highlights can pulse once when the block becomes active. If a target is missing, the note shows the text without a pointer and logs the miss (dev bridge reports it).

#### 6.3.7 The checker in Rack

Port `server/checker.mjs` semantics to C++ (`plugin/src/check/`): modules by plugin/slug with `ref` assignment search, cables by port names, params by display ranges, probes by sampling output voltages (on the engine thread via the module's own `process()` sampling into a ring buffer, read by the UI). The check data is in the pack. Scope of a check: in a `practice` block, the modules between the row's `Bench` markers plus the modules listed in the block's `given` list (block level only); in a `debug` block, the block's own modules (their ids are recorded in the pack). Add a check kind `fixed` (`expect_cable` / `expect_param`) to **both** the Node checker and the C++ checker, with fixtures. Results are shown in `Task` with hint ladders (6.4). The C++ checker must agree with the Node checker on every reference case (10.3).

#### 6.3.8 Dev bridge (testing only)

When the env var `ATLAS_DEV_PORT` is set, the host serves a tiny HTTP API on `127.0.0.1:<port>` (cpp-httplib, single header, MIT): `GET /state` (lesson, block, panel state, overflow report of every text area drawn since the last `goto`, missing pointer targets, check results), `POST /open {lesson}`, `POST /goto {block}`, `POST /check`, `POST /quiz {item, option}`, `POST /quit` (closes Rack from the host's `step()`). Handlers run on httplib threads: they enqueue the action for the host, wait on a future with a 10 s timeout, and serve `/state` from the host's last published snapshot. Overflow for a block counts only after `POST /goto` and two drawn frames (Rack does not draw rows out of view). Never compiled out, never active without the env var. Used only by the automated tests in section 10.

### 6.4 The course pack

Source of truth for content: `course/lessons/<id>.yaml`, one file per lesson, written from the templates in Appendix A and validated by a JSON Schema. The compiler (`compiler/`, Node) turns all lessons into:

- `plugin/res/course/pack.json` — every block's text (already split into pages by the text-fit tool), quizzes, FAQ, hints, figures, checks, glossary;
- `plugin/res/course/patches/<id>.vcv` — the lesson patch: rows of blocks laid out by presets (6.6), with Atlas modules carrying references and standard modules carrying cables and parameter values;
- `build/report.json` — per-lesson stats: blocks, pages per text, overflow warnings, missing targets.

Lesson YAML top level:

```yaml
id: p3-05-first-voice        # same ids as facts
num: "3.5"
part: 3
title: Первый голос
goal: Собрать с нуля голос синтезатора, который звучит только пока нажата кнопка.
builds: Первый инструмент курса — на нём держатся бас и лид из части 8.
minutes: 35
requires: [p3-01-vcf, p3-03-vca, p3-04-adsr]
blocks: [ … ]                 # see Appendix A
glossary: [огибающая, VCA, гейт]
sources: [lm-first-voice, samesound-first-patch, vcv-free-manual]
```

Each block has: `id`, `type` (Appendix A), `title`, a layout preset (6.6) and type-specific content. Every block that has text also has `faq` (2–4 prepared questions with answers) and every practice has a hint ladder — these are what make the tutor fast.

### 6.5 Figure templates

The content writer never draws. A figure is a template name plus parameters; the plugin draws it with nanovg in the theme:

| Template | Parameters | Shows |
| --- | --- | --- |
| `waveforms` | `shapes: [sine, triangle, saw, square]`, `periods`, `labels` | Waveforms one above another |
| `harmonics` | `shape`, `count` (1–16), `law: 1/n \| 1/n2 \| odd` | Bar spectrum of a waveform |
| `adsr` | `a, d, s, r`, `gate` (seconds), `markers: true` | Envelope shape with the gate underneath |
| `voltages` | `ranges: [{label, min, max}]` | Voltage ranges on a vertical scale (audio ±5 V, CV 0–10 V, gate 10 V, trigger) |
| `keyboard` | `from`, `to`, `marks: [{note, volts}]` | Piano strip with 1V/oct marks (0 V = C4) |
| `flow` | `nodes: [{id, label, kind: audio\|cv\|gate}]`, `edges: [{from, to, kind}]`, `layout: row \| two-rows` | Signal-flow map, audio edges thick, CV dashed, gates dotted |
| `filter` | `type: lp \| hp`, `cutoff`, `resonance` | Response curve with the cutoff marked |
| `clock` | `bpm`, `divisions: [1,2,4]`, `bars` | Clock and divided clocks as gate lanes |
| `steps` | `values: [...]`, `gates: [...]` | A step-sequencer pattern |
| `mixer` | `channels: [{label, level}]`, `headroom` | Levels against headroom |
| `stereo` | `sources: [{label, pan, width}]` | Positions in the stereo field |
| `table` | `rows: [[…]]` (≤ 5 × 4) | A small table when a figure is really a table |

New templates may be added only by the main session (not by content subagents), with a preview in the visual QA gallery (10.4).

### 6.6 Layout presets

The compiler places modules; writers only choose a preset. Rows are 380 px apart (one Rack row). Block `n` is row `n` (block 1 = row 1). X positions in HP, packed left to right from x = 0 with 1 HP gaps; standard module widths come from snapshots (`width`) or a width table in the compiler.

| Preset | Order in the row |
| --- | --- |
| `cover` | `Page` (36 HP) · `Figure` (optional, 16 HP) |
| `story` | `Page` (24 HP) · `Figure` (16 HP, optional) · `Page` (24 HP, optional second column) |
| `live` | `Page` (20 HP) · demo modules in signal order · `Note`s right after the module they point at · `Figure` (optional) |
| `anatomy` | `Page` (16 HP) · the module · `Note`s around it (up to 6) |
| `try` | `Task` (steps, no check) · demo modules · `Note`s |
| `quiz` | `Page` (12 HP, optional lead-in) · `Quiz` |
| `practice` | `Task` · given modules · `Bench` (left) · 40 HP empty · `Bench` (right) |
| `debug` | `Task` · the broken patch · `Note` («здесь что-то не так» without revealing) |
| `recap` | `Page` (24 HP) · `Quiz` (optional) · `Page` (12 HP: «Дальше» — the next lesson and what it builds) |

Fixed parts of every lesson patch: the Navigator is the **first module of row 1**, before the cover block's modules; the MCP Server (`VCVRackMcpServer/RackMcpServer`, port 2600, enabled) and the `Core/AudioInterface2` live at the end of row 1. **Only the active block sounds.** Every block that produces sound is wired into its own channel of a mixer chain at the end of row 1 (`Fundamental/VCMixer`, 4 channels each, chained when a lesson needs more), and the mixer feeds the Audio module. All channel levels start at 0. On every glide the host sets only the active block's channel to its level (`APP->engine->setParamValue`) and the others to 0. Practice benches have their own channel that is always open while the practice block is active. The validator checks the channel map.

A row must fit **96 HP**. The compiler fails the build if a preset produces a wider row; the writer then splits the block.

## 7. The tutor — fast by design

### 7.1 What the tutor is for

Only for what preparation cannot cover: a free question, a free answer to grade, reviewing the learner's patch in words, «объясни иначе» beyond the prepared «Объясни проще». Everything else is local: block text, FAQ chips, quiz verdicts, hint ladders, check results.

### 7.2 One warm process per lesson

Replace one-process-per-turn for Rack lessons with **one persistent `claude -p` process per open lesson**, started when the lesson opens:

```
cwd: tutor/run/          (a folder with no .claude/ inside it or above it up to REPO)
claude -p --input-format stream-json --output-format stream-json --verbose --include-partial-messages
  --model <tutorModel, default sonnet> --effort <tutorEffort, default low>
  --append-system-prompt-file tutor/TUTOR.md
  --tools "Read"
  --allowedTools <the exact MCP read tools: mcp__vcvrack__vcvrack_get_status, …_list_modules, …_get_module, …_get_params, …_list_cables, …_get_rack_layout> "Read(//<REPO absolute path>/course/**)"
  --disallowedTools <the exact MCP write tools: …_add_module, …_delete_module, …_set_params, …_add_cable, …_delete_cable, …_set_layout_preferences>
  --mcp-config <vcvrack http 127.0.0.1:2600 only> --strict-mcp-config
  --setting-sources local --disable-slash-commands
  --permission-mode dontAsk --permission-prompts none
  --no-session-persistence --exclude-dynamic-system-prompt-sections
env: childEnv() minus ANTHROPIC_API_KEY/ANTHROPIC_AUTH_TOKEN, plus ENABLE_TOOL_SEARCH=false
```

- The session must not run `claude` itself (it is denied), so check flags against the CLI reference (https://code.claude.com/docs/en/cli-reference) and build a **probe-and-fallback** into the runner: if the process exits with an "unknown option" error, retry without the optional flags in this order — `--exclude-dynamic-system-prompt-sections`, `--permission-prompts`, `--effort`, `--append-system-prompt-file` (replaced by `--append-system-prompt` with the file's text, passed as a spawn argument, never through a shell) — and log which were dropped. Do **not** use `--bare` (it does not use the subscription login).
- MCP permission rules: write the exact tool names, not partial-name wildcards; derive the REPO path at runtime.
- `--setting-sources local` with the `tutor/run/` cwd keeps the owner's user settings and hooks (the study-pipeline hooks) out of tutor turns.
- **Warm-up:** right after start, send one hidden message («Готов?» with an instruction to answer «ok») and discard its reply, so MCP is connected and the system prompt is cached before the first real question.
- The tutor has **no write tools to the patch**: demos are prebuilt in lesson patches. It can read the patch and course files.
- Idle for 15 minutes → close stdin and let the process exit; restart on the next question (each message carries the context header, 7.3, so nothing important is lost). Lesson switch → close the old process, start the new one.
- `tutorModel` and `tutorEffort` are Atlas settings («Настройки» → «Репетитор»: модель `sonnet` / `haiku` / `opus`, усилие `low` / `medium` / `high`).

### 7.3 Context sent with each message

The server wraps each learner message in a compact header so the tutor does not need to read files:

```
[lesson p3-05-first-voice «Первый голос», block b4 «Практика: собери голос» (practice)]
[block text] <the block's page text, plain, ≤ 1200 chars>
[patch] <one line per module in the bench and given modules: «VCO → VCF.In», params that differ from default>
[check] <last check result: passed items / first failing item>
[quiz] <for grade: question, options, correct option, the learner's text>
[kind] ask | grade | simpler | example | review
[learner] <text>
```

The patch line is built by the server from REST `:2600` (or from what the plugin posts with the question when MCP is not present).

### 7.4 Reply templates (in `TUTOR.md`)

`tutor/TUTOR.md` is short (≤ 700 words, English instructions, Russian examples). It fixes the reply shape so the model does not plan:

- **ask:** 1–3 sentences that answer directly. Then, if useful, one line starting «Покрути:» or «Посмотри:» naming a real module and knob on screen. At most 90 words. No headings, no lists longer than 3 items, no tables.
- **grade:** first character `✓`, `✗` or `≈`. Then one sentence: what is right or wrong in the learner's own words. Then «Верно так:» + the correct idea in one sentence. At most 60 words.
- **simpler:** the same idea with an everyday analogy, 2–4 sentences, no new terms.
- **example:** one concrete patch or listening example with knob values from the block, 2–4 sentences.
- **review:** read the `[patch]` and `[check]` lines; name what works, then the single most important fix with the exact port or knob name; at most 80 words; never patch for the learner.
- Always Russian, «ты», plain text with `**bold**` only; module and knob names as on the panel; no meta talk («как ИИ…»); never claim what it sounds like — ask what the learner hears.

### 7.5 Server endpoints for Rack

In `server/` (new), mounted at `/api/rack/*`. Ports: the owner's Atlas service is 5175 (never touched by the session); the session's test Atlas server is 5185 (`dev:fixtures`); a second test server for the lesson-switch test uses 5186.

- `GET /api/rack/health` — Atlas up, tutor model, Rack MCP reachable.
- `POST /api/rack/lesson/open {lesson}` — start the warm tutor process, mark the vault note `study → learning`.
- `POST /api/rack/ask {lesson, block, kind, text, patch?}` → `{stream}`; `GET /api/rack/stream/:id?after=n` → `{chunks, done}`.
- `POST /api/rack/progress {lesson, block?, event, data}` — block reached, quiz answer, check result, lesson completed; appends to the lesson note and, on completion, sets `status: learned`.
- `GET /api/rack/progress` — progress for the Navigator's course map (merged with local progress by latest timestamp).

The vault note per lesson (`VAULT_STUDY/vcv/notes/<id>.md`, path from the existing `notes.mjs`) gets a compact log instead of the md-log mirror: blocks reached, quiz results, check results, and each tutor exchange (question and answer). The old web lesson runner stays for the web app; Rack lessons do not use md-log or the `/learn` skill.

### 7.6 Stub for tests

`tutor/stub-claude.mjs` behaves like the persistent process: reads stream-json lines, emits `stream_event` text deltas and a `result` per message, reply text built from the template for the `[kind]`. Atlas settings point to it in fixtures; costs no model calls.

## 8. The test Rack instance

For loading lesson patches with the new plugin, the session runs its own Rack:

- User folder `REPO/.rack-test/` (gitignored; `.rack-shots/` and `rack/.fakehome/` too). Plugins: copy from the owner's `~/Library/Application Support/Rack2/plugins-mac-arm64/` (read-only): `ImpromptuModular`, `Valley`, `AudibleInstruments`, `VCV-Recorder`, the Neural-Harmonics folder; `Fundamental` is unpacked by Rack on first start; install the freshly built test package of `Atlas` (below).
- Seed `.rack-test/settings.json` before the first start with the keys that turn off the tips window and update checks (verify key names in the Rack source, `src/settings.cpp`; expected `showTipsOnLaunch: false`, `autoCheckUpdates: false`).
- **Test builds are separate.** `scripts/build.sh --test` compiles patches with the MCP Server on port 2610 and the Audio module without a device into `build/test-res/` and installs a test copy of the plugin into `.rack-test` only. The shipped build (`build.sh` without flags) writes `plugin/res/course/` with port 2600; a build gate checks that every shipped patch has MCP on 2600 and no test settings.
- Before each start: delete `log.txt` and `autosave*` in the test folder (a truncated log makes Rack ask a blocking crash question; an autosave overrides the requested patch).
- Start in the background with a hard time limit (macOS has no `timeout`): `perl -e 'alarm shift; exec @ARGV' 600 "/Applications/VCV Rack 2 Free.app/Contents/MacOS/Rack" -u "$REPO/.rack-test" <patch>` with `ATLAS_DEV_PORT=2700` and `ATLAS_URL=http://127.0.0.1:5185`. Stop it with the dev bridge's `POST /quit`; the alarm is the safety net. Do not use `-d`.
- Screenshot mode for visual QA uses a separate user folder `.rack-shots/` that contains only `Atlas` (and the Fundamental Rack unpacks): delete `screenshots/Atlas/` before each run (existing files are skipped), run `Rack -u "$REPO/.rack-shots" -t 2` once per preview variant with `ATLAS_PREVIEW` set per model (`Page=<item>;Quiz=<item>;…`), copy the PNGs to `qa/previews/<variant>/`. Verify the output folder on the first run.
- **If Rack cannot start inside the sandbox** (no window, permission errors): record it, keep all deterministic checks (10.1–10.3, 10.5), and move the in-Rack checks to the owner's checklist. Do not try to run Rack outside the sandbox.

## 9. Workstreams (do them in this order; no owner gates)

**W0 — Orientation.** Find the Rack app (`ls /Applications | grep -i rack`; expected `VCV Rack 2 Free.app`) and use its real path everywhere. Read this plan, `docs/vcv/REPORT.md`, `facts/course.json`, three lessons (`p1-01`, `p3-05`, `p8-01`), two cards, two checks, `vcv/tutor-plugin/src/Tutor.cpp`, `app/server/lessons.mjs`, `app/server/vcv/*.mjs`. Inventory the toolchain (Xcode CLT, `make`, `node`, SDK). Create `docs/PROGRESS.md`, branch `rack-native`.

**W1 — Design system.** Fonts (6.1), `Theme.hpp`, `design-tokens.json`.

**W2 — Plugin core.** Plugin skeleton written by hand (no `helper.py`: it is interactive): `plugin.json`, `Makefile`, `src/plugin.{hpp,cpp}`. The text engine (6.3.3) as a standalone library with unit tests. The host (6.3.1), theme switch, resize handles, panel drawing, previews.

**W3 — Modules.** First export checker fixtures: for every check in `facts/checks/`, the reference patch that `live-check.mjs` builds and the expected result, as JSON in `plugin/tests/fixtures/`. Then `Page`, `Note` + pointer overlay, `Figure` + all templates, `Quiz`, `Task`, `Bench`, the C++ checker, `Navigator` + course map + lesson loading (6.3.2) + docked panel, `Tutor` module and panel UI, the Atlas client (tested against a local stub server here; end-to-end in W6), the dev bridge.

**W4 — Compiler and tools.** JSON Schema for lesson YAML; compiler (pack, patches, report); layout presets; text-fit tool with `opentype.js`; param resolver (`displayValue` target → raw value using snapshots: exact match among sampled points, linear/exponential interpolation from `min/mid/max` when the unit allows, otherwise a calibration on the live Rack cached in `course/calibration.json`); the course validator (10.2); `scripts/build.sh` = schema → validate → compile → `make` → `make dist` (and `--test`, section 8). At the end of W4, the compiler generates `qa/gallery.vcv`: every module in both themes and every figure template.

**W5 — Pilot lesson.** Write `p1-01-sound-wave` in the new format by hand (main session) to the highest standard; it becomes the reference example for writers. Run every check in section 10 on it, including the test instance and screenshots. Fix the plugin and tools until the pilot is excellent. Then write `p3-05-first-voice` (practice-heavy) the same way.

**W6 — Tutor runtime.** `TUTOR.md`, the persistent runner, `/api/rack/*`, vault note log, Atlas settings for the tutor, the stub, tests (10.6). Remove nothing from the web runner.

**W7 — Course conversion.** One subagent per part (0–9) converts the old lessons with the mapping in Appendix D and the templates in Appendix A, following the style guide (Appendix B) and the FAQ rules (Appendix C). Give each subagent: this plan's appendices, the pilot lessons as examples, its part's old lessons, the cards and snapshots of its modules, its checks, the glossary, `quotes.json`. After each part: build, validators, text-fit, live verification of demo blocks (10.5), fix-ups.

**W8 — Atlas companion.** Web «Модульный синтез» screen shows Rack-native progress (from `/api/rack/progress`), a short «Как заниматься в Rack» page, links to lesson notes. No new web lesson features.

**W9 — Whole-course QA.** Everything in section 10 over all 52 lessons. Read-through pass (10.7). Fix and rebuild until clean.

**W10 — Hand-over.** `scripts/install.sh` (section 13), final build, `docs/REPORT.md`, `docs/OWNER_CHECKLIST.md`.

## 10. Self-verification (deterministic first)

### 10.1 Build gates (every build)

Schema valid · every block id unique · every reference resolves (modules, ports, params by name in snapshots; figures; glossary terms; sources; quotes in `quotes.json`) · every row ≤ 96 HP · every lesson's plugins exist · every practice has a check and a hint ladder · every text block has 2–4 FAQ · plugin compiles without errors.

### 10.2 Content validator (`compiler/validate.mjs`)

- Text fit: no page over its box; no block over 3 pages; `Note` ≤ 40 words; quiz question ≤ 30 words, each option ≤ 12 words, each explanation ≤ 40 words; FAQ answer ≤ 70 words.
- Style lint (Appendix B): sentences ≤ 22 words (warn at 18), no banned phrases, no English sentences in learner text, Rack names in Latin only where they match a snapshot name, «ты» form, no exclamation marks except in quotes, a normal space between a number and its unit («440 Гц»).
- Terms: every `[[термин]]` exists in the glossary; first use of a term in a lesson is bold or a glossary link.
- Coverage: every module card's controls that the old lesson's «Анатомия» covered appear in the new anatomy block (by name).
- Pedagogy: every lesson has cover, at least one live or anatomy block, at least one quiz, practice when the old lesson had one, recap; block order follows Appendix D.

### 10.3 Unit tests

C++ (plain `clang++`, no Rack): text engine (markdown parsing, line breaking, pagination against fixtures), checker (every case in `facts/checks/*` with the reference patches that `live-check.mjs` builds, exported as JSON fixtures), figure parameter parsing. Node: compiler, layout presets, param resolver, text-fit parity with the C++ engine on a 200-paragraph corpus (same line breaks), tutor runner with the stub, `/api/rack/*`. Existing tests must keep passing.

### 10.4 Visual QA

- `qa/previews/` — for each module and each figure template, `Rack -t` screenshots with `ATLAS_PREVIEW` pointing at chosen pack items (long page, short page, page with list and terms, long quiz, failing check, streaming tutor, every figure). Look at every image yourself (Read the PNG). Fix what looks wrong: clipping, crowding, misaligned baselines, poor contrast, inconsistent spacing.
- Test instance (section 8): open every lesson patch via the dev bridge; `GET /state` must report no overflow, no missing pointer target, no missing plugin, Navigator present, block count as compiled; `POST /goto` through every block; run every practice check on the template patch (must fail with the first hint) and, for the pilot lessons, on a reference patch built over the test instance's MCP (port 2610) (must pass).
- Keep `qa/VISUAL_LOG.md`: what you looked at, what you changed.

### 10.5 Live verification of demo blocks (owner's Rack, standard modules only)

For each `live`, `anatomy`, `try` and `debug` block, build its standard modules, cables and params in the owner's agent patch over MCP `:2600` in the session's own row (as `live-check.mjs` does), read back params (`displayValue` matches the block's targets), check that the named outputs carry the expected kind of signal (connected outputs only; sample 5 times 100 ms apart where motion is expected), delete everything. Report per block. If the owner's Rack on :2600 is not reachable, run the same verification on the test instance's MCP (:2610) — it has the same Fundamental modules. A block that does not behave as its text claims is fixed in the text or in the setup.

### 10.6 Tutor tests

With the stub: lesson open starts one process; three messages reuse it; lesson switch replaces it; idle exit; stream polling delivers chunks in order; context header contains the right block text, patch line and check line; vault note log written in fixtures. Reply-shape check on stub output (first-character verdict for grade, word limits).

### 10.7 Read-through

After W7, a fresh subagent (no part in writing) reads every lesson's learner-facing text in order, as a beginner, against Appendix B and a checklist: does each block make one point, is anything assumed that was not taught, are captions pointing at what the text discusses, are FAQ questions the ones a beginner would ask. It returns a list of concrete edits; apply them, rebuild, revalidate.

## 11. Atlas web and old pieces

- The old web lesson runner, `vcv/workspace/`, `facts/` and their tests stay and must keep passing; `facts/` remains the fact base (cards, snapshots, checks, sources).
- `vcv/tutor-plugin/` stays as source but is no longer built by default.
- The Atlas «Модульный синтез» screen switches to Rack-native progress (W8).

## 12. Directory layout (new)

```
rack/
  plugin/                    the Atlas plugin (C++)
    src/
      plugin.hpp/.cpp
      host/                  AtlasHost, actions, progress, lesson loading, panel, overlay, dev bridge
      ui/                    Theme, Button, TextArea, Pagination, ResizeHandle, Panel
      text/                  text engine (no Rack deps) + nanovg adapter
      figures/               figure templates
      check/                 checker (no Rack deps) + engine adapter
      modules/               Navigator, Page, Note, Figure, Quiz, Task, Bench, Tutor
      net/                   Atlas client
    res/
      fonts/                 PT Sans, PT Serif, JetBrains Mono + licences + SOURCES.md
      course/                pack.json, patches/*.vcv (generated)
    tests/                   C++ unit tests (no Rack)
    Makefile, plugin.json
  compiler/                  Node: schema, compile, layout, textfit, resolve, validate
  course/
    lessons/*.yaml           new lesson sources
    design-tokens.json
    calibration.json
    templates/               Appendix A templates as files (copied from this plan)
  qa/                        gallery.vcv, previews/, VISUAL_LOG.md
  scripts/                   build.sh, rack-test.mjs, install.sh
  .rack-test/                test Rack user folder (gitignored)
  build/                     compiler output reports (gitignored)
tutor/                   TUTOR.md, settings.json, stub-claude.mjs
server/             /api/rack/* endpoints, tutor runner
docs/                   PROGRESS.md, REPORT.md, OWNER_CHECKLIST.md
```

## 13. Owner's part

**Before the run** (the owner does this once): update `.claude/settings.local.json` (the version in the chat message that delivered this plan adds read access to the Rack plugins folder and the test ports); open VCV Rack on the agent patch with MCP Server on port 2600; start the session with the launch prompt.

**After the run**, the owner runs one command:

```
cd ~/Documents/personal/learn-with-claude && scripts/install.sh
```

`install.sh` (written by you; never run against the real home folder — test it with `HOME=$REPO/rack/.fakehome scripts/install.sh --dry-run` and a fake plugins folder; in `--dry-run` it only prints the commands it would run, including the `launchctl` line): moves `~/Library/Application Support/Rack2/plugins-mac-arm64/AtlasTutor` to `…/AtlasTutor.bak-<date>` if present; copies `plugin/dist/Atlas/` into the plugins folder (replacing an older `Atlas` after backing it up); restarts the Atlas service (`launchctl kickstart -k gui/$(id -u)/com.nikrr.atlas`, or prints how to start it); prints «Перезапусти VCV Rack, добавь модуль Atlas → Навигатор и нажми «Продолжить»» and the path to `docs/OWNER_CHECKLIST.md`.

`OWNER_CHECKLIST.md` (Russian): 10–15 checks in the order of the acceptance story (5.1), each one line with the expected result, plus every item the run could not verify.

## 14. Final report

`docs/REPORT.md` (English): what was built, per workstream; numbers (lessons, blocks, pages, quizzes, FAQ items, figures, checks); every validator and test result; the visual QA log summary; what is `UNVERIFIED` and why; deviations from this plan with reasons; tutor latency measured with the stub path (process start, first chunk) and the expected real-model behaviour. Close with a short Russian summary to the owner in chat.

---

## Appendix A. Block templates

Write blocks by filling these templates; do not invent new block types. Text fields are Markdown subset strings (6.3.3). `faq` and hints are mandatory where shown. Copy these templates into `course/templates/` as files.

Field rules for all templates:

- `display` targets are the numeric part only («261.63»), the unit comes from the snapshot; for switches use the option text exactly as the snapshot shows it («Hard»).
- Pointer targets: `{module: <ref>, param: <name>}` or `input:` / `output:` / `light:` instead of `param:`.
- Glossary links: `[[огибающая]]`, or `[[огибающая|огибающей]]` for an inflected form (the first part must be a glossary entry).
- Hint ladders are keyed with the existing checker's item keys: `module:<ref>`, `cable:<i>`, `param:<i>`, `probe:<i>` (1-based, in the order of the check file), plus `fixed` for debug checks. `validate.mjs --keys <check>` prints the expected keys.
- Optional fields: `lead` (a short page before a quiz), `page2` (a second page column in `story`), `simpler` (an everyday analogy, Appendix C) on any text block.
- `plugins:` in the lesson header is inferred by the compiler; do not write it.

**Port and parameter names in these examples are illustrative.** In real lessons every name must be copied from the module's snapshot (`facts/modules/_snapshot/`); the validator rejects anything else.

### A.1 `cover`

```yaml
- id: b1
  type: cover
  preset: cover
  title: Звук как волна
  page: |
    # 1.1 Звук как волна
    **Ты научишься:** отличать четыре формы волны на слух и на экране, читать **Scope** (осциллограф).
    **Ты соберёшь:** VCO → Scope → Audio — «глаз» курса, он нужен в каждом уроке.
    **Понадобится:** 25 минут, наушники или колонки на тихой громкости.
  figure: { template: waveforms, shapes: [sine, triangle, saw, square], periods: 2 }
  faq:
    - q: Зачем мне Scope, если я и так слышу звук?
      a: Ухо говорит «ярче» или «глуше», а Scope показывает почему — форму волны. Дальше по курсу ты будешь проверять свои патчи по экрану, не только на слух.
```

### A.2 `story`

```yaml
- id: b2
  type: story
  preset: story
  title: Частота и амплитуда
  page: |
    # Частота и амплитуда
    Звук — это колебание. Один повтор колебания — **период**, число повторов в секунду — **частота**, она измеряется в **герцах (Гц)**.
    Чем выше частота, тем выше тон. Октава вверх — частота вдвое больше: ля первой октавы — 440 Гц, ля второй — 880 Гц.
    **Амплитуда** — размах колебания. Для уха это громкость. У VCO в Rack размах постоянный, около ±5 В — громкость мы будем менять дальше по цепочке.
  figure: { template: waveforms, shapes: [sine, sine], periods: [2, 4], labels: [440 Гц, 880 Гц] }
  faq:
    - q: Почему октава — это ровно вдвое?
      a: Так устроен слух: два звука с частотами 1:2 воспринимаются как «та же нота, только выше». Поэтому в Rack 1 вольт на входе V/Oct поднимает частоту ровно вдвое.
    - q: Громкость и амплитуда — одно и то же?
      a: Почти. Амплитуда — размах напряжения, громкость — то, как мы его слышим. Больше размах — громче звук.
```

### A.3 `live`

```yaml
- id: b3
  type: live
  preset: live
  title: Четыре формы волны
  page: |
    # Четыре формы волны
    Перед тобой VCO, подключённый к Scope и к выходу. Сейчас на экране **пила** (Sawtooth).
    Переключи кабель на другие выходы VCO: Sine, Triangle, Square. Высота тона не меняется — меняется **тембр**.
  modules:
    - { ref: vco, plugin: Fundamental, slug: VCO, params: { Frequency: { display: "261.63" } } }
    - { ref: scope, plugin: Fundamental, slug: Scope, params: { Time: { display: "5" } } }
  cables:
    - { from: [vco, Sawtooth], to: [scope, Ch 1] }
    - { from: [vco, Sawtooth], to: [audio, Left input] }
  notes:
    - { after: vco, text: "Четыре выхода — четыре формы. Попробуй каждую.", targets: [{ module: vco, output: Sine }, { module: vco, output: Sawtooth }] }
    - { after: scope, text: "Time — сколько миллисекунд помещается на экран.", targets: [{ module: scope, param: Time }] }
  figure: { template: harmonics, shape: saw, count: 8, law: 1/n }
  faq:
    - q: Почему пила звучит ярче синуса?
      a: В пиле много гармоник — призвуков, кратных основной частоте. В синусе только одна. Больше гармоник — ярче тембр.
    - q: Кабель на Sine — а звука почти не слышно. Это нормально?
      a: Да. Синус без гармоник звучит мягко и кажется тише. Подними уровень на Audio или переключись на пилу для сравнения.
```

`audio` is the lesson's `Core/AudioInterface2` in row 1 (always available as a ref). Port and param names must match the snapshot exactly. `display` targets are resolved to raw values by the compiler; write `raw` only when a parameter has no meaningful display.

### A.4 `anatomy`

```yaml
- id: b4
  type: anatomy
  preset: anatomy
  title: Панель Scope
  page: |
    # Панель Scope
    Обойдём панель сверху вниз. Подписи справа указывают на каждую ручку.
  module: { ref: scope, plugin: Fundamental, slug: Scope }
  notes:
    - { text: "**Time** — ширина экрана во времени. Меньше число — волна растянута.", targets: [{ module: scope, param: Time }] }
    - { text: "**Ch 1 scale** — масштаб по вертикали для первого канала.", targets: [{ module: scope, param: "Ch 1 scale" }] }
    - { text: "**Trigger** — порог, с которого начинается развёртка: волна перестаёт «бежать».", targets: [{ module: scope, param: Trigger position }] }
  later:
    - { what: "Режим X×Y", lesson: "4.6" }
  faq:
    - q: Волна на экране бежит. Что крутить?
      a: Порог синхронизации (Trigger). Поставь его внутрь размаха волны — картинка остановится.
```

At most 6 notes per anatomy block; split big modules into two anatomy blocks.

### A.5 `try`

```yaml
- id: b5
  type: try
  preset: try
  title: Покрути сам
  steps:
    - do: "Поверни **Frequency** у VCO вправо на четверть оборота."
      see: "Волна на Scope сжимается, тон выше."
    - do: "Уменьши **Time** у Scope до 2."
      see: "Периоды становятся шире — проще разглядеть форму."
  modules: [ … same as live … ]
  cables: [ … ]
  faq:
    - q: Ничего не меняется, когда кручу Frequency.
      a: Проверь, что крутишь ручку VCO, к которому подключён Scope, и что кабель идёт с выхода, а не на вход.
```

### A.6 `quiz`

```yaml
- id: b6
  type: quiz
  preset: quiz
  title: Проверь себя
  questions:
    - q: Ты сыграл ту же ноту на октаву выше. Что изменилось в волне?
      options:
        - { text: Частота выросла вдвое, correct: true, why: "Октава — это удвоение частоты, период стал вдвое короче." }
        - { text: Амплитуда выросла вдвое, why: "Выше — не значит громче: размах волны не изменился." }
        - { text: Изменилась форма волны, why: "Форма отвечает за тембр, а не за высоту." }
      free: true          # allow «Свой ответ» graded by the tutor
```

Wrong options come from real misconceptions (the old lessons' «Пробник» lists them). «Не знаю» is added by the module.

### A.7 `practice`

```yaml
- id: b7
  type: practice
  preset: practice
  title: Практика: голос с огибающей
  task: |
    1. Поставь на верстак VCO, VCF, VCA, ADSR и Push.
    2. Пила VCO → вход VCF, выход LPF → вход VCA, выход VCA → вход 1 Audio.
    3. Огибающая ADSR → CV-вход VCA. Push → Gate ADSR.
    4. Нажми Push: звук есть, пока кнопка нажата.
  given: [audio]
  check: p3-05-first-voice           # reuse facts/checks/<id>.json (names verified live)
  hints:                              # ladder per check item, from vague to exact
    "cable:3":
      - Звук есть всегда, даже без нажатия? Подумай, что решает, когда звучать.
      - Посмотри на VCA — что управляет его громкостью?
      - Выход ADSR (Envelope) → вход CV у VCA.
  faq:
    - q: Можно взять VCA Mix вместо VCA?
      a: Можно, если проверка не требует именно VCA. Здесь требует — урок про VCA, возьми VCA.
```

### A.8 `debug`

```yaml
- id: b8
  type: debug
  preset: debug
  title: Найди поломку
  task: |
    Этот голос молчит. Найди одну ошибку и исправь её.
  modules: [ … the working voice … ]
  cables: [ … ]
  break: { remove_cable: [env, Envelope, vca, CV] }
  check: { kind: fixed, expect_cable: [env, Envelope, vca, CV] }
  hints:
    fixed:
      - Нажимаешь Push — что должно открыть звук?
      - Проследи, куда идёт выход ADSR.
      - Соедини Envelope у ADSR с CV у VCA.
  faq: [ … ]
```

### A.9 `recap`

```yaml
- id: b9
  type: recap
  preset: recap
  title: Итог
  page: |
    # Итог
    - **Частота** — высота тона, **амплитуда** — громкость, **форма волны** — тембр.
    - Гармоники объясняют, почему пила ярче синуса.
    - Scope показывает напряжение во времени; Trigger останавливает картинку.
    **Источники:** Gordon Reid, «Synth Secrets», ч. 1 (Sound On Sound); Chris Meyer, «The Basic Concepts of Synthesis».
  next: |
    # Дальше: 1.2
    Напряжение — единственный язык модулей. Разберём, чем аудио отличается от CV, гейта и триггера.
  questions: [ … 1–2 recap questions in the quiz format … ]
  faq: [ … ]
```

## Appendix B. Style guide for learner text

- Address the learner as «ты». Short sentences (≤ 18 words preferred, 22 max). One idea per paragraph, 1–3 sentences per paragraph.
- Start with what the learner sees or hears, then the term. Bold a term on first use in the lesson or link it as `[[термин]]`.
- Rack names exactly as printed on the panel (Latin), with a Russian gloss on first use: «**Frequency** (частота)». Port names in the form «выход **Sawtooth**», «вход **V/Oct**».
- Numbers with units: «440 Гц», «±5 В», «1 мс», «120 BPM».
- No filler: banned — «давай разберёмся», «важно отметить», «стоит отметить», «как мы видим», «итак», «в этом уроке мы», «следует», «является», «осуществляет», «данный», exclamation marks.
- Never say what the learner will feel («это просто»). Never claim what something sounds like in a way the learner cannot verify; describe what to compare.
- Captions are imperative or descriptive, ≤ 40 words: «Поверни вправо — фильтр открывается».
- FAQ questions are written as a beginner would ask them, in their words, including confused ones («Почему ничего не звучит?»).
- Sources: names of authors and works on the recap page, in plain text. Quotes only from `quotes.json`, at most one per lesson.

## Appendix C. FAQ and hints — so the tutor rarely needs to think

- Every text-bearing block: 2–4 FAQ items. Sources for questions: the old lesson's misconceptions («Пробник» wrong options), «Проверка» failure comments, «Слом» symptoms, the card's «Типовые ошибки», and the obvious «что будет, если…» about the block's knobs.
- Each FAQ answer ≤ 70 words, answers the question in the first sentence, points to something on screen when possible.
- Every practice check item: a 3-step hint ladder (vague → where to look → exact fix with port/knob names). The `Task` module shows step 1 on the first failure, step 2 on «Ещё подсказка», step 3 on the next.
- Every quiz option: `why` ≤ 40 words.
- `simpler` text per story block (optional, ≤ 60 words): an everyday analogy. The «Объясни проще» button uses it locally when present; only if absent does it go to the tutor.

## Appendix D. Mapping old lessons to blocks

| Old section | New block(s) |
| --- | --- |
| «Зачем» | `cover` (goal, builds) |
| «Пробник» | `quiz` early in the lesson (after cover) with the same misconceptions; also FAQ sources |
| «Понятие» (steps) | one `story` block per step; the drawing it asks for → a figure template |
| «Анатомия» | `anatomy` block(s), ≤ 6 notes each, controls in the card's order |
| «Демо» | `live` block: modules, cables, param targets as the demo lists them |
| «Покрути сам» | `try` block, one step per knob turn with «see» |
| «Практикум» + «Проверка» | `practice` block with the existing check file and a hint ladder from the check's hints |
| «Слом» | `debug` block: the working demo + the break as a remove/param change |
| «Итог» | `recap` block: points, sources, next lesson, review questions |

Typical lesson: cover · quiz (probe) · story ×1–3 · live · anatomy ×1–2 · try · practice · debug · recap = 8–12 blocks. Lessons in part 0 (setup) may skip live/anatomy where the topic is the Rack UI itself; use `story` with figures and `try` steps on the real UI.

## Appendix E. Rack facts to rely on (checked 2026-10-18/19)

- `rack::patch::Manager`: `save(path)`, `load(path)`, `loadAction(path)` (sets path and recents), `clear()` — no dialogs; `loadDialog`, `saveDialog`, `revertDialog` show dialogs. A patch that references a missing module shows a blocking dialog (`checkUnavailableModulesJson`).
- `RackWidget`: `loadSelection(path)`, `pasteJsonAction(json)`, `addModule`, `setModulePosForce/Nearest/Squeeze`, `requestModulePos`, selection API.
- `RackScrollWidget`: `getGridOffset`, `setGridOffset`, `getZoom`, `setZoom(zoom[, pivot])`, `zoomToModules`, `zoomToBound(Rect)`.
- `Window`: `loadFont`, `loadImage`, `loadSvg` (cached); `screenshot` is private.
- Standalone flags: `-u` user dir, `-s` system dir, `-d` dev mode (do not use), `-h` headless, `-t <zoom>` module screenshots, `-a` safe mode. A truncated `log.txt` from a previous crash triggers a blocking question at start.
- Rack 2 loads a patch file without the zstd magic as legacy plain JSON.
- Voltage standards: audio ±5 V; CV 0–10 V or ±5 V; 1V/oct with 0 V = C4; gate 10 V; trigger 10 V for 1 ms; 1 sample delay per cable; up to 16 polyphony channels.
- Neural-Harmonics MCP Server: 17 tools, port param + enabled param, no patch save/load, edits on the UI thread (menus open → timeouts).
