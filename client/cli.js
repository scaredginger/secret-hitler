const Client = require('./client');

// const domain = 'localhost';
const domain = '13.211.160.125'
const port = '4545';

process.stdin.setEncoding('utf8');
let stdinBuffer = '';
let online = null;
process.stdin.on('readable', () => {
	// Use a loop to make sure we read all available data.
	while ((chunk = process.stdin.read()) !== null) {
		const i = chunk.indexOf('\n');
		if (online && i >= 0) {
			const a = chunk.substr(0, i);
			stdinBuffer = chunk.substr(i + 1);
			online(a.trim());
		} else {
			stdinBuffer += chunk;
		}
	}
});

process.stdin.on('end', () => {
	process.stdout.write('end');
});

function getline() {
	return new Promise((resolve, reject) => {
		const i = stdinBuffer.indexOf('\r\n');
		if (i >= 0) {
			const a = stdinBuffer.substr(0, i);
			stdinBuffer = stdinBuffer.substr(i + 1);
			return resolve(a);
		}
		online = resolve;
	});
}

async function getChancellorNomination(options) {
	console.log('Nominate your chancellor.');
	for (const o of options) {
		if (o.eligible) {
			console.log(`${o.id}) ${o.player}`);
		}
	}
	process.stdout.write('chancellor> ');
	const l = await getline();
	const n = parseInt(l);

	if (!(0 <= n && n < options.length)) {
		console.log('Invalid choice. Please enter a number.');
		return getChancellorNomination(options);
	}
	if (!options[n] || !options[n].eligible) {
		console.log('Invalid choice.');
		return getChancellorNomination(options);
	}
	return n;
}

async function getVote(president, chancellor) {
	console.log('President:', president);
	console.log('Chancellor:', chancellor);
	console.log('Vote, ja or nein.')
	process.stdout.write('vote> ');
	const l = await getline();
	if (l !== 'ja' && l !== 'nein') {
		console.log('Please vote, ja or nein');
		return getVote(president, chancellor);
	}
	return l;
}

async function getVictim(options) {
	console.log('Pick somebody to kill:');
	for (const o of options) {
		if (o.eligible) {
			console.log(`${o.id}) ${o.player}`);
		}
	}
	process.stdout.write('victim> ');
	const l = await getline();
	const n = parseInt(l);

	if (!(0 <= n && n < options.length)) {
		console.log('Invalid choice. Please enter a number.');
		return getVictim(options);
	}
	if (!options[n].eligible) {
		console.log('Invalid choice.');
		return getVictim(options);
	}
	return n;
}

async function getPresidentPolicy(policies) {
	console.log('Choose a policy to ELIMINATE');
	console.log(`0) ${policies[0]}`);
	console.log(`1) ${policies[1]}`);
	console.log(`2) ${policies[2]}`);

	process.stdout.write('policy> ');
	const l = await getline();
	const n = parseInt(l);
	if (n !== 0 && n !== 1 && n !== 2) {
		console.log('Please choose, 0, 1 or 2');
		return getPresidentPolicy(policies);
	}
	return n;
}

async function getChancellorPolicy(policies, canVeto) {
	console.log('Choose a policy to ELIMINATE');
	console.log(`0) ${policies[0]}`);
	console.log(`1) ${policies[1]}`);
	if (canVeto) {
		console.log('You may also choose to veto.');
	}

	process.stdout.write('policy> ');
	const l = await getline();
	const n = parseInt(l);
	if (n !== 0 && n !== 1 && !(canVeto && l === 'veto')) {
		console.log('Please choose, 0' + canVeto ? ', 1 or veto' : ' or 1');
		return getChancellorPolicy(policies, canVeto);
	}
	return isNaN(n) ? l : n;
}

async function getVeto(chancellor) {
	console.log(`${chancellor} has proposed a veto. Do you accept? (ja/nein)`);
	process.stdout.write('accept> ');
	const l = await getline();
	if (l !== 'ja' && l !== 'nein') {
		console.log('Please respond, ja or nein');
		return getVeto(chancellor);
	}
	return l === 'ja';
}

async function getInvestigation(options) {
	console.log('Pick somebody to investigate:');
	for (const o of options) {
		if (o.eligible) {
			console.log(`${o.id}) ${o.player}`);
		}
	}
	process.stdout.write('player> ');
	const l = await getline();
	const n = parseInt(l);

	if (!(0 <= n && n < options.length)) {
		console.log('Invalid choice. Please enter a number.');
		return getInvestigation(options);
	}
	if (!options[n].eligible) {
		console.log('Invalid choice.');
		return getInvestigation(options);
	}
	return n;
}

async function getSpecialNomination(options) {
	console.log('Choose the next president:');
	for (const o of options) {
		if (o.eligible) {
			console.log(`${o.id}) ${o.player}`);
		}
	}
	process.stdout.write('player> ');
	const l = await getline();
	const n = parseInt(l);

	if (!(0 <= n && n < options.length)) {
		console.log('Invalid choice. Please enter a number.');
		return getSpecialNomination(options);
	}
	if (!options[n].eligible) {
		console.log('Invalid choice.');
		return getSpecialNomination(options);
	}
	return n;
}

let round = 1;

const createHandlers = (client) => ({
	async gameKey({ key }) {
		console.log('Game key:', key);
	},
	async rename() {},
	async playerReady({ player }) {
	},
	async team(data) {
		console.log('Your role is:', data.role);
		console.log();
		if (data.roles) {
			for ({ player, role } of data.roles) {
				console.log(player, ':', role);
			}
		}
		console.log();
	},
	async chancellorNomination({ president, options }) {
		console.log('############################');
		console.log('Round', round++);
		console.log();
		console.log(president, 'is picking a chancellor.');
		console.log('Their choices are:');
		for (const o of options) {
			if (o.eligible) {
				console.log(`* ${o.player}`);
			}
		}
		console.log();
	},
	async requestChancellorNomination({ options }) {
		console.log('############################');
		console.log('Round', round++);
		console.log();
		const choice = await getChancellorNomination(options);
		client.nominateChancellor(choice);
		console.log();
	},
	async election({ president, chancellor }) {
		if (client.selfDead) {
			return;
		}
		const vote = await getVote(president, chancellor);
		client.vote(vote);
		console.log();
	},
	async electionComplete({ votes, success }) {
		const msg = success ? 'Election passed.' : 'Election failed.';
		console.log(msg);
		console.log();
		for ({ player, vote } of votes) {
			console.log(player, ':', vote);
		}
		console.log();
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async voteCounted() {},
	async presidentEliminatingPolicy({ president }) {
		console.log('The president,', president + ', is choosing a policy');
		console.log();
	},
	async chancellorEliminatingPolicy({ chancellor }) {
		console.log('The chancellor,', chancellor + ', is choosing a policy');
		console.log();
	},
	async requestPresidentPolicyElimination({ policies }) {
		const policy = await getPresidentPolicy(policies);
		client.eliminatePolicy(policy);
		console.log();
	},
	async requestChancellorPolicyElimination({ policies, canVeto }) {
		const policy = await getChancellorPolicy(policies, canVeto);
		client.eliminatePolicy(policy);
		console.log();
	},
	async regularFascistPolicy() {
		console.log('fascist policy passed');
		console.log('fascist policies:', client.fascistPolicies);
		console.log('liberal policies:', client.liberalPolicies);
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async chaoticFascistPolicy() {
		console.log('The country is in turmoil. Fascist policy passed');
		console.log('fascist policies:', client.fascistPolicies);
		console.log('liberal policies:', client.liberalPolicies);
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async regularLiberalPolicy() {
		console.log('liberal policy passed');
		console.log('fascist policies:', client.fascistPolicies);
		console.log('liberal policies:', client.liberalPolicies);
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async chaoticLiberalPolicy() {
		console.log('The country is in turmoil. Liberal policy passed');
		console.log('fascist policies:', client.fascistPolicies);
		console.log('liberal policies:', client.liberalPolicies);
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async presidentRevealPolicies() {
		console.log('The president now knows the top three policies.\n');
		console.log();
	},
	async revealPolicies({ policies }) {
		console.log('Next three policies:');
		console.log(policies.join(', '));
		console.log();
	},
	async kill({ president, options }) {
		console.log(president, 'can kill somebody:');
		for (const o of options) {
			if (o.eligible) {
				console.log(`* ${o.player}`);
			}
		}
		console.log();
	},
	async requestKill({ options }) {
		const victim = await getVictim(options);
		client.killPlayer(victim);
		console.log();
	},
	async death({ player, president }) {
		console.log(`${president} killed ${player}.`);
		console.log();
	},
	async dead({ president }) {
		if (role === 'hitler') {
			console.log(president, 'killed you.');
		} else {
			const msg = `${president} killed you. You will no longer participate in the game, ` +
				`and remember, dead people don\'t talk.`; 
			console.log(msg);
		}
		console.log();
	},
	async requestVeto({ chancellor }) {
		const accept = await getVeto(chancellor);
		client.respondToVeto(accept);
		console.log();
	},
	async presidentRequestVeto({ president, chancellor }) {
		console.log(`${chancellor} has proposed a veto, waiting for ${president} to respond.`);
		console.log();
	},
	async successfulVeto({ president, chancellor }) {
		console.log(`${president} accepted ${chancellor}'s veto.`);
		console.log('election tracker:', client.electionTracker);
		console.log();
	},
	async failedVeto({ president, chancellor }) {
		console.log(`${president} rejected ${chancellor}'s veto.`);
		console.log();
	},
	async fascistHitlerWin() {
		console.log('Hitler was elected. Fascists win!');
		console.log();
		client.ws.terminate();
	},
	async liberalHitlerWin() {
		console.log('Hitler was killed. Liberals win!');
		console.log();
		client.ws.terminate();
	},
	async fascistPolicyWin() {
		console.log('Six fascist policies were passed. Fascists win!');
		console.log();
		client.ws.terminate();
	},
	async liberalPolicyWin() {
		console.log('Five liberal policies were passed. Liberals win!');
		console.log();
		client.ws.terminate();
	},
	async requestInvestigation({ options }) {
		const choice = await getInvestigation(options);
		client.revealPlayer(choice);
		console.log();
	},
	async investigation({ president, options }) {
		console.log(president, 'can reveal a player\'s team:');
		for (const o of options) {
			if (o.eligible) {
				console.log(`* ${o.player}`);
			}
		}
		console.log();
	},
	async revealTeam({ player, team }) {
		console.log(`${player} is a ${team}.`);
		console.log();
	},
	async presidentRevealTeam({ player, president }) {
		console.log(president, 'knows the party of', player + '.');
		console.log();
	},
	async requestSpecialNomination({ options }) {
		const choice = await getSpecialNomination(options);
		client.specialNomination(choice);
		console.log();
	},
	async specialNomination({ president, options }) {
		console.log(president, 'must choose the next president:');
		for (const o of options) {
			if (o.eligible) {
				console.log(`* ${o.player}`);
			}
		}
		console.log();
	},
	async disconnect({ player }) {
		console.log(player, 'has disconnected. You will not be able to finish this game.');
		console.log();
		client.ws.terminate();
	},
});

async function init(client) {
	process.stdout.write('> ');
	const str = await getline();
	str = str.trim().split(/\s+/)
	switch(str[0]) {
		case 'join':
			if (str.length < 2) {
				console.log('Need a game key to join');
				return init();
			}
			return client.join(str[1]);
		case 'create':
			if (str.length > 1 && !str.slice(1).reduce((a, b) => a && b == '', true)) {
				console.log('You gave an extraneous argument. Did you mean to join a game?');
				return init();
			}
			return client.create();
		default:
			console.log('Commands: "join [gameKey]" or "create"');
			return init(client);
	}
}

async function getName(client) {
	console.log('Select a name');
	process.stdout.write('name> ');
	const name = await getline();
	const isAnon = /^anon[0-9]+$/
	if (isAnon.exec(name)) {
		console.log('Invalid choice.');
		return getName(client);
	}
	const isWhitespace = /^\s*$/
	if (isWhitespace.exec(name)) {
		console.log('Invalid choice.');
		return getName(client);
	}
	for (const other of client.players) {
		if (other === name) {
			console.log('Another player chose that name.');
			return getName(client);
		}
	}
	return name;
}

(async function() {
	const client = new Client(domain, port);
	const handlers = createHandlers(client);
	client.subscribe((eventName, data) => {
		if (handlers[eventName])
			handlers[eventName](data);
		else {
			console.log('unknown', eventName);
		}
	});
	await init(client)
	const name = await getName(client);
	client.setName(name);
	console.log('Waiting for others to join...');
	client.ready();
})();
