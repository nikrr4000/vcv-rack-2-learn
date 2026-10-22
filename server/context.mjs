/**
 * The compact header around each learner message (plan §7.3), so the tutor does not need to read
 * files: lesson and block, the block's text, the patch line, the last check, the quiz, the kind.
 *
 * The course pack is `plugin/res/course/pack.json` (env `ATLAS_PACK_JSON` overrides), read
 * again when its mtime changes. The patch line comes from the plugin with the question (it knows
 * the block's bench and given modules); without it the server summarises the whole patch from the
 * Rack REST API, best effort.
 */

import fs from "node:fs";
import path from "node:path";

import { REPO } from "./config.mjs";
import * as rack from "./rack-mcp.mjs";

export const KINDS = ["ask", "grade", "simpler", "example", "review"];
export const BLOCK_TEXT_MAX = 1200;
const PATCH_MAX = 700;
const LEARNER_MAX = 2000;

const packFile = () => process.env.ATLAS_PACK_JSON || path.join(REPO, "plugin", "res", "course", "pack.json");
let cache = null;

export function loadPack() {
	const file = packFile();
	const mtime = fs.statSync(file).mtimeMs;
	if (!cache || cache.file !== file || cache.mtime !== mtime) cache = { file, mtime, pack: JSON.parse(fs.readFileSync(file, "utf8")) };
	return cache.pack;
}

export const packLesson = (id) => loadPack().lessons?.[id] || null;

/** The markdown subset → one plain line per paragraph. */
export function plain(md) {
	return String(md || "")
		.replace(/\*\*([^*]+)\*\*/g, "$1")
		.replace(/`([^`]+)`/g, "$1")
		.replace(/\[([^\]]+)\]\([^)]*\)/g, "$1")
		.replace(/^#+\s*/gm, "")
		.replace(/[ \t]+/g, " ")
		.replace(/\n{2,}/g, "\n")
		.trim();
}

/** The learner-facing text of a block: the compiler's `plain` (or the items' text), then quiz questions. */
export function blockText(block, max = BLOCK_TEXT_MAX) {
	const parts = [];
	if (block?.plain) parts.push(plain(block.plain));
	for (const it of block?.items || []) {
		if (it.kind === "quiz") for (const q of it.questions || []) parts.push(plain(q.q));
		else if (!block.plain && it.text) parts.push(plain(it.text));
		else if (!block.plain && it.caption) parts.push(plain(it.caption));
	}
	const t = parts.filter(Boolean).join("\n");
	return t.length <= max ? t : `${t.slice(0, max - 1).trimEnd()}…`;
}

const oneLine = (s, max) => {
	const t = String(s || "")
		.replace(/\s+/g, " ")
		.trim();
	return t.length <= max ? t : `${t.slice(0, max - 1)}…`;
};

/** The whole patch from REST, without Atlas and MCP Server modules: «A.Out → B.In; …». */
export async function restPatchLine() {
	try {
		const list = (await rack.modules()).filter((m) => m.plugin !== "Atlas" && m.plugin !== "VCVRackMcpServer");
		const ids = new Set(list.map((m) => m.id));
		const details = {};
		for (const m of list) details[m.id] = await rack.module(m.id).catch(() => null);
		const port = (id, kind, i) => details[id]?.[kind]?.find((p) => p.id === i)?.name || `#${i}`;
		const name = (id) => details[id]?.name || list.find((m) => m.id === id)?.name || String(id);
		const cables = (await rack.cables()).filter((c) => c && ids.has(c.outputModuleId) && ids.has(c.inputModuleId));
		const lines = cables.map((c) => `${name(c.outputModuleId)}.${port(c.outputModuleId, "outputs", c.outputId)} → ${name(c.inputModuleId)}.${port(c.inputModuleId, "inputs", c.inputId)}`);
		const lonely = list.filter((m) => !cables.some((c) => c.outputModuleId === m.id || c.inputModuleId === m.id)).map((m) => m.name);
		if (lonely.length) lines.push(`без кабелей: ${lonely.join(", ")}`);
		return lines.join("; ") || "пусто";
	} catch {
		return "";
	}
}

/**
 * `req`: {lesson, block, kind, text, patch?, check?, quiz?}. Returns the message text.
 * `patchLine` (async) is used when `req.patch` is missing.
 */
export async function buildHeader(req, { patchLine = restPatchLine } = {}) {
	const L = packLesson(req.lesson);
	const B = L?.blocks.find((b) => b.id === req.block) || null;
	const kind = KINDS.includes(req.kind) ? req.kind : "ask";
	const lines = [];
	lines.push(`[lesson ${req.lesson} «${L?.title || "?"}», block ${B?.id || req.block || "?"} «${B?.title || "?"}» (${B?.type || "?"})]`);
	lines.push(`[block text] ${B ? blockText(B).replace(/\n/g, " / ") : "нет"}`);
	const patch = req.patch ? String(req.patch) : await patchLine();
	lines.push(`[patch] ${oneLine(patch, PATCH_MAX) || "нет данных"}`);
	lines.push(`[check] ${oneLine(req.check, 400) || "нет"}`);
	if (req.quiz) lines.push(`[quiz] ${oneLine(req.quiz, 800)}`);
	lines.push(`[kind] ${kind}`);
	lines.push(`[learner] ${oneLine(req.text, LEARNER_MAX)}`);
	return lines.join("\n");
}

export const BRIEF_MAX = 6000;

/**
 * The warm-up message: the whole lesson in brief (title, goal, every block with its type and opening
 * lines), sent once when the tutor process starts, so every later question has the lesson around it.
 * It still asks for a one-word «ok» (the reply is discarded).
 */
export function lessonBrief(id, max = BRIEF_MAX) {
	const L = id ? packLesson(id) : null;
	if (!L) return null;
	const head = [`[lesson ${id} «${L.title}»${L.num ? `, урок ${L.num}` : ""}]`];
	if (L.goal) head.push(`[goal] ${oneLine(plain(L.goal), 300)}`);
	if (L.builds) head.push(`[builds] ${oneLine(plain(L.builds), 300)}`);
	const blocks = L.blocks.map((b, i) => `${i + 1}. ${b.id} «${b.title}» (${b.type}): ${oneLine(blockText(b, 400), 260)}`);
	let outline = blocks.join("\n");
	if (outline.length > max) outline = `${outline.slice(0, max - 1)}…`;
	return [
		...head,
		"[outline]",
		outline,
		"Это план урока, который ученик сейчас проходит в Rack. Держи его в голове: дальше каждое сообщение придёт с заголовком текущего блока. Готов? Это служебная проверка связи: ответь одним словом «ok».",
	].join("\n");
}
