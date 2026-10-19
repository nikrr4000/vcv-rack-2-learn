/**
 * The API of the Atlas plugin in VCV Rack (plan §7.5):
 *
 *   GET   /api/rack/health                 Atlas up, tutor model/effort, tutor process, Rack MCP reachable
 *   POST  /api/rack/lesson/open            {lesson} → the warm tutor process; note `study → learning`
 *   POST  /api/rack/ask                    {lesson, block, kind, text, patch?, check?, quiz?} → {stream}
 *   GET   /api/rack/stream/:id?after=n     → {chunks, done, error}
 *   POST  /api/rack/progress               {lesson, block?, event, data, at} → the lesson's progress
 *   GET   /api/rack/progress               progress for the Navigator's course map
 *
 * One tutor process for the lesson open in Rack (`runner.mjs`); each message goes out wrapped in
 * the context header (`context.mjs`). Exchanges, blocks, quiz answers and checks go into the
 * lesson's vault note (`progress.mjs`). The web lesson runner is not involved.
 */

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

import { rackPort, readAppSettings, tutorClaudePath } from "../app-settings.mjs";
import { childEnv, HttpError, readJson } from "../http.mjs";
import { APP_STATE_DIR, REPO, VCV_DIR } from "../paths.mjs";
import { courseLesson } from "../vcv/course.mjs";
import * as rack from "../vcv/rack.mjs";
import { buildHeader, KINDS, packLesson } from "./context.mjs";
import { appendLog, lessonOpened, progressForRack, recordEvent } from "./progress.mjs";
import { EFFORTS, MODELS, TutorRunner, tutorArgs, writeTutorMcpConfig } from "./runner.mjs";

export const TUTOR_DIR = path.join(VCV_DIR, "tutor");
export const TUTOR_RUN_DIR = path.join(TUTOR_DIR, "run");
export const TUTOR_PROMPT = path.join(TUTOR_DIR, "TUTOR.md");
const STREAM_TTL_MS = 10 * 60 * 1000;

export function tutorSettings(s = readAppSettings()) {
	return { model: MODELS.includes(s.tutorModel) ? s.tutorModel : "sonnet", effort: EFFORTS.includes(s.tutorEffort) ? s.tutorEffort : "low" };
}

const logFile = () => path.join(APP_STATE_DIR, "rack-tutor.jsonl");
function log(rec) {
	try {
		fs.mkdirSync(APP_STATE_DIR, { recursive: true });
		fs.appendFileSync(logFile(), `${JSON.stringify({ at: new Date().toISOString(), ...rec })}\n`);
	} catch {
		// logging is best effort
	}
}

let runner = null;
let runnerKey = "";

/** The runner for the current claude path and tutor settings (a change restarts it). */
function getRunner() {
	const command = tutorClaudePath();
	if (!command) throw new HttpError(503, "Не найден claude: укажи путь в настройках Atlas");
	const { model, effort } = tutorSettings();
	const key = `${command}|${model}|${effort}|${rackPort()}`;
	if (runner && runnerKey === key) return runner;
	const lesson = runner?.lesson || null;
	runner?.close("settings changed");
	fs.mkdirSync(TUTOR_RUN_DIR, { recursive: true });
	const mcpConfig = writeTutorMcpConfig(path.join(APP_STATE_DIR, "mcp"), rackPort());
	runner = new TutorRunner({
		command,
		cwd: TUTOR_RUN_DIR,
		env: childEnv({ ENABLE_TOOL_SEARCH: "false", CLAUDE_CODE_DISABLE_AUTO_MEMORY: "1" }),
		args: (drop) => tutorArgs({ model, effort, promptFile: TUTOR_PROMPT, mcpConfig, repo: REPO, drop }),
		idleMs: Number(process.env.RACK_TUTOR_IDLE_MS) || undefined,
		log,
	});
	runnerKey = key;
	if (lesson) runner.lesson = lesson;
	return runner;
}

/** For tests and shutdown. */
export function stopTutor() {
	runner?.close("shutdown");
	runner = null;
	runnerKey = "";
}

// ── streams ──

const streams = new Map();

function createStream(onEnd) {
	const now = Date.now();
	for (const [id, s] of streams) if (now - s.created > STREAM_TTL_MS) streams.delete(id);
	const id = crypto.randomUUID();
	const s = { id, created: now, chunks: [], done: false, error: null };
	s.sink = {
		push: (t) => s.chunks.push(t),
		end: (full) => {
			s.done = true;
			onEnd?.(full, null);
		},
		fail: (err) => {
			s.done = true;
			s.error = String(err);
			onEnd?.(null, s.error);
		},
	};
	streams.set(id, s);
	return s;
}

function readStream(id, after) {
	const s = streams.get(id);
	if (!s) throw new HttpError(404, "нет такого ответа");
	const n = Math.max(0, Number.parseInt(after, 10) || 0);
	return { chunks: s.chunks.slice(n), done: s.done, error: s.error };
}

// ── handlers ──

const LESSON_RE = /^p\d-\d{2}-[a-z0-9-]+$/;
function requireLesson(id) {
	if (typeof id !== "string" || !LESSON_RE.test(id)) throw new HttpError(400, "lesson must be a course lesson id");
	if (!packLesson(id) && !courseLesson(id)) throw new HttpError(404, `нет урока ${id}`);
	return id;
}

export async function health() {
	let rackUp = false;
	try {
		await rack.status();
		rackUp = true;
	} catch {
		// Rack or its MCP server is off
	}
	const { model, effort } = tutorSettings();
	return { ok: true, tutor: { model, effort, claude: Boolean(tutorClaudePath()), lesson: runner?.lesson || null, pid: runner?.pid || null, busy: Boolean(runner?.busy), dropped: runner ? [...runner.drop] : [] }, rack: { reachable: rackUp, port: rackPort() } };
}

export function openLesson(body) {
	const lesson = requireLesson(body.lesson);
	const progress = lessonOpened(lesson);
	const r = getRunner();
	const started = r.open(lesson);
	return { lesson, started, pid: r.pid, progress };
}

export async function ask(body) {
	const lesson = requireLesson(body.lesson);
	const kind = KINDS.includes(body.kind) ? body.kind : "ask";
	const text = String(body.text || "").trim();
	if (!text && kind !== "review") throw new HttpError(400, "пустой вопрос");
	const r = getRunner();
	if (r.lesson !== lesson) r.open(lesson);
	const header = await buildHeader({ ...body, lesson, kind, text });
	const stream = createStream((full, error) => {
		try {
			appendLog(lesson, [`${kind === "grade" ? "ответ на тест" : "вопрос"}${body.block ? ` (${body.block})` : ""}: «${short(text || "проверь мой патч")}»`, error ? `ошибка: ${short(error)}` : `репетитор: «${short(full)}»`]);
		} catch (err) {
			log({ event: "log-failed", error: err.message });
		}
	});
	r.send(header, stream.sink);
	return { stream: stream.id };
}

const short = (s, max = 300) => {
	const t = String(s || "")
		.replace(/\s+/g, " ")
		.trim();
	return t.length <= max ? t : `${t.slice(0, max - 1)}…`;
};

export function progressEvent(body) {
	const lesson = requireLesson(body.lesson);
	if (typeof body.event !== "string") throw new HttpError(400, "event is required");
	return recordEvent({ lesson, block: typeof body.block === "string" ? body.block : "", event: body.event, data: body.data && typeof body.data === "object" ? body.data : {}, at: body.at });
}

export const rackRoutes = [
	["GET", /^\/api\/rack\/health$/, () => health()],
	["POST", /^\/api\/rack\/lesson\/open$/, async (_, req) => openLesson(await readJson(req))],
	["POST", /^\/api\/rack\/ask$/, async (_, req) => ask(await readJson(req))],
	["GET", /^\/api\/rack\/stream\/([\w-]+)$/, ([id], req) => readStream(id, new URL(req.url, "http://x").searchParams.get("after"))],
	["POST", /^\/api\/rack\/progress$/, async (_, req) => progressEvent(await readJson(req))],
	["GET", /^\/api\/rack\/progress$/, () => progressForRack()],
];
