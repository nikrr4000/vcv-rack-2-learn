#!/usr/bin/env node
/**
 * Calibration of params whose display is not linear or exponential (dB levels, …): sweeps the
 * raw range on the owner's live Rack (port 2600, the session's own row, every module deleted
 * afterwards) and stores [raw, display] pairs in course/calibration.json.
 *
 *   node compiler/calibrate.mjs                      the needs the last compile reported
 *   node compiler/calibrate.mjs Core/AudioInterface2 Level
 */
import fs from "node:fs";
import path from "node:path";

import { rack, rackReady, session } from "../scripts/lib/live.mjs";
import { RACK } from "./lib/data.mjs";

const file = path.join(RACK, "course/calibration.json");
const cal = fs.existsSync(file) ? JSON.parse(fs.readFileSync(file, "utf8")) : {};
const argv = process.argv.slice(2);
let needs = argv.length >= 2 ? [{ key: argv[0], param: argv.slice(1).join(" ") }] : [];
if (!needs.length) {
	const p = path.join(RACK, "build/calibration-needs.json");
	needs = fs.existsSync(p) ? JSON.parse(fs.readFileSync(p, "utf8")) : [];
}
const todo = new Map();
for (const n of needs) if (!cal[n.key]?.[n.param]) todo.set(`${n.key}|${n.param}`, n);
if (!todo.size) {
	console.log("nothing to calibrate");
	process.exit(0);
}
if (!(await rackReady())) process.exit(1);
const s = session();
try {
	for (const { key, param } of todo.values()) {
		const [plugin, slug] = key.split("/");
		const id = await s.add(plugin, slug);
		if (plugin === "Core" && slug.startsWith("AudioInterface")) await rack.setParams(id, [{ id: 0, value: 0 }]);
		const ps = await rack.params(id);
		const p = ps.find((x) => x.name === param);
		if (!p) throw new Error(`${key} has no param «${param}»`);
		const table = [];
		for (let i = 0; i <= 80; i++) {
			const v = p.min + ((p.max - p.min) * i) / 80;
			await rack.setParams(id, [{ id: p.id, value: v }]);
			const now = (await rack.params(id)).find((x) => x.id === p.id);
			table.push([Math.round(v * 1e6) / 1e6, now.displayValue]);
		}
		(cal[key] ||= {})[param] = table;
		console.log(`${key} «${param}»: ${table.length} points (${table[0][1]} … ${table.at(-1)[1]})`);
		await s.remove(id);
	}
} finally {
	console.log(`cleanup: ${await s.cleanup()}`);
}
fs.writeFileSync(file, `${JSON.stringify(cal, null, 1)}\n`);
