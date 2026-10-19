#!/usr/bin/env node
/**
 * Build gates on the compiled output (plan §10.1, §8): shipped patches carry the MCP Server on 2600
 * (test builds on 2610) and no test settings; every pack lesson has its patch; every Atlas module in
 * a patch points at an item of the pack; every row fits 96 HP; every practice has a check and ladders;
 * every module of a patch exists in a snapshot (or is an Atlas module).
 */
import fs from "node:fs";
import path from "node:path";

import { facts, RACK } from "./lib/data.mjs";

const test = process.argv.includes("--test");
const root = test ? path.join(RACK, "build/test-res/course") : path.join(RACK, "plugin/res/course");
const pack = JSON.parse(fs.readFileSync(path.join(root, "pack.json"), "utf8"));
const { snapshots, widths } = facts();
const errors = [];
let patches = 0;
for (const [id, L] of Object.entries(pack.lessons)) {
	if (id.startsWith("_")) continue;
	const file = path.join(root, L.patch);
	if (!fs.existsSync(file)) {
		errors.push(`${id}: patch ${L.patch} missing`);
		continue;
	}
	patches++;
	const p = JSON.parse(fs.readFileSync(file, "utf8"));
	const mcp = p.modules.filter((m) => m.plugin === "VCVRackMcpServer");
	if (mcp.length !== 1) errors.push(`${id}: ${mcp.length} MCP Server modules`);
	else {
		const port = mcp[0].params.find((x) => x.id === 0)?.value;
		if (port !== (test ? 2610 : 2600)) errors.push(`${id}: MCP Server on ${port}, expected ${test ? 2610 : 2600}`);
		if (mcp[0].params.find((x) => x.id === 1)?.value !== 1) errors.push(`${id}: MCP Server not enabled`);
	}
	if (!p.modules.some((m) => m.model === "Navigator" && m.pos[0] === 0 && m.pos[1] === 0)) errors.push(`${id}: Navigator is not the first module of row 0`);
	const rowsHp = {};
	for (const m of p.modules) {
		if (m.plugin === "Atlas") {
			if (m.model === "Navigator") continue;
			const b = L.blocks.find((x) => x.id === m.data?.block);
			if (!b || !b.items.some((it) => it.id === m.data.item)) errors.push(`${id}: ${m.model} ${m.id} → ${m.data?.block}/${m.data?.item} not in the pack`);
		} else if (m.plugin !== "VCVRackMcpServer" && !snapshots[`${m.plugin}/${m.model}`]) errors.push(`${id}: ${m.plugin}/${m.model} has no snapshot`);
		const w = m.plugin === "Atlas" ? (m.model === "Navigator" ? 10 : L.blocks.flatMap((b) => b.items).find((it) => it.id === m.data?.item && L.blocks.find((b) => b.id === m.data.block)?.items.includes(it))?.width || 0) : widths[`${m.plugin}/${m.model}`] || 0;
		rowsHp[m.pos[1]] = Math.max(rowsHp[m.pos[1]] || 0, m.pos[0] + w);
	}
	for (const [row, hp] of Object.entries(rowsHp)) if (hp > 96) errors.push(`${id}: row ${row} is ${hp} HP`);
	for (const b of L.blocks)
		for (const it of b.items)
			if (it.kind === "task" && it.mode === "practice") {
				if (!it.check) errors.push(`${id}/${b.id}: practice without a check`);
				if (!Object.keys(it.hints || {}).length) errors.push(`${id}/${b.id}: practice without hint ladders`);
			}
	if (JSON.stringify(p).includes("2610") && !test) errors.push(`${id}: shipped patch mentions the test port 2610`);
}
for (const e of errors) console.log(`gate: ${e}`);
console.log(`gates: ${patches} patches, ${errors.length} problems`);
process.exit(errors.length ? 1 : 0);
