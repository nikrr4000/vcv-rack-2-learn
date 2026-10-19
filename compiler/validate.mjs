#!/usr/bin/env node
/**
 * Course validator (plan §10.2): content rules the compiler does not enforce.
 *
 *   node rack/compiler/validate.mjs [<lesson id> …] [--json <file>] [--strict]
 *   node rack/compiler/validate.mjs --keys <check id>      the hint keys a check produces
 *
 * Errors (exit 1): word limits (quiz, options, explanations, FAQ, notes), FAQ count, banned phrases,
 * «вы» instead of «ты», exclamation marks, number without a space before its unit, unknown glossary
 * terms, unverified quotes, missing lesson structure (cover, quiz, live/anatomy, practice, recap).
 * Warnings: long sentences (> 18 words; > 22 is an error), Latin words that are not Rack names,
 * first use of a lesson term without bold or a link, anatomy coverage of the old lesson.
 */
import fs from "node:fs";
import path from "node:path";

import { facts, lessonFiles, loadCheck, loadLesson, oldLesson, RACK } from "./lib/data.mjs";
import { parse, plain, terms } from "./lib/textfit.mjs";

const argv = process.argv.slice(2);
if (argv[0] === "--keys") {
	const c = loadCheck(argv[1]);
	if (!c) {
		console.error(`no check ${argv[1]}`);
		process.exit(1);
	}
	const keys = [...(c.modules || []).map((m) => `module:${m.ref}`), ...(c.cables || []).map((x, i) => `cable:${i + 1}  ${x.from.join(".")} → ${x.to.join(".")}`), ...(c.params || []).map((x, i) => `param:${i + 1}  ${x.module}.${x.name}`), ...(c.probes || []).map((x, i) => `probe:${i + 1}  ${x.module}.${x.output}`)];
	console.log(keys.join("\n"));
	process.exit(0);
}
const jsonOut = argv.includes("--json") ? argv[argv.indexOf("--json") + 1] : null;
const strict = argv.includes("--strict");
const only = argv.filter((a, i) => !a.startsWith("--") && argv[i - 1] !== "--json");

const { snapshots, glossary, quotes, course, sources } = facts();

const BANNED = ["давай разберёмся", "давайте разберёмся", "важно отметить", "стоит отметить", "как мы видим", "итак", "в этом уроке мы", "следует", "является", "являются", "осуществляет", "данный", "данная", "данное", "данные", "это просто", "легко"];
const ACRONYMS = new Set("VCO VCF VCA LFO ADSR CV MIDI Rack VCV Free Pro Fundamental Core Audio Scope BPM HP OK DC FM PWM XY S&H MCP USB VST DAW Atlas Plateau Clouds Clocked Recorder SEQ Mix Mixer Push Noise Octave Quantizer Viz Logic Compare Gates Mutes Merge Split Sum Rescale Random Pulses Process Delay Fade MidSide Mult Wavetable Valley Mutable Instruments ImpromptuModular AudibleInstruments Impromptu Neural Harmonics Ctrl Cmd Shift Alt Enter Esc Tab macOS Windows Linux Library Manual Sound On Sound Synth Secrets Gordon Reid Chris Meyer Learning Modular Doepfer Omri Cohen Music Hackspace samesound zwook Geogenik A4 C4 C3 C5 A3 A5 E4 G4 dB V ms Hz kHz".split(" "));

// Rack 2 menu and dialog labels, controller and audio words the course names on purpose. Not in the
// module snapshots, so listed here; anything else Latin is still reported.
const UI_WORDS = "File Edit View Engine Help New Open Save as Load Undo Redo Duplicate Delete Preset Update all Add Included Library Unmapped Channels Rotate Reuse Reset Panic Driver Device Append Output Block Recording Heartbeat Mid Side MidSide mono stereo wet dry WAV VU dBFS Eurorack MiniLab mkII Arturia aftertouch velocity acid Pan law vcvrack com Es".split(" ");

function latinWhitelist() {
	const w = new Set([...ACRONYMS, ...UI_WORDS]);
	for (const s of Object.values(snapshots)) {
		for (const t of [s.name, s.slug, s.plugin, ...(s.params || []).flatMap((p) => [p.name, ...(p.options || [])]), ...(s.inputs || []).map((p) => p.name), ...(s.outputs || []).map((p) => p.name)])
			for (const word of String(t || "").split(/[^A-Za-z0-9&/]+/)) if (word) w.add(word);
	}
	return w;
}
const LATIN_OK = latinWhitelist();
const words = (s) => plain(s).split(/\s+/).filter(Boolean).length;
const quoteTexts = new Map(quotes.map((q) => [q.text.trim(), q.source]));

function textsOf(L) {
	// Every learner-facing text with where it lives and its kind.
	const out = [];
	const add = (where, kind, text) => text && out.push({ where, kind, text });
	for (const b of L.blocks) {
		add(`${b.id}.page`, "page", b.page);
		add(`${b.id}.page2`, "page", b.page2);
		add(`${b.id}.lead`, "page", b.lead);
		add(`${b.id}.task`, "page", b.task);
		add(`${b.id}.next`, "page", b.next);
		add(`${b.id}.simpler`, "simpler", b.simpler);
		add(`${b.id}.figure`, "caption", b.figure?.caption);
		(b.steps || []).forEach((s, i) => {
			add(`${b.id}.step${i + 1}`, "step", s.do);
			add(`${b.id}.step${i + 1}.see`, "step", s.see);
		});
		(b.notes || []).forEach((n, i) => add(`${b.id}.note${i + 1}`, "note", n.text));
		(b.questions || []).forEach((q, i) => {
			add(`${b.id}.q${i + 1}`, "question", q.q);
			q.options.forEach((o, k) => {
				add(`${b.id}.q${i + 1}.o${k + 1}`, "option", o.text);
				add(`${b.id}.q${i + 1}.o${k + 1}.why`, "why", o.why);
			});
		});
		(b.faq || []).forEach((f, i) => {
			add(`${b.id}.faq${i + 1}.q`, "faqq", f.q);
			add(`${b.id}.faq${i + 1}.a`, "faqa", f.a);
		});
		for (const [k, ladder] of Object.entries(b.hints || {})) ladder.forEach((h, i) => add(`${b.id}.hint.${k}.${i + 1}`, "hint", h));
	}
	return out;
}

function sentences(s) {
	return plain(s)
		.replace(/«[^»]*»/g, (m) => m.replace(/[.?!]/g, ""))
		.split(/(?<=[.?!…])\s+|\n/)
		.map((x) => x.trim())
		.filter(Boolean);
}

/** Text without code values and verbatim quotes (for Latin and punctuation rules). */
function proseOf(s) {
	return String(s)
		.replace(/`[^`]*`/g, " ")
		.replace(/"[^"]*"/g, " ")
		.replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (m, a, b) => b || a);
}

/** First use of a term in page text (headings skipped): is it inside **bold** or a [[link]]? */
function firstUse(texts, term) {
	const t = term.toLowerCase();
	for (const x of texts) {
		if (x.kind !== "page" && x.kind !== "note" && x.kind !== "step") continue;
		for (const line of x.text.split("\n")) {
			if (line.trim().startsWith("#")) continue;
			const low = line.toLowerCase();
			const i = low.indexOf(t.slice(0, Math.max(4, t.length - 2)));
			if (i < 0) continue;
			const before = line.slice(0, i);
			const bold = (before.match(/\*\*/g) || []).length % 2 === 1;
			const link = before.lastIndexOf("[[") > before.lastIndexOf("]]");
			return { where: x.where, marked: bold || link };
		}
	}
	return null;
}

function validateLesson(L) {
	const errors = [];
	const warnings = [];
	const E = (w, m) => errors.push(`${L.id}/${w}: ${m}`);
	const Wn = (w, m) => warnings.push(`${L.id}/${w}: ${m}`);
	const texts = textsOf(L);
	const limits = { question: 30, option: 12, why: 40, faqa: 70, note: 40, simpler: 60 };
	let longSentences = 0;
	for (const t of texts) {
		const n = words(t.text);
		if (limits[t.kind] && n > limits[t.kind]) E(t.where, `${n} words (at most ${limits[t.kind]})`);
		const prose = proseOf(t.text);
		const low = prose.toLowerCase();
		for (const b of BANNED) if (new RegExp(`(^|[^а-яё])${b}([^а-яё]|$)`, "i").test(low)) E(t.where, `banned phrase «${b}»`);
		if (/(^|[^а-яё])(вы|вам|вас|ваш|ваша|ваше|ваши|вашего|вашей)([^а-яё]|$)/i.test(low)) E(t.where, "address the learner as «ты», not «вы»");
		if (prose.includes("!")) E(t.where, "no exclamation marks");
		const unit = /(\d)(Гц|кГц|В|мс|с|дБ|%|BPM)(?![а-яёa-z])/.exec(prose);
		if (unit) E(t.where, `a space between the number and «${unit[2]}» («${unit[1]} ${unit[2]}»)`);
		for (const s of sentences(t.text)) {
			const k = s.split(/\s+/).filter((w) => /[A-Za-zА-Яа-яЁё0-9]/.test(w)).length;
			if (k > 22) E(t.where, `sentence of ${k} words (at most 22): «${s.slice(0, 60)}…»`);
			else if (k > 18) longSentences++;
			const latin = s.match(/[A-Za-z]{3,}/g) || [];
			if (!/[А-Яа-яЁё]/.test(s) && latin.filter((w) => !LATIN_OK.has(w)).length >= 3 && t.kind !== "page") E(t.where, `an English sentence in learner text: «${s.slice(0, 60)}»`);
		}
		if (!t.where.endsWith(".page") || !/Источники/.test(t.text)) {
			// Whole tokens first («8vert», «1V/octave», «Mid/Side»), then their slash-separated parts; notes like C2 or G5 are fine.
			for (const tok of prose.match(/[A-Za-z0-9][A-Za-z0-9&/]*/g) || []) {
				if (!/[A-Za-z]/.test(tok) || LATIN_OK.has(tok) || /^[A-G][#b]?-?\d$/.test(tok)) continue;
				for (const w of tok.split("/")) if (/[A-Za-z]/.test(w) && w.length > 1 && !LATIN_OK.has(w) && !LATIN_OK.has(w.replace(/^\d+/, "")) && !/^[A-G][#b]?-?\d$/.test(w)) Wn(t.where, `Latin word «${w}» is not a Rack name (check the spelling against the snapshot)`);
			}
		}
		// Quotes: straight double quotes with 4+ words must be verified quotes.
		for (const m of String(t.text).matchAll(/"([^"]{12,})"/g)) {
			const q = m[1].trim();
			if (q.split(/\s+/).length >= 4 && !quoteTexts.has(q) && !/device (output|input)/.test(q)) E(t.where, `quote not in quotes.json: «${q.slice(0, 50)}»`);
			if (q.split(/\s+/).length >= 15) E(t.where, "a quote of 15 words or more");
		}
	}
	if (longSentences > 6) Wn("style", `${longSentences} sentences of 19–22 words (prefer ≤ 18)`);
	// Glossary terms.
	const used = texts.flatMap((t) => terms(t.text));
	for (const k of used) if (!glossary[k.toLowerCase()]) E("terms", `[[${k}]] is not in the glossary`);
	for (const g of L.glossary || []) {
		if (!glossary[g.toLowerCase()]) E("glossary", `«${g}» is not in the glossary`);
		const first = firstUse(texts, g);
		if (!first) Wn("glossary", `term «${g}» never appears on a page`);
		else if (!first.marked) Wn(first.where, `first use of «${g}» should be bold or a [[link]]`);
	}
	// FAQ counts.
	for (const b of L.blocks) {
		const hasText = b.page || b.task || b.steps;
		const n = (b.faq || []).length;
		if (hasText && (n < 2 || n > 4) && b.type !== "debug") E(`${b.id}.faq`, `${n} FAQ items (2–4 per text block)`);
		if (b.type === "debug" && n < 1) E(`${b.id}.faq`, "a debug block needs at least one FAQ item");
	}
	// Sources.
	for (const s of L.sources || []) if (!sources[s]) E("sources", `unknown source «${s}»`);
	const quotesPerSource = {};
	for (const t of texts) for (const m of String(t.text).matchAll(/"([^"]{12,})"/g)) if (quoteTexts.has(m[1].trim())) quotesPerSource[quoteTexts.get(m[1].trim())] = (quotesPerSource[quoteTexts.get(m[1].trim())] || 0) + 1;
	if (Object.values(quotesPerSource).reduce((a, b) => a + b, 0) > 1) E("quotes", "at most one quote per lesson");
	// Pedagogy.
	const types = L.blocks.map((b) => b.type);
	if (types[0] !== "cover") E("structure", "the first block must be a cover");
	if (types.at(-1) !== "recap") E("structure", "the last block must be a recap");
	if (!types.includes("quiz") && !L.blocks.some((b) => b.type === "recap" && b.questions?.length)) E("structure", "at least one quiz");
	if (!types.includes("quiz")) Wn("structure", "no probe quiz early in the lesson");
	else if (types.indexOf("quiz") > 3) Wn("structure", "the probe quiz should come right after the cover");
	if (L.part > 0 && !types.some((t) => t === "live" || t === "anatomy")) E("structure", "at least one live or anatomy block");
	const meta = course.lessons[L.id];
	if (meta?.check && !types.includes("practice")) E("structure", `the old lesson has a check (${meta.check}) — add a practice block`);
	for (const b of L.blocks) if (b.type === "practice" && typeof b.check === "string" && meta?.check && !meta.check.includes(b.check)) Wn(b.id, `practice uses check ${b.check}, the old lesson used ${meta.check}`);
	// Anatomy coverage: control names the old «Анатомия» named should appear in the new lesson.
	const old = oldLesson(L.id);
	if (old?.sections["Анатомия"]) {
		const all = texts.map((t) => t.text).join("\n") + JSON.stringify(L.blocks.map((b) => b.later || []));
		const mods = new Set(L.blocks.flatMap((b) => [b.module, ...(b.modules || [])].filter(Boolean).map((m) => `${m.plugin}/${m.slug}`)));
		const missing = [];
		for (const key of mods) {
			const s = snapshots[key];
			if (!s) continue;
			for (const name of [...s.params.map((p) => p.name), ...s.inputs.map((p) => p.name), ...s.outputs.map((p) => p.name)]) {
				if (!name || name.length < 3) continue;
				if (old.sections["Анатомия"].includes(name) && !all.includes(name) && !all.includes(name.split(" ")[0])) missing.push(`${s.slug}.${name}`);
			}
		}
		if (missing.length) Wn("anatomy", `controls the old anatomy named but this lesson does not: ${[...new Set(missing)].join(", ")}`);
	}
	return { errors, warnings };
}

const results = [];
for (const f of lessonFiles()) {
	const L = loadLesson(f);
	if (only.length && !only.includes(L.id)) continue;
	results.push({ lesson: L.id, ...validateLesson(L) });
}
const errors = results.flatMap((r) => r.errors);
const warnings = results.flatMap((r) => r.warnings);
for (const w of warnings) console.log(`warning: ${w}`);
for (const e of errors) console.log(`error: ${e}`);
console.log(`validate: ${results.length} lessons, ${errors.length} errors, ${warnings.length} warnings`);
if (jsonOut) fs.writeFileSync(jsonOut, `${JSON.stringify(results, null, 1)}\n`);
else {
	fs.mkdirSync(path.join(RACK, "build"), { recursive: true });
	fs.writeFileSync(path.join(RACK, "build/validate.json"), `${JSON.stringify(results, null, 1)}\n`);
}
process.exit(errors.length || (strict && warnings.length) ? 1 : 0);
