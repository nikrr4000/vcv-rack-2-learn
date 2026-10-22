/**
 * The tutor for Rack lessons: one warm `claude -p` process per open lesson (plan §7.2).
 *
 * The process reads stream-json user messages on stdin and answers each with partial-message
 * `stream_event` text deltas and a `result`. Messages are sent one at a time; the rest wait in a
 * queue. Right after start a hidden warm-up («Готов?») connects MCP and caches the system prompt.
 * Idle for `idleMs` → stdin is closed and the process exits; the next message starts it again.
 * Opening another lesson closes the old process and starts a new one.
 *
 * The CLI's `system/init` event says whether the Rack MCP server connected; `mcpOk` keeps it, so the
 * server can restart a process that started while Rack was still loading the lesson patch.
 *
 * Probe-and-fallback: a CLI that does not know an optional flag exits at once with «unknown
 * option»; the runner drops that flag (or the next one in OPTIONAL_FLAGS order), remembers it for
 * the server's lifetime, logs it and starts again with the same queue.
 */

import { spawn } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

export const READ_TOOLS = ["get_status", "list_modules", "get_module", "get_params", "list_cables", "get_rack_layout"].map((t) => `mcp__vcvrack__vcvrack_${t}`);
export const WRITE_TOOLS = ["add_module", "delete_module", "set_params", "add_cable", "delete_cable", "set_layout_preferences"].map((t) => `mcp__vcvrack__vcvrack_${t}`);
/** Dropped in this order when the CLI refuses an option it does not know. */
export const OPTIONAL_FLAGS = ["--exclude-dynamic-system-prompt-sections", "--permission-prompts", "--effort", "--append-system-prompt-file"];
export const WARMUP = "Готов? Это служебная проверка связи: ответь одним словом «ok».";
export const IDLE_MS = 15 * 60 * 1000;
export const MODELS = ["sonnet", "haiku", "opus"];
export const EFFORTS = ["low", "medium", "high"];

/** The argv of the tutor process; `drop` holds optional flags the CLI refused. */
export function tutorArgs({ model = "sonnet", effort = "low", promptFile, mcpConfig, repo, drop = new Set() }) {
	const a = ["-p", "--input-format", "stream-json", "--output-format", "stream-json", "--verbose", "--include-partial-messages", "--model", model];
	if (!drop.has("--effort")) a.push("--effort", effort);
	if (!drop.has("--append-system-prompt-file")) a.push("--append-system-prompt-file", promptFile);
	else a.push("--append-system-prompt", fs.readFileSync(promptFile, "utf8"));
	a.push("--tools", "Read");
	a.push("--allowedTools", ...READ_TOOLS, `Read(/${path.join(repo, "rack", "course")}/**)`);
	a.push("--disallowedTools", ...WRITE_TOOLS);
	a.push("--mcp-config", mcpConfig, "--strict-mcp-config");
	a.push("--setting-sources", "local", "--disable-slash-commands");
	a.push("--permission-mode", "dontAsk");
	if (!drop.has("--permission-prompts")) a.push("--permission-prompts", "none");
	a.push("--no-session-persistence");
	if (!drop.has("--exclude-dynamic-system-prompt-sections")) a.push("--exclude-dynamic-system-prompt-sections");
	return a;
}

/** The MCP config of the tutor: the Rack MCP server over HTTP, nothing else. */
export function writeTutorMcpConfig(dir, rackPort) {
	fs.mkdirSync(dir, { recursive: true });
	const file = path.join(dir, "rack-tutor.json");
	fs.writeFileSync(file, `${JSON.stringify({ mcpServers: { vcvrack: { type: "http", url: `http://127.0.0.1:${rackPort}/mcp` } } }, null, 2)}\n`);
	return file;
}

/** Which optional flag an «unknown option» error names (or the next one still in use). */
export function flagToDrop(stderr, drop) {
	if (!/unknown option|unrecognized option|unknown argument/i.test(stderr)) return null;
	const named = OPTIONAL_FLAGS.find((f) => !drop.has(f) && stderr.includes(f));
	return named || OPTIONAL_FLAGS.find((f) => !drop.has(f)) || null;
}

/**
 * `opts`: {command, cwd, env, args(drop) → argv, idleMs, log(event), warmup(lesson) → text}.
 * `message.sink`: {push(text), end(full), fail(error)}.
 */
export class TutorRunner {
	constructor(opts) {
		this.opts = { log: () => {}, ...opts, idleMs: opts.idleMs > 0 ? opts.idleMs : IDLE_MS };
		this.drop = new Set();
		this.lesson = null;
		this.proc = null;
		this.queue = [];
		this.current = null;
		this.spawns = 0;
		this.idleTimer = null;
		this.mcpOk = null; // null: not reported yet; true / false from the CLI's init event
		this.tools = [];
	}

	/** Stops the process and starts it again for the same lesson (queued messages are kept). */
	restart(reason) {
		const keep = [this.current, ...this.queue].filter((m) => m && !m.hidden);
		const p = this.proc;
		this.proc = null;
		this.current = null;
		this.queue = [];
		clearTimeout(this.idleTimer);
		if (p) {
			this.opts.log({ event: "restart", reason, lesson: this.lesson, pid: p.pid });
			p.stdin.end();
			const t = setTimeout(() => p.kill(), 5000);
			p.once("exit", () => clearTimeout(t));
		}
		for (const m of keep) m.deltas = "";
		this.queue = keep;
		this.start();
	}

	/** Starts (or keeps) the process of this lesson. */
	open(lesson) {
		if (this.lesson === lesson && this.proc) return false;
		if (this.lesson !== lesson) this.close("lesson switch");
		this.lesson = lesson;
		this.start();
		return true;
	}

	/** Queues a message; starts the process when it is not running. */
	send(text, sink, { hidden = false } = {}) {
		this.queue.push({ text, sink, hidden, deltas: "" });
		if (!this.proc) this.start();
		else this.pump();
	}

	close(reason = "close") {
		clearTimeout(this.idleTimer);
		const p = this.proc;
		this.proc = null;
		for (const m of [this.current, ...this.queue]) if (m && !m.hidden) m.sink.fail("Репетитор перезапущен. Спроси ещё раз.");
		this.current = null;
		this.queue = [];
		if (p) {
			this.opts.log({ event: "close", reason, lesson: this.lesson, pid: p.pid });
			p.stdin.end();
			// A process that ignores closed stdin is stopped after 5 s.
			const t = setTimeout(() => p.kill(), 5000);
			p.once("exit", () => clearTimeout(t));
		}
	}

	get pid() {
		return this.proc?.pid ?? null;
	}

	get busy() {
		return Boolean(this.current) || this.queue.some((m) => !m.hidden);
	}

	start() {
		const { command, cwd, env } = this.opts;
		const args = this.opts.args(this.drop);
		const proc = spawn(command, args, { cwd, env, stdio: ["pipe", "pipe", "pipe"] });
		this.proc = proc;
		this.spawns++;
		this.mcpOk = null;
		this.tools = [];
		const state = { stdout: false, stderr: "" };
		this.opts.log({ event: "spawn", lesson: this.lesson, pid: proc.pid, dropped: [...this.drop] });
		proc.on("error", (err) => {
			state.stderr += String(err.message);
		});
		proc.stderr.on("data", (d) => {
			state.stderr = (state.stderr + d).slice(-4000);
		});
		let buf = "";
		proc.stdout.on("data", (d) => {
			state.stdout = true;
			buf += d;
			let i;
			while ((i = buf.indexOf("\n")) >= 0) {
				const line = buf.slice(0, i);
				buf = buf.slice(i + 1);
				if (line.trim()) this.onLine(proc, line);
			}
		});
		proc.on("exit", (code) => this.onExit(proc, code, state));
		// The warm-up goes first, ahead of anything already queued.
		const warm = (this.opts.warmup && this.opts.warmup(this.lesson)) || WARMUP;
		this.queue.unshift({ text: warm, sink: { push() {}, end() {}, fail() {} }, hidden: true, deltas: "" });
		this.pump();
	}

	pump() {
		if (!this.proc || this.current || !this.queue.length) return;
		clearTimeout(this.idleTimer);
		this.current = this.queue.shift();
		const msg = { type: "user", message: { role: "user", content: [{ type: "text", text: this.current.text }] } };
		this.proc.stdin.write(`${JSON.stringify(msg)}\n`);
	}

	onLine(proc, line) {
		if (proc !== this.proc) return;
		let ev;
		try {
			ev = JSON.parse(line);
		} catch {
			return;
		}
		if (ev.type === "system" && ev.subtype === "init") {
			const servers = Array.isArray(ev.mcp_servers) ? ev.mcp_servers : [];
			const rack = servers.find((s) => s.name === "vcvrack");
			this.mcpOk = rack ? rack.status === "connected" : servers.length ? false : null;
			this.tools = Array.isArray(ev.tools) ? ev.tools : [];
			this.opts.log({ event: "init", lesson: this.lesson, pid: proc.pid, mcp: servers.map((s) => `${s.name}:${s.status}`), tools: this.tools.length, mcpTools: this.tools.filter((t) => String(t).startsWith("mcp__")).length });
			return;
		}
		const m = this.current;
		if (!m) return;
		if (ev.type === "assistant" && Array.isArray(ev.message?.content)) {
			for (const c of ev.message.content) if (c.type === "tool_use") this.opts.log({ event: "tool", lesson: this.lesson, pid: proc.pid, name: c.name });
		}
		if (ev.type === "stream_event" && ev.event?.type === "content_block_delta" && ev.event.delta?.type === "text_delta") {
			m.deltas += ev.event.delta.text;
			if (!m.hidden) m.sink.push(ev.event.delta.text);
		} else if (ev.type === "result") {
			const full = typeof ev.result === "string" ? ev.result : m.deltas;
			if (!m.hidden) {
				// No partial messages (an old CLI): the whole reply as one chunk.
				if (!m.deltas && full) m.sink.push(full);
				if (ev.is_error) m.sink.fail(full || "Репетитор не смог ответить.");
				else m.sink.end(full || m.deltas);
			}
			this.opts.log({ event: m.hidden ? "warmup" : "reply", lesson: this.lesson, pid: proc.pid, chars: (full || "").length });
			this.current = null;
			this.armIdle();
			this.pump();
		}
	}

	onExit(proc, code, state) {
		if (proc !== this.proc) return;
		this.proc = null;
		clearTimeout(this.idleTimer);
		const flag = !state.stdout ? flagToDrop(state.stderr, this.drop) : null;
		if (flag) {
			this.drop.add(flag);
			this.opts.log({ event: "fallback", dropped: flag, stderr: state.stderr.trim().slice(0, 300) });
			// The same queue again, without the warm-up that start() adds back.
			if (this.current) this.queue.unshift(this.current);
			this.current = null;
			this.queue = this.queue.filter((m) => !m.hidden);
			this.start();
			return;
		}
		this.opts.log({ event: "exit", lesson: this.lesson, pid: proc.pid, code, stderr: state.stderr.trim().slice(-300) });
		const failed = [this.current, ...this.queue].filter((m) => m && !m.hidden);
		this.current = null;
		this.queue = [];
		const why = state.stderr.trim().split("\n").pop() || `код ${code}`;
		for (const m of failed) m.sink.fail(`Репетитор остановился: ${why}`);
	}

	armIdle() {
		clearTimeout(this.idleTimer);
		if (this.current || this.queue.length) return;
		this.idleTimer = setTimeout(() => {
			if (!this.proc || this.current || this.queue.length) return;
			const p = this.proc;
			this.proc = null;
			this.opts.log({ event: "idle-exit", lesson: this.lesson, pid: p.pid });
			p.stdin.end();
		}, this.opts.idleMs);
		this.idleTimer.unref?.();
	}
}
