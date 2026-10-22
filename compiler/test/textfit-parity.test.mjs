/**
 * Text-fit parity: the compiler's Node layout against the plugin's C++ engine with real nanovg
 * measuring (plugin/tests/bin/render layout). Corpus: 200 paragraphs from the course's old
 * lessons at several widths. Lines must match exactly for at least 95 % of entries and never
 * differ by more than one line per paragraph.
 */
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { test } from "node:test";
import { fileURLToPath } from "node:url";

import { layout, paginate, textStyle } from "../lib/textfit.mjs";

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const RENDER = path.join(ROOT, "plugin/tests/bin/render");

function corpus() {
	const dir = path.join(ROOT, "facts/lessons");
	const paras = [];
	for (const f of fs.readdirSync(dir).sort()) {
		for (const line of fs.readFileSync(path.join(dir, f), "utf8").split("\n")) {
			const t = line.trim();
			if (t.length > 60 && !t.startsWith("|") && !t.startsWith("#") && !t.startsWith("---")) paras.push(t.replace(/\[([^\]]+)\]\([^)]+\)/g, "$1"));
		}
	}
	const widths = [96, 156, 186, 276, 336, 516];
	const out = [];
	for (let i = 0; i < 200; i++) {
		const md = paras[(i * 37) % paras.length];
		out.push({ md: i % 10 === 0 ? `# ${md.slice(0, 40)}\n${md}` : i % 7 === 0 ? `- ${md}` : md, width: widths[i % widths.length], height: 300, title: i % 20 === 0 });
	}
	return out;
}

test("Node text-fit matches the C++ engine on 200 paragraphs", { skip: !fs.existsSync(RENDER) && "build plugin/tests (make render) first" }, () => {
	const c = corpus();
	const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "parity-"));
	fs.writeFileSync(path.join(tmp, "corpus.json"), JSON.stringify(c));
	execFileSync(RENDER, ["layout", path.join(tmp, "corpus.json"), path.join(tmp, "out.json")], { cwd: ROOT });
	const cpp = JSON.parse(fs.readFileSync(path.join(tmp, "out.json"), "utf8"));
	let exact = 0;
	const diffs = [];
	c.forEach((e, i) => {
		const st = textStyle({ title: e.title });
		const doc = layout(e.md, e.width, st);
		const lines = doc.blocks.flatMap((b) => b.lines.map((l) => l.text));
		const pages = paginate(doc, e.height, st).length;
		if (JSON.stringify(lines) === JSON.stringify(cpp[i].lines) && pages === cpp[i].pages) exact++;
		else diffs.push({ i, width: e.width, node: lines.length, cpp: cpp[i].lines.length, first: lines.find((l, k) => l !== cpp[i].lines[k]), cppFirst: cpp[i].lines.find((l, k) => l !== lines[k]) });
		assert.ok(Math.abs(lines.length - cpp[i].lines.length) <= 1, `entry ${i}: ${lines.length} vs ${cpp[i].lines.length} lines`);
	});
	if (diffs.length) console.log(`differences (${diffs.length}):`, JSON.stringify(diffs.slice(0, 5), null, 1));
	console.log(`exact: ${exact}/${c.length}`);
	assert.ok(exact >= c.length * 0.95, `only ${exact}/${c.length} entries match exactly`);
});
