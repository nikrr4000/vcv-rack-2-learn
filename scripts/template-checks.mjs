#!/usr/bin/env node
/**
 * Checks on the lesson templates without Rack (plan §10.4, the part that needs no running Rack):
 *   - every practice check fails on the untouched template (empty bench) and its first failing item
 *     shows step 1 of its hint ladder;
 *   - every debug check fails on the template (the break is in place) and passes once the break is
 *     repaired (the expected cable added / the expected param set).
 * The patch comes from the compiled .vcv (modules, cables, raw params), display values are derived
 * from the snapshots (switch options, linear/exponential laws, calibration tables).
 *
 *   node scripts/template-checks.mjs [<lesson> …]
 */
import fs from "node:fs";
import path from "node:path";

import { runCheck } from "../server/checker.mjs";
import { facts, RACK } from "../compiler/lib/data.mjs";
import { lawOf } from "../compiler/lib/resolve.mjs";

const pack = JSON.parse(fs.readFileSync(path.join(RACK, "plugin/res/course/pack.json"), "utf8"));
const { snapshots, calibration } = facts();
const ids = process.argv.slice(2).length ? process.argv.slice(2) : Object.keys(pack.lessons).filter((x) => !x.startsWith("_"));
const num = (s) => Number.parseFloat(String(s).replace(",", "."));

function displayOf(key, p, raw) {
	if (Array.isArray(p.options)) return p.options[Math.round(raw - p.min)] ?? "";
	const law = calibration[key]?.[p.name] ? null : lawOf(p);
	const t = (raw - p.min) / (p.max - p.min);
	const a = num(p.display?.min);
	const b = num(p.display?.max);
	if (law === "linear") return String(Math.round((a + (b - a) * t) * 1000) / 1000);
	if (law === "exp") return String(Math.round(a * (b / a) ** t * 1000) / 1000);
	const table = calibration[key]?.[p.name];
	if (table) {
		let best = table[0];
		for (const row of table) if (Math.abs(row[0] - raw) < Math.abs(best[0] - raw)) best = row;
		return best[1];
	}
	// No law, no table: the points the snapshot saw (exact), else piecewise-linear between them.
	const seen = [[p.min, p.display?.min], [(p.min + p.max) / 2, p.display?.mid], [p.max, p.display?.max], [p.default, p.displayDefault]]
		.filter(([, d]) => d !== undefined && Number.isFinite(num(d)))
		.map(([r, d]) => [r, num(d)])
		.sort((x, y) => x[0] - y[0]);
	const hit = seen.find(([r]) => Math.abs(r - raw) < 1e-6);
	if (hit) return String(hit[1]);
	for (let i = 0; i + 1 < seen.length; i++) if (raw >= seen[i][0] && raw <= seen[i + 1][0]) return String(seen[i][1] + ((raw - seen[i][0]) / (seen[i + 1][0] - seen[i][0])) * (seen[i + 1][1] - seen[i][1]));
	return p.displayDefault ?? "";
}

function fakeRack(patch, scope, extraCables = [], paramOverrides = {}) {
	const mods = patch.modules.filter((m) => scope.includes(m.id));
	const detail = (m) => {
		const key = `${m.plugin}/${m.model}`;
		const s = snapshots[key];
		return {
			id: String(m.id),
			plugin: m.plugin,
			slug: m.model,
			name: s?.name || m.model,
			params: (s?.params || []).map((p) => {
				const set = paramOverrides[`${m.id}:${p.id}`] ?? m.params.find((x) => x.id === p.id)?.value ?? p.default;
				return { id: p.id, name: p.name, unit: p.unit, displayValue: displayOf(key, p, set) };
			}),
			inputs: (s?.inputs || []).map((x) => ({ id: x.id, name: x.name })),
			outputs: (s?.outputs || []).map((x) => ({ id: x.id, name: x.name, voltage: 0 })),
		};
	};
	const cables = [...patch.cables, ...extraCables].filter((c) => scope.includes(c.outputModuleId) && scope.includes(c.inputModuleId)).map((c) => ({ ...c, outputModuleId: String(c.outputModuleId), inputModuleId: String(c.inputModuleId) }));
	return {
		modules: async () => mods.map((m) => ({ id: String(m.id), plugin: m.plugin, slug: m.model })),
		module: async (id) => detail(mods.find((m) => String(m.m ?? m.id) === String(id))),
		cables: async () => cables,
	};
}

let bad = 0;
let n = 0;
for (const id of ids) {
	const L = pack.lessons[id];
	const patch = JSON.parse(fs.readFileSync(path.join(RACK, "plugin/res/course", L.patch), "utf8"));
	for (const b of L.blocks)
		for (const it of b.items) {
			if (it.kind !== "task" || !it.check) continue;
			n++;
			const scope = it.mode === "debug" ? it.scope : it.given;
			const r = await runCheck(it.check, { rack: fakeRack(patch, scope), sleep: async () => {} });
			const first = r.items.find((x) => !x.ok);
			const ladder = first && it.hints?.[first.key];
			const msgs = [];
			if (r.passed) msgs.push("passes on the untouched template");
			if (first && ladder && it.mode === "practice" && !ladder[0]) msgs.push(`no ladder step 1 for ${first.key}`);
			if (it.mode === "debug") {
				// Repair the break: the expected cable(s) and param(s).
				const ref = Object.fromEntries((it.check.modules || []).map((m) => [m.ref, null]));
				// Each module takes one ref (two VCOs → vco_a, vco_b in patch order).
				for (const m of patch.modules.filter((x) => scope.includes(x.id))) {
					const k = Object.keys(ref).find((r) => !ref[r] && it.check.modules.find((x) => x.ref === r)?.slug === m.model);
					if (k) ref[k] = m;
				}
				const extra = [];
				const over = {};
				const ec = Array.isArray(it.check.expect_cable?.[0]) ? it.check.expect_cable : it.check.expect_cable ? [it.check.expect_cable] : [];
				for (const c of ec) {
					const a = ref[c[0]];
					const z = ref[c[2]];
					const o = snapshots[`${a.plugin}/${a.model}`].outputs.find((x) => x.name === c[1]);
					const i = snapshots[`${z.plugin}/${z.model}`].inputs.find((x) => x.name === c[3]);
					extra.push({ outputModuleId: a.id, outputId: o.id, inputModuleId: z.id, inputId: i.id });
				}
				const ep = Array.isArray(it.check.expect_param) ? it.check.expect_param : it.check.expect_param ? [it.check.expect_param] : [];
				for (const p of ep) {
					const m = ref[p.module];
					const sp = snapshots[`${m.plugin}/${m.model}`].params.find((x) => x.name === p.name);
					if (p.display_in && sp.options) over[`${m.id}:${sp.id}`] = sp.min + sp.options.indexOf(p.display_in[0]);
					else {
						for (let k = 0; k <= 20000; k++) {
							const v = sp.min + ((sp.max - sp.min) * k) / 20000;
							const d = num(displayOf(`${m.plugin}/${m.model}`, sp, v));
							const inList = !p.display_in || p.display_in.some((x) => num(x) === d);
							if (inList && (p.display_min === undefined || d >= p.display_min) && (p.display_max === undefined || d <= p.display_max)) {
								over[`${m.id}:${sp.id}`] = v;
								break;
							}
						}
					}
				}
				const fixed = await runCheck(it.check, { rack: fakeRack(patch, scope, extra, over), sleep: async () => {} });
				if (!fixed.passed) msgs.push(`still fails after the repair: ${fixed.items.filter((x) => !x.ok).map((x) => x.key).join(", ")}`);
			}
			if (msgs.length) bad++;
			console.log(`${msgs.length ? "FAIL" : "ok  "} ${id}/${b.id} (${it.mode}) first failing: ${first?.key || "—"} → «${(ladder?.[0] || first?.hint || "").slice(0, 60)}»${msgs.length ? ` — ${msgs.join("; ")}` : ""}`);
		}
}
console.log(`template checks: ${n - bad}/${n} ok`);
process.exit(bad ? 1 : 0);
