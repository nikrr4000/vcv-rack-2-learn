#!/usr/bin/env node
// Updates a row of docs/PROGRESS.md: progress.mjs "<task prefix>" <STATUS> [commit] [notes]
import fs from "node:fs";
const file = new URL("../docs/PROGRESS.md", import.meta.url);
const [prefix, status, commit = "", notes] = process.argv.slice(2);
const lines = fs.readFileSync(file, "utf8").split("\n");
let hit = 0;
const out = lines.map((l) => {
	const cells = l.split("|");
	if (cells.length < 7 || !(cells[2].trim() + " ").startsWith(prefix + " ")) return l;
	hit++;
	cells[3] = ` ${status} `;
	if (commit) cells[4] = ` ${commit} `;
	if (notes !== undefined) cells[5] = ` ${notes} `;
	return cells.join("|");
});
if (!hit) {
	console.error(`no row starts with «${prefix}»`);
	process.exit(1);
}
fs.writeFileSync(file, out.join("\n"));
console.log(`${hit} row(s) → ${status}`);
