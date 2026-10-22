/**
 * REST client for the VCV Rack MCP Server plugin (Neural-Harmonics) on
 * `127.0.0.1:<rackPort>`. Read-only, except for the helpers the scripted live
 * checks use (`addModule`, `addCable`, `deleteModule`, `deleteCable`, `setParams`).
 *
 * Every response is `{status: "ok", data}`. Two failures are kept apart:
 *   RackDown — no answer (connection refused, 3 s timeout): Rack or the server module is off;
 *   RackBusy — HTTP 503 "UI thread not responding": Rack is up, an open menu or dialog blocks it.
 */

import { rackPort } from "./config.mjs";

const TIMEOUT_MS = 3000;

export class RackDown extends Error {
	constructor(message = "Rack не отвечает") {
		super(message);
		this.kind = "down";
	}
}

export class RackBusy extends Error {
	constructor(message = "Rack занят: закройте открытое меню или диалог в Rack") {
		super(message);
		this.kind = "busy";
	}
}

export const rackBase = (port = rackPort()) => `http://127.0.0.1:${port}`;

export async function rackRequest(method, url, body, { port } = {}) {
	let res;
	try {
		res = await fetch(rackBase(port) + url, {
			method,
			headers: body === undefined ? undefined : { "content-type": "application/json" },
			body: body === undefined ? undefined : JSON.stringify(body),
			signal: AbortSignal.timeout(TIMEOUT_MS),
		});
	} catch {
		throw new RackDown();
	}
	let json = null;
	try {
		json = await res.json();
	} catch {
		// non-JSON body
	}
	const message = json?.error || json?.message || "";
	if (res.status === 503 && /UI thread/i.test(message)) throw new RackBusy();
	if (!res.ok || !json || json.status === "error") {
		const err = new Error(message || `Rack: HTTP ${res.status}`);
		err.status = res.status;
		throw err;
	}
	return json.data;
}

const get = (url, opts) => rackRequest("GET", url, undefined, opts);

export const status = (opts) => get("/status", opts);
/** `[{id, plugin, slug, name, params, inputs, outputs, x, y, width}]` */
export const modules = (opts) => get("/modules", opts);
/** `{id, plugin, slug, name, x, y, params:[…], inputs:[{id, name, connected, channels, voltage}], outputs:[…]}` */
export const module = (id, opts) => get(`/modules/${id}`, opts);
/** `[{id, name, unit, min, max, default, value, displayValue, options?, snap}]` */
export const params = (id, opts) => get(`/modules/${id}/params`, opts);
/** `[{id, outputModuleId, outputId, inputModuleId, inputId}]` */
export const cables = (opts) => get("/cables", opts);
export const library = (q, opts) => get(q ? `/library?q=${encodeURIComponent(q)}` : "/library", opts);
export const layout = (opts) => get("/rack/layout", opts);

export const addModule = (plugin, slug, x, y, opts) => rackRequest("POST", "/modules/add", { plugin, slug, x, y }, opts);
export const deleteModule = (id, opts) => rackRequest("DELETE", `/modules/${id}`, undefined, opts);
export const addCable = (outputModuleId, outputId, inputModuleId, inputId, opts) =>
	rackRequest("POST", "/cables", { outputModuleId, outputId, inputModuleId, inputId }, opts);
export const deleteCable = (id, opts) => rackRequest("DELETE", `/cables/${id}`, undefined, opts);
export const setParams = (id, list, opts) => rackRequest("POST", `/modules/${id}/params`, list, opts);
