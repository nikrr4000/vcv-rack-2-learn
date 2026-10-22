/** HTTP helpers of the tutor server. */

export class HttpError extends Error {
	constructor(code, message, extra = {}) {
		super(message);
		this.code = code;
		this.extra = extra;
	}
}

export function send(res, code, body) {
	res.writeHead(code, { "content-type": "application/json; charset=utf-8", "cache-control": "no-store" });
	res.end(JSON.stringify(body));
}

export function readBody(req, limit = 1e6) {
	return new Promise((resolve, reject) => {
		let data = "";
		req.on("data", (chunk) => {
			data += chunk;
			if (data.length > limit) reject(new HttpError(413, "body too large"));
		});
		req.on("end", () => resolve(data));
		req.on("error", reject);
	});
}

/** The request body as a JSON object; `{}` when empty. */
export async function readJson(req) {
	const text = await readBody(req);
	if (!text.trim()) return {};
	try {
		const value = JSON.parse(text);
		if (value && typeof value === "object" && !Array.isArray(value)) return value;
	} catch {
		// fall through
	}
	throw new HttpError(400, "invalid JSON");
}

// Variables of a running Claude Code session: a child `claude` must not think it is nested in one.
const SESSION_VARS = /^(CLAUDECODE|CLAUDE_PID|CLAUDE_EFFORT|CLAUDE_CODE_(SESSION_ID|CHILD_SESSION|SESSION_ATTENDED|ENTRYPOINT|EXECPATH|MESSAGING_\w+))$/;

/** Env for a child `claude`: no API keys (the CLI uses its own login), no parent-session variables. */
export function childEnv(extra = {}) {
	const env = { ...process.env, ...extra };
	delete env.ANTHROPIC_API_KEY;
	delete env.ANTHROPIC_AUTH_TOKEN;
	for (const key of Object.keys(env)) if (SESSION_VARS.test(key)) delete env[key];
	return env;
}
