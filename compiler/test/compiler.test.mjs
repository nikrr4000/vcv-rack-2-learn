/** Compiler, layout presets, param resolver and validator rules on the pilot and on synthetic lessons. */
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { test } from "node:test";

import YAML from "yaml";

import { compileLesson, ROW_HP } from "../lib/compile.mjs";
import { loadLesson, RACK } from "../lib/data.mjs";
import { resolveParam } from "../lib/resolve.mjs";

const pilot = loadLesson(path.join(RACK, "course/lessons/p1-01-sound-wave.yaml"));

test("resolver: exponential, switch and snapped params", () => {
	assert.equal(resolveParam("Fundamental", "VCO", "Frequency", { display: "261.63" }).value, 0);
	assert.ok(Math.abs(resolveParam("Fundamental", "VCO", "Frequency", "523.25").value - 12) < 0.01);
	assert.equal(resolveParam("Fundamental", "Scope", "Trigger", "Enabled").value, 0);
	assert.equal(resolveParam("Fundamental", "Scope", "Gain 1", { display: "10" }).value, 1);
	const t = resolveParam("Fundamental", "Scope", "Time", { display: "500" }).value;
	assert.ok(Math.abs(t - 1) < 0.01, `Time 500 → ${t}`);
	assert.equal(resolveParam("Fundamental", "VCO", "Pulse width", "50").value, 0.5);
	assert.throws(() => resolveParam("Fundamental", "Scope", "Trigger", "On"), /not one of/);
	assert.throws(() => resolveParam("Fundamental", "VCO", "Freq", "1"), /no param/);
	assert.throws(() => resolveParam("Core", "AudioInterface2", "Level", "6"), /calibration/);
});

test("pilot compiles: rows, fixed parts, bus routing, practice and debug", () => {
	const r = compileLesson(pilot);
	assert.deepEqual(r.errors, []);
	const mods = r.patch.modules;
	assert.equal(new Set(mods.map((m) => m.id)).size, mods.length, "module ids unique");
	const nav = mods.find((m) => m.model === "Navigator");
	assert.deepEqual(nav.pos, [0, 0], "Navigator is the first module of row 0");
	assert.equal(nav.data.state.layoutHash, r.pack.layoutHash);
	const mcp = mods.find((m) => m.plugin === "VCVRackMcpServer");
	assert.deepEqual(mcp.params, [{ value: 2600, id: 0 }, { value: 1, id: 1 }]);
	assert.equal(mcp.pos[1], 0);
	for (const w of r.report.rowWidths) assert.ok(w <= ROW_HP, `row ${w} HP`);
	// Demo sound goes through the bus: no demo cable reaches an Audio module directly except the bus.
	const audio = mods.filter((m) => m.model === "AudioInterface2");
	const busAudio = audio.find((m) => m.pos[1] === 0);
	const mixer = mods.find((m) => m.model === "VCMixer");
	for (const c of r.patch.cables.filter((c) => audio.some((a) => a.id === c.inputModuleId))) assert.equal(c.outputModuleId, mixer.id);
	assert.ok(r.patch.cables.some((c) => c.inputModuleId === busAudio.id && c.inputId === 1), "mono bus feeds both inputs");
	assert.deepEqual(mixer.params.map((p) => p.value), [0, 0, 0, 0], "bus channels start closed");
	const sounding = r.pack.blocks.filter((b) => b.channels.length);
	assert.deepEqual(sounding.map((b) => b.id), ["b5", "b7", "b9"]);
	// Practice: Task, its own Audio, two benches 40 HP apart, given = that Audio.
	const b8 = r.pack.blocks.find((b) => b.id === "b8");
	const benches = mods.filter((m) => m.model === "Bench");
	assert.equal(benches.length, 2);
	assert.equal(benches[1].pos[0] - benches[0].pos[0], 2 + 1 + 40, "40 HP of bench between the markers");
	const practiceAudio = audio.find((m) => m.pos[1] === b8.row);
	assert.ok(practiceAudio);
	assert.deepEqual(b8.items.find((i) => i.kind === "task").given, [practiceAudio.id]);
	// Debug: Trigger broken to Disabled, check fixed on Enabled, scope = the block's modules.
	const b9 = r.pack.blocks.find((b) => b.id === "b9");
	const task = b9.items.find((i) => i.kind === "task");
	assert.equal(task.check.kind, "fixed");
	const scope = mods.find((m) => m.model === "Scope" && m.pos[1] === b9.row);
	assert.equal(scope.params.find((p) => p.id === 7).value, 1, "Trigger = Disabled (the break)");
	assert.ok(task.scope.includes(scope.id));
	// Notes point at real ports.
	const b5 = r.pack.blocks.find((b) => b.id === "b5");
	const note = b5.items.find((i) => i.id === "note-1");
	assert.equal(note.targets[0].kind, "output");
	assert.equal(note.targets[0].name, "Sine");
});

test("a row wider than 96 HP fails the build", () => {
	const L = structuredClone(pilot);
	const b = L.blocks.find((x) => x.id === "b5");
	for (let i = 0; i < 6; i++) b.modules.push({ ref: `extra${i}`, plugin: "Fundamental", slug: "SEQ3" });
	const r = compileLesson(L);
	assert.ok(r.errors.some((e) => /HP wide/.test(e)));
});

test("bad references fail the build", () => {
	const L = structuredClone(pilot);
	L.blocks.find((x) => x.id === "b5").cables.push({ from: ["vco", "Saw"], to: ["scope", "Ch 1"] });
	L.blocks.find((x) => x.id === "b5").notes.push({ text: "x", targets: [{ module: "scope", param: "Zoom" }] });
	const r = compileLesson(L);
	assert.ok(r.errors.some((e) => /no output «Saw»/.test(e)));
	assert.ok(r.errors.some((e) => /no param «Zoom»/.test(e)));
});

test("validator catches style and limits", () => {
	const dir = fs.mkdtempSync(path.join(os.tmpdir(), "lessons-"));
	const L = structuredClone(pilot);
	L.blocks[2].page += "\nВы видите, что это является важным!\nЧастота 440Гц.";
	L.blocks[1].questions[0].options[0].why = "слово ".repeat(45);
	fs.writeFileSync(path.join(dir, `${L.id}.yaml`), YAML.stringify(L));
	let out = "";
	try {
		execFileSync("node", [path.join(RACK, "compiler/validate.mjs"), "--json", path.join(dir, "v.json")], { env: { ...process.env, ATLAS_LESSONS_DIR: dir }, encoding: "utf8" });
	} catch (e) {
		out = e.stdout;
	}
	assert.match(out, /«ты»/);
	assert.match(out, /banned phrase «является»/);
	assert.match(out, /exclamation/);
	assert.match(out, /a space between the number and «Гц»/);
	assert.match(out, /words \(at most 40\)/);
});
