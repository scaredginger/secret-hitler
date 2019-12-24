const WebSocket = require('ws');

function encodeKey(key) {
	const chars = Math.ceil(Math.log2(1 + 15/16 * (key + 1)) / 4);
	const residue = key - 16 * (Math.pow(16, chars - 1) - 1) / 15;
	let t = residue;
	let s = '';
	while (t > 0) {
		s += 'abcdefghijklmnop'[t % 16];
		t >>= 4;
	}
	while (s.length < chars) {
		s += 'a';
	}
	return s;
}

const MessageCode = [
	'announceElection',
	'requestPresidentPolicyChoice',
	'requestChancellorPolicyChoice',
	'requestInvestigation',
	'requestKill',
	'sendLoyalty',
	'topCards',
	'voteReceived',
	'ballot',
	'disconnect',
	'readyToStart',
	'notReady',
	'team',
	'changeName',
	'extended',
];

const ExtendedMessageCode = [
	'requestPresidentVeto',
	'liberalPolicyWin',
	'liberalHitlerWin',
	'fascistPolicyWin',
	'fascistHitlerWin',
	'requestSpecialNomination',
	'reassign',
	'regularFascistPolicy',
	'chaoticFascistPolicy',
	'regularLiberalPolicy',
	'chaoticLiberalPolicy',
	'requestChancellorNomination',
	'gameKey',
];

class Client {
	constructor() {
		this.players = [];
		this.id = -1;
		this.name = '';
		this.key = '';
		this.subscribers = {};
		this.subscriberPk = 0;
		this.noFascistPolicies = 0;
		this.noLiberalPolicies = 0;
		this.ws = null;
		this.vetoFlag = false;
	}

	sendName() {
		const encoder = new TextEncoder();
		const text = encoder.encode(this.name);
		const msg = new Uint8Array(text.length + 1);
		msg[0] = 4 * 8 + 7;
		for (let i = 0; i < text.length; i++) {
			msg[i + 1] = text[i];
		}
		this.ws.send(msg);
	}

	extractFlags(arr) {
		const flags = [];
		flags[0] = !!((arr[0] >> 6) & 1);
		flags[1] = !!((arr[0] >> 7) & 1);
		for (let i = 0; i < noPlayers - 2; i++) {
			flags[i] = !!(arr[1] & (1 << i));
		}
		return flags;
	}

	getOptions(arr) {
		return this.extractFlags(arr).map((flag, i) => ({
			name: this.players[i],
			id: i,
			eligible: flag,
		}));
	}

	announceElection(arr) {
		this.chancellorId = arr[0] >> 4;
		const president = this.players[this.presidentId];
		const chancellor = this.players[this.chancellorId];
		this.publishEvent('election', { president, chancellor });
	}

	requestPresidentPolicyChoice(arr) {
		if (this.id == this.presidentId) {
			const policy1 = (arr[0] >> 5) & 1 ? 'liberal' : 'fascist';
			const policy2 = (arr[0] >> 6) & 1 ? 'liberal' : 'fascist';
			const policy3 = (arr[0] >> 7) & 1 ? 'liberal' : 'fascist';
			this.publishEvent('requestPresidentPolicyElimination', {
				policies: [policy1, policy2, policy3],
			});
		} else {
			const president = this.players[this.presidentId];
			this.publishEvent('presidentChoosing', { president });
		}
	}

	requestChancellorPolicyChoice(arr) {
		if (this.vetoFlag) {
			this.veto(false);
			this.vetoFlag = false;
		}
		if (this.id == this.chancellorId) {
			const policy1 = (arr[0] >> 5) & 1 ? 'liberal' : 'fascist';
			const policy2 = (arr[0] >> 6) & 1 ? 'liberal' : 'fascist';
			const canVeto = !!((arr[0] >> 7) & 1);
			this.publishEvent('requestPresidentPolicyElimination', {
				policies: [policy1, policy2],
				canVeto,
			});
		} else {
			const chancellor = this.players[this.chancellorId];
			this.publishEvent('presidentChoosing', { chancellor });
		}
	}

	requestInvestigation(arr) {
		const options = this.getOptions(arr);
		if (this.id == this.presidentId) {
			this.publishEvent('requestInvestigation', { options });
		} else {
			const president = this.players[this.presidentId];
			this.publishEvent('investigation', { president, options });
		}
	}

	requestKill(arr) {
		const options = this.getOptions(arr);
		if (this.id == this.presidentId) {
			this.publishEvent('requestKill', { options });
		} else {
			const president = this.players[this.presidentId];
			this.publishEvent('kill', { president, options });
		}
	}

	sendLoyalty(arr) {
		const id = arr[0] >> 4;
		const player = this.players[id];
		if (arr.length == 2) {
			const team = arr[1] ? 'liberal' : 'fascist';
			this.publishEvent('revealTeam', { player, team });
		} else {
			const president = this.players[this.presidentId];
			this.publishEvent('presidentRevealTeam', { player, president });
		}
	}

	topCards(arr) {
		const policiesShown = !!(arr[0] & 16);
		if (policiesShown) {
			const policy1 = arr[0] & 32 ? 'liberal' : 'fascist';
			const policy2 = arr[0] & 64 ? 'liberal' : 'fascist';
			const policy3 = arr[0] & 128 ? 'liberal' : 'fascist';
			this.publishEvent('revealPolicies', {
				policies: [policy1, policy2, policy3],
			});
		} else {
			const president = this.players[this.presidentId];
			this.publishEvent('presidentRevealPolicies', { president });
		}
	}

	voteReceived(arr) {
		const playerId = arr[0] >> 4;
		const player = this.players[playerId];
		this.publishEvent('voteCounted', { player });
	}

	ballot(arr) {
		const votes = this.getFlags(arr).map((flag, i) => ({
			player: this.players[i],
			vote: flag ? 'ja' : 'nein',
		}));
		const success = votes.filter(a => a.vote === 'ja').length > players.length / 2;
		if (!success) {
			this.electionTracker++;
		}
		this.publishEvent('electionComplete', { votes, success });
	}

	disconnect(arr) {
		const playerId = arr[0] >> 4;
		const player = this.players[playerId];
		this.players[playerId] = undefined;
		this.publishEvent('disconnect', { player });
	}

	readyToStart(arr) {
		const playerId = arr[0] >> 4;
		const player = this.players[playerId];
		this.publishEvent('playerReady', { player });
	}

	notReady(arr) {
		const playerId = arr[0] >> 4;
		const player = this.players[playerId];
		this.publishEvent('playerNotReady', { player });
	}

	team(arr) {
		if (arr.length === 1) {
			const otherPlayer = arr[0] >> 4;
			if (otherPlayer === 0) {
				return this.publishEvent('team', { role: 'liberal' });
			} else if (otherPlayer === 15) {
				return this.publishEvent('team', { role: 'hitler' });
			} else {
				return this.publishEvent('team', { role: 'hitler', teamMates: [otherPlayer] });
			}
		} else {
			const roles = this.getFlags(arr).map((flag, i) => ({
				player: this.players[i],
				role: flag ? 'liberal' : 'fascist',
			}));
			const hitlerOffset = (arr[0] >> 4) & 3;
			for (let i = 0; i < players.length; i++) {
				if (players[i].role === 'fascist') {
					hitlerOffset--;
					if (hitlerOffset < 0) {
						players[i].role = 'hitler';
						break;
					}
				}
			}
			this.publishEvent('team', { role: 'fascist', roles });
		}
	}

	changeName(arr) {
		const i = arr[0] >> 4;
		const oldName = this.players[i];
		const text = arr.slice(1);
		const decoder = new TextDecoder();
		const newName = decoder.decode(text);

		this.players[i] = newName;
		this.publishEvent('rename', { oldName, newName });
		console.log(this.name, this.players);
	}

	parse(arr) {
		const methodName = MessageCode[arr[0] & 15];
		this[methodName](arr);
	}

	extended(arr) {
		const methodName = ExtendedMessageCode[arr[0] >> 4];
		this[methodName](arr);
	}

	requestPresidentVeto(arr) {
		if (this.presidentId === this.id) {
			return this.publishEvent('requestVeto', {});
		}
		const president = this.players[this.presidentId];
		this.vetoFlag = true;
		this.publishEvent('presidentRequestVeto', { president });
	}

	liberalPolicyWin(arr) {
		this.publishEvent('liberalPolicyWin', {});
	}

	liberalHitlerWin(arr) {
		this.publishEvent('liberalHitlerWin', {});
	}

	fascistPolicyWin(arr) {
		this.publishEvent('fascistPolicyWin', {});
	}

	fascistHitlerWin(arr) {
		this.publishEvent('fascistHitlerWin', {});
	}

	requestSpecialNomination(arr) {
		const options = this.players.map((player, id) => ({
			player,
			id,
			eligible: true,
		}));
		options[this.presidentId].eligible = false;
		if (this.presidentId === this.id) {
			this.publishEvent('presidentSpecialNomination', { options });
		} else {
			this.publishEvent('specialNomination', { options });
		}
	}

	reassign(arr) {
		const oldId = arr[1] & 15;
		const newId = arr[1] >> 4;
		this.players[newId] = this.players[oldId];
		this.players[oldId] = null;
	}

	regularFascistPolicy(arr) {
		this.electionTracker = 0;
		this.publishEvent('regularFascistPolicy', {});
	}

	chaoticFascistPolicy(arr) {
		if (this.vetoFlag) {
			this.veto(true);
		}
		this.electionTracker = 0;
		this.publishEvent('chaoticFascistPolicy', {});
	}

	regularLiberalPolicy(arr) {
		this.electionTracker = 0;
		this.publishEvent('regularLiberalPolicy', {});
	}

	chaoticLiberalPolicy(arr) {
		if (this.vetoFlag) {
			this.veto(true);
		}
		this.electionTracker = 0;
		this.publishEvent('chaoticLiberalPolicy', {});
	}

	requestChancellorNomination(arr) {
		if (this.vetoFlag) {
			this.veto(true);
			this.vetoFlag = false;
		}
		this.presidentId = arr[1];
		const options = this.getOptions(arr.slice(1));
		if (this.presidentId == this.id) {
			this.publishEvent('requestChancellorNomination', { options });
		} else {
			this.publishEvent('chancellorNomination', { options });
		}
	}

	gameKey(arr) {
		const intKey = (arr[1] << 24) | (arr[2] << 16) | (arr[3] << 8) | arr[4];
		this.key = encodeKey(intKey);
		this.publishEvent('gameKey', { key: this.key });
	}

	veto(accepted) {
		const president = this.players[this.presidentId];
		const chancellor = this.players[this.chancellorId];
		if (accepted) {
			this.electionTracker++;
			this.publishEvent('successfulVeto', { president, chancellor });
		} else {
			this.publishEvent('failedVeto', { president, chancellor });
		}
	}

	publishEvent(name, data) {
		for (const key in this.subscribers) {
			this.subscribers[key](name, data);
		}
	}

	onmessage(e) {
		const arr = new Uint8Array(e.data);
		this.parse(arr);
	}

	connect(url) {
		return new Promise((resolve, reject) => {
			try {
				this.ws = new WebSocket(url);
				this.ws.on('open', (e) => {
					resolve(this);
				});
				this.ws.onmessage = (e) => {
					const arr = new Uint8Array(e.data);
					this.id = arr[0];
					this.ws.onmessage = this.onmessage.bind(this);
				};
			} catch (e) {
				reject(e);
			}
		});
	}

	create() {
		return this.connect('ws://localhost:4545/create/');
	}

	join(key) {
		key = key.toLowerCase();
		this.key = key;
		return this.connect('ws://localhost:4545/join/' + key);
	}

	subscribe(f) {
		const k = this.subscriberPk++;
		this.subscribers[k] = f;
		return () => {
			delete this.subscribers[k];
		}
	}

	setName(name) {
		this.name = name;
		this.players[this.id] = name;
		this.sendName();
	}
}

function connect(name, key) {
	const c = new Client();
	c.subscribe((eventName, data) => {
		console.log(name, eventName, data);
	});
	c.join(key).then(() => {
		c.setName(name);
	});
	return c;
}

const handlers = {
	gameKey({ key }) {
		const j = connect('jr', key);
		const l = connect('lj', key);
		const r = connect('rb', key);
		const k = connect('kj', key);
		setTimeout(() => { k.ws.close() }, 1000)
	},
};

(function() {
	const client = new Client();
	client.subscribe((eventName, data) => {
		console.log('ak', eventName, data);
		if (handlers[eventName])
			handlers[eventName](data);
	});
	client.create().then(() => {
		client.setName('ak');
	});
})();
