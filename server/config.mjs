/**
 * Every setting and path of the tutor server, resolved once from env or `state/settings.json`.
 *
 *   PORT                 server port                         default 5175 (the plugin's default address)
 *   STATE_DIR            progress, tutor log, settings       default `state/`
 *   NOTES_DIR            lesson notes with «Журнал в Rack»   default `state/notes` (point it at an Obsidian folder)
 *   RACK_PORT            VCV Rack MCP Server port            default 2600
 *   RACK_TUTOR_CLAUDE    the `claude` CLI to run             default: settings `claudePath`, else found on PATH
 *   TUTOR_MODEL          sonnet | opus | haiku               default: settings `tutorModel`, else sonnet
 *   TUTOR_EFFORT         low | medium | high                 default: settings `tutorEffort`, else low
 *   ATLAS_PACK_JSON      the course pack                     default `plugin/res/course/pack.json`
 */

import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

export const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
export const STATE_DIR = path.resolve(process.env.STATE_DIR || path.join(REPO, "state"));
export const TUTOR_DIR = path.join(REPO, "tutor");
export const PORT = Number(process.env.PORT) || 5175;

const settingsFile = () => path.join(STATE_DIR, "settings.json");

export function readSettings() {
	try {
		return JSON.parse(fs.readFileSync(settingsFile(), "utf8"));
	} catch {
		return {};
	}
}

export function writeSettings(patch) {
	const next = { ...readSettings(), ...patch };
	fs.mkdirSync(STATE_DIR, { recursive: true });
	fs.writeFileSync(settingsFile(), `${JSON.stringify(next, null, 2)}\n`);
	return next;
}

export const notesDir = () => path.resolve(process.env.NOTES_DIR || readSettings().notesDir || path.join(STATE_DIR, "notes"));

const isExecutable = (file) => {
	try {
		fs.accessSync(file, fs.constants.X_OK);
		return fs.statSync(file).isFile();
	} catch {
		return false;
	}
};

function detectClaude() {
	const which = spawnSync("which", ["claude"], { encoding: "utf8" });
	const found = which.status === 0 ? which.stdout.trim() : "";
	if (found && path.isAbsolute(found) && isExecutable(found)) return found;
	const candidates = [path.join(os.homedir(), ".local", "bin", "claude"), "/opt/homebrew/bin/claude", "/usr/local/bin/claude", path.join(os.homedir(), ".claude", "local", "claude")];
	return candidates.find(isExecutable) || null;
}

/** The `claude` of the tutor: env, else the stored path, else found now and stored. */
export function tutorClaudePath() {
	if (process.env.RACK_TUTOR_CLAUDE) return process.env.RACK_TUTOR_CLAUDE;
	const s = readSettings();
	if (s.claudePath) return s.claudePath;
	const found = detectClaude();
	if (found) writeSettings({ claudePath: found });
	return found;
}

/** The VCV Rack MCP Server port: env `RACK_PORT`, else the stored setting, else 2600. */
export function rackPort() {
	const port = Number(process.env.RACK_PORT || readSettings().rackPort);
	return Number.isInteger(port) && port > 0 && port < 65536 ? port : 2600;
}

/** The tutor's model and effort as stored (env first); `routes.mjs` validates them. */
export function tutorChoice() {
	const s = readSettings();
	return { tutorModel: process.env.TUTOR_MODEL || s.tutorModel, tutorEffort: process.env.TUTOR_EFFORT || s.tutorEffort };
}
