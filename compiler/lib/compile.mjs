/**
 * Lesson compiler: one lesson YAML → its pack entry (text, items, checks, FAQ) and its Rack patch
 * (rows of blocks laid out by presets, plan §6.6).
 *
 * Patch layout:
 *   - block i is rack row i (y = i × 380 px); x in HP from 0, 1 HP gaps; a row fits 96 HP;
 *   - row 0 starts with the Navigator and ends with the fixed tail: MCP Server, the demo bus
 *     (VCA Mix per 4 sounding blocks, a Mix to sum them when there are several, mono or stereo)
 *     and the Audio module the bus feeds;
 *   - a practice row gets its own Audio module before the bench when its check needs one
 *     (`given: [audio]`), so the learner patches inside the row;
 *   - only the active block sounds: every demo cable that goes to `audio` is routed to its block's
 *     bus channel instead; channels start at 0 and the host opens the active block's channel.
 * Deviation from the plan (recorded in PROGRESS.md): practice rows have their own Audio module
 * (the checks need a free «To "device output 1"»), the bus keeps the row-0 Audio.
 */
import crypto from "node:crypto";

import { facts, loadCheck, RACK_VERSION, VERSIONS } from "./data.mjs";
import { findParam, findPort, ResolveError, resolveParam, snapshot } from "./resolve.mjs";
import { fitPage, plain, terms, TOKENS } from "./textfit.mjs";

export const ROW_HP = 96;
const GAP = 1;
const HP = 15;
const ROW_PX = 380;

const W = {
	navigator: 10,
	page: { cover: 28, story: 24, live: 20, anatomy: 16, quiz: 12, recap: 24, next: 12 },
	figure: 16,
	quiz: 14,
	task: 14,
	bench: 2,
	benchGap: 40,
};

export function noteWidth(text) {
	const words = plain(text).split(/\s+/).filter(Boolean).length;
	return words <= 12 ? 6 : words <= 25 ? 8 : 10;
}

function moduleWidth(plugin, slug) {
	const w = facts().widths[`${plugin}/${slug}`];
	return w || null;
}

const AUDIO_IN = ['To "device output 1"', 'To "device output 2"'];

export function compileLesson(L, { lessonIndex = 0, mcpPort = 2600, test = false } = {}) {
	const errors = [];
	const warnings = [];
	const err = (where, msg) => errors.push(`${L.id}${where ? `/${where}` : ""}: ${msg}`);
	const warn = (where, msg) => warnings.push(`${L.id}${where ? `/${where}` : ""}: ${msg}`);
	const needs = [];

	let nextId = (L.part * 100 + Number(L.num.split(".")[1])) * 1000 + 1;
	const modules = [];
	const cables = [];
	const newId = () => nextId++;
	const place = (row, x, plugin, model, extra = {}) => {
		const m = { id: newId(), plugin, model, version: VERSIONS[plugin] || "2.0.0", params: [], pos: [x, row], ...extra };
		modules.push(m);
		return m;
	};
	const cable = (out, outId, inp, inId) => cables.push({ id: newId(), outputModuleId: out.id, outputId: outId, inputModuleId: inp.id, inputId: inId, color: "#C0583B" });

	// ── which blocks sound, mono or stereo ──
	const soundBlocks = [];
	let stereo = false;
	L.blocks.forEach((b, i) => {
		if (b.type === "practice") return;
		const toAudio = (b.cables || []).filter((c) => c.to?.[0] === "audio");
		if (!toAudio.length) return;
		soundBlocks.push(i);
		if (toAudio.some((c) => c.to[1] === AUDIO_IN[1])) stereo = true;
	});
	const buses = stereo ? 2 : 1;
	const mixersPerBus = Math.max(1, Math.ceil(soundBlocks.length / 4));
	const tailHp = 6 + GAP + buses * mixersPerBus * (9 + GAP) + (mixersPerBus > 1 ? buses * (3 + GAP) : 0) + 5;

	// ── row 0 head: Navigator ──
	const atlasData = (block, item) => ({ lesson: L.id, block, item, state: {} });
	const nav = place(0, 0, "Atlas", "Navigator", { data: { lesson: L.id, block: "", item: "", state: { layoutHash: "", blockIndex: 0 } } });
	const rows = L.blocks.map(() => ({ x: 0 }));
	rows[0].x = W.navigator + GAP;

	// ── the fixed tail of row 0 (placed after the cover so the cover can use the space) ──
	const tail = {};
	function placeTail(startX) {
		let x = startX;
		tail.mcp = place(0, x, "VCVRackMcpServer", "RackMcpServer");
		tail.mcp.params = [
			{ value: mcpPort, id: 0 },
			{ value: 1, id: 1 },
		];
		x += 6 + GAP;
		tail.mixers = [];
		for (let b = 0; b < buses; b++) {
			const list = [];
			for (let k = 0; k < mixersPerBus; k++) {
				const m = place(0, x, "Fundamental", "VCMixer");
				m.params = [1, 2, 3, 4].map((id) => ({ value: 0, id }));
				list.push(m);
				x += 9 + GAP;
			}
			tail.mixers.push(list);
		}
		tail.summers = [];
		if (mixersPerBus > 1)
			for (let b = 0; b < buses; b++) {
				const s = place(0, x, "Fundamental", "Mixer");
				tail.summers.push(s);
				x += 3 + GAP;
			}
		tail.demoAudio = place(0, x, "Core", "AudioInterface2");
		tail.demoAudio.params = [{ value: 1, id: 0 }];
		x += 5;
		// Bus wiring: mixers → (summer) → demo audio.
		for (let b = 0; b < buses; b++) {
			const outs = mixersPerBus > 1 ? [tail.summers[b]] : tail.mixers[b];
			if (mixersPerBus > 1) tail.mixers[b].forEach((m, k) => cable(m, 0, tail.summers[b], k));
			const src = outs[0];
			if (buses === 1) {
				cable(src, 0, tail.demoAudio, 0);
				cable(src, 0, tail.demoAudio, 1);
			} else cable(src, 0, tail.demoAudio, b);
		}
		return x;
	}

	// Bus channel of a sound block: {mixer per bus, input id, param id}.
	const channelOf = (blockIndex) => {
		const k = soundBlocks.indexOf(blockIndex);
		if (k < 0) return null;
		const mixer = Math.floor(k / 4);
		const ch = k % 4;
		return tail.mixers.map((list) => ({ mixer: list[mixer], input: 1 + ch, param: 1 + ch }));
	};

	const packBlocks = [];
	const reports = [];
	const allTerms = [];

	const pageItem = (b, id, text, hp, opts = {}) => {
		const fit = fitPage(text, hp, { title: opts.title });
		reports.push({ block: b.id, item: id, kind: "page", hp, pages: fit.pages, splitWords: fit.splitWords });
		if (fit.pages > 3) err(b.id, `${id}: ${fit.pages} pages at ${hp} HP (at most 3) — split the block`);
		if (fit.overflow) err(b.id, `${id}: a line does not fit the page`);
		if (fit.splitWords) warn(b.id, `${id}: a word longer than the line is hyphenated`);
		allTerms.push(...terms(text));
		return { id, kind: "page", text, width: hp, ...(opts.title ? { title: true } : {}) };
	};

	L.blocks.forEach((b, bi) => {
		const row = bi;
		const r = rows[bi];
		const items = [];
		const local = {}; // ref → placed module
		const localSpecs = {};
		const putAtlas = (kind, model, hp, item) => {
			const m = place(row, r.x, "Atlas", model, { data: atlasData(b.id, item.id) });
			r.x += hp + GAP;
			items.push(item);
			return m;
		};
		const putModule = (spec) => {
			if (local[spec.ref]) {
				err(b.id, `module ref «${spec.ref}» used twice in the block`);
				return null;
			}
			const snap = snapshot(spec.plugin, spec.slug);
			const w = moduleWidth(spec.plugin, spec.slug);
			if (!snap) err(b.id, `no snapshot for ${spec.plugin}/${spec.slug}`);
			if (!w) err(b.id, `no width for ${spec.plugin}/${spec.slug}`);
			const m = place(row, r.x, spec.plugin, spec.slug);
			r.x += (w || 10) + GAP;
			for (const [name, v] of Object.entries(spec.params || {})) {
				try {
					const p = resolveParam(spec.plugin, spec.slug, name, v);
					m.params.push({ value: p.value, id: p.id });
					(m.targets ||= []).push({ name, display: p.display, raw: p.value });
				} catch (e) {
					if (e instanceof ResolveError && e.need) needs.push(e.need);
					err(b.id, e.message);
				}
			}
			if (spec.data) m.data = spec.data;
			local[spec.ref] = m;
			localSpecs[spec.ref] = spec;
			return m;
		};
		// `audio`: in demos the block's bus channel (see wire); in practice the block's own Audio module.
		const resolveEnd = (ref) => (local[ref] ? { m: local[ref], spec: localSpecs[ref] } : ref === "audio" && tail.demoAudio ? { m: tail.demoAudio, spec: { plugin: "Core", slug: "AudioInterface2" } } : null);
		const wire = (c, { skip = null } = {}) => {
			const from = resolveEnd(c.from[0]);
			if (!from || c.from[0] === "audio") return err(b.id, `cable from unknown module «${c.from[0]}»`);
			const out = findPort(snapshot(from.spec.plugin, from.spec.slug), "output", c.from[1]);
			if (!out) return err(b.id, `${from.spec.slug} has no output «${c.from[1]}»`);
			if (c.to[0] === "audio") {
				const k = AUDIO_IN.indexOf(c.to[1]);
				if (k < 0) return err(b.id, `Audio has no input «${c.to[1]}»`);
				// Demo sound goes through the block's bus channel.
				const ch = channelOf(bi);
				const targets = buses === 1 ? [ch[0]] : k === 0 && !(b.cables || []).some((x) => x.to[0] === "audio" && x.to[1] === AUDIO_IN[1]) ? ch : [ch[k]];
				for (const t of targets) cable(from.m, out.id, t.mixer, t.input);
				return;
			}
			const to = resolveEnd(c.to[0]);
			if (!to) return err(b.id, `cable to unknown module «${c.to[0]}»`);
			const inp = findPort(snapshot(to.spec.plugin, to.spec.slug), "input", c.to[1]);
			if (!inp) return err(b.id, `${to.spec.slug} has no input «${c.to[1]}»`);
			if (skip && skip(c)) return;
			cable(from.m, out.id, to.m, inp.id);
		};
		const target = (t) => {
			const end = resolveEnd(t.module);
			if (!end) {
				err(b.id, `note target: unknown module «${t.module}»`);
				return null;
			}
			const snap = snapshot(end.spec.plugin, end.spec.slug);
			const kind = t.param ? "param" : t.input ? "input" : t.output ? "output" : null;
			const name = t.param || t.input || t.output;
			if (!kind) {
				err(b.id, "note target needs param, input or output");
				return null;
			}
			const found = kind === "param" ? findParam(snap, name) : findPort(snap, kind, name);
			if (!found) {
				err(b.id, `note target: ${end.spec.slug} has no ${kind} «${name}»`);
				return null;
			}
			return { moduleId: end.m.id, module: t.module, kind, index: found.id, name };
		};
		let noteN = 0;
		const putNote = (n) => {
			noteN++;
			const words = plain(n.text).split(/\s+/).filter(Boolean).length;
			if (words > 40) err(b.id, `note ${noteN}: ${words} words (at most 40)`);
			const hp = noteWidth(n.text);
			const item = { id: `note-${noteN}`, kind: "note", text: n.text, width: hp, targets: (n.targets || []).map(target).filter(Boolean) };
			allTerms.push(...terms(n.text));
			putAtlas("note", "Note", hp, item);
		};
		const putFigure = (fig, n = 1) => {
			const { caption, ...figure } = fig;
			putAtlas("figure", "Figure", W.figure, { id: `figure-${n}`, kind: "figure", width: W.figure, figure, caption: caption || "" });
		};
		const putQuiz = (questions) => {
			for (const q of questions) {
				const nCorrect = q.options.filter((o) => o.correct).length;
				if (nCorrect !== 1) err(b.id, `question «${q.q.slice(0, 40)}…» has ${nCorrect} correct options (need 1)`);
			}
			putAtlas("quiz", "Quiz", W.quiz, { id: "quiz-1", kind: "quiz", width: W.quiz, questions: questions.map((q) => ({ q: q.q, free: Boolean(q.free), options: q.options.map((o) => ({ text: o.text, correct: Boolean(o.correct), why: o.why })) })) });
		};

		const preset = b.preset || b.type;
		if (preset !== b.type) warn(b.id, `preset «${preset}» differs from type «${b.type}»`);
		if (bi === 0 && b.type !== "cover") warn(b.id, "the first block is not a cover");

		switch (b.type) {
			case "cover": {
				const figW = b.figure ? W.figure + GAP : 0;
				const avail = ROW_HP - r.x - tailHp - GAP - figW;
				const hp = Math.max(16, Math.min(W.page.cover, avail));
				const page = pageItem(b, "page-1", b.page, hp, { title: true });
				putAtlas("page", "Page", hp, page);
				if (b.figure) putFigure(b.figure);
				break;
			}
			case "story": {
				putAtlas("page", "Page", W.page.story, pageItem(b, "page-1", b.page, W.page.story));
				if (b.figure) putFigure(b.figure);
				if (b.page2) putAtlas("page", "Page", W.page.story, pageItem(b, "page-2", b.page2, W.page.story));
				break;
			}
			case "live": {
				putAtlas("page", "Page", W.page.live, pageItem(b, "page-1", b.page, W.page.live));
				const notes = b.notes || [];
				for (const spec of b.modules || []) {
					putModule(spec);
					for (const n of notes.filter((x) => x.after === spec.ref)) putNote(n);
				}
				for (const n of notes.filter((x) => !x.after || !local[x.after])) putNote(n);
				for (const c of b.cables || []) wire(c);
				if (b.figure) putFigure(b.figure);
				break;
			}
			case "anatomy": {
				const later = (b.later || []).map((l) => `- ${l.what} — урок ${l.lesson}`).join("\n");
				putAtlas("page", "Page", W.page.anatomy, pageItem(b, "page-1", later ? `${b.page.trimEnd()}\n**Позже в курсе:**\n${later}` : b.page, W.page.anatomy));
				if (b.module && b.module.ref !== "audio") putModule(b.module);
				if ((b.notes || []).length > 6) err(b.id, `${b.notes.length} notes (at most 6) — split the anatomy`);
				for (const n of b.notes || []) putNote(n);
				for (const c of b.cables || []) wire(c);
				break;
			}
			case "try": {
				const steps = (b.steps || []).map((s) => ({ do: s.do, see: s.see || "" }));
				for (const s of steps) allTerms.push(...terms(s.do), ...terms(s.see));
				putAtlas("task", "Task", W.task, { id: "task-1", kind: "task", mode: "try", width: W.task, text: b.task || "", steps });
				const notes = b.notes || [];
				for (const spec of b.modules || []) {
					putModule(spec);
					for (const n of notes.filter((x) => x.after === spec.ref)) putNote(n);
				}
				for (const n of notes.filter((x) => !x.after || !local[x.after])) putNote(n);
				for (const c of b.cables || []) wire(c);
				break;
			}
			case "quiz": {
				if (b.lead) putAtlas("page", "Page", W.page.quiz, pageItem(b, "page-1", b.lead, W.page.quiz));
				putQuiz(b.questions || []);
				break;
			}
			case "practice": {
				const check = typeof b.check === "string" ? loadCheck(b.check) : null;
				if (!check) err(b.id, `check file «${b.check}» not found in vcv/course/checks`);
				allTerms.push(...terms(b.task));
				const taskItem = { id: "task-1", kind: "task", mode: "practice", width: W.task, text: b.task, check, hints: b.hints || {}, given: [] };
				putAtlas("task", "Task", W.task, taskItem);
				for (const spec of b.modules || []) putModule(spec);
				// The learner's output for this practice: its own Audio module (several Audio modules share one device in Rack 2).
				if (check && (check.modules || []).some((m) => m.plugin === "Core" && m.slug === "AudioInterface2") && !local.audio) {
					const a = putModule({ ref: "audio", plugin: "Core", slug: "AudioInterface2" });
					if (a) a.params = [{ value: 1, id: 0 }];
				}
				for (const c of b.cables || []) wire(c);
				putAtlas("bench", "Bench", W.bench, { id: "bench-left", kind: "bench", side: "left", width: W.bench });
				r.x += W.benchGap;
				putAtlas("bench", "Bench", W.bench, { id: "bench-right", kind: "bench", side: "right", width: W.bench });
				// Given modules: the learner's Audio and anything prebuilt in the block.
				for (const g of b.given || []) {
					const end = resolveEnd(g);
					if (!end) err(b.id, `given: unknown module «${g}»`);
					else if (!taskItem.given.includes(end.m.id)) taskItem.given.push(end.m.id);
				}
				for (const m of Object.values(local)) if (!taskItem.given.includes(m.id)) taskItem.given.push(m.id);
				if (check) checkKeys(b, check, b.hints || {}, err, warn);
				break;
			}
			case "debug": {
				allTerms.push(...terms(b.task));
				const taskItem = { id: "task-1", kind: "task", mode: "debug", width: W.task, text: b.task, hints: b.hints || {}, scope: [] };
				putAtlas("task", "Task", W.task, taskItem);
				const brk = b.break || {};
				for (const spec of b.modules || []) {
					const s = { ...spec, params: { ...(spec.params || {}) } };
					if (brk.set_param && brk.set_param.module === spec.ref) s.params[brk.set_param.name] = brk.set_param.value ?? brk.set_param.display ?? { raw: brk.set_param.raw };
					putModule(s);
				}
				const removed = brk.remove_cable;
				let removedOne = false;
				for (const c of b.cables || []) {
					const isBreak = removed && c.from[0] === removed[0] && c.from[1] === removed[1] && c.to[0] === removed[2] && c.to[1] === removed[3];
					if (isBreak) {
						removedOne = true;
						continue;
					}
					wire(c);
				}
				if (removed && !removedOne) err(b.id, `break.remove_cable ${removed.join(" ")} is not one of the block's cables`);
				const notes = b.notes?.length ? b.notes : [{ text: "Здесь что-то не так. Найди и исправь." }];
				for (const n of notes) putNote(n);
				taskItem.scope = Object.values(local).map((m) => m.id);
				const fixed = { ...(typeof b.check === "object" ? b.check : {}) };
				const refs = new Set();
				const ec = Array.isArray(fixed.expect_cable?.[0]) ? fixed.expect_cable : fixed.expect_cable ? [fixed.expect_cable] : [];
				for (const c of ec) {
					refs.add(c[0]);
					refs.add(c[2]);
					const a = localSpecs[c[0]];
					const z = localSpecs[c[2]];
					if (!a || !z) err(b.id, `check.expect_cable refers to unknown modules ${c[0]}, ${c[2]}`);
					else {
						if (!findPort(snapshot(a.plugin, a.slug), "output", c[1])) err(b.id, `check: ${a.slug} has no output «${c[1]}»`);
						if (!findPort(snapshot(z.plugin, z.slug), "input", c[3])) err(b.id, `check: ${z.slug} has no input «${c[3]}»`);
					}
				}
				const ep = Array.isArray(fixed.expect_param) ? fixed.expect_param : fixed.expect_param ? [fixed.expect_param] : [];
				for (const p of ep) {
					refs.add(p.module);
					const s = localSpecs[p.module];
					if (!s || !findParam(snapshot(s.plugin, s.slug), p.name)) err(b.id, `check.expect_param: no param «${p.name}» on «${p.module}»`);
				}
				fixed.modules = [...refs].filter((x) => localSpecs[x]).map((x) => ({ ref: x, plugin: localSpecs[x].plugin, slug: localSpecs[x].slug }));
				fixed.hints = Object.fromEntries(Object.entries(b.hints || {}).map(([k, v]) => [k, v[0]]));
				taskItem.check = fixed;
				if (!b.hints?.fixed) err(b.id, "debug needs hints.fixed (3 steps)");
				if (!ec.length && !ep.length) err(b.id, "debug check needs expect_cable or expect_param");
				break;
			}
			case "recap": {
				putAtlas("page", "Page", W.page.recap, pageItem(b, "page-1", b.page, W.page.recap));
				if (b.questions?.length) putQuiz(b.questions);
				putAtlas("page", "Page", W.page.next, pageItem(b, "page-2", b.next, W.page.next));
				break;
			}
			default:
				err(b.id, `unknown block type «${b.type}»`);
		}
		if (bi === 0) {
			const end = placeTail(r.x);
			r.x = end + GAP;
		}
		const width = r.x - GAP;
		if (width > ROW_HP) err(b.id, `row is ${width} HP wide (at most ${ROW_HP}) — split the block`);
		for (const f of b.faq || []) allTerms.push(...terms(f.a));
		const plainText = [b.page, b.page2, b.lead, b.task, ...(b.steps || []).map((s) => `${s.do} ${s.see || ""}`), b.next]
			.filter(Boolean)
			.map(plain)
			.join("\n")
			.slice(0, 1200);
		packBlocks.push({
			id: b.id,
			type: b.type,
			title: b.title,
			row,
			bounds: { x: 0, y: row * ROW_PX, w: width * HP, h: ROW_PX },
			items,
			faq: b.faq || [],
			simpler: b.simpler || "",
			plain: plainText,
			_channelBlock: bi,
		});
	});

	// Channels and `@audio` given ids (the tail exists now).
	for (const pb of packBlocks) {
		const ch = channelOf(pb._channelBlock);
		pb.channels = ch ? ch.map((c) => ({ mixerId: c.mixer.id, param: c.param, level: 1 })) : [];
		pb.channel = pb.channels[0] || null;
		delete pb._channelBlock;
	}

	const layoutHash = crypto
		.createHash("sha1")
		.update(JSON.stringify({ m: modules.map((m) => [m.id, m.plugin, m.model, m.pos]), c: cables.map((c) => [c.outputModuleId, c.outputId, c.inputModuleId, c.inputId]) }))
		.digest("hex")
		.slice(0, 12);
	nav.data.state.layoutHash = layoutHash;

	const patch = {
		version: RACK_VERSION,
		zoom: 1,
		gridOffset: [-1, -1],
		modules: modules.map((m) => {
			const out = { id: m.id, plugin: m.plugin, model: m.model, version: m.version, params: m.params, pos: m.pos };
			if (m.data) out.data = m.data;
			return out;
		}),
		cables,
	};

	const pack = {
		num: L.num,
		part: L.part,
		title: L.title,
		goal: L.goal,
		builds: L.builds,
		minutes: L.minutes,
		requires: L.requires || [],
		patch: `patches/${L.id}.vcv`,
		layoutHash,
		blocks: packBlocks,
	};

	// What live verification and reports need: every standard module with its targets.
	const demos = modules.filter((m) => m.plugin !== "Atlas" && m.pos[1] > 0 && m.targets).map((m) => ({ id: m.id, plugin: m.plugin, model: m.model, row: m.pos[1], targets: m.targets }));
	return { pack, patch, errors, warnings, needs, report: { lesson: L.id, blocks: packBlocks.length, texts: reports, stereo, soundBlocks: soundBlocks.length, rowWidths: rows.map((r) => r.x - GAP), demos, terms: allTerms } };
}

/** Hint ladders must use the check's item keys; every check item should have a ladder. */
function checkKeys(b, check, hints, err, warn) {
	const keys = [
		...(check.modules || []).map((m) => `module:${m.ref}`),
		...(check.cables || []).map((_, i) => `cable:${i + 1}`),
		...(check.params || []).map((_, i) => `param:${i + 1}`),
		...(check.probes || []).map((_, i) => `probe:${i + 1}`),
	];
	for (const k of Object.keys(hints)) if (!keys.includes(k)) err(b.id, `hints key «${k}» is not an item of check ${b.check} (keys: ${keys.join(", ")})`);
	for (const k of keys) if (!k.startsWith("module:") && !hints[k]) warn(b.id, `no hint ladder for ${k} (the check file's single hint is used)`);
}

export { TOKENS };
