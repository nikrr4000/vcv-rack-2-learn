#!/usr/bin/env node
/**
 * Runs the client end-to-end test (`client_e2e.cpp`): starts the Atlas server on 127.0.0.1:5185
 * with fixture paths (state and course notes in app/.fixtures/rack-client-test) and the stub tutor,
 * runs the test binary from bin/rackrun (where the Rack app's libRack is linked), stops the server.
 *
 *   node rack/plugin/tests/client/run.mjs <client_e2e binary>
 */
import { spawn, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../../..");
const app = path.join(repo, "app");
const bin = path.resolve(process.argv[2]);
const dir = path.join(app, ".fixtures", "rack-client-test");
const fx = (n) => path.join(app, ".fixtures", n);
const PORT = 5185;

fs.rmSync(dir, { recursive: true, force: true });
for (const d of ["state", "vcv", "pipeline-state"]) fs.mkdirSync(path.join(dir, d), { recursive: true });
const env = {
	...process.env,
	PORT: String(PORT),
	TOPICS_DIR: fx("topics"),
	LESSONS_DIR: fx("lessons"),
	IMG_DIR: fx("img"),
	PIPELINE_STATE_DIR: path.join(dir, "pipeline-state"),
	PIPELINE_CONFIG: fx("pipeline-config.json"),
	MD_LOG_CONFIG: fx("md-log.config.json"),
	CLAUDE_SETTINGS_PATH: fx("claude-settings.json"),
	LAUNCH_AGENTS_DIR: fx("launch-agents"),
	APP_STATE_DIR: path.join(dir, "state"),
	VCV_NOTES_DIR: path.join(dir, "vcv"),
	VCV_MD_LOG_CONFIG: fx("vcv-md-log.config.json"),
	RACK_TUTOR_CLAUDE: path.join(repo, "vcv", "tutor", "stub-claude.mjs"),
	RACK_PORT: "2659",
	STUB_DELAY_MS: "10",
};
const server = spawn(process.execPath, [path.join(app, "server", "index.mjs")], { cwd: app, env, stdio: "ignore" });
let code = 1;
try {
	let up = false;
	for (let i = 0; i < 100 && !up; i++) {
		up = await fetch(`http://127.0.0.1:${PORT}/api/rack/health`).then((r) => r.ok, () => false);
		if (!up) await new Promise((r) => setTimeout(r, 100));
	}
	if (!up) throw new Error(`the Atlas server did not start on ${PORT}`);
	const r = spawnSync(bin, [`http://127.0.0.1:${PORT}`, "p3-05-first-voice", "b6"], { cwd: path.join(path.dirname(bin), "rackrun"), stdio: "inherit", timeout: 120000 });
	code = r.status ?? 1;
	const note = fs.readFileSync(path.join(dir, "vcv", "p3-05-first-voice.md"), "utf8");
	const logged = /репетитор: «/.test(note) && /## Журнал в Rack/.test(note);
	console.log(`${logged ? "PASS" : "FAIL"} the exchange is in the lesson note log`);
	if (!logged) code = code || 1;
} finally {
	server.kill();
}
process.exit(code);
