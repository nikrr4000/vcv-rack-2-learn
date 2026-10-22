/**
 * Lesson notes: one Markdown note per lesson in `notesDir()/<id>.md` with `status: study |
 * learning | learned` in the front matter and the «Журнал в Rack» log at the end
 * (`progress.mjs`). The folder is `state/notes` unless NOTES_DIR (or settings `notesDir`)
 * points elsewhere, e.g. at a folder of an Obsidian vault. A note is created on first use;
 * a status change rewrites only the `status:` line.
 */

import fs from "node:fs";
import path from "node:path";

import { notesDir } from "./config.mjs";
import { packLesson } from "./context.mjs";

export const STATUSES = ["study", "learning", "learned"];

export const noteFile = (id) => path.join(notesDir(), `${id}.md`);

function readNote(id) {
	const file = noteFile(id);
	if (!fs.existsSync(file)) return null;
	return { file, text: fs.readFileSync(file, "utf8") };
}

const statusOf = (text) => /^---\n[\s\S]*?^status:\s*(\S+)\s*$[\s\S]*?^---$/m.exec(text)?.[1] || "study";

/** A missing note counts as `study`. */
export const lessonStatus = (id) => {
	const note = readNote(id);
	return note ? statusOf(note.text) : "study";
};

export function ensureNote(id) {
	const have = readNote(id);
	if (have) return have;
	const L = packLesson(id);
	const title = L ? `${L.num} ${L.title}` : id;
	fs.mkdirSync(notesDir(), { recursive: true });
	const text = `---\ntitle: ${JSON.stringify(title)}\nstatus: study\ncourse: vcv-rack\n---\n\nУрок ${title}. Статус меняет сервер репетитора: начало урока — «изучаю», кнопка «Урок пройден» — «изучено».\n`;
	fs.writeFileSync(noteFile(id), text, { flag: "wx" });
	return { file: noteFile(id), text };
}

export function setLessonStatus(id, status) {
	if (!STATUSES.includes(status)) throw new Error(`status must be one of ${STATUSES.join(", ")}`);
	const note = ensureNote(id);
	const text = /^status:.*$/m.test(note.text) ? note.text.replace(/^status:.*$/m, `status: ${status}`) : note.text.replace(/^---\n/, `---\nstatus: ${status}\n`);
	fs.writeFileSync(note.file, text);
	return status;
}
