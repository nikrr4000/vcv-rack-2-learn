/**
 * Parameter resolver: a lesson's `display` target («261.63», «Enabled») → the raw param value,
 * using the live snapshots (display at min / mid / max, options):
 *   - switches: the option index;
 *   - numeric: a calibration table measured on the live Rack when there is one; otherwise linear or
 *     exponential interpolation when the snapshot's mid point fits that law (within 1 %); otherwise an error
 *     (rack/course/calibration.json, written by calibrate.mjs).
 * Names are matched exactly as Rack prints them.
 */
import { facts } from "./data.mjs";

const num = (s) => {
	if (typeof s === "number") return s;
	const n = Number.parseFloat(String(s).replace(",", "."));
	return Number.isFinite(n) ? n : null;
};

export class ResolveError extends Error {
	constructor(message, need = null) {
		super(message);
		this.need = need; // {key, param} when calibration is missing
	}
}

export function snapshot(plugin, slug) {
	return facts().snapshots[`${plugin}/${slug}`] || null;
}

export function findParam(snap, name) {
	return (snap?.params || []).find((p) => p.name === name) || null;
}

export function findPort(snap, kind, name) {
	return (snap?.[kind === "input" ? "inputs" : "outputs"] || []).find((p) => p.name === name) || null;
}

/** The law of a numeric param from its snapshot: "linear" | "exp" | null. */
export function lawOf(p) {
	const d = p.display || {};
	const a = num(d.min);
	const m = num(d.mid);
	const b = num(d.max);
	if (a === null || m === null || b === null || a === b) return null;
	if (Math.abs(m - (a + b) / 2) <= 0.01 * Math.max(1, Math.abs(b - a))) return "linear";
	if (a * b > 0 && Math.abs(m - Math.sign(a) * Math.sqrt(a * b)) <= 0.01 * Math.abs(m)) return "exp";
	return null;
}

/**
 * Resolves one param value spec. Returns {id, value, display} (display = the target text).
 * `spec`: "Enabled" | "261.63" | 261.63 | {display} | {raw}.
 */
export function resolveParam(plugin, slug, name, spec) {
	const snap = snapshot(plugin, slug);
	if (!snap) throw new ResolveError(`no snapshot for ${plugin}/${slug}`);
	const p = findParam(snap, name);
	if (!p) throw new ResolveError(`${plugin}/${slug} has no param «${name}» (names: ${snap.params.map((x) => x.name).join(", ")})`);
	if (spec && typeof spec === "object" && "raw" in spec) {
		if (spec.raw < p.min || spec.raw > p.max) throw new ResolveError(`${slug}.«${name}»: raw ${spec.raw} outside ${p.min}…${p.max}`);
		return { id: p.id, value: spec.raw, display: null };
	}
	const target = spec && typeof spec === "object" ? spec.display : spec;
	const text = String(target).trim();
	if (Array.isArray(p.options)) {
		const i = p.options.findIndex((o) => o.trim() === text);
		if (i < 0) throw new ResolveError(`${slug}.«${name}»: «${text}» is not one of ${p.options.join(" / ")}`);
		return { id: p.id, value: p.min + i, display: text };
	}
	const t = num(text);
	if (t === null) throw new ResolveError(`${slug}.«${name}»: «${text}» is not a number`);
	// Exactly a value the snapshot saw: use that raw value (no rounding drift).
	const seen = [[p.displayDefault, p.default], [p.display?.min, p.min], [p.display?.mid, (p.min + p.max) / 2], [p.display?.max, p.max]].find(([d]) => d !== undefined && num(d) === t);
	if (seen) return { id: p.id, value: p.snap ? Math.round(seen[1]) : seen[1], display: text };
	// A measured table wins over a law guessed from three points (Clocked ratios fit a line by chance).
	const measured = facts().calibration[`${plugin}/${slug}`]?.[name];
	const law = measured ? null : lawOf(p);
	const d = p.display;
	let value = null;
	if (law === "linear") value = p.min + ((t - num(d.min)) / (num(d.max) - num(d.min))) * (p.max - p.min);
	else if (law === "exp") {
		if (t * num(d.min) <= 0) throw new ResolveError(`${slug}.«${name}»: ${t} has the wrong sign for this exponential param`);
		value = p.min + (Math.log(t / num(d.min)) / Math.log(num(d.max) / num(d.min))) * (p.max - p.min);
	} else {
		const key = `${plugin}/${slug}`;
		const table = facts().calibration[key]?.[name];
		if (!table) throw new ResolveError(`${slug}.«${name}»: needs calibration (non-linear display)`, { key, param: name });
		value = fromTable(table, t);
		if (value === null) throw new ResolveError(`${slug}.«${name}»: ${t} outside the calibrated range`);
	}
	if (p.snap) value = Math.round(value);
	const lo = Math.min(p.min, p.max);
	const hi = Math.max(p.min, p.max);
	if (value < lo - 1e-6 || value > hi + 1e-6) throw new ResolveError(`${slug}.«${name}»: ${text} is outside the knob's range (${d?.min}…${d?.max})`);
	value = Math.min(hi, Math.max(lo, value));
	return { id: p.id, value: Math.round(value * 1e6) / 1e6, display: text };
}

/** Linear interpolation in a calibration table [[raw, display], …] sorted by raw. */
function fromTable(table, t) {
	const pts = table.map(([raw, disp]) => [raw, num(disp)]).filter(([, d]) => d !== null);
	for (let i = 0; i + 1 < pts.length; i++) {
		const [r0, d0] = pts[i];
		const [r1, d1] = pts[i + 1];
		if ((t >= d0 && t <= d1) || (t <= d0 && t >= d1)) return d1 === d0 ? r0 : r0 + ((t - d0) / (d1 - d0)) * (r1 - r0);
	}
	return null;
}
