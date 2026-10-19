/**
 * Text fit: the plugin's text layout (rack/plugin/src/text/Layout.cpp) in Node, with the same
 * fonts and tokens, measuring like nanovg + fontstash do at zoom 1 on a 2× display:
 * glyph advances truncated to 1/10 px and rounded to whole device pixels, GPOS pair kerning,
 * Rack's DejaVu Sans as the fallback for glyphs the paper fonts lack.
 *
 * The runtime in Rack is the authority (it lays text out for the real width and zoom); this tool
 * reports pages and overflow at build time with a 3 % width margin and one spare line.
 */
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

import opentype from "opentype.js";

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
export const TOKENS = JSON.parse(fs.readFileSync(path.join(ROOT, "course/design-tokens.json"), "utf8"));
const FONT_DIR = path.join(ROOT, "plugin/res/fonts");
const RACK_RES = process.env.RACK_SYSTEM_DIR || path.join(os.homedir(), "Applications/VCV Rack 2 Free.app/Contents/Resources");

export const F = { HEADING: 0, BODY: 1, BOLD: 2, ITALIC: 3, MONO: 4 };
const FILES = [TOKENS.fonts.heading, TOKENS.fonts.body, TOKENS.fonts.bodyBold, TOKENS.fonts.bodyItalic, TOKENS.fonts.mono];

let fonts = null;
let fallback = null;
function load() {
	if (fonts) return;
	fonts = FILES.map((f) => opentype.loadSync(path.join(FONT_DIR, f)));
	const fb = path.join(RACK_RES, "res/fonts/DejaVuSans.ttf");
	fallback = fs.existsSync(fb) ? opentype.loadSync(fb) : null;
}

const PX = 2; // device pixel ratio the measurements emulate

function glyphOf(font, cp) {
	const idx = font.charToGlyphIndex(String.fromCodePoint(cp));
	return idx > 0 ? font.glyphs.get(idx) : null;
}

const cache = new Map();

/** Advance of `s` in font `fid` at `size` (Rack px at zoom 1), like nvgTextBounds. */
export function advance(fid, size, s) {
	load();
	const key = `${fid}|${size}|${s}`;
	const hit = cache.get(key);
	if (hit !== undefined) return hit;
	const font = fonts[fid];
	const isize = Math.trunc(size * PX * 10);
	const px = isize / 10;
	const scaleOf = (f) => px / (f.tables.hhea.ascender - f.tables.hhea.descender);
	let x = 0;
	let prev = null;
	let prevFont = null;
	for (const ch of s) {
		const cp = ch.codePointAt(0);
		let f = font;
		let g = glyphOf(font, cp);
		if (!g && fallback) {
			const fg = glyphOf(fallback, cp);
			if (fg) {
				f = fallback;
				g = fg;
			}
		}
		if (!g) g = font.glyphs.get(0);
		const scale = scaleOf(f);
		if (prev && prevFont === f) {
			const k = f.getKerningValue(prev, g) * scale;
			x += Math.trunc(k + 0.5);
		}
		const xadv = Math.trunc(scale * g.advanceWidth * 10);
		x += Math.trunc(xadv / 10 + 0.5);
		prev = g;
		prevFont = f;
	}
	const w = x / PX;
	if (cache.size > 200000) cache.clear();
	cache.set(key, w);
	return w;
}

export function metrics(fid, size) {
	load();
	const f = fonts[fid];
	const isize = Math.trunc(size * PX * 10) / 10;
	const s = isize / (f.tables.hhea.ascender - f.tables.hhea.descender);
	return { ascender: (f.tables.hhea.ascender * s) / PX, descender: (-f.tables.hhea.descender * s) / PX };
}

// ── markdown subset (same rules as Markdown.cpp) ──

export function parseInline(line) {
	const out = [];
	let buf = "";
	let bold = false;
	let italic = false;
	const style = () => (bold ? "bold" : italic ? "italic" : "regular");
	const push = (text, st, term = "") => {
		if (!text) return;
		const last = out.at(-1);
		if (last && !term && !last.term && last.style === st) last.text += text;
		else out.push({ text, style: st, term });
	};
	let i = 0;
	while (i < line.length) {
		const c = line[i];
		if (c === "`") {
			const end = line.indexOf("`", i + 1);
			if (end >= 0) {
				push(buf, style());
				buf = "";
				push(line.slice(i + 1, end), "mono");
				i = end + 1;
				continue;
			}
		} else if (c === "[" && line[i + 1] === "[") {
			const end = line.indexOf("]]", i + 2);
			if (end >= 0) {
				push(buf, style());
				buf = "";
				const inner = line.slice(i + 2, end);
				const bar = inner.indexOf("|");
				const key = bar >= 0 ? inner.slice(0, bar) : inner;
				const shown = bar >= 0 ? inner.slice(bar + 1) : inner;
				push(shown, style(), key);
				i = end + 2;
				continue;
			}
		} else if (c === "*" && line[i + 1] === "*") {
			if (bold || line.indexOf("**", i + 2) >= 0) {
				push(buf, style());
				buf = "";
				bold = !bold;
				i += 2;
				continue;
			}
		} else if (c === "*") {
			let closes = italic;
			if (!italic) {
				for (let j = i + 1; j < line.length; j++) {
					if (line[j] === "*") {
						if (line[j + 1] === "*") {
							j++;
							continue;
						}
						closes = true;
						break;
					}
				}
			}
			if (closes) {
				push(buf, style());
				buf = "";
				italic = !italic;
				i += 1;
				continue;
			}
		}
		buf += c;
		i++;
	}
	push(buf, style());
	return out;
}

export function parse(md) {
	const blocks = [];
	for (const raw of String(md ?? "").split("\n")) {
		let line = raw.trim();
		if (!line) continue;
		let kind = "p";
		let number = 0;
		if (line.startsWith("# ") || line.startsWith("## ")) {
			kind = "h";
			line = line.slice(line.indexOf(" ") + 1).trim();
		} else if (line.startsWith("- ")) {
			kind = "ul";
			line = line.slice(2).trim();
		} else {
			const m = /^(\d+)\. /.exec(line);
			if (m) {
				kind = "ol";
				number = Number(m[1]);
				line = line.slice(m[0].length).trim();
			}
		}
		blocks.push({ kind, number, spans: parseInline(line) });
	}
	return blocks;
}

export function plain(md) {
	return parse(md)
		.map((b) => (b.kind === "ul" ? "— " : b.kind === "ol" ? `${b.number}. ` : "") + b.spans.map((s) => s.text).join(""))
		.join("\n");
}

export function terms(md) {
	return parse(md).flatMap((b) => b.spans.filter((s) => s.term).map((s) => s.term));
}

// ── layout (same algorithm as Layout.cpp) ──

export function textStyle({ title = false, bodySize, lineHeight } = {}) {
	return {
		bodySize: bodySize ?? TOKENS.type.body,
		headingSize: title ? TOKENS.type.title : TOKENS.type.heading,
		monoSize: TOKENS.type.mono,
		lineHeight: lineHeight ?? TOKENS.type.lineHeight,
		headingLineHeight: 1.25,
		paragraphGap: TOKENS.type.paragraphGap,
		headingGapAfter: 2,
		listIndent: TOKENS.type.listIndent,
	};
}

const fontFor = (style) => (style === "bold" ? F.BOLD : style === "italic" ? F.ITALIC : style === "mono" ? F.MONO : F.BODY);
const isDash = (t) => t === "—" || t === "–" || t === "→";
const isShortWord = (t) => [...t].length === 1 && /[A-Za-zА-Яа-яЁё]/.test(t);
const isNumber = (t) => /^[\d.,+-]*\d$/.test(t) && t.length > 0;

function atomize(block, st) {
	const atoms = [];
	let cur = { parts: [], hasSpace: false, spaceBefore: null };
	let pendingSpace = null;
	const flush = () => {
		if (cur.parts.length) atoms.push(cur);
		cur = { parts: [], hasSpace: false, spaceBefore: null };
	};
	const addText = (t, f, size, term) => {
		if (!t) return;
		if (pendingSpace) {
			flush();
			cur.hasSpace = atoms.length > 0;
			cur.spaceBefore = pendingSpace;
			pendingSpace = null;
		}
		const last = cur.parts.at(-1);
		if (last && last.font === f && last.term === term && !last.space) last.text += t;
		else cur.parts.push({ text: t, font: f, size, term, space: false });
	};
	for (const sp of block.spans) {
		const f = block.kind === "h" ? F.HEADING : fontFor(sp.style);
		const size = block.kind === "h" ? st.headingSize : sp.style === "mono" ? st.monoSize : st.bodySize;
		if (sp.style === "mono" && block.kind !== "h" && [...sp.text].length <= 12) {
			addText(sp.text, f, size, sp.term);
			continue;
		}
		let word = "";
		for (const c of sp.text) {
			if (c === " " || c === "\t") {
				addText(word, f, size, sp.term);
				word = "";
				if (!pendingSpace) pendingSpace = { text: " ", font: f, size, term: sp.term, space: true };
			} else word += c;
		}
		addText(word, f, size, sp.term);
	}
	flush();
	const text = (a) => a.parts.map((p) => p.text).join("");
	const glued = [];
	for (const a of atoms) {
		if (glued.length && a.hasSpace && isDash(text(a))) {
			const prev = glued.at(-1);
			prev.parts.push(a.spaceBefore, ...a.parts);
			continue;
		}
		glued.push(a);
	}
	const lastWord = (a) => {
		let t = "";
		for (let i = a.parts.length - 1; i >= 0; i--) {
			if (a.parts[i].space) break;
			t = a.parts[i].text + t;
		}
		return t;
	};
	const out = [];
	for (let i = 0; i < glued.length; i++) {
		const a = glued[i];
		while (i + 1 < glued.length && glued[i + 1].hasSpace) {
			const last = lastWord(a);
			const next = glued[i + 1];
			const unit = isNumber(last) && [...text(next)].length <= 4 && !isNumber(text(next));
			if (!isShortWord(last) && !unit) break;
			a.parts.push(next.spaceBefore, ...next.parts);
			i++;
		}
		out.push(a);
	}
	for (const a of out) {
		a.width = 0;
		for (const p of a.parts) {
			p.width = advance(p.font, p.size, p.text);
			a.width += p.width;
		}
		if (a.hasSpace) a.spaceBefore.width = advance(a.spaceBefore.font, a.spaceBefore.size, " ");
	}
	return out;
}

function splitAtom(a, w) {
	const out = [];
	let cur = { parts: [], width: 0 };
	let curW = 0;
	for (const p of a.parts) {
		for (const ch of p.text) {
			const chW = advance(p.font, p.size, ch);
			const hyW = advance(p.font, p.size, "-");
			if (cur.parts.length && curW + chW + hyW > w) {
				cur.parts.push({ text: "-", font: p.font, size: p.size, term: p.term, space: false, width: hyW });
				cur.width = curW + hyW;
				out.push(cur);
				cur = { parts: [], width: 0 };
				curW = 0;
			}
			const last = cur.parts.at(-1);
			if (last && last.font === p.font && last.term === p.term && !last.space) {
				last.text += ch;
				last.width += chW;
			} else cur.parts.push({ ...p, text: ch, width: chW });
			curW += chW;
		}
	}
	if (cur.parts.length) {
		cur.width = curW;
		out.push(cur);
	}
	if (out.length) {
		out[0].hasSpace = a.hasSpace;
		out[0].spaceBefore = a.spaceBefore;
	}
	return out;
}

/** Lays out markdown at `width`; returns {blocks: [{kind, lines: [{text, width}], height}], splitWords}. */
export function layout(md, width, st = textStyle()) {
	const doc = { blocks: [], splitWords: false };
	for (const b of parse(md)) {
		const list = b.kind === "ul" || b.kind === "ol";
		const indent = list ? st.listIndent : 0;
		const w = Math.max(10, width - indent);
		const lh = b.kind === "h" ? st.headingSize * st.headingLineHeight : st.bodySize * st.lineHeight;
		const fitted = [];
		for (const a of atomize(b, st)) {
			if (a.width > w) {
				fitted.push(...splitAtom(a, w));
				doc.splitWords = true;
			} else fitted.push(a);
		}
		const lines = [[]];
		let x = 0;
		for (const a of fitted) {
			const cur = lines.at(-1);
			if (!cur.length) {
				cur.push(...a.parts);
				x = a.width;
				continue;
			}
			const sw = a.hasSpace ? a.spaceBefore.width : 0;
			if (x + sw + a.width <= w + 0.01) {
				if (a.hasSpace) cur.push(a.spaceBefore);
				cur.push(...a.parts);
				x += sw + a.width;
			} else {
				lines.push([...a.parts]);
				x = a.width;
			}
		}
		const laid = lines.map((ps) => {
			let s = 0;
			let e = ps.length;
			while (s < e && ps[s].space) s++;
			while (e > s && ps[e - 1].space) e--;
			const kept = ps.slice(s, e);
			return { text: kept.map((p) => p.text).join(""), width: kept.reduce((n, p) => n + p.width, 0) + indent, height: lh };
		});
		doc.blocks.push({ kind: b.kind, lines: laid, height: laid.length * lh + (b.kind === "h" ? st.headingGapAfter : 0) });
	}
	return doc;
}

const gapBetween = (prev, next, st) => ((prev === "ul" || prev === "ol") && (next === "ul" || next === "ol") ? st.paragraphGap * 0.45 : st.paragraphGap);

/** Pages of a laid-out document (same rules as paginate() in Layout.cpp): line counts per page and overflow. */
export function paginate(doc, pageHeight, st = textStyle()) {
	const pages = [{ lines: [], height: 0 }];
	let y = 0;
	let lastKind = null;
	let lastBlockLines = 0;
	const add = (lines, at) => {
		for (const l of lines) pages.at(-1).lines.push({ ...l, top: at });
	};
	for (const b of doc.blocks) {
		if (!b.lines.length) continue;
		let gap = pages.at(-1).lines.length ? gapBetween(lastKind, b.kind, st) : 0;
		if (y + gap + b.height <= pageHeight + 0.01) {
			add(b.lines, y + gap);
			y += gap + b.height;
		} else if (b.height <= pageHeight + 0.01) {
			let carry = [];
			let carryH = 0;
			if (lastKind === "h" && pages.at(-1).lines.length) {
				carry = pages.at(-1).lines.splice(pages.at(-1).lines.length - lastBlockLines, lastBlockLines);
				carryH = carry.reduce((n, l) => n + l.height, 0) + st.headingGapAfter;
			}
			pages.at(-1).height = y;
			pages.push({ lines: [], height: 0 });
			y = 0;
			if (carry.length) {
				pages.at(-1).lines.push(...carry);
				y = carryH;
				gap = gapBetween("h", b.kind, st);
			} else gap = 0;
			add(b.lines, y + gap);
			y += gap + b.height;
		} else {
			let i = 0;
			let at = y + gap;
			while (i < b.lines.length) {
				let j = i;
				let h = 0;
				while (j < b.lines.length && at + h + b.lines[j].height <= pageHeight + 0.01) {
					h += b.lines[j].height;
					j++;
				}
				if (j === i) {
					if (!pages.at(-1).lines.length) {
						j = i + 1;
						h = b.lines[i].height;
						pages.at(-1).overflow = true;
					} else {
						pages.at(-1).height = y;
						pages.push({ lines: [], height: 0 });
						y = 0;
						at = 0;
						continue;
					}
				}
				add(b.lines.slice(i, j), at);
				y = at + h;
				i = j;
				if (i < b.lines.length) {
					pages.at(-1).height = y;
					pages.push({ lines: [], height: 0 });
					y = 0;
					at = 0;
				}
			}
		}
		lastKind = b.kind;
		lastBlockLines = b.lines.length;
	}
	pages.at(-1).height = y;
	if (pages.length > 1 && !pages.at(-1).lines.length) pages.pop();
	return pages;
}

export function flatHeight(doc, st = textStyle()) {
	let y = 0;
	let last = null;
	for (const b of doc.blocks) {
		if (!b.lines.length) continue;
		y += (last ? gapBetween(last, b.kind, st) : 0) + b.height;
		last = b.kind;
	}
	return y;
}

/** Content box of a paper module of `hp` width (see drawPanel in Draw.cpp). */
export function pageBox(hp, { footer = false } = {}) {
	const p = TOKENS.panel;
	const w = hp * p.hp - p.padLeft - p.padRight;
	const top = p.accentRule + p.padTop - 2 + p.headerHeight;
	const h = p.rackHeight - top - p.padBottom - (footer ? p.footerHeight : 0);
	return { w, h };
}

/**
 * Fit report for a text in a paper module (Page): pages at the build-time margins
 * (3 % narrower, one spare line), and whether a word had to be hyphenated.
 */
export function fitPage(md, hp, { title = false } = {}) {
	const st = textStyle({ title });
	const { w, h } = pageBox(hp);
	const margin = TOKENS.textfit;
	const wFit = w * (1 - margin.widthMargin);
	const hFit = h - margin.spareLines * st.bodySize * st.lineHeight;
	const doc = layout(md, wFit, st);
	let pages = paginate(doc, hFit, st);
	if (pages.length > 1) {
		const { h: h2 } = pageBox(hp, { footer: true });
		pages = paginate(doc, h2 - margin.spareLines * st.bodySize * st.lineHeight, st);
	}
	return { pages: pages.length, overflow: pages.some((p) => p.overflow), splitWords: doc.splitWords };
}
