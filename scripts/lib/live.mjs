/**
 * Shared rules for scripts that touch the owner's live Rack (agent patch, port 2600):
 *   - `GET /status` before every live step; no answer → wait 60 s, retry once, else skip;
 *   - modules go only into the session's own row (`new_row` from `/rack/layout`);
 *   - every created module id is tracked and deleted in `cleanup()`, so the patch ends
 *     as the owner left it. Modules the session did not create are never touched.
 */

import * as rack from "../../../app/server/vcv/rack.mjs";

export { rack };

export const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

/** `true` when Rack answers `/status`, after one 60 s retry. */
export async function rackReady({ retryMs = 60000, log = console.log } = {}) {
	try {
		await rack.status();
		return true;
	} catch (err) {
		log(`rack: ${err.message} — waiting ${retryMs / 1000} s and retrying once`);
		await sleep(retryMs);
		try {
			await rack.status();
			return true;
		} catch (err2) {
			log(`rack: still no answer (${err2.message}) — step UNVERIFIED`);
			return false;
		}
	}
}

export function session() {
	const created = [];
	let row = null;
	let nextX = null;
	return {
		created,
		/** The session row: `new_row` from the layout, fixed at the first call. */
		async row() {
			if (!row) {
				const layout = await rack.layout();
				const pos = layout.suggested_positions.find((p) => p.label === "new_row");
				if (!pos) throw new Error("no new_row position in /rack/layout");
				row = { x: pos.x, y: pos.y };
				nextX = pos.x;
			}
			return row;
		},
		/** Adds a module at the next free x of the session row. → the module id. */
		async add(plugin, slug) {
			const { y } = await this.row();
			const res = await rack.addModule(plugin, slug, nextX, y);
			created.push(String(res.id));
			const mods = await rack.modules();
			const mine = mods.find((m) => String(m.id) === String(res.id));
			nextX += mine?.width || 150;
			return String(res.id);
		},
		/** Deletes one created module and waits until it is gone (deletes are queued). */
		async remove(id) {
			if (!created.includes(String(id))) throw new Error(`refusing to delete ${id}: not created by this session`);
			await rack.deleteModule(id);
			for (let i = 0; i < 30; i++) {
				const mods = await rack.modules();
				if (!mods.some((m) => String(m.id) === String(id))) break;
				await sleep(100);
			}
			created.splice(created.indexOf(String(id)), 1);
			if (!created.length && row) nextX = row.x;
		},
		async cleanup() {
			for (const id of [...created].reverse()) {
				try {
					await this.remove(id);
				} catch (err) {
					console.error(`cleanup: module ${id}: ${err.message}`);
				}
			}
			return created.length === 0;
		},
	};
}
