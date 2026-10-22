#!/usr/bin/env node
/**
 * In-Rack QA on a separate test instance (plan §8, §10.4). The session that built the course could
 * not run it — Rack does not start inside its sandbox — so this is the owner's one command:
 *
 *   scripts/build.sh --test && node scripts/rack-test.mjs [<lesson> …] [--shots]
 *
 * Test instance: user folder .rack-test (plugins copied read-only from the owner's Rack folder on
 * first run, the test build of Atlas installed by build.sh --test), dev bridge on 127.0.0.1:2700,
 * MCP Server of the test patches on 2610, Atlas URL 127.0.0.1:5185 (start `npm run dev:fixtures` there
 * if you want the tutor in the test). For every lesson: open it, step through every block, and check
 * GET /state: no text overflow, no missing pointer target, no missing plugin, Navigator present, the
 * block count as compiled; run every practice check on the template (it must fail) and every debug
 * check (it must fail). Results: build/rack-test.json. Rack is closed with POST /quit; a 10-minute
 * alarm is the safety net.
 *
 * --shots: module screenshots instead (`Rack -t 2`, user folder .rack-shots, one run per
 * ATLAS_PREVIEW variant) into qa/previews/rack-<variant>/.
 */
import { spawn } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const RACK_BIN = process.env.RACK_BIN || path.join(os.homedir(), "Applications/VCV Rack 2 Free.app/Contents/MacOS/Rack");
const OWNER_PLUGINS = path.join(os.homedir(), "Library/Application Support/Rack2/plugins-mac-arm64");
const COPY = ["ImpromptuModular", "Valley", "AudibleInstruments", "VCV-Recorder", "VCVRackMcpServer"];
const BRIDGE = "http://127.0.0.1:2700";
const argv = process.argv.slice(2);
const shots = argv.includes("--shots");
const only = argv.filter((a) => !a.startsWith("--"));
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function prepare(dir) {
	fs.mkdirSync(path.join(dir, "plugins-mac-arm64"), { recursive: true });
	for (const f of ["log.txt", "autosave"]) fs.rmSync(path.join(dir, f), { recursive: true, force: true });
	for (const f of fs.readdirSync(dir)) if (f.startsWith("autosave")) fs.rmSync(path.join(dir, f), { recursive: true, force: true });
	const settings = path.join(dir, "settings.json");
	if (!fs.existsSync(settings)) fs.writeFileSync(settings, JSON.stringify({ showTipsOnLaunch: false, autoCheckUpdates: false }));
	for (const p of COPY) {
		const dst = path.join(dir, "plugins-mac-arm64", p);
		if (!fs.existsSync(dst) && fs.existsSync(path.join(OWNER_PLUGINS, p))) fs.cpSync(path.join(OWNER_PLUGINS, p), dst, { recursive: true });
	}
}

function startRack(dir, args, env) {
	const child = spawn("perl", ["-e", "alarm shift; exec @ARGV", "600", RACK_BIN, "-u", dir, ...args], { env: { ...process.env, ...env }, stdio: "ignore" });
	return child;
}

async function bridge(method, url, body) {
	const res = await fetch(BRIDGE + url, { method, headers: body ? { "content-type": "application/json" } : undefined, body: body ? JSON.stringify(body) : undefined, signal: AbortSignal.timeout(15000) });
	return res.json();
}

async function waitBridge() {
	for (let i = 0; i < 120; i++) {
		try {
			return await bridge("GET", "/state");
		} catch {
			await sleep(500);
		}
	}
	throw new Error("the dev bridge did not answer in 60 s");
}

async function testLessons() {
	const dir = path.join(REPO, ".rack-test");
	prepare(dir);
	const pack = JSON.parse(fs.readFileSync(path.join(REPO, "build/test-res/course/pack.json"), "utf8"));
	const ids = only.length ? only : pack.course.parts.flatMap((p) => p.lessons);
	const child = startRack(dir, [], { ATLAS_DEV_PORT: "2700", ATLAS_URL: "http://127.0.0.1:5185" });
	const results = [];
	try {
		await waitBridge();
		for (const id of ids) {
			const L = pack.lessons[id];
			const r = { lesson: id, problems: [] };
			await bridge("POST", "/open", { lesson: id });
			await sleep(800);
			let st = await bridge("GET", "/state");
			if (st.lesson !== id) r.problems.push(`opened «${st.lesson}», expected ${id}`);
			if (!st.navigator) r.problems.push("no Navigator");
			if (st.blockCount !== L.blocks.length) r.problems.push(`${st.blockCount} blocks, compiled ${L.blocks.length}`);
			if (st.missingPlugins.length) r.problems.push(`missing plugins: ${st.missingPlugins.join(", ")}`);
			for (const b of L.blocks) {
				await bridge("POST", "/goto", { block: b.id });
				await sleep(400);
				st = await bridge("GET", "/state");
				if (st.overflow.length) r.problems.push(`${b.id}: overflow in ${st.overflow.join(", ")}`);
				if (st.missingTargets.length) r.problems.push(`${b.id}: missing pointer targets ${st.missingTargets.join(", ")}`);
				const task = b.items.find((it) => it.kind === "task" && it.check);
				if (task) {
					const c = await bridge("POST", "/check");
					if (c.passed) r.problems.push(`${b.id}: the ${task.mode} check passes on the untouched template`);
				}
			}
			results.push(r);
			console.log(`${r.problems.length ? "FAIL" : "ok  "} ${id}${r.problems.length ? ` — ${r.problems.join("; ")}` : ""}`);
		}
	} finally {
		await bridge("POST", "/quit").catch(() => {});
		await sleep(1500);
		child.kill();
	}
	fs.mkdirSync(path.join(REPO, "build"), { recursive: true });
	fs.writeFileSync(path.join(REPO, "build/rack-test.json"), `${JSON.stringify(results, null, 1)}\n`);
	const bad = results.filter((r) => r.problems.length).length;
	console.log(`rack test: ${results.length - bad}/${results.length} lessons clean`);
	return bad ? 1 : 0;
}

async function screenshots() {
	const dir = path.join(REPO, ".rack-shots");
	fs.mkdirSync(path.join(dir, "plugins-mac-arm64"), { recursive: true });
	fs.rmSync(path.join(dir, "plugins-mac-arm64/Atlas"), { recursive: true, force: true });
	fs.cpSync(path.join(REPO, "plugin/dist/Atlas"), path.join(dir, "plugins-mac-arm64/Atlas"), { recursive: true });
	const variants = {
		default: "",
		pilot: "Page=p1-01-sound-wave/b3/page-1;Note=p1-01-sound-wave/b5/note-2;Figure=p1-01-sound-wave/b4/figure-1;Quiz=p1-01-sound-wave/b2/quiz-1;Task=p1-01-sound-wave/b8/task-1",
		voice: "Page=p3-05-first-voice/b3/page-1;Figure=p3-05-first-voice/b1/figure-1;Task=p3-05-first-voice/b6/task-1",
	};
	for (const [name, preview] of Object.entries(variants)) {
		fs.rmSync(path.join(dir, "screenshots/Atlas"), { recursive: true, force: true });
		fs.rmSync(path.join(dir, "log.txt"), { force: true });
		const child = startRack(dir, ["-t", "2"], preview ? { ATLAS_PREVIEW: preview } : {});
		await new Promise((r) => child.on("exit", r));
		const out = path.join(REPO, `qa/previews/rack-${name}`);
		fs.mkdirSync(out, { recursive: true });
		const src = path.join(dir, "screenshots/Atlas");
		if (fs.existsSync(src)) for (const f of fs.readdirSync(src)) fs.copyFileSync(path.join(src, f), path.join(out, f));
		console.log(`${name}: ${fs.existsSync(src) ? fs.readdirSync(src).length : 0} screenshots → ${path.relative(REPO, out)}`);
	}
	return 0;
}

process.exit(shots ? await screenshots() : await testLessons());
