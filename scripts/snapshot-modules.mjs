#!/usr/bin/env node
/**
 * Live module snapshots for the course: `facts/modules/_snapshot/<Plugin>.<Slug>.json`.
 *
 *   node scripts/snapshot-modules.mjs [Plugin/Slug …]     (default: every module of course.json)
 *
 * One module at a time: the exact slug is confirmed in `GET /library/<plugin>`, the module is
 * added in the session's own row, `GET /modules/:id` and `/params` are saved, every parameter
 * is moved to min, middle and max to record its `displayValue` there and set back to its
 * default, and the module is deleted. The MCP Server module is never added (a second copy would
 * start a second server): its snapshot is read from the module already in the patch. The
 * Recorder's parameters are not moved: its record button would write a file outside the repo.
 * Shape: `{plugin, slug, name, origin: "live", capturedAt, library, params, inputs, outputs}`.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { rack, rackReady, session } from "./lib/live.mjs";

const COURSE = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "facts");
const OUT = path.join(COURSE, "modules", "_snapshot");
const course = JSON.parse(fs.readFileSync(path.join(COURSE, "course.json"), "utf8"));
const wanted = process.argv.slice(2).length ? process.argv.slice(2) : [...new Set(Object.values(course.lessons).flatMap((l) => l.modules))].sort();

const port = (p) => ({ id: p.id, name: p.name, description: p.description || "" });
const libraries = {};

async function model(plugin, slug) {
	if (!libraries[plugin]) libraries[plugin] = await rack.rackRequest("GET", `/library/${encodeURIComponent(plugin)}`).catch(() => null);
	return libraries[plugin]?.modules?.find((m) => m.slug === slug) || null;
}

/** displayValue at min, middle and max; the parameter is set back to its value before. */
async function sweep(id, params) {
	const out = {};
	for (const p of params) {
		const points = { min: p.min, mid: p.snap ? Math.round((p.min + p.max) / 2) : (p.min + p.max) / 2, max: p.max };
		const display = {};
		for (const [k, v] of Object.entries(points)) {
			await rack.setParams(id, [{ id: p.id, value: v }]);
			const now = (await rack.params(id)).find((x) => x.id === p.id);
			display[k] = now ? now.displayValue : null;
		}
		await rack.setParams(id, [{ id: p.id, value: p.value }]);
		out[p.id] = display;
	}
	return out;
}

const results = [];
const live = session();
try {
	for (const name of wanted) {
		const [plugin, slug] = name.split("/");
		if (!(await rackReady())) {
			results.push({ module: name, status: "UNVERIFIED: Rack did not answer" });
			continue;
		}
		const info = await model(plugin, slug);
		if (!info) {
			results.push({ module: name, status: "NOT FOUND in the live library" });
			continue;
		}
		let id;
		let reused = false;
		if (plugin === "VCVRackMcpServer") {
			const existing = (await rack.modules()).find((m) => m.plugin === plugin && m.slug === slug);
			if (!existing) {
				results.push({ module: name, status: "no MCP Server module in the patch" });
				continue;
			}
			id = String(existing.id);
			reused = true;
		} else id = await live.add(plugin, slug);
		try {
			const detail = await rack.module(id);
			const params = await rack.params(id);
			const noSweep = reused || plugin === "VCV-Recorder";
			const display = noSweep ? {} : await sweep(id, params);
			const snap = {
				plugin,
				slug,
				name: detail.name,
				origin: "live",
				capturedAt: new Date().toISOString(),
				...(reused ? { note: "read from the MCP Server module already in the patch; parameters were not moved" } : {}),
				...(plugin === "VCV-Recorder" ? { note: "parameters were not moved: the record button would start writing a file" } : {}),
				library: { name: info.name, description: info.description || "", tags: info.tags || [] },
				params: params.map((p) => ({
					id: p.id,
					name: p.name,
					unit: p.unit,
					min: p.min,
					max: p.max,
					default: p.default,
					displayDefault: p.displayValue,
					...(display[p.id] ? { display: display[p.id] } : {}),
					...(p.options ? { options: p.options } : {}),
					snap: p.snap,
				})),
				inputs: detail.inputs.map(port),
				outputs: detail.outputs.map(port),
			};
			fs.mkdirSync(OUT, { recursive: true });
			fs.writeFileSync(path.join(OUT, `${plugin}.${slug}.json`), `${JSON.stringify(snap, null, 2)}\n`);
			results.push({ module: name, status: "ok", params: snap.params.length, inputs: snap.inputs.length, outputs: snap.outputs.length });
		} finally {
			if (!reused) await live.remove(id);
		}
		console.log(JSON.stringify(results.at(-1)));
	}
} finally {
	const clean = await live.cleanup();
	console.log(clean ? "cleanup: no session modules left" : `cleanup: LEFT ${live.created.join(", ")}`);
}
fs.writeFileSync(path.join(OUT, "_report.json"), `${JSON.stringify(results, null, 2)}\n`);
