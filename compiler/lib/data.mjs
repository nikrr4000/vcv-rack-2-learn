/**
 * The fact base the compiler and validator read: lessons (YAML), module snapshots and widths,
 * check files, glossary, sources, verified quotes, the old course map, plugin versions.
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import YAML from "yaml";

export const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
export const RACK = path.join(REPO, "rack");
export const COURSE_OLD = path.join(REPO, "vcv/course");
export const LESSONS_DIR = process.env.ATLAS_LESSONS_DIR || path.join(RACK, "course/lessons");

const readJson = (p) => JSON.parse(fs.readFileSync(p, "utf8"));

let cache = null;

export function facts() {
	if (cache) return cache;
	const snapshots = {};
	const snapDir = path.join(COURSE_OLD, "modules/_snapshot");
	for (const f of fs.readdirSync(snapDir)) {
		if (!f.endsWith(".json") || f.startsWith("_")) continue;
		const s = readJson(path.join(snapDir, f));
		if (!Array.isArray(s)) snapshots[`${s.plugin}/${s.slug}`] = s;
	}
	const widths = readJson(path.join(RACK, "course/module-widths.json")).widths;
	const course = readJson(path.join(COURSE_OLD, "course.json"));
	const quotes = readJson(path.join(COURSE_OLD, "quotes.json"));
	const sourcesText = fs.readFileSync(path.join(COURSE_OLD, "sources.md"), "utf8");
	const sources = {};
	for (const m of sourcesText.matchAll(/^\*\*([a-z0-9][a-z0-9-]*)\*\* — (.+)$/gm)) sources[m[1]] = m[2];
	for (const [id, s] of Object.entries(course.extraSources || {})) sources[id] = s.title;
	cache = { snapshots, widths, course, quotes, sources, glossary: glossary(), calibration: calibration() };
	return cache;
}

/** Glossary: lowercase key (and aliases) → {term, definition}. */
export function glossary() {
	const text = fs.readFileSync(path.join(COURSE_OLD, "glossary.md"), "utf8");
	const out = {};
	for (const m of text.matchAll(/^- \*\*(.+?)\*\*(?: \(англ\. [^)]*\))? — (.+)$/gm)) {
		const term = m[1];
		const def = m[2].replace(/\s*Уроки? [\d., ]+\.?\s*$/, "").trim();
		const names = term
			.replace(/\s*\(([^)]*)\)/g, ", $1")
			.split(/,\s*/)
			.map((x) => x.trim().toLowerCase())
			.filter(Boolean);
		for (const n of names) if (!out[n]) out[n] = { term, definition: def };
	}
	return out;
}

export function calibration() {
	const p = path.join(RACK, "course/calibration.json");
	return fs.existsSync(p) ? readJson(p) : {};
}

export function loadCheck(id) {
	const p = path.join(COURSE_OLD, "checks", `${id}.json`);
	if (!fs.existsSync(p)) return null;
	const c = readJson(p);
	delete c.reference;
	return c;
}

export function lessonFiles() {
	if (!fs.existsSync(LESSONS_DIR)) return [];
	return fs
		.readdirSync(LESSONS_DIR)
		.filter((f) => f.endsWith(".yaml"))
		.sort()
		.map((f) => path.join(LESSONS_DIR, f));
}

export function loadLesson(file) {
	const text = fs.readFileSync(file, "utf8");
	const doc = YAML.parse(text, { prettyErrors: true });
	return doc;
}

/** Plugin versions written into lesson patches (Rack 2.6.6, the owner's installed plugins). */
export const VERSIONS = {
	Core: "2.6.6",
	Fundamental: "2.6.4",
	ImpromptuModular: "2.5.0",
	Valley: "2.4.5",
	AudibleInstruments: "2.0.0",
	"VCV-Recorder": "2.0.3",
	VCVRackMcpServer: "2.1.1",
	Atlas: JSON.parse(fs.readFileSync(path.join(RACK, "plugin/plugin.json"), "utf8")).version,
};

export const RACK_VERSION = "2.6.6";

/** The old lesson's sections ("## Зачем", …) for coverage checks. */
export function oldLesson(id) {
	const p = path.join(COURSE_OLD, "lessons", `${id}.md`);
	if (!fs.existsSync(p)) return null;
	const text = fs.readFileSync(p, "utf8");
	const sections = {};
	let cur = null;
	for (const line of text.split("\n")) {
		const m = /^## (.+)$/.exec(line);
		if (m) {
			cur = m[1].trim();
			sections[cur] = "";
		} else if (cur) sections[cur] += `${line}\n`;
	}
	return { text, sections };
}
