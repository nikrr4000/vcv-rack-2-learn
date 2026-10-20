#!/usr/bin/env node
/**
 * Applies a read-through edit list (plan §10.7) to the lesson YAML files:
 *   node rack/scripts/apply-edits.mjs <edits.json> […] [--dry]
 * Each edit {lesson, block, old, new, why}: `old` is replaced by `new` at its first occurrence after
 * the block's `- id: <block>` line and before the next block. An edit whose `old` is not found there
 * is reported and skipped; a file whose YAML no longer parses is left unchanged.
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import YAML from "../compiler/node_modules/yaml/dist/index.js";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const dry = process.argv.includes("--dry");
const lists = process.argv.slice(2).filter((a) => !a.startsWith("--"));
const byLesson = {};
for (const f of lists) for (const e of JSON.parse(fs.readFileSync(f, "utf8"))) (byLesson[e.lesson] ||= []).push(e);
let applied = 0;
const skipped = [];
for (const [id, edits] of Object.entries(byLesson)) {
	const file = path.join(REPO, "rack/course/lessons", `${id}.yaml`);
	let text = fs.readFileSync(file, "utf8");
	for (const e of edits) {
		const start = text.search(new RegExp(`^  - id: ${e.block}\\s*$`, "m"));
		if (start < 0) {
			skipped.push(`${id}/${e.block}: no such block`);
			continue;
		}
		const nextRel = text.slice(start + 1).search(/^  - id: /m);
		const end = nextRel < 0 ? text.length : start + 1 + nextRel;
		const at = text.indexOf(e.old, start);
		if (!e.old || at < 0 || at >= end) {
			skipped.push(`${id}/${e.block}: «${String(e.old).slice(0, 50)}» not found`);
			continue;
		}
		text = text.slice(0, at) + e.new + text.slice(at + e.old.length);
		applied++;
	}
	try {
		YAML.parse(text);
	} catch (err) {
		skipped.push(`${id}: YAML broken after edits (${err.message.split("\n")[0]}) — file left unchanged`);
		continue;
	}
	if (!dry) fs.writeFileSync(file, text);
}
console.log(`applied ${applied}, skipped ${skipped.length}`);
for (const s of skipped) console.log(`  skip ${s}`);
