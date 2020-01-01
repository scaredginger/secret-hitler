import { readable } from 'svelte/store';

let fascistPoliciesSetter,
    liberalPoliciesSetter,
    presidentSetter,
    chancellorSetter,
    previousPresidentSetter,
    previousChancellorSetter,
    presidentQueueSetter,
    electionTrackerSetter,
    policiesInDeckSetter,
    votedSetter,
    votesSetter,
    statusTextSetter,
    deadPlayersSetter,
    gameKeySetter,
    connectedSetter,
    startedSetter,
    playerSetter;

export const fascistPolicies = readable(0, (set) => {
  fascistPoliciesSetter = set;
});
fascistPolicies.subscribe(() => {});

export const liberalPolicies = readable(0, (set) => {
  liberalPoliciesSetter = set;
});
liberalPolicies.subscribe(() => {});

export const policiesInDeck = readable(17, (set) => {
  policiesInDeckSetter = set;
});
policiesInDeck.subscribe(() => {});

export const president = readable('', (set) => {
  presidentSetter = set;
});
president.subscribe(() => {});

export const chancellor = readable('', (set) => {
  chancellorSetter = set;
});
chancellor.subscribe(() => {});

export const previousPresident = readable('', (set) => {
  previousPresidentSetter = set;
});
previousPresident.subscribe(() => {});

export const previousChancellor = readable('', (set) => {
  previousChancellorSetter = set;
});
previousChancellor.subscribe(() => {});

export const presidentQueue = readable([], (set) => {
  presidentQueueSetter = set;
});
presidentQueue.subscribe(() => {});

export const electionTracker = readable(0, (set) => {
  electionTrackerSetter = set;
});
electionTracker.subscribe(() => {});

export const voted = readable([], (set) => {
  votedSetter = set;
});
voted.subscribe(() => {});

export const connected = readable(false, (set) => {
  connectedSetter = set;
});
connected.subscribe(() => {});

export const statusText = readable('', (set) => {
  statusTextSetter = set;
});
statusText.subscribe(() => {});

export const deadPlayers = readable([], (set) => {
  deadPlayersSetter = set;
});
deadPlayers.subscribe(() => {});

export const gameKey = readable('', (set) => {
  gameKeySetter = set;
});
gameKey.subscribe(() => {});

export const players = readable('', (set) => {
  playerSetter = set;
});
players.subscribe(() => {});

export const started = readable(false, (set) => {
  startedSetter = set;
});
started.subscribe(() => {});

const functionDict = client => ({
  connect() {
    connectedSetter(true);
  },
  election({ president, chancellor }) {
    presidentSetter(president);
    chancellorSetter(chancellor);
    statusTextSetter(`Election in progress for ${president} and ${chancellor}.`);
  },
  electionComplete({ president, chancellor, success }) {
    if (success) {
      previousPresidentSetter(president);
      previousChancellorSetter(chancellor);
    } else {
      electionTrackerSetter(client.electionTracker);
    }
  },
  requestPresidentPolicyChoice() {
    statusTextSetter('');
  },
  presidentEliminatingPolicy({ president }) {
    statusTextSetter(`President ${president} is eliminating a policy.`);
  },
  requestChancellorPolicyChoice() {
    statusTextSetter('');
  },
  chancellorEliminatingPolicy({ chancellor }) {
    statusTextSetter(`Chancellor ${chancellor} is eliminating a policy.`);
  },
  requestInvestigation() {
    statusTextSetter('');
  },
  investigation({ president }) {
    statusTextSetter(`President ${president} is investigating somebody.`); 
  },
  requestKill() {
    statusTextSetter('');
  },
  kill({ president }) {
    statusTextSetter(`President ${president} is killing somebody.`);
  },
  disconnect({ player }) {
    statusTextSetter(`${player} has disconnected. You will not be able to finish this game.`);
  },
  presidentRequestVeto({ president, chancellor }) {
    statusTextSetter(`Chancellor ${chancellor} has asked President ${president} to veto the policies.`);
  },
  liberalPolicyWin({}) {
    statusTextSetter('The Liberals won by passing five policies');
  },
  liberalHitlerWin({}) {
    statusTextSetter('The Liberals won by killing Hitler.');
  },
  fascistPolicyWin({}) {
    statusTextSetter('The Fascists won by passing six policies.');
  },
  fascistHitlerWin({}) {
    statusTextSetter('The Fascists won by electing Hitler as chancellor.');
  },
  specialNomination({ president }) {
    statusTextSetter(`President ${president} is choosing the next president.`)
  },
  regularFascistPolicy({}) {
    policiesInDeckSetter(client.policiesInDeck);
    fascistPoliciesSetter(client.fascistPolicies);
    electionTrackerSetter(0);
  },
  chaoticFascistPolicy({}) {
    policiesInDeckSetter(client.policiesInDeck);
    fascistPoliciesSetter(client.fascistPolicies);
    electionTrackerSetter(0);
  },
  regularLiberalPolicy({}) {
    policiesInDeckSetter(client.policiesInDeck);
    liberalPoliciesSetter(client.liberalPolicies);
    electionTrackerSetter(0);
  },
  chaoticLiberalPolicy({}) {
    policiesInDeckSetter(client.policiesInDeck);
    liberalPoliciesSetter(client.liberalPolicies);
    electionTrackerSetter(0);
  },
  requestChancellorNomination() {
    startedSetter(true);
  },
  chancellorNomination({ president }) {
    statusTextSetter(`President ${president} is nominating a chancellor.`);
    startedSetter(true);
  },
  gameKey({ key }) {
    gameKeySetter(key);
  },
  successfulVeto({}) {
    policiesInDeckSetter(client.policiesInDeck);
    electionTrackerSetter(client.electionTracker);
  },
  failedVeto({ chancellor }) {
    statusTextSetter(`Chancellor ${chancellor} is eliminating a policy.`);
  },
  rename({ }) {
    playerSetter(client.players.map(a => a));
  },
});

export function subscribeToClient(client) {
  const dispatcher = functionDict(client);
  client.subscribe((messageType, args) => {
    console.log(messageType, args);
    if (dispatcher[messageType]) {
      dispatcher[messageType](args);
    } else {
      console.warn(`Unknown message type, ${messageType}`);
    }
  });
}