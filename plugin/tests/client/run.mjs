#!/usr/bin/env node
/**
 * Runs the client end-to-end test (`client_e2e.cpp`): starts the tutor server on 127.0.0.1:5185
 * with test paths (state and lesson notes in .test-tmp/rack-client-test) and the stub tutor,
 * runs the test binary from bin/rackrun (where the Rack app's libRack is linked), stops the server.
 *
 *   node plugin/tests/client/run.mjs <client_e2e binary>
 */
import { spawn, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
const bin = path.resolve(process.argv[2]);
const dir = path.join(repo, ".test-tmp", "rack-client-test");
const PORT = 5185;

fs.rmSync(dir, { recursive: true, force: true });
fs.mkdirSync(path.join(dir, "state"), { recursive: true });
const env = {
	...process.env,
	PORT: String(PORT),
	STATE_DIR: path.join(dir, "state"),
	NOTES_DIR: path.join(dir, "notes"),
	RACK_TUTOR_CLAUDE: path.join(repo, "tutor", "stub-claude.mjs"),
	RACK_PORT: "2659",
	STUB_DELAY_MS: "10",
};
const server = spawn(process.execPath, [path.join(repo, "server", "index.mjs")], { cwd: repo, env, stdio: "ignore" });
let code = 1;
try {
	let up = false;
	for (let i = 0; i < 100 && !up; i++) {
		up = await fetch(`http://127.0.0.1:${PORT}/api/rack/health`).then((r) => r.ok, () => false);
		if (!up) await new Promise((r) => setTimeout(r, 100));
	}
	if (!up) throw new Error(`the tutor server did not start on ${PORT}`);
	const r = spawnSync(bin, [`http://127.0.0.1:${PORT}`, "p3-05-first-voice", "b6"], { cwd: path.join(path.dirname(bin), "rackrun"), stdio: "inherit", timeout: 120000 });
	code = r.status ?? 1;
	const note = fs.readFileSync(path.join(dir, "notes", "p3-05-first-voice.md"), "utf8");
	const logged = /репетитор: «/.test(note) && /## Журнал в Rack/.test(note);
	console.log(`${logged ? "PASS" : "FAIL"} the exchange is in the lesson note log`);
	if (!logged) code = code || 1;
} finally {
	server.kill();
}
process.exit(code);
