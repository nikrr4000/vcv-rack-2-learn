#!/usr/bin/env node
/**
 * Course compiler: rack/course/lessons/*.yaml → the plugin's course pack and lesson patches.
 *
 *   node rack/compiler/compile.mjs [--test] [--only <id,…>] [--out <dir>]
 *
 * Shipped build (default): rack/plugin/res/course/{pack.json, patches/*.vcv}, MCP Server on 2600.
 * Test build (--test): rack/build/test-res/course/…, MCP Server on 2610.
 * Always: rack/build/report.json (per-lesson stats, text pages, warnings) and rack/qa/gallery.vcv.
 * Exit 1 on any error (schema, references, row widths, text that does not fit, …).
 */
import fs from "node:fs";
import path from "node:path";

import Ajv from "ajv";

import { compileLesson } from "./lib/compile.mjs";
import { facts, lessonFiles, loadLesson, RACK, RACK_VERSION, VERSIONS } from "./lib/data.mjs";
import { gallery } from "./lib/gallery.mjs";

const argv = process.argv.slice(2);
const opt = (n) => {
	const i = argv.indexOf(n);
	return i < 0 ? null : argv[i + 1];
};
const test = argv.includes("--test");
const only = opt("--only")?.split(",") || null;
const outRoot = opt("--out") || (test ? path.join(RACK, "build/test-res/course") : path.join(RACK, "plugin/res/course"));
const mcpPort = test ? 2610 : 2600;

const schema = JSON.parse(fs.readFileSync(path.join(RACK, "compiler/schema/lesson.schema.json"), "utf8"));
const ajv = new Ajv({ allErrors: true, strict: false });
const validateSchema = ajv.compile(schema);

const { course, glossary } = facts();
const order = course.parts.flatMap((p) => p.lessons);
const errors = [];
const warnings = [];
const lessons = {};
const reports = [];
const patches = {};
const needs = [];

const files = lessonFiles().filter((f) => !only || only.includes(path.basename(f, ".yaml")));
for (const file of files) {
	let L;
	try {
		L = loadLesson(file);
	} catch (e) {
		errors.push(`${path.basename(file)}: YAML: ${e.message}`);
		continue;
	}
	if (!validateSchema(L)) {
		for (const e of validateSchema.errors) errors.push(`${L?.id || path.basename(file)}: schema ${e.instancePath || "/"} ${e.message}${e.params?.allowedValues ? ` (${e.params.allowedValues.join(", ")})` : ""}${e.params?.missingProperty ? ` «${e.params.missingProperty}»` : ""}`);
		continue;
	}
	if (path.basename(file, ".yaml") !== L.id) errors.push(`${L.id}: file name must be ${L.id}.yaml`);
	if (!order.includes(L.id)) errors.push(`${L.id}: not a lesson of the course (vcv/course/course.json)`);
	const ids = L.blocks.map((b) => b.id);
	if (new Set(ids).size !== ids.length) errors.push(`${L.id}: block ids are not unique`);
	const r = compileLesson(L, { lessonIndex: order.indexOf(L.id), mcpPort, test });
	errors.push(...r.errors);
	warnings.push(...r.warnings);
	needs.push(...r.needs);
	lessons[L.id] = r.pack;
	patches[L.id] = r.patch;
	reports.push(r.report);
}

// Next lesson per lesson, in course order among the compiled ones (the full course when all are built).
for (const id of Object.keys(lessons)) {
	const i = order.indexOf(id);
	lessons[id].next = order[i + 1] || "";
}

const g = gallery();
lessons._gallery = g.pack;

const glossaryOut = {};
for (const [k, v] of Object.entries(glossary)) glossaryOut[k] = v.definition;

const pack = {
	version: 1,
	builtAt: new Date().toISOString(),
	rack: RACK_VERSION,
	plugin: VERSIONS.Atlas,
	course: {
		title: "Модульный синтез с нуля",
		parts: course.parts.map((p) => ({ n: p.n, title: p.title, lessons: p.lessons.filter((l) => lessons[l]) })),
	},
	glossary: glossaryOut,
	lessons,
};

fs.mkdirSync(path.join(outRoot, "patches"), { recursive: true });
for (const f of fs.readdirSync(path.join(outRoot, "patches"))) if (f.endsWith(".vcv") && !only) fs.unlinkSync(path.join(outRoot, "patches", f));
fs.writeFileSync(path.join(outRoot, "pack.json"), `${JSON.stringify(pack)}\n`);
for (const [id, p] of Object.entries(patches)) fs.writeFileSync(path.join(outRoot, "patches", `${id}.vcv`), `${JSON.stringify(p, null, 1)}\n`);
fs.mkdirSync(path.join(RACK, "qa"), { recursive: true });
fs.writeFileSync(path.join(RACK, "qa/gallery.vcv"), `${JSON.stringify(g.patch, null, 1)}\n`);
fs.mkdirSync(path.join(RACK, "build"), { recursive: true });
fs.writeFileSync(path.join(RACK, "build/report.json"), `${JSON.stringify({ builtAt: pack.builtAt, test, lessons: reports, errors, warnings, needs }, null, 1)}\n`);
if (needs.length) fs.writeFileSync(path.join(RACK, "build/calibration-needs.json"), `${JSON.stringify(needs, null, 1)}\n`);

for (const w of warnings) console.log(`warning: ${w}`);
for (const e of errors) console.log(`error: ${e}`);
const blocks = reports.reduce((n, r) => n + r.blocks, 0);
console.log(`${Object.keys(patches).length} lessons, ${blocks} blocks → ${path.relative(path.resolve(RACK, ".."), outRoot)}; ${errors.length} errors, ${warnings.length} warnings`);
if (needs.length) console.log(`calibration needed for ${needs.length} param(s): node rack/compiler/calibrate.mjs`);
process.exit(errors.length ? 1 : 0);
