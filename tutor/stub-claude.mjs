#!/usr/bin/env node
/**
 * A stub of the persistent tutor process (`claude -p --input-format stream-json …`) for tests:
 * no model is called. It reads stream-json user messages from stdin, one per line, and answers
 * each with `stream_event` text deltas, one `assistant` message and a `result`, like the real CLI
 * with `--include-partial-messages`. The reply follows the `TUTOR.md` template for the message's
 * `[kind]` line; the hidden warm-up «Готов?» gets «ok». It exits when stdin closes.
 *
 * Env:
 *   STUB_CLAUDE_LOG      JSON lines: one `start` record (pid, argv, cwd, env keys the tests check),
 *                        one `message` record per message (the full text), one `exit` record
 *   STUB_REJECT_FLAGS    comma-separated options to refuse like an old CLI does
 *                        («error: unknown option '--x'», exit 1) — tests the runner's fallback
 *   STUB_DELAY_MS        pause between deltas (default 15)
 */

import crypto from "node:crypto";
import fs from "node:fs";
import readline from "node:readline";
import { pathToFileURL } from "node:url";

const args = process.argv.slice(2);
const sessionId = crypto.randomUUID();
const delay = Number(process.env.STUB_DELAY_MS ?? 15);
const log = (rec) => {
	if (process.env.STUB_CLAUDE_LOG) fs.appendFileSync(process.env.STUB_CLAUDE_LOG, `${JSON.stringify({ at: new Date().toISOString(), pid: process.pid, ...rec })}\n`);
};

const rejected = (process.env.STUB_REJECT_FLAGS || "").split(",").filter(Boolean);
const bad = rejected.find((f) => args.includes(f));
if (bad) {
	process.stderr.write(`error: unknown option '${bad}'\n`);
	process.exit(1);
}

const env = Object.fromEntries(["ENABLE_TOOL_SEARCH", "ANTHROPIC_API_KEY", "ANTHROPIC_AUTH_TOKEN", "CLAUDE_CODE_DISABLE_AUTO_MEMORY"].map((k) => [k, process.env[k] ?? null]));
log({ event: "start", cwd: process.cwd(), args, env });

const out = (msg) => process.stdout.write(`${JSON.stringify(msg)}\n`);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const line = (text, tag) => text.match(new RegExp(`^\\[${tag}\\] ?(.*)$`, "m"))?.[1]?.trim() || "";

/** The reply for one message, shaped like TUTOR.md asks. */
export function reply(text) {
	if (/Готов\?/.test(text) && !/\[kind\]/.test(text)) return "ok";
	const kind = line(text, "kind") || "ask";
	const block = text.match(/block \S+ «([^»]*)»/)?.[1] || "этот блок";
	const learner = line(text, "learner");
	switch (kind) {
		case "grade":
			return /правильн|верн/i.test(learner) ? "✓ Ты верно уловил главное.\nВерно так: это и есть правильная мысль блока." : "≈ Часть мысли верна, но не вся.\nВерно так: сравни свой ответ с текстом блока ещё раз.";
		case "simpler":
			return `Представь водопроводный кран: ручка решает, сколько воды течёт. В блоке «${block}» ручка делает то же со звуком.`;
		case "example":
			return `Возьми модуль из блока «${block}» и поверни его главную ручку до середины. Послушай, что изменилось, и верни обратно.`;
		case "review": {
			const check = line(text, "check");
			return `Модули на месте, звук идёт. Главное сейчас: ${check && check !== "нет" ? `разберись с пунктом «${check.slice(0, 60)}»` : "нажми «Проверить», и я посмотрю результат"}.`;
		}
		default:
			return `Коротко: это про блок «${block}». Ты спросил: «${learner.slice(0, 60)}».\nПосмотри: на модули этого блока справа.`;
	}
}

async function answer(text) {
	const msgId = `msg_${crypto.randomUUID().slice(0, 8)}`;
	const r = reply(text);
	out({ type: "stream_event", event: { type: "message_start", message: { id: msgId, role: "assistant" } }, session_id: sessionId });
	out({ type: "stream_event", event: { type: "content_block_start", index: 0, content_block: { type: "text", text: "" } }, session_id: sessionId });
	for (const piece of r.match(/\S+\s*|\s+/g) || []) {
		out({ type: "stream_event", event: { type: "content_block_delta", index: 0, delta: { type: "text_delta", text: piece } }, session_id: sessionId });
		if (delay) await sleep(delay);
	}
	out({ type: "stream_event", event: { type: "content_block_stop", index: 0 }, session_id: sessionId });
	out({ type: "assistant", message: { id: msgId, role: "assistant", content: [{ type: "text", text: r }] }, session_id: sessionId });
	out({ type: "result", subtype: "success", is_error: false, num_turns: 1, duration_ms: 5, result: r, session_id: sessionId });
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
	out({ type: "system", subtype: "init", session_id: sessionId, cwd: process.cwd(), tools: ["Read"] });
	const rl = readline.createInterface({ input: process.stdin });
	let chain = Promise.resolve();
	rl.on("line", (raw) => {
		if (!raw.trim()) return;
		let msg;
		try {
			msg = JSON.parse(raw);
		} catch {
			return;
		}
		const content = msg.message?.content;
		const text = typeof content === "string" ? content : (content || []).map((c) => c.text || "").join("");
		log({ event: "message", text });
		chain = chain.then(() => answer(text));
	});
	rl.on("close", () =>
		chain.then(() => {
			log({ event: "exit" });
			process.exit(0);
		}),
	);
}
