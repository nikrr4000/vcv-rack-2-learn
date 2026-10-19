/**
 * Node checker on the fixtures exported by `vcv/scripts/live-check.mjs --fixtures` (the same fixtures the
 * C++ checker of the Atlas plugin replays), plus the `fixed` kind used by debug blocks.
 */
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { test } from "node:test";
import { fileURLToPath } from "node:url";

import { normalizeCheck, runCheck } from "./checker.mjs";

const DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../../rack/plugin/tests/fixtures/checks");

/** A fake REST client over a fixture case: details per module, samples served in turn. */
export function fixtureRack(c) {
	const calls = {};
	const detail = (id) => {
		const m = c.modules.find((x) => x.id === String(id));
		const k = calls[id] = (calls[id] ?? -1) + 1;
		const row = c.samples[id]?.[k % c.samples[id].length] || [];
		return { ...m, outputs: m.outputs.map((o, i) => ({ ...o, voltage: row[i] ?? 0 })), inputs: m.inputs };
	};
	return {
		modules: async () => c.modules.map((m) => ({ id: m.id, plugin: m.plugin, slug: m.slug, name: m.name })),
		module: async (id) => detail(id),
		cables: async () => c.cables,
	};
}

const files = fs.existsSync(DIR) ? fs.readdirSync(DIR).filter((f) => f.endsWith(".json")).sort() : [];

test("fixtures exist", () => assert.ok(files.length > 0));

for (const f of files) {
	test(`fixture ${f}`, async () => {
		const fx = JSON.parse(fs.readFileSync(path.join(DIR, f), "utf8"));
		for (const c of fx.cases) {
			const r = await runCheck(fx.check, { rack: fixtureRack(c), sleep: async () => {} });
			assert.equal(r.passed, c.expected.passed, `${f} [${c.name}] passed`);
			assert.deepEqual(r.items.map((i) => ({ key: i.key, ok: i.ok, hint: i.hint })), c.expected.items, `${f} [${c.name}] items`);
		}
	});
}

test("fixed kind: one expected cable keyed `fixed`", async () => {
	const spec = {
		kind: "fixed",
		modules: [
			{ ref: "env", plugin: "Fundamental", slug: "ADSR" },
			{ ref: "vca", plugin: "Fundamental", slug: "VCA-1" },
		],
		expect_cable: ["env", "Envelope", "vca", "CV"],
		hints: { fixed: "Проследи, куда идёт выход ADSR." },
	};
	assert.equal(normalizeCheck(spec).spec.cables.length, 1);
	const c = {
		modules: [
			{ id: "1", plugin: "Fundamental", slug: "ADSR", name: "ADSR", params: [], inputs: [], outputs: [{ id: 0, name: "Envelope" }] },
			{ id: "2", plugin: "Fundamental", slug: "VCA-1", name: "VCA", params: [], inputs: [{ id: 0, name: "CV" }, { id: 1, name: "Channel" }], outputs: [] },
		],
		cables: [],
		samples: {},
	};
	let r = await runCheck(spec, { rack: fixtureRack(c) });
	assert.equal(r.passed, false);
	assert.equal(r.items.at(-1).key, "fixed");
	assert.equal(r.items.at(-1).hint, "Проследи, куда идёт выход ADSR.");
	c.cables.push({ outputModuleId: "1", outputId: 0, inputModuleId: "2", inputId: 0 });
	r = await runCheck(spec, { rack: fixtureRack(c) });
	assert.equal(r.passed, true);
});
