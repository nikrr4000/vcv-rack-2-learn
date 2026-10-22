/**
 * Rack-native progress and the lesson note log.
 *
 * Progress lives in `STATE_DIR/rack-progress.json` in the plugin's format
 * (`{lessons: {<id>: {status, block, reached, updatedAt}}, last, updatedAt}`, times in Unix
 * seconds), so the Navigator merges it as is (newest wins, «learned» never goes back).
 *
 * The note of the lesson (`notes.mjs`) gets a compact log at its end, under
 * «## Журнал в Rack»: blocks reached, quiz answers, check results, tutor exchanges. Status:
 * opening a lesson moves `study → learning`; «completed» sets `learned`.
 */

import fs from "node:fs";
import path from "node:path";

import { STATE_DIR as APP_STATE_DIR } from "./config.mjs";
import { ensureNote, lessonStatus, noteFile, setLessonStatus } from "./notes.mjs";

export const LOG_HEADING = "## Журнал в Rack";
const file = () => path.join(APP_STATE_DIR, "rack-progress.json");
const now = () => Date.now() / 1000;

export function readProgress() {
	try {
		const p = JSON.parse(fs.readFileSync(file(), "utf8"));
		return { lessons: p.lessons || {}, last: p.last || "", updatedAt: p.updatedAt || 0 };
	} catch {
		return { lessons: {}, last: "", updatedAt: 0 };
	}
}

function writeProgress(p) {
	fs.mkdirSync(APP_STATE_DIR, { recursive: true });
	const tmp = `${file()}.tmp`;
	fs.writeFileSync(tmp, `${JSON.stringify(p, null, 1)}\n`);
	fs.renameSync(tmp, file());
}

/** Progress for the Navigator: the store, plus lessons the vault marks learned (e.g. in the web app). */
export function progressForRack() {
	const p = readProgress();
	for (const [id, status] of Object.entries(vaultStatuses())) {
		if (status !== "learned" || p.lessons[id]?.status === "learned") continue;
		let at = 0;
		try {
			at = fs.statSync(noteFile(id)).mtimeMs / 1000;
		} catch {
			// no note
		}
		p.lessons[id] = { ...(p.lessons[id] || { block: "", reached: [] }), status: "learned", updatedAt: Math.max(at, p.lessons[id]?.updatedAt || 0) };
	}
	return p;
}

function vaultStatuses() {
	const out = {};
	try {
		for (const f of fs.readdirSync(path.dirname(noteFile("x")))) {
			const id = f.replace(/\.md$/, "");
			if (f.endsWith(".md") && /^p\d-\d{2}-/.test(id)) out[id] = lessonStatus(id);
		}
	} catch {
		// no notes folder yet
	}
	return out;
}

const stamp = (at) => {
	const d = new Date((at || now()) * 1000);
	const pad = (n) => String(n).padStart(2, "0");
	return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
};
const short = (s, max = 300) => {
	const t = String(s || "")
		.replace(/\s+/g, " ")
		.trim();
	return t.length <= max ? t : `${t.slice(0, max - 1)}…`;
};

/** Appends lines to the lesson note's log section (creates the section once). */
export function appendLog(lesson, lines, at) {
	const note = ensureNote(lesson);
	let text = note.text;
	if (!text.includes(`\n${LOG_HEADING}\n`)) text = `${text.replace(/\s*$/, "")}\n\n${LOG_HEADING}\n\n`;
	const body = lines.map((l, i) => (i === 0 ? `- ${stamp(at)} · ${l}` : `  ${l}`)).join("\n");
	fs.writeFileSync(note.file, `${text.replace(/\n*$/, "\n")}${body}\n`);
}

/** The log is a side record: a note that cannot be written must not lose the progress itself. */
function safeLog(lesson, lines, at) {
	try {
		appendLog(lesson, lines, at);
	} catch (err) {
		console.error(`rack log ${lesson}: ${err.message}`);
	}
}

export function lessonOpened(lesson) {
	const p = readProgress();
	const L = p.lessons[lesson] || { status: "", block: "", reached: [], updatedAt: 0 };
	if (L.status !== "learned") L.status = "learning";
	L.updatedAt = now();
	p.lessons[lesson] = L;
	p.last = lesson;
	p.updatedAt = L.updatedAt;
	writeProgress(p);
	if (lessonStatus(lesson) === "study") setLessonStatus(lesson, "learning");
	safeLog(lesson, ["урок открыт в Rack"]);
	return L;
}

/** One event from the plugin: block | quiz | check | tutor | completed. */
export function recordEvent({ lesson, block, event, data = {}, at }) {
	const t = typeof at === "number" ? at : now();
	const p = readProgress();
	const L = p.lessons[lesson] || { status: "learning", block: "", reached: [], updatedAt: 0 };
	if (event === "block" && block) {
		const again = L.block === block;
		L.block = block;
		if (!L.reached.includes(block)) L.reached.push(block);
		if (L.status !== "learned") L.status = "learning";
		if (!again) safeLog(lesson, [`блок ${block}${data.title ? ` «${short(data.title, 80)}»` : ""}`], t);
	} else if (event === "quiz") {
		safeLog(lesson, [`тест${block ? ` (${block})` : ""}: «${short(data.question, 160)}» → «${short(data.answer, 120)}» ${data.correct ? "✓" : "✗"}`], t);
	} else if (event === "check") {
		const items = Array.isArray(data.items) ? data.items : [];
		const ok = items.filter((i) => i.ok).length;
		const first = items.find((i) => !i.ok);
		safeLog(lesson, [`проверка${block ? ` (${block})` : ""}: ${data.passed ? "пройдена" : "не пройдена"}, ${ok}/${items.length}${first ? `; первое: ${short(first.what, 120)}` : ""}`], t);
	} else if (event === "tutor") {
		safeLog(lesson, [`вопрос${data.local ? " (ответ из урока)" : ""}: «${short(data.q)}»`, `ответ: «${short(data.a)}»`], t);
	} else if (event === "completed") {
		L.status = "learned";
		setLessonStatus(lesson, "learned");
		safeLog(lesson, ["урок пройден"], t);
	} else {
		return { ignored: event };
	}
	L.updatedAt = Math.max(L.updatedAt || 0, t);
	p.lessons[lesson] = L;
	p.last = lesson;
	p.updatedAt = L.updatedAt;
	writeProgress(p);
	return L;
}
