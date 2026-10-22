/**
 * Tutor runtime tests (plan §10.6), all on the stub (`tutor/stub-claude.mjs`): no model calls.
 *
 *   runner: lesson open starts one process; three messages reuse it; lesson switch replaces it;
 *           idle exit; «unknown option» fallback; deltas arrive in order;
 *   header: block text, patch line and check line of the right block;
 *   stub replies keep the TUTOR.md shapes (verdict first for grade, word limits);
 *   HTTP: the Atlas server on 5185 (and 5186 for the lesson switch) with fixture paths:
 *         open → ask → stream polling in order, progress events, the vault note log.
 */

import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { after, before, describe, test } from "node:test";
import { fileURLToPath } from "node:url";

import { reply } from "../tutor/stub-claude.mjs";
import { blockText, buildHeader, lessonBrief, packLesson } from "./context.mjs";
import { flagToDrop, IDLE_MS, OPTIONAL_FLAGS, READ_TOOLS, TutorRunner, tutorArgs, WRITE_TOOLS } from "./runner.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, "..");
const STUB = path.join(repo, "tutor", "stub-claude.mjs");
const PROMPT = path.join(repo, "tutor", "TUTOR.md");
const TMP = path.join(repo, ".test-tmp", "rack-tutor-test");
const LESSON = "p3-05-first-voice";
const OTHER = "p1-01-sound-wave";

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const words = (s) => s.split(/\s+/).filter(Boolean).length;
const readLog = (file) =>
	fs.existsSync(file)
		? fs
				.readFileSync(file, "utf8")
				.trim()
				.split("\n")
				.filter(Boolean)
				.map((l) => JSON.parse(l))
		: [];

async function until(fn, ms = 5000) {
	const end = Date.now() + ms;
	while (Date.now() < end) {
		const v = await fn();
		if (v) return v;
		await sleep(25);
	}
	throw new Error("timed out");
}

function makeRunner(name, extraEnv = {}, idleMs, warmup) {
	const stubLog = path.join(TMP, `${name}.jsonl`);
	const events = [];
	const runner = new TutorRunner({
		command: STUB,
		cwd: path.join(TMP, "run"),
		env: { ...process.env, STUB_CLAUDE_LOG: stubLog, STUB_DELAY_MS: "2", ...extraEnv },
		args: (drop) => tutorArgs({ promptFile: PROMPT, mcpConfig: path.join(TMP, "mcp.json"), repo, drop }),
		idleMs,
		warmup,
		log: (e) => events.push(e),
	});
	return { runner, stubLog, events };
}

function ask(runner, text) {
	return new Promise((resolve, reject) => {
		const chunks = [];
		runner.send(text, { push: (t) => chunks.push(t), end: (full) => resolve({ chunks, full }), fail: reject });
	});
}

before(() => {
	fs.rmSync(TMP, { recursive: true, force: true });
	fs.mkdirSync(TMP, { recursive: true });
	fs.mkdirSync(path.join(TMP, "run"), { recursive: true });
});

describe("runner", () => {
	test("argv: exact MCP tools, course read rule, no write tools allowed", () => {
		const a = tutorArgs({ promptFile: PROMPT, mcpConfig: "/x.json", repo, drop: new Set() });
		const allowed = a.slice(a.indexOf("--allowedTools") + 1, a.indexOf("--disallowedTools"));
		assert.deepEqual(allowed.slice(0, READ_TOOLS.length), READ_TOOLS);
		assert.equal(allowed.at(-1), `Read(/${repo}/course/**)`);
		assert.ok(allowed.at(-1).startsWith("Read(//"));
		for (const w of WRITE_TOOLS) assert.ok(a.includes(w) && !allowed.includes(w));
		for (const f of ["--strict-mcp-config", "--no-session-persistence", "--include-partial-messages", "--disable-slash-commands"]) assert.ok(a.includes(f), f);
		assert.equal(a[a.indexOf("--setting-sources") + 1], "local");
		assert.equal(a[a.indexOf("--permission-mode") + 1], "dontAsk");
		assert.ok(!a.includes("--bare"));
		const old = tutorArgs({ promptFile: PROMPT, mcpConfig: "/x.json", repo, drop: new Set(OPTIONAL_FLAGS) });
		for (const f of OPTIONAL_FLAGS) assert.ok(!old.includes(f), f);
		assert.equal(old[old.indexOf("--append-system-prompt") + 1], fs.readFileSync(PROMPT, "utf8"));
	});

	test("idle default is 15 minutes unless set", () => {
		assert.equal(new TutorRunner({ args: () => [] }).opts.idleMs, IDLE_MS);
		assert.equal(new TutorRunner({ args: () => [], idleMs: undefined }).opts.idleMs, IDLE_MS);
		assert.equal(new TutorRunner({ args: () => [], idleMs: 200 }).opts.idleMs, 200);
	});

	test("flagToDrop: the named flag first, else the next in order", () => {
		assert.equal(flagToDrop("error: unknown option '--effort'", new Set()), "--effort");
		assert.equal(flagToDrop("error: unknown option '--weird'", new Set([OPTIONAL_FLAGS[0]])), OPTIONAL_FLAGS[1]);
		assert.equal(flagToDrop("some other failure", new Set()), null);
	});

	test("lesson open starts one process; three messages reuse it; deltas in order", async () => {
		const { runner, stubLog, events } = makeRunner("reuse");
		runner.open(LESSON);
		const pid = runner.pid;
		assert.ok(pid);
		for (let i = 0; i < 3; i++) {
			const r = await ask(runner, `[lesson ${LESSON} «Первый голос», block b3 «Фильтр» (story)]\n[kind] ask\n[learner] вопрос ${i}`);
			assert.equal(r.chunks.join(""), r.full, "chunks join to the result");
			assert.ok(r.chunks.length > 3, "streamed in pieces");
			assert.equal(runner.pid, pid);
		}
		assert.equal(runner.spawns, 1);
		assert.equal(runner.open(LESSON), false, "same lesson keeps the process");
		const log = readLog(stubLog);
		assert.equal(log.filter((e) => e.event === "start").length, 1);
		const msgs = log.filter((e) => e.event === "message");
		assert.equal(msgs.length, 4);
		assert.match(msgs[0].text, /Готов\?/, "warm-up first");
		assert.ok(events.some((e) => e.event === "warmup"));
		const start = log.find((e) => e.event === "start");
		assert.equal(start.env.ENABLE_TOOL_SEARCH, null, "test env passes through as given");
		assert.equal(path.basename(start.cwd), "run");
		runner.close();
	});

	test("lesson switch replaces the process", async () => {
		const { runner, stubLog } = makeRunner("switch");
		runner.open(LESSON);
		await ask(runner, "[kind] ask\n[learner] раз");
		const first = runner.pid;
		runner.open(OTHER);
		assert.notEqual(runner.pid, first);
		assert.equal(runner.lesson, OTHER);
		await ask(runner, "[kind] ask\n[learner] два");
		await until(() => readLog(stubLog).some((e) => e.event === "exit" && e.pid === first));
		assert.equal(runner.spawns, 2);
		runner.close();
	});

	test("idle exit, then the next message starts it again", async () => {
		const { runner, stubLog, events } = makeRunner("idle", {}, 200);
		runner.open(LESSON);
		await ask(runner, "[kind] ask\n[learner] раз");
		const first = runner.pid;
		await until(() => events.some((e) => e.event === "idle-exit"));
		await until(() => readLog(stubLog).some((e) => e.event === "exit" && e.pid === first));
		assert.equal(runner.pid, null);
		const r = await ask(runner, "[kind] ask\n[learner] два");
		assert.ok(r.full.length > 0);
		assert.notEqual(runner.pid, first);
		assert.equal(runner.spawns, 2);
		runner.close();
	});

	test("warm-up carries the lesson brief; init reports MCP; restart keeps the queue", async () => {
		const { runner, stubLog, events } = makeRunner("brief", {}, undefined, (lesson) => lessonBrief(lesson));
		runner.open(LESSON);
		await ask(runner, "[kind] ask\n[learner] раз");
		const warm = readLog(stubLog).find((e) => e.event === "message").text;
		const L = packLesson(LESSON);
		assert.ok(warm.includes(`«${L.title}»`));
		for (const b of L.blocks) assert.ok(warm.includes(`«${b.title}»`), b.id);
		assert.match(warm, /Готов\?/);
		assert.equal(runner.mcpOk, true);
		const init = events.find((e) => e.event === "init");
		assert.deepEqual(init.mcp, ["vcvrack:connected"]);
		assert.equal(init.mcpTools, 3);
		const first = runner.pid;
		const pending = ask(runner, "[kind] ask\n[learner] два");
		runner.restart("test");
		const r = await pending;
		assert.ok(r.full.length > 0, "the queued question is answered by the new process");
		assert.notEqual(runner.pid, first);
		runner.close();
	});

	test("MCP not connected is reported as mcpOk = false", async () => {
		const { runner, events } = makeRunner("nomcp", { STUB_MCP_STATUS: "failed" });
		runner.open(LESSON);
		await ask(runner, "[kind] ask\n[learner] раз");
		assert.equal(runner.mcpOk, false);
		assert.deepEqual(events.find((e) => e.event === "init").mcp, ["vcvrack:failed"]);
		runner.close();
	});

	test("unknown options are dropped one by one and the queue survives", async () => {
		const { runner, events } = makeRunner("fallback", { STUB_REJECT_FLAGS: "--exclude-dynamic-system-prompt-sections,--permission-prompts" });
		runner.open(LESSON);
		const r = await ask(runner, "[kind] grade\n[learner] это верно");
		assert.match(r.full, /^✓/);
		assert.deepEqual([...runner.drop], ["--exclude-dynamic-system-prompt-sections", "--permission-prompts"]);
		assert.equal(events.filter((e) => e.event === "fallback").length, 2);
		assert.equal(runner.spawns, 3);
		runner.close();
	});
});

describe("context header", () => {
	test("block text, patch line and check line of the asked block", async () => {
		const L = packLesson(LESSON);
		const B = L.blocks.find((b) => b.type === "practice");
		const h = await buildHeader({ lesson: LESSON, block: B.id, kind: "review", text: "", patch: "VCO.Sawtooth → VCF.Audio", check: "3/5 на месте; первое: кабель" }, { patchLine: async () => "unused" });
		const lines = h.split("\n");
		assert.equal(lines[0], `[lesson ${LESSON} «${L.title}», block ${B.id} «${B.title}» (practice)]`);
		assert.ok(lines[1].startsWith("[block text] "));
		assert.ok(lines[1].includes(blockText(B).split("\n")[0].slice(0, 40)));
		assert.ok(blockText(B).length <= 1200);
		assert.equal(lines[2], "[patch] VCO.Sawtooth → VCF.Audio");
		assert.equal(lines[3], "[check] 3/5 на месте; первое: кабель");
		assert.equal(lines.at(-2), "[kind] review");
		assert.ok(lines.at(-1).startsWith("[learner]"));
	});

	test("no patch from the plugin → the REST line; quiz line for grade", async () => {
		const h = await buildHeader({ lesson: OTHER, block: "b2", kind: "grade", text: "частота", quiz: "Вопрос: …; верно: Частота выросла вдвое" }, { patchLine: async () => "VCO.Sine → Audio.In" });
		assert.match(h, /^\[patch\] VCO\.Sine → Audio\.In$/m);
		assert.match(h, /^\[quiz\] Вопрос/m);
		assert.match(h, /^\[check\] нет$/m);
	});
});

describe("reply shapes (stub)", () => {
	const base = `[lesson ${LESSON} «Первый голос», block b6 «Практика — свой голос» (practice)]\n[check] 3/5 на месте; первое: кабель VCF → VCA\n`;
	test("grade starts with a verdict, ≤ 60 words, has «Верно так:»", () => {
		for (const learner of ["это верно", "не знаю"]) {
			const r = reply(`${base}[kind] grade\n[learner] ${learner}`);
			assert.ok(["✓", "✗", "≈"].includes([...r][0]), r);
			assert.ok(words(r) <= 60);
			assert.match(r, /Верно так:/);
		}
	});
	test("word limits: ask ≤ 90, review ≤ 80; simpler and example 2–4 sentences", () => {
		assert.ok(words(reply(`${base}[kind] ask\n[learner] зачем VCA?`)) <= 90);
		assert.ok(words(reply(`${base}[kind] review\n[learner] `)) <= 80);
		for (const k of ["simpler", "example"]) {
			const n = reply(`${base}[kind] ${k}\n[learner] x`).split(/[.?]\s/).length;
			assert.ok(n >= 2 && n <= 4, k);
		}
		assert.equal(reply("Готов? ответь ok"), "ok");
	});
});

// ── HTTP, two servers ──

function startServer(port, name) {
	const dir = path.join(TMP, name);
	const notes = path.join(dir, "notes");
	fs.mkdirSync(path.join(dir, "state"), { recursive: true });
	const env = {
		...process.env,
		PORT: String(port),
		STATE_DIR: path.join(dir, "state"),
		NOTES_DIR: notes,
		RACK_TUTOR_CLAUDE: STUB,
		RACK_TUTOR_MCP_WAIT_MS: "0",
		RACK_PORT: "2659",
		STUB_CLAUDE_LOG: path.join(dir, "stub.jsonl"),
		STUB_DELAY_MS: "5",
	};
	const child = spawn(process.execPath, [path.join(repo, "server", "index.mjs")], { cwd: repo, env, stdio: ["ignore", "pipe", "pipe"] });
	let out = "";
	child.stdout.on("data", (d) => (out += d));
	child.stderr.on("data", (d) => (out += d));
	const base = `http://127.0.0.1:${port}`;
	const call = async (method, url, body) => {
		const res = await fetch(base + url, { method, headers: body ? { "content-type": "application/json" } : undefined, body: body ? JSON.stringify(body) : undefined });
		const json = await res.json();
		if (!res.ok) throw new Error(`${method} ${url}: ${res.status} ${json.error}`);
		return json;
	};
	return { child, call, dir, notes, stubLog: env.STUB_CLAUDE_LOG, output: () => out };
}

async function poll(call, id) {
	const got = [];
	for (let i = 0; i < 200; i++) {
		const s = await call("GET", `/api/rack/stream/${id}?after=${got.length}`);
		got.push(...s.chunks);
		if (s.error) throw new Error(s.error);
		if (s.done) return got;
		await sleep(30);
	}
	throw new Error("stream did not finish");
}

describe("HTTP /api/rack", () => {
	let a;
	let b;
	before(async () => {
		a = startServer(5185, "srv-5185");
		b = startServer(5186, "srv-5186");
		for (const s of [a, b]) await until(() => s.call("GET", "/api/rack/health").catch(() => null), 15000);
	});
	after(() => {
		for (const s of [a, b]) s?.child.kill();
	});

	test("open → ask → stream in order; progress; lesson note log", async () => {
		const h = await a.call("GET", "/api/rack/health");
		assert.equal(h.tutor.model, "sonnet");
		assert.equal(h.rack.reachable, false);
		const o = await a.call("POST", "/api/rack/lesson/open", { lesson: LESSON });
		assert.ok(o.pid);
		const note = path.join(a.notes, `${LESSON}.md`);
		assert.match(fs.readFileSync(note, "utf8"), /^status: learning$/m);

		const replies = [];
		for (const kind of ["ask", "grade", "review"]) {
			const { stream } = await a.call("POST", "/api/rack/ask", { lesson: LESSON, block: "b6", kind, text: kind === "grade" ? "это верно" : "зачем фильтр?", patch: "VCO.Sawtooth → VCF.Audio", check: "3/5 на месте" });
			replies.push((await poll(a.call, stream)).join(""));
		}
		assert.match(replies[1], /^✓/);
		const log = readLog(a.stubLog);
		assert.equal(log.filter((e) => e.event === "start").length, 1, "one process for the lesson");
		const warmup = log.find((e) => e.event === "message").text;
		assert.match(warmup, /^\[lesson p3-05-first-voice «Первый голос»/, "the server warms the tutor up with the lesson brief");
		assert.match(warmup, /\[outline\]/);
		const asked = log.filter((e) => e.event === "message").slice(1);
		assert.equal(asked.length, 3);
		assert.match(asked[0].text, /^\[lesson p3-05-first-voice «Первый голос», block b6 /);
		assert.match(asked[0].text, /^\[patch\] VCO\.Sawtooth → VCF\.Audio$/m);
		assert.match(asked[0].text, /^\[check\] 3\/5 на месте$/m);
		const start = log.find((e) => e.event === "start");
		assert.equal(start.env.ENABLE_TOOL_SEARCH, "false");
		assert.equal(start.env.ANTHROPIC_API_KEY, null);
		assert.ok(start.args.includes("--input-format"));

		await a.call("POST", "/api/rack/progress", { lesson: LESSON, block: "b2", event: "block", data: { title: "Сначала угадай" }, at: 1000 });
		await a.call("POST", "/api/rack/progress", { lesson: LESSON, block: "b2", event: "quiz", data: { question: "Что делает VCA?", answer: "Громкость", correct: true }, at: 1001 });
		await a.call("POST", "/api/rack/progress", { lesson: LESSON, block: "b6", event: "check", data: { passed: false, items: [{ ok: true, what: "VCO" }, { ok: false, what: "кабель VCF → VCA" }] }, at: 1002 });
		await a.call("POST", "/api/rack/progress", { lesson: LESSON, block: "b6", event: "tutor", data: { q: "Зачем?", a: "Затем.", local: true }, at: 1003 });
		const p = await a.call("GET", "/api/rack/progress");
		assert.deepEqual(p.lessons[LESSON].reached, ["b2"]);
		assert.equal(p.lessons[LESSON].status, "learning");
		await a.call("POST", "/api/rack/progress", { lesson: LESSON, event: "completed", at: 1004 });
		const p2 = await a.call("GET", "/api/rack/progress");
		assert.equal(p2.lessons[LESSON].status, "learned");
		const text = fs.readFileSync(note, "utf8");
		assert.match(text, /^status: learned$/m);
		const journal = text.slice(text.indexOf("## Журнал в Rack"));
		for (const re of [/урок открыт в Rack/, /блок b2 «Сначала угадай»/, /тест \(b2\): «Что делает VCA\?» → «Громкость» ✓/, /проверка \(b6\): не пройдена, 1\/2; первое: кабель VCF → VCA/, /вопрос \(ответ из урока\): «Зачем\?»/, /вопрос \(b6\): «зачем фильтр\?»/, /репетитор: «/, /урок пройден/]) assert.match(journal, re);
	});

	test("lesson switch over HTTP replaces the process (server 5186)", async () => {
		const o1 = await b.call("POST", "/api/rack/lesson/open", { lesson: LESSON });
		const { stream } = await b.call("POST", "/api/rack/ask", { lesson: LESSON, block: "b1", kind: "ask", text: "что это?" });
		await poll(b.call, stream);
		const o2 = await b.call("POST", "/api/rack/lesson/open", { lesson: OTHER });
		assert.notEqual(o2.pid, o1.pid);
		const h = await b.call("GET", "/api/rack/health");
		assert.equal(h.tutor.lesson, OTHER);
		await until(() => readLog(b.stubLog).some((e) => e.event === "exit" && e.pid === o1.pid));
		const again = await b.call("POST", "/api/rack/lesson/open", { lesson: OTHER });
		assert.equal(again.started, false);
		assert.equal(again.pid, o2.pid);
	});

	test("bad requests", async () => {
		await assert.rejects(a.call("POST", "/api/rack/lesson/open", { lesson: "../x" }), /400/);
		await assert.rejects(a.call("GET", "/api/rack/stream/nope?after=0"), /404/);
		await assert.rejects(a.call("POST", "/api/rack/ask", { lesson: LESSON, kind: "ask", text: "  " }), /400/);
	});
});

after(() => {
	// the runners of the unit tests closed themselves; the servers are killed above
});
