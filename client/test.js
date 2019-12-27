const Client = require('./client');

function connect(name, key) {
	const c = new Client();
	const handlers = makeHandlers(c);
	c.subscribe((eventName, data) => {
		console.log(name, eventName, data);
		handlers[eventName](data);
	});
	c.join(key).then(() => {
		c.setName(name);
	});
	return c;
}

function makeHandlers(c) {
	return {
		gameKey({ key }) {
			const j = connect('jr', key);
			const l = connect('lj', key);
			const r = connect('rb', key);
			const k = connect('kj', key);
			const s = connect('sh', key);
			const p = connect('pd', key);
			const i = connect('it', key);
			const t = connect('tm', key);
			setTimeout(() => {
				j.ready();
				l.ready();
				r.ready();
				k.ready();
				s.ready();
				p.ready();
				i.ready();
				t.ready();
			}, 1000);
		},
		rename() {},
		playerReady() {},
		chancellorNomination() {},
		team() {},
		requestChancellorNomination({ options }) {
			const choice = options.reduce((acc, current, i) => current.eligible ? i : acc, 0);
			c.nominateChancellor(choice);
		},
		election() {
			c.vote('ja');
		},
		electionComplete() {},
		voteCounted() {},
		presidentEliminatingPolicy() {},
		chancellorEliminatingPolicy() {},
		requestPresidentPolicyElimination() { c.eliminatePolicy(0) },
		requestChancellorPolicyElimination() { c.eliminatePolicy(0) },
		regularFascistPolicy() {},
		chaoticFascistPolicy() {},
		regularLiberalPolicy() {},
		chaoticLiberalPolicy() {},
		presidentRevealPolicies() {},
		revealPolicies() {},
		kill() {},
		requestKill({ options }) {
			const choice = options.reduce((acc, current, i) => current.eligible ? i : acc, 0);
			c.killPlayer(choice);
		},
		fascistHitlerWin() {
			c.ws.close();
		},
		liberalHitlerWin() {
			c.ws.close();
		},
		fascistPolicyWin() {
			c.ws.close();
		},
		liberalPolicyWin() {
			c.ws.close();
		},
	};
}

(function() {
	const client = new Client();
	const handlers = makeHandlers(client);
	client.subscribe((eventName, data) => {
		console.log('ak', eventName, data);
		if (handlers[eventName])
			handlers[eventName](data);
	});
	client.create().then(() => {
		client.setName('ak');
	});
	setTimeout(() => {
		client.ready();
	}, 1000);
})();
