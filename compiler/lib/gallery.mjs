/**
 * qa/gallery.vcv: every Atlas module and every figure template in one patch, for looking at
 * the look in Rack (switch «Prefer dark panels» for the dark theme). Its items live in the pack as
 * the hidden lesson `_gallery` (not in any part, so the course map never shows it).
 */
import fs from "node:fs";
import path from "node:path";

import { RACK, RACK_VERSION, VERSIONS } from "./data.mjs";

export function gallery() {
	const figures = JSON.parse(fs.readFileSync(path.join(RACK, "course/figure-samples.json"), "utf8"));
	const modules = [];
	let id = 990001;
	const blocks = [];
	const put = (row, x, model, block, item) => modules.push({ id: id++, plugin: "Atlas", model, version: VERSIONS.Atlas, params: [], pos: [x, row], data: { lesson: "_gallery", block, item, state: {} } });
	// Row 0: the text modules.
	const items0 = [
		{ id: "page-1", kind: "page", width: 24, text: "# Галерея модулей Atlas\nЭто **Страница**: заголовок, абзацы, списки, `значения` и [[гейт|термины]] с подсказкой.\n- Пункт списка.\n- Ещё пункт → со стрелкой.\n1. Нумерованный пункт.\nДлинный текст листается: точки внизу показывают страницы." },
		{ id: "note-1", kind: "note", width: 8, text: "**Подпись** указывает на ручку или гнездо кольцом.", targets: [] },
		{ id: "quiz-1", kind: "quiz", width: 14, questions: [{ q: "Что меняет ручка Frequency у VCO?", free: true, options: [{ text: "Высоту тона", correct: true, why: "Frequency — частота: больше частота, выше тон." }, { text: "Громкость", why: "Громкость меняют дальше по цепочке, например VCA." }, { text: "Форму волны", why: "Форма зависит от выхода: Sine, Triangle, Sawtooth, Square." }] }] },
		{ id: "task-1", kind: "task", width: 14, mode: "try", text: "", steps: [{ do: "Поверни **Frequency** вправо.", see: "Тон выше." }, { do: "Переключи кабель на `Square`.", see: "Звук ярче." }] },
		{ id: "bench-left", kind: "bench", side: "left", width: 2 },
		{ id: "bench-right", kind: "bench", side: "right", width: 2 },
	];
	const models = { page: "Page", note: "Note", quiz: "Quiz", task: "Task", bench: "Bench" };
	let x = 0;
	modules.push({ id: id++, plugin: "Atlas", model: "Navigator", version: VERSIONS.Atlas, params: [], pos: [0, 0], data: { lesson: "", block: "", item: "", state: {} } });
	x = 11;
	for (const it of items0) {
		put(0, x, models[it.kind], "b1", it.id);
		x += it.width + 1;
	}
	modules.push({ id: id++, plugin: "Atlas", model: "Tutor", version: VERSIONS.Atlas, params: [], pos: [x, 0], data: { lesson: "_gallery", block: "b1", item: "", state: {} } });
	blocks.push({ id: "b1", type: "story", title: "Модули", row: 0, bounds: { x: 0, y: 0, w: (x + 18) * 15, h: 380 }, items: items0, faq: [{ q: "Что это за патч?", a: "Галерея: все модули Atlas и все шаблоны схем в одном месте." }], simpler: "", plain: "", channels: [] });
	// Rows 1…: every figure template, five per row.
	const per = 5;
	for (let r = 0; r * per < figures.length; r++) {
		const items = figures.slice(r * per, r * per + per).map((f, k) => ({ id: `figure-${k + 1}`, kind: "figure", width: 16, figure: f, caption: `Шаблон ${f.template}.` }));
		items.forEach((it, k) => put(r + 1, k * 17, "Figure", `b${r + 2}`, it.id));
		blocks.push({ id: `b${r + 2}`, type: "story", title: `Схемы ${r + 1}`, row: r + 1, bounds: { x: 0, y: (r + 1) * 380, w: items.length * 17 * 15, h: 380 }, items, faq: [], simpler: "", plain: "", channels: [] });
	}
	const patch = { version: RACK_VERSION, zoom: 1, gridOffset: [-1, -1], modules, cables: [] };
	const pack = { num: "—", part: 0, title: "Галерея", goal: "", builds: "", minutes: 0, requires: [], patch: "", layoutHash: "gallery", next: "", blocks };
	return { patch, pack };
}
