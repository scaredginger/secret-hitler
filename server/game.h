#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <memory>
#include <tuple>
#include <random>

#include "common.h"
#include "communicationManager.h"
#include "player.h"

#ifndef SERVER_GAME_H
#define SERVER_GAME_H

template <GameType, int>
struct PowerParams;

/** The main game
 * @tparam gameType: the number of players in the game
 * @tparam CommunicationManager: how we send results (useful for testing)
 *
 * The class is a finite state machine, with transitions caused by calling public methods
 * The pattern is that the CommunicationManager will call a public method, and
 * 		the class will transition, then run a callback method on the communication manager
 * If the arguments given to a method are invalid, a transition won't occur
 * Otherwise, this class trusts its input. It doesn't check if it's in the correct state, nothing is bounds checked,
 * 		and it doesn't check if inputs come from the correct players. This should be done by the CommunicationManager
 *
 * The deck is a bitset. Cards are accessed by bitshifting.
 * The end of the deck is marked with a set bit, and all bits above it are zero.
 * Therefore, if we shift down and have no set bits, we must reshuffle.
 */
template <GameType gameType, typename CommunicationManager>
class Game {
public:
	enum State {
		AWAITING_CHANCELLOR_NOMINATION,
		VOTING,
		AWAITING_PRESIDENT_POLICY,
		AWAITING_CHANCELLOR_POLICY,
		AWAITING_CHANCELLOR_POLICY_NO_VETO,
		AWAITING_ALLEGIENCE_PEEK_CHOICE,
		AWAITING_SPECIAL_PRESIDENT_CHOICE,
		AWAITING_KILL_CHOICE,
		AWAITING_VETO,
		LIBERAL_POLICY_WIN,
		LIBERAL_HITLER_WIN,
		FASCIST_POLICY_WIN,
		FASCIST_HITLER_WIN,
	};

	enum PolicyChoice {
		FIRST = 0,
		SECOND = 1,
		THIRD = 2,
		VETO = 2,
	};

static constexpr int noPlayers = getNoPlayers<gameType>();
static constexpr int totalLiberalPolicies = 6;
static constexpr int totalFascistPolicies = 11;
static constexpr int policyCount = totalFascistPolicies + totalLiberalPolicies;

private:
	CommunicationManager &comms;
	std::array<std::unique_ptr<Player>, noPlayers> players;
	std::minstd_rand0 rng;
	std::bitset<policyCount> deck;

	State state = AWAITING_CHANCELLOR_NOMINATION;
	int liberalPolicies = 0;
	int fascistPolicies = 0;

	int hitler = -1;
	int presidentCounter;
	int presidentId;
	int chancellorId;

	int electionTracker = 0;
	bool specialElection = false;

	int previousPresidentId = -1;
	int previousChancellorId = -1;

	Team firstPolicy;
	Team secondPolicy;
	Team thirdPolicy;


private:
	void init() {
		shuffleDeck();
		auto playerSelector = std::uniform_int_distribution(0, noPlayers - 1);
		presidentId = playerSelector(rng);
		presidentCounter = presidentId;
		hitler = playerSelector(rng);
	}

public:
	Game(CommunicationManager &comms) : comms(comms) {
		using namespace std::chrono;
		auto t = high_resolution_clock::now().time_since_epoch();
		auto value = duration_cast<duration<unsigned long long>>(t).count();
		rng.seed(value);
		init();
	}

	Game(CommunicationManager &comms, unsigned long long seed) : comms(comms) {
		rng.seed(seed);
		init();
	}

private:
	void shuffleDeck() {
		deck.reset();
		const int noPolicies = policyCount - liberalPolicies - fascistPolicies;
		int i;
		int j = totalLiberalPolicies - liberalPolicies;
		for (i = 0; j && i < noPolicies; i++) {
			if (std::uniform_int_distribution(j, noPolicies - 1)(rng) < j) {
				j--;
				deck[i] = LIBERAL;
			}
		}
		for (i = 0; i < noPolicies; i++) {
			deck[i] = FASCIST;
		}
		deck[noPolicies] = 1;
	}

	[[nodiscard]] Team servePolicy() {
		Team p = deck[0];
		deck >>= 1;
	}

	void reshuffleIfNecessary() {
		if (!(deck >> 3).any()) {
			shuffleDeck();
		}
	}

	std::tuple<Team, Team, Team> peekThreeCards() {
		return { deck[0], deck[1], deck[2] };
	}

public:
	void addVote(int playerId, Vote v) {
		Player &player = players[playerId];
		if (player.hasVoted() || !player.alive()) {
			return;
		}
		if (v == JA) {
			player.lastVote(JA);
		} else if (v == NEIN) {
			player.lastVote(NEIN);
		} else {
			return;
		}
		player.hasVoted(true);
		runElectionIfAllHaveVoted();
	}

private:
	void runElectionIfAllHaveVoted() {
		int jaVotes = 0;
		int neinVotes = 0;
		for (auto &p : players) {
			if (!p.alive()) {
				continue;
			}
			if (!p.voted()) {
				return;
			}
			if (p.lastVote() == JA) {
				jaVotes++;
			} else {
				neinVotes++;
			}
		}
		runElection(jaVotes, neinVotes);
	}

	void checkForFascistHitlerWin() {
		return fascistPolicies >= 3 && chancellorId == hitler;
	}

	void runElection(int jaVotes, int neinVotes) {
		if (jaVotes > neinVotes) {
			successfulElection();
		} else {
			failedGovernment();
		}
	}

	void successfulElection() {
		if (checkForFascistHitlerWin()) {
			return fascistHitlerWin();
		}
		previousPresidentId = presidentId;
		previousChancellorId = chancellorId;
		sendPresidentPolicy();
	}

	void failedGovernment() {
		electionTracker++;
		if (electionTracker == 3) {
			auto p = servePolicy();
			if (p == FASCIST) {
				fascistPolicies++;
				if (fascistPolicies == 6) {
					return fascistPolicyWin();
				}
			} else {
				liberalPolicies++;
				if (liberalPolicies == 5) {
					return liberalPolicyWin();
				}
			}
			electionTracker = 0;
			reshuffleIfNecessary();
			previousPresidentId = -1;
			previousChancellorId = -1;
		}
		moveToNextPresident();
	}

	void sendPresidentPolicy() {
		firstPolicy = servePolicy();
		secondPolicy = servePolicy();
		thirdPolicy = servePolicy();
		state = AWAITING_PRESIDENT_POLICY;
		comms.sendPresidentPolicyChoice();
	}

public:
	void enactPresidentPolicy(PolicyChoice p) {
		using std::swap;
		switch (p) {
			case FIRST:
				swap(firstPolicy, thirdPolicy);
				break;
			case SECOND:
				swap(secondPolicy, thirdPolicy);
				break;
			case THIRD:
				break;
			default:
				break;
		}
		sendChancellorPolicy();
	}

private:
	void sendChancellorPolicy() {
		state = AWAITING_CHANCELLOR_POLICY;
		comms.sendChancellorPolicyChoice();
	}

public:
	void enactChancellorPolicy(PolicyChoice p) {
		switch(p) {
			case FIRST:
				enactPolicy(firstPolicy);
				break;
			case SECOND:
				enactPolicy(secondPolicy);
				break;
			case VETO:
				requestVeto();
			default:
				break;
		}
	}

private:
	void enactPolicy(Team p) {
		electionTracker = 0;
		if (p == FASCIST) {
			fascistPolicies++;
			switch (fascistPolicies) {
				case 1:
					return power0();
				case 2:
					return power1();
				case 3:
					return power2();
				case 4:
					return power3();
				case 5:
					return power4();
				case 6:
					return fascistPolicyWin();
				default:
					return;
			}
		} else {
			liberalPolicies++;
			if (liberalPolicies == 5) {
				return liberalPolicyWin();
			}
			moveToNextPresident();
		}
	}

	void requestVeto() {
		if (fascistPolicies != 5) {
			return;
		}
		state = AWAITING_VETO;
		comms.sendPresidentVetoOption();
	}

public:
	void presidentVeto(bool accept) {
		if (!accept) {
			state = AWAITING_CHANCELLOR_POLICY_NO_VETO;
			return comms.sendChancellorPolicyChoice();
		}
		failedGovernment();
	}

private:
	void liberalPolicyWin() {
		state = LIBERAL_POLICY_WIN;
		comms.liberalPolicyWin();
	}

	void liberalHitlerWin() {
		state = LIBERAL_HITLER_WIN;
		comms.liberalHitlerWin();
	}

	void fascistHitlerWin() {
		state = FASCIST_HITLER_WIN;
		comms.fascistHitlerWin();
	}

	void fascistPolicyWin() {
		state = FASCIST_POLICY_WIN;
		comms.fascistPolicyWin();
	}

	void moveToNextPresident() {
		if (specialElection) {
			specialElection = false;
		} else {
			presidentCounter = (presidentCounter + 1) % noPlayers;
		}
		while (!players[presidentCounter].alive()) {
			presidentCounter = (presidentCounter + 1) % noPlayers;
		}
		presidentId = presidentCounter;
		state = AWAITING_CHANCELLOR_NOMINATION;
		comms.requestChancellorNomination();
	}

	bool chancellorIsValid(int id) {
		if (!players[id].alive() || previousChancellorId == id) {
			return false;
		}
		if (id != previousPresidentId) {
			return true;
		}
		int alivePlayers = std::count_if(players.begin(), players.end(), [](Player p) { return p.alive(); });
		return alivePlayers <= 5;
	}

public:
	void nominateChancellor(int id) {
		if (!chancellorIsValid(id)) {
			return;
		}
		chancellorId = id;
		startVoting();
	}

private:
	void startVoting() {
		for (auto &p : players) {
			p.hasVoted(false);
		}
		state = VOTING;
		comms.announceVote();
	}

	void nullPower() {
		moveToNextPresident();
	}

	void investigateLoyalty() {
		state = AWAITING_ALLEGIENCE_PEEK_CHOICE;
		comms.requestInvestigation();
	}

public:
	void revealLoyalty(int playerId) {
		if (players[playerId].investigated() || playerId == presidentId || !players[playerId].alive()) {
			return;
		}
		players[playerId].investigated(true);
		comms.sendLoyalty(playerId, players[playerId].loyalty());
		moveToNextPresident();
	}

private:
	void showPresidentTopCards() {
		comms.sendTopCards();
		moveToNextPresident();
	}

	void runSpecialElection() {
		state = AWAITING_SPECIAL_PRESIDENT_CHOICE;
		comms.requestSpecialPresidentNomination();
	}

public:
	void useSpecialPresident(int id) {
		if (id == presidentId || !players[id].alive()) {
			return;
		}
		specialElection = true;
		presidentId = id;
		state = AWAITING_CHANCELLOR_NOMINATION;
		comms.requestChancellorNomination();
	}

private:
	void makePresidentKillPlayer() {
		state = AWAITING_KILL_CHOICE;
		comms.requestKill();
	}

public:
	void killPlayer(int id) {
		if (hitler == id) {
			return liberalHitlerWin();
		}
		if (!players[id].alive()) {
			return;
		}
		players[id].alive(false);
		moveToNextPresident();
	}

private:
	void power0() {
		switch(noPlayers) {
			case FIVE:
			case SIX:
			case SEVEN:
			case EIGHT:
				nullPower();
				break;
			case NINE:
			case TEN:
				investigateLoyalty();
				break;
			default:
				break;
		}
	}

	void power1() {
		switch(noPlayers) {
			case FIVE:
			case SIX:
				nullPower();
				break;
			case SEVEN:
			case EIGHT:
			case NINE:
			case TEN:
				investigateLoyalty();
				break;
			default:
				break;
		}
	}

	void power2() {
		switch(noPlayers) {
			case FIVE:
			case SIX:
				showPresidentTopCards();
				break;
			case SEVEN:
			case EIGHT:
			case NINE:
			case TEN:
				runSpecialElection();
				break;
			default:
				break;
		}
	}

	void power3() {
		makePresidentKillPlayer();
	}

	void power4() {
		makePresidentKillPlayer();
	}
};

#endif //SERVER_GAME_H
