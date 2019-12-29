import { readable } from 'svelte/store';

let setQueue = a => a;
let queue = [];

export const messageStore = readable(queue, (set) => {
  setQueue = set;
});

function push(a) {
  queue = queue.concat([a]);
  console.log(queue);
  setQueue(queue);
}

export function popMessage() {
  queue = queue.slice(1);
  setQueue(queue);
}

export function subscribe(client) {
  console.log(client);
  const handlers = makeHandlers(client);
  client.subscribe((code, args) => {
    console.log('ms', code)
    if (handlers[code]) {
      handlers[code](args);
    }
  });
}

const makeHandlers = (client) => ({
  election({ president, chancellor }) {
    if (client.selfDead) {
      return;
    }
    push({
      type: 'decision',
      title: 'Election',
      info: `${president} and ${chancellor} are up for election, as president and chancellor, respectively.`,
      handler: (a) => client.vote(a),
      options: [
        {
          text: 'JA',
          eligible: true,
          id: 'ja',
        },
        {
          text: 'NEIN',
          eligible: true,
          id: 'nein',
        },
      ]
    });
  },
  requestPresidentPolicyElimination({ policies }) {
    const options = policies.map((text, i) => ({
      text: text.toUpperCase(),
      id: i,
      eligible: true,
    }));
    push({
      type: 'secretDecision',
      title: 'Eliminate policy',
      info: 'Select a policy to ELIMINATE',
      handler: a => client.eliminatePolicy(a),
      options,
    });
  },
  requestChancellorPolicyElimination({ policies, canVeto }) {
    const options = policies.map((text, i) => ({
      text: text.toUpperCase(),
      id: i,
      eligible: true,
    }));
    options.push({
      text: 'VETO',
      id: 'veto',
      eligible: canVeto,
    });
    let info = 'Select a policy to ELIMINATE.';
    if (canVeto) {
      info += ' You may also veto if you wish to discard both policies.';
    }
    push({
      type: 'secretDecision',
      title: 'Eliminate policy',
      info,
      handler: a => client.eliminatePolicy(a),
      options,
    });
  },
  requestChancellorNomination({ options }) {
    push({
      type: 'decision',
      title: 'Chancellor Nomination',
      info: 'Choose a player to nominate as Chancellor.',
      handler: a => client.nominateChancellor(a),
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  chancellorNomination({ president, options }) {
    push({
      type: 'decisionAlert',
      title: 'Chancellor Nomination',
      info: `President ${president} is nominating their chancellor.`,
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  requestInvestigation({ options }) {
    push({
      type: 'decision',
      title: 'Investigation',
      info: 'Choose a player to reveal their identity. Only you will be shown; telling the truth is optional.',
      handler: a => client.revealPlayer(a),
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  investigation({ president, options }) {
    push({
      type: 'decisionAlert',
      title: 'Investigation',
      info: `President ${president} is investigating a player. They will know that player's team.`,
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  requestSpecialNomination({ options }) {
    push({
      type: 'decision',
      title: 'Special Election',
      info: 'Choose a player to be the next president.',
      handler: a => client.specialNomination(a),
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  specialNomination({ options }) {
    push({
      type: 'decisionAlert',
      title: 'Investigation',
      info: `President ${president} is choosing the next president.`,
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  requestKill({ options }) {
    push({
      type: 'decision',
      title: 'Kill Somebody',
      info: `Choose somebody to die. If they are Hitler, the Liberals win.`,
      handler: a => client.killPlayer(a),
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  kill({ president, options }) {
    push({
      type: 'decisionAlert',
      title: 'Kill',
      info: `President ${president} is killing a player. If that player is Hitler, the Liberals win.`,
      options: options.map(({ eligible, id, player }) => ({ eligible, id, text: player })),
    });
  },
  revealTeam({ player, team }) {
    if (player[player.length - 1].toLowerCase() === 's') {
      player += "'";
    } else {
      player += "'s";
    }
    push({
      type: 'secret',
      title: 'Investigation Result',
      info: `Find out ${player} team.`,
      text: team,
      handler: a => client.eliminatePolicy(a),
    });
  },
  presidentRevealTeam({ president, player }) {
    if (player[player.length - 1].toLowerCase() === 's') {
      player += "'";
    } else {
      player += "'s";
    }
    push({
      type: 'text',
      title: 'Investigation Result',
      info: '',
      text: `President ${president} knows ${player} identity.`,
      handler: a => client.eliminatePolicy(a),
    });
  },
  revealPolicies({ policies }) {
    policies = policies.map(p => p === 'fascist' ? 'Fascist' : 'Liberal');
    push({
      type: 'secret',
      title: 'Policy reveal',
      info: 'Reveal the first three policies in the deck.',
      text: `${policies[0]}, ${policies[1]} and ${policies[2]}`,
      handler: a => client.eliminatePolicy(a),
    });
  },
  presidentRevealPolicies({ president }) {
    push({
      type: 'text',
      title: 'Policy reveal',
      info: '',
      text: `President ${president} knows the first three policies in the deck.`,
      handler: a => client.eliminatePolicy(a),
    });
  },
  electionComplete({ votes, success }) {
    push({
      type: 'electionResult',
      title: success ? 'Successful election' : 'Failed election',
      votes, 
    })
  },
  disconnect({ player }) {
    push({
      type: 'text',
      title: 'Policy reveal',
      info: '',
      text: `${player} has disconnected. You will not be able to complete this game.`,
      handler: a => client.eliminatePolicy(a),
    });
  },
  team({ role, roles }) {
    queue = [];
    push({
      type: 'team',
      role,
      roles,
    });
  },
  rename({ oldName, newName }) {
    push({
      type: 'text',
      title: 'Renaming',
      info: '',
      text: `${oldName} has renamed themselves to ${newName}`,
    });
  },
  dead({ president }) {
    text = client.role === 'hitler' ? '' : 'You will no longer affect this game. Remember, the dead don\'t speak.';
    push({
      type: 'text',
      title: 'You Are Dead',
      info: `President ${president} killed you.`,
      text,
    });
  },
  death({ president, player }) {
    push({
      type: 'text',
      title: 'Death',
      info: '',
      text: `President ${president} killed ${player}.`,
    });
  },
  requestVeto({}) {
    const options = [
      {
        text: 'JA',
        id: 'ja',
      },
      {
        text: 'NEIN',
        id: 'nein',
      },
    ];
    push({
      type: 'decision',
      title: 'Veto',
      info: 'Accept the veto to discard the policies you were given.',
      handler: (a) => client.respondToVeto(a),
      options,
    });
  },
  liberalPolicyWin({}) {
    push({
      type: 'text',
      title: 'Liberal Victory',
      info: '',
      text: 'The Liberals won by passing five policies.'
    });
  },
  liberalHitlerWin({}) {
    push({
      type: 'text',
      title: 'Liberal Victory',
      info: '',
      text: 'The Liberals won by killing Hitler.'
    });
  },
  fascistPolicyWin({}) {
    push({
      type: 'text',
      title: 'Fascist Victory',
      info: '',
      text: 'The Fascists won by passing six policies.'
    });
  },
  fascistHitlerWin({}) {
    push({
      type: 'text',
      title: 'Fascist Victory',
      info: '',
      text: 'The Fascists won by electing Hitler as chancellor.'
    });
  },
  reshuffle({ policiesInDeck }) {
    push({
      type: 'text',
      title: 'Reshuffle',
      info: '',
      text: `The deck has been reshuffled. There are now ${policiesInDeck} policies left.`,
    });
  },
  regularFascistPolicy({ president, chancellor }) {
    push({
      type: 'text',
      title: 'Fascist Policy Enacted',
      info: '',
      text: `President ${president} and Chancellor ${chancellor} worked together to enact a Fascist policy.`,
    });
  },
  chaoticFascistPolicy({}) {
    push({
      type: 'text',
      title: 'Fascist Policy Enacted',
      info: '',
      text: 'The country has descended into chaos, and the first policy in the deck was played.',
    });
  },
  regularLiberalPolicy({ president, chancellor }) {
    push({
      type: 'text',
      title: 'Liberal Policy Enacted',
      info: '',
      text: `President ${president} and Chancellor ${chancellor} worked together to enact a Liberal policy.`,
    });
  },
  chaoticLiberalPolicy({}) {
    push({
      type: 'text',
      title: 'Liberal Policy Enacted',
      info: '',
      text: 'The country has descended into chaos, and the first policy in the deck was played.',
    });
  },
  successfulVeto({ president, chancellor }) {
    if (client.playerIsPresident) {
      return;
    }
    if (client.playerIsChancellor) {
      push({
        type: 'text',
        title: 'Veto Accepted',
        info: '',
        text: `President ${president} accepted your veto request.`,
      });
    } else {
      push({
        type: 'text',
        title: 'Veto',
        info: '',
        text: `President ${president} and Chancellor ${chancellor} vetoed their policies.`,
      });
    }
  },
  failedVeto({ president, chancellor }) {
    if (client.playerIsPresident) {
      return;
    }
    chancellor += chancellor[chancellor.length - 1] === 's' ? "'" : "'s";
    if (client.playerIsChancellor) {
      push({
        type: 'text',
        title: 'Veto Denied',
        info: '',
        text: `President ${president} denied your veto request.`,
      });
    } else {
      push({
        type: 'text',
        title: 'Veto',
        info: '',
        text: `President ${president} denied Chancellor ${chancellor} veto request.`,
      });
    }
  },
});
