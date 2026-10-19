/**
 * Deterministic practice checks: the live patch (REST) against `checks/<id>.json`.
 * No model is involved; the result is a list of `{ok, what, hint}` items in Russian
 * plus `passed`.
 *
 * Check file (appendix B of the plan):
 *   modules  [{ref, plugin, slug}]                      — `ref` is a local name
 *   cables   [{from: [ref, outputName], to: [ref, inputName]}]
 *   params   [{module: ref, name, display_min?, display_max?, display_in?: [..]}]
 *   probes   [{module: ref, output, ask, samples, interval_ms, expect: {max_gt?, min_lt?, range_gt?}}]
 *   hints    {"module:<ref>" | "cable:<i>" | "param:<i>" | "probe:<i>": text}   (i is 1-based)
 *
 * Kind `fixed` (debug blocks of the Rack course): `{kind: "fixed", modules, expect_cable: [[ref, out, ref, in], …],
 * expect_param: [{module, name, display_*}]}` — the expectations become cables/params whose items all
 * carry the key `fixed` (hint `hints.fixed`). Every result item has a `key` (`module:<ref>`, `cable:<i>`, …).
 *
 * Ports and parameters are matched by **name** from `GET /modules/:id`, never by index.
 * Several modules of one kind: every assignment of refs to distinct modules is tried and
 * the one that satisfies the most cables and params wins.
 */

import * as defaultRack from "./rack.mjs";

const MAX_ASSIGNMENTS = 20000;

const norm = (s) => String(s ?? "").trim();
const label = (detail, ref) => (detail ? detail.name || detail.slug : ref);
const portIndex = (ports, name) => {
	const p = (ports || []).find((x) => norm(x.name) === norm(name));
	return p ? p.id : -1;
};
const displayNumber = (s) => {
	const n = Number.parseFloat(String(s).replace(",", "."));
	return Number.isFinite(n) ? n : null;
};

function paramOk(spec, param) {
	if (!param) return false;
	if (Array.isArray(spec.display_in)) return spec.display_in.map(norm).includes(norm(param.displayValue));
	const v = displayNumber(param.displayValue);
	if (v === null) return false;
	if (spec.display_min !== undefined && v < spec.display_min) return false;
	if (spec.display_max !== undefined && v > spec.display_max) return false;
	return true;
}

function paramText(spec) {
	if (Array.isArray(spec.display_in)) return `«${spec.display_in.join("» или «")}»`;
	if (spec.display_min !== undefined && spec.display_max !== undefined) return `от ${spec.display_min} до ${spec.display_max}`;
	if (spec.display_max !== undefined) return `не больше ${spec.display_max}`;
	if (spec.display_min !== undefined) return `не меньше ${spec.display_min}`;
	return "";
}

/** All assignments ref → module id (distinct modules), each ref with its candidates. */
function* assignments(refs, candidates, i = 0, used = new Set(), acc = {}) {
	if (i === refs.length) {
		yield { ...acc };
		return;
	}
	const ref = refs[i];
	const list = candidates[ref];
	if (!list.length) {
		yield* assignments(refs, candidates, i + 1, used, { ...acc, [ref]: null });
		return;
	}
	for (const id of list) {
		if (used.has(id)) continue;
		used.add(id);
		yield* assignments(refs, candidates, i + 1, used, { ...acc, [ref]: id });
		used.delete(id);
	}
	if (list.every((id) => used.has(id))) yield* assignments(refs, candidates, i + 1, used, { ...acc, [ref]: null });
}

function evaluate(spec, assign, details, cables) {
	const cableResults = (spec.cables || []).map((c) => {
		const out = details[assign[c.from[0]]];
		const inp = details[assign[c.to[0]]];
		if (!out || !inp) return false;
		const outId = portIndex(out.outputs, c.from[1]);
		const inId = portIndex(inp.inputs, c.to[1]);
		if (outId < 0 || inId < 0) return false;
		return cables.some((k) => String(k.outputModuleId) === String(out.id) && k.outputId === outId && String(k.inputModuleId) === String(inp.id) && k.inputId === inId);
	});
	const paramResults = (spec.params || []).map((p) => {
		const d = details[assign[p.module]];
		return d ? paramOk(p, (d.params || []).find((x) => norm(x.name) === norm(p.name))) : false;
	});
	return { cableResults, paramResults, score: cableResults.filter(Boolean).length + paramResults.filter(Boolean).length };
}

/**
 * Runs a check. `exclude` — module ids that must not count (the agent's own demo modules).
 * `rack` — the REST client (tests pass a fake). `sleep` — for probes.
 */
/** Normalizes a `fixed` check into the plain shape; keys every item as `fixed`. */
export function normalizeCheck(spec) {
	if (spec?.kind !== "fixed") return { spec, fixed: false };
	const cables = (Array.isArray(spec.expect_cable?.[0]) ? spec.expect_cable : spec.expect_cable ? [spec.expect_cable] : []).map((c) => ({ from: [c[0], c[1]], to: [c[2], c[3]] }));
	const params = Array.isArray(spec.expect_param) ? spec.expect_param : spec.expect_param ? [spec.expect_param] : [];
	return { spec: { ...spec, cables, params, probes: [] }, fixed: true };
}

export async function runCheck(rawSpec, { exclude = [], rack = defaultRack, sleep = (ms) => new Promise((r) => setTimeout(r, ms)) } = {}) {
	const { spec, fixed } = normalizeCheck(rawSpec);
	const keyOf = (k) => (fixed && !k.startsWith("module:") ? "fixed" : k);
	const skip = new Set(exclude.map(String));
	const all = (await rack.modules()).filter((m) => m && !skip.has(String(m.id)));
	const refs = (spec.modules || []).map((m) => m.ref);
	const candidates = {};
	const details = {};
	for (const m of spec.modules || []) {
		candidates[m.ref] = all.filter((x) => x.plugin === m.plugin && x.slug === m.slug).map((x) => String(x.id));
		for (const id of candidates[m.ref]) if (!details[id]) details[id] = await rack.module(id);
	}
	const cables = (await rack.cables()).filter(Boolean);

	let best = null;
	let n = 0;
	for (const assign of assignments(refs, candidates)) {
		const result = evaluate(spec, assign, details, cables);
		if (!best || result.score > best.result.score) best = { assign, result };
		if (++n >= MAX_ASSIGNMENTS) break;
	}
	const assign = best?.assign || Object.fromEntries(refs.map((r) => [r, null]));
	const { cableResults, paramResults } = best?.result || evaluate(spec, assign, details, cables);
	const hints = spec.hints || {};
	const nameOf = (ref) => {
		const m = (spec.modules || []).find((x) => x.ref === ref);
		return label(details[assign[ref]], m ? m.slug : ref);
	};
	const items = [];

	for (const m of spec.modules || []) {
		const ok = Boolean(assign[m.ref]);
		items.push({
			key: `module:${m.ref}`,
			ok,
			what: `Модуль ${m.slug} в патче`,
			hint: ok ? null : hints[`module:${m.ref}`] || `Не нашёл модуль ${m.slug} (${m.plugin}). Добавь его из браузера модулей.`,
		});
	}
	(spec.cables || []).forEach((c, i) => {
		const ok = cableResults[i];
		items.push({
			key: keyOf(`cable:${i + 1}`),
			ok,
			what: `Кабель ${nameOf(c.from[0])} «${c.from[1]}» → ${nameOf(c.to[0])} «${c.to[1]}»`,
			hint: ok ? null : hints[keyOf(`cable:${i + 1}`)] || `Нет кабеля из выхода «${c.from[1]}» модуля ${nameOf(c.from[0])} во вход «${c.to[1]}» модуля ${nameOf(c.to[0])}.`,
		});
	});
	(spec.params || []).forEach((p, i) => {
		const ok = paramResults[i];
		const d = details[assign[p.module]];
		const now = d ? (d.params || []).find((x) => norm(x.name) === norm(p.name)) : null;
		items.push({
			key: keyOf(`param:${i + 1}`),
			ok,
			what: `${nameOf(p.module)}: «${p.name}» ${paramText(p)}`.trim(),
			hint: ok ? null : hints[keyOf(`param:${i + 1}`)] || (now ? `Сейчас «${p.name}» = ${now.displayValue}${now.unit || ""}.` : `Ручка «${p.name}» не найдена.`),
		});
	});
	for (const [i, p] of (spec.probes || []).entries()) {
		const id = assign[p.module];
		let ok = false;
		let seen = [];
		if (id) {
			for (let s = 0; s < (p.samples || 10); s++) {
				const d = await rack.module(id);
				const port = (d.outputs || []).find((x) => norm(x.name) === norm(p.output));
				if (port) seen.push(Number(port.voltage) || 0);
				if (s < (p.samples || 10) - 1) await sleep(p.interval_ms || 50);
			}
			if (seen.length) {
				const max = Math.max(...seen);
				const min = Math.min(...seen);
				const e = p.expect || {};
				ok = (e.max_gt === undefined || max > e.max_gt) && (e.min_lt === undefined || min < e.min_lt) && (e.range_gt === undefined || max - min > e.range_gt);
			}
		}
		items.push({
			key: `probe:${i + 1}`,
			ok,
			what: `Сигнал на выходе «${p.output}» модуля ${nameOf(p.module)}`,
			hint: ok ? null : hints[`probe:${i + 1}`] || p.ask || "Сигнал не меняется.",
		});
	}
	return { passed: items.length > 0 && items.every((i) => i.ok), items, at: new Date().toISOString() };
}

/** The message for the lesson: «Ученик нажал «Проверить». Результат: …». */
export function checkMessage(result) {
	const lines = result.items.map((i) => (i.ok ? `✓ ${i.what}` : `✗ ${i.what} — ${i.hint}`));
	return `Ученик нажал «Проверить». Результат: ${result.passed ? "проверка пройдена" : "есть ошибки"}.\n${lines.join("\n")}`;
}
