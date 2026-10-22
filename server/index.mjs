/**
 * The tutor server of the Atlas plugin: `node server/index.mjs` (or `npm start`).
 * Serves `/api/rack/*` on 127.0.0.1:PORT (see `routes.mjs`); settings in `config.mjs`.
 */

import http from "node:http";

import { notesDir, PORT, STATE_DIR } from "./config.mjs";
import { HttpError, send } from "./http.mjs";
import { rackRoutes, stopTutor } from "./routes.mjs";

const routes = [["GET", /^\/$/, () => ({ ok: true, service: "vcv-rack-2-learn tutor", api: "/api/rack/health" })], ...rackRoutes];

const server = http.createServer(async (req, res) => {
	try {
		const { pathname } = new URL(req.url, "http://x");
		let methodMismatch = false;
		for (const [method, pattern, handler] of routes) {
			const m = pattern.exec(pathname);
			if (!m) continue;
			if (method !== req.method) {
				methodMismatch = true;
				continue;
			}
			const params = m.slice(1).map((p) => decodeURIComponent(p));
			const body = await handler(params, req, res);
			if (body !== undefined && !res.headersSent) send(res, 200, body);
			return;
		}
		return send(res, methodMismatch ? 405 : 404, { error: methodMismatch ? "method not allowed" : "not found" });
	} catch (err) {
		if (res.headersSent) return res.end();
		if (err instanceof HttpError) return send(res, err.code, { error: err.message, ...err.extra });
		console.error(err);
		return send(res, 500, { error: err.message });
	}
});

server.headersTimeout = 60000;
server.on("error", (err) => {
	if (err.code !== "EADDRINUSE") throw err;
	console.error(`port ${PORT} is busy — another tutor server (or Atlas) is running there; set PORT to use another one`);
	process.exit(1);
});
server.listen(PORT, "127.0.0.1", () => {
	console.log(`tutor on http://127.0.0.1:${PORT} — state: ${STATE_DIR}, notes: ${notesDir()}`);
});

for (const sig of ["SIGINT", "SIGTERM"])
	process.on(sig, () => {
		stopTutor();
		server.close();
		process.exit(0);
	});
