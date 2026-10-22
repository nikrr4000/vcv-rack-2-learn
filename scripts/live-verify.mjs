#!/usr/bin/env node
/**
 * Live verification of demo blocks (plan §10.5): for each live / anatomy / try / debug block of the
 * compiled lessons, builds its standard modules, cables and params on the owner's Rack (MCP :2600,
 * the session's own row, every module deleted afterwards — nothing else is touched), reads the params
 * back (displayValue must match the block's targets) and checks that every output with a cable carries
 * a moving signal (5 samples 100 ms apart). Demo cables to the bus are wired to a muted Audio 2 here.
 *
 *   node scripts/live-verify.mjs [<lesson> …] [--port 2610] [--json <file>]
 */
import fs from "node:fs";
import path from "node:path";

import * as rackApi from "../server/rack-mcp.mjs";
import { rackReady, session, sleep } from "./lib/live.mjs";
import { RACK } from "../compiler/lib/data.mjs";

const argv = process.argv.slice(2);
const opt = (n) => {
	const i = argv.indexOf(n);
	if (i < 0) return null;
	const v = argv[i + 1];
	argv.splice(i, 2);
	return v;
};
const port = opt("--port");
const jsonOut = opt("--json");
if (port) process.env.RACK_PORT = port;
const pack = JSON.parse(fs.readFileSync(path.join(RACK, "plugin/res/course/pack.json"), "utf8"));
const report = JSON.parse(fs.readFileSync(path.join(RACK, "build/report.json"), "utf8"));
const targetsOf = new Map(report.lessons.flatMap((l) => l.demos.map((d) => [d.id, d.targets])));
const ids = argv.length ? argv : Object.keys(pack.lessons).filter((x) => !x.startsWith("_"));

const num = (s) => {
	const n = Number.parseFloat(String(s).replace(",", "."));
	return Number.isFinite(n) ? n : null;
};
const close = (a, b) => {
	const x = num(a);
	const y = num(b);
	if (x === null || y === null) return String(a).trim() === String(b).trim();
	return Math.abs(x - y) <= Math.max(0.02 * Math.abs(y), 0.02);
};

if (!(await rackReady())) {
	console.log("UNVERIFIED: Rack did not answer");
	process.exit(2);
}

const results = [];
for (const id of ids) {
	const L = pack.lessons[id];
	const patch = JSON.parse(fs.readFileSync(path.join(RACK, "plugin/res/course", L.patch), "utf8"));
	for (const b of L.blocks) {
		if (!["live", "anatomy", "try", "debug"].includes(b.type)) continue;
		const mods = patch.modules.filter((m) => m.pos[1] === b.row && m.plugin !== "Atlas");
		if (!mods.length) continue;
		const s = session();
		const r = { lesson: id, block: b.id, type: b.type, params: [], signals: [], errors: [] };
		try {
			const idOf = {};
			for (const m of mods) {
				idOf[m.id] = await s.add(m.plugin, m.model);
				if (m.plugin === "Core" && m.model.startsWith("AudioInterface")) await rackApi.setParams(idOf[m.id], [{ id: 0, value: 0 }]);
				else if (m.params.length) await rackApi.setParams(idOf[m.id], m.params.map((p) => ({ id: p.id, value: p.value })));
			}
			// Bus cables (to the row-0 mixer) go to a muted Audio 2 so the outputs count as connected.
			let sink = null;
			for (const c of patch.cables) {
				const fromHere = idOf[c.outputModuleId];
				const toHere = idOf[c.inputModuleId];
				if (fromHere && toHere) await rackApi.addCable(Number(fromHere), c.outputId, Number(toHere), c.inputId);
				else if (fromHere) {
					if (!sink) {
						sink = await s.add("Core", "AudioInterface2");
						await rackApi.setParams(sink, [{ id: 0, value: 0 }]);
					}
					await rackApi.addCable(Number(fromHere), c.outputId, Number(sink), 0).catch(() => {});
				}
			}
			await sleep(400);
			// Params: the compiled targets read back.
			for (const m of mods) {
				const live = await rackApi.params(idOf[m.id]);
				for (const p of m.params) {
					const lp = live.find((x) => x.id === p.id);
					const target = (targetsOf.get(m.id) || []).find((t) => t.name === lp?.name);
					const shown = Boolean(lp) && (!target?.display || close(lp.displayValue, target.display));
					r.params.push({ module: m.model, id: p.id, name: lp?.name, raw: p.value, target: target?.display ?? null, display: lp?.displayValue, ok: Boolean(lp) && Math.abs(lp.value - p.value) < 1e-3 && shown });
				}
			}
			// Signals: every output with a cable, 5 samples 100 ms apart.
			const outs = new Map();
			for (const c of patch.cables) if (idOf[c.outputModuleId]) outs.set(`${c.outputModuleId}:${c.outputId}`, c);
			const samples = {};
			for (let k = 0; k < 5; k++) {
				for (const [key, c] of outs) {
					const d = await rackApi.module(idOf[c.outputModuleId]);
					const o = d.outputs.find((x) => x.id === c.outputId);
					(samples[key] ||= []).push(Number(o?.voltage) || 0);
				}
				await sleep(100);
			}
			for (const [key, c] of outs) {
				const v = samples[key];
				const m = mods.find((x) => x.id === c.outputModuleId);
				const range = Math.max(...v) - Math.min(...v);
				const peak = Math.max(...v.map(Math.abs));
				r.signals.push({ module: m.model, output: c.outputId, range: Math.round(range * 1000) / 1000, peak: Math.round(peak * 1000) / 1000, moving: range > 0.05, present: peak > 0.05 });
			}
		} catch (e) {
			r.errors.push(e.message);
		} finally {
			r.cleanedUp = await s.cleanup();
		}
		r.ok = !r.errors.length && r.cleanedUp && r.params.every((p) => p.ok) && r.signals.every((x) => x.present);
		results.push(r);
		const bad = [...r.errors, ...r.params.filter((p) => !p.ok).map((p) => `param ${p.module}.${p.name}`), ...r.signals.filter((x) => !x.present).map((x) => `silent ${x.module} out ${x.output}`)];
		console.log(`${r.ok ? "PASS" : "FAIL"} ${id}/${b.id} (${b.type}) params ${r.params.length}, signals ${r.signals.map((x) => `${x.module}:${x.output}=${x.range}`).join(" ")}${bad.length ? ` — ${bad.join("; ")}` : ""}`);
	}
}
if (jsonOut) fs.writeFileSync(jsonOut, `${JSON.stringify(results, null, 1)}\n`);
const failed = results.filter((r) => !r.ok).length;
console.log(`live verify: ${results.length - failed}/${results.length} blocks ok`);
process.exit(failed ? 1 : 0);
