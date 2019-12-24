#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <memory>
#include <tuple>
#include <random>

#include "common.h"
#include "manager.h"
#include "player.h"

/** The main game
 * @tparam gameType: the number of players in the game
 * @tparam CommunicationManager: how we send results (useful for testing)
 *
 * The class is a finite state machine, with transitions caused by calling public methods
 * The pattern is that the CommunicationManager will call a public method, and
 * 		the class will transition, then run a callback method on the communication comms
 * If the arguments given to a method are invalid, a transition won't occur
 * Otherwise, this class trusts its input. It doesn't check if it's in the correct state, nothing is bounds checked,
 * 		and it doesn't check if inputs come from the correct players. This should be done by the CommunicationManager
 *
 * The deck is a bitset. Cards are accessed by bitshifting.
 * The end of the deck is marked with a set bit, and all bits above it are zero.
 * Therefore, if we shift down and have no set bits, we must reshuffle.
 */
template <typename CommunicationManager>
class Game {
public:
	enum State {
		NOT_STARTED = 0,
		VOTING,
		AWAITING_CHANCELLOR_NOMINATION,
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

static constexpr int totalLiberalPolicies = 6;
static constexpr int totalFascistPolicies = 11;
static constexpr int policyCount = totalFascistPolicies + totalLiberalPolicies;

private:
	CommunicationManager &comms;
	std::array<Player, 10> players;
	std::minstd_rand0 rng;
	int playerCount = 0;
	std::bitset<policyCount + 1> deck;

	State state = NOT_STARTED;
	int liberalPolicies = 0;
	int fascistPolicies = 0;

	int hitler = -1;
	int presidentCounter = -1;
	int presidentId = -1;
	int chancellorId = -1;

	int electionTracker = 0;
	bool specialElection = false;

	int previousPresidentId = -1;
	int previousChancellorId = -1;

	Team firstPolicy = FASCIST;
	Team secondPolicy = FASCIST;
private:
	Team thirdPolicy = FASCIST;


private:
	void assignRoles() {
		auto playerSelector = std::uniform_int_distribution(0, playerCount - 1);
		hitler = playerSelector(rng);
		players[hitler].team(FASCIST);
		int remainingFascists; // not including Hitler
		switch(playerCount) {
			case FIVE:
			case SIX:
				remainingFascists = 1;
				break;
			case SEVEN:
			case EIGHT:
				remainingFascists = 2;
				break;
			case NINE:
			case TEN:
				remainingFascists = 3;
				break;
		}
		for (auto i = 0; i < hitler; i++) {
			if (std::uniform_int_distribution(0, playerCount - i - 1)(rng) < remainingFascists) {
				remainingFascists--;
				players[i].team(FASCIST);
			} else {
				players[i].team(LIBERAL);
			}
		}
		for (auto i = hitler + 1; i < playerCount; i++) {
			if (std::uniform_int_distribution(0, playerCount - i - 1)(rng) < remainingFascists) {
				remainingFascists--;
				players[i].team(FASCIST);
			} else {
				players[i].team(LIBERAL);
			}
		}
	}

public:
	explicit Game(CommunicationManager &comms) : comms(comms) {
	}

	void init(unsigned long long seed) {
		playerCount = comms.getClientCount();
		rng.seed(seed);
		shuffleDeck();
		auto playerSelector = std::uniform_int_distribution(0, playerCount - 1);
		presidentId = playerSelector(rng);
		presidentCounter = presidentId;
		assignRoles();
		for (auto i = 0; i < playerCount; i++) {
            players[i].alive(true);
		}
	}

	void start() {
		moveToNextPresident();
	}

	void init() {
		using namespace std::chrono;
		auto t = high_resolution_clock::now().time_since_epoch();
		auto seed = duration_cast<duration<unsigned long long>>(t).count();
		init(seed);
	}

private:
	void shuffleDeck() {
		deck.reset();
		const int remainingPolicies = policyCount - liberalPolicies - fascistPolicies;
		int i;
		int j = totalLiberalPolicies - liberalPolicies;
		for (i = 0; j && i < remainingPolicies; i++) {
			if (std::uniform_int_distribution(0, remainingPolicies - i - 1)(rng) < j) {
				j--;
				deck[i] = LIBERAL;
			} else {
				deck[i] = FASCIST;
			}
		}
		for (; i < remainingPolicies; i++) {
			deck[i] = FASCIST;
		}
		deck[remainingPolicies] = 1;
	}

	[[nodiscard]] Team servePolicy() {
		bool isSet = deck[0];
		deck >>= 1;
		Team p = static_cast<Team>(static_cast<int>(isSet));
		return p;
	}

	void reshuffleIfNecessary() {
		if (!(deck >> 3).any()) {
			shuffleDeck();
		}
	}

public:
	std::tuple<Team, Team, Team> peekTopCards() {
		return { Team(bool(deck[0])), Team(bool(deck[1])), Team(bool(deck[2])) };
	}

	void addVote(int playerId, Vote v) {
		Player &player = players[playerId];
		if (player.voted() || !player.alive()) {
			return;
		}
		if (v == JA) {
			player.lastVote(JA);
		} else if (v == NEIN) {
			player.lastVote(NEIN);
		} else {
			return;
		}
		player.voted(true);
		runElectionIfAllHaveVoted();
	}

private:
	void runElectionIfAllHaveVoted() {
		int jaVotes = 0;
		int neinVotes = 0;
		for (int i = 0; i < playerCount; i++) {
			auto &p = players[i];
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

	[[nodiscard]] bool checkForFascistHitlerWin() {
		return fascistPolicies >= 3 && chancellorId == hitler;
	}

	void runElection(int jaVotes, int neinVotes) {
		if (jaVotes > neinVotes) {
			comms.successfulElection();
			successfulElection();
		} else {
			comms.failedElection();
			incrementElectionTracker();
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

	void incrementElectionTracker() {
		electionTracker++;
		if (electionTracker == 3) {
			auto p = servePolicy();
			if (p == FASCIST) {
				fascistPolicies++;
				comms.chaoticFascistPolicy();
				if (fascistPolicies == 6) {
					return fascistPolicyWin();
				}
			} else {
				liberalPolicies++;
				comms.chaoticLiberalPolicy();
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
	void removePresidentPolicy(PolicyChoice removedPolicy) {
		using std::swap;
		switch (removedPolicy) {
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
	void removeChancellorPolicy(PolicyChoice p) {
		switch(p) {
			case FIRST:
				enactPolicy(secondPolicy);
				break;
			case SECOND:
				enactPolicy(firstPolicy);
				break;
			case VETO:
				if (fascistPolicies == 5 && state == AWAITING_CHANCELLOR_POLICY)
					requestVeto();
				break;
			default:
				break;
		}
	}

private:
	void enactPolicy(Team p) {
		electionTracker = 0;
		if (p == FASCIST) {
			fascistPolicies++;
			comms.regularFascistPolicy();
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
			comms.regularLiberalPolicy();
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
		incrementElectionTracker();
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
			presidentCounter = (presidentCounter + 1) % playerCount;
		}
		while (!players[presidentCounter].alive()) {
			presidentCounter = (presidentCounter + 1) % playerCount;
		}
		presidentId = presidentCounter;
		state = AWAITING_CHANCELLOR_NOMINATION;
		comms.requestChancellorNomination();
	}

	[[nodiscard]] bool chancellorIsValid(int id) {
		if (!players[id].alive() || previousChancellorId == id || id == presidentId) {
			return false;
		}
		if (id != previousPresidentId) {
			return true;
		}
		int alivePlayers = std::count_if(players.begin(), players.begin() + playerCount, [](Player p) { return p.alive(); });
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
		for (auto i = 0; i < playerCount; i++) {
			players[i].voted(false);
		}
		state = VOTING;
		comms.announceElection();
	}

	void nullPower() {
		moveToNextPresident();
	}

	void investigateLoyalty() {
		state = AWAITING_ALLEGIENCE_PEEK_CHOICE;
		comms.requestInvestigation();
	}

	bool canBeInvestigated(int playerId) {
		return !players[playerId].investigated() && playerId != presidentId && players[playerId].alive();
	}
public:
	void revealLoyalty(int playerId) {
		if (!canBeInvestigated(playerId)) {
			return;
		}
		players[playerId].investigated(true);
		comms.sendLoyalty(playerId, players[playerId].team());
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
		switch(playerCount) {
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
		switch(playerCount) {
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
		switch(playerCount) {
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

public:
	State getState() const {
		return state;
	}

	int getLiberalPolicies() const {
		return liberalPolicies;
	}

	int getFascistPolicies() const {
		return fascistPolicies;
	}

	int getPresidentCounter() const {
		return presidentCounter;
	}

	int getPresidentId() const {
		return presidentId;
	}

	int getChancellorId() const {
		return chancellorId;
	}

	int getElectionTracker() const {
		return electionTracker;
	}

	int getPreviousPresidentId() const {
		return previousPresidentId;
	}

	int getPreviousChancellorId() const {
		return previousChancellorId;
	}

	int getHitler() const {
		return hitler;
	}

	Team getFirstPolicy() const {
		return firstPolicy;
	}

	Team getSecondPolicy() const {
		return secondPolicy;
	}

	Team getThirdPolicy() const {
		return thirdPolicy;
	}

	// TODO: rename this
	std::bitset<10> getEligibleChancellors() {
		std::bitset<10> result(0);
		for (auto i = 0; i < playerCount; i++) {
			result[i] = chancellorIsValid(i);
		}
		return result;
	}

	std::bitset<10> eligibleForInvestigation() {
		std::bitset<10> result(0);
		for (auto i = 0; i < playerCount; i++) {
			result[i] = canBeInvestigated(i);
		}
		return result;
	}

	std::bitset<10> alive() {
		std::bitset<10> result(0);
		for (auto i = 0; i < playerCount; i++) {
			result[i] = players[i].alive();
		}
		return result;
	}

	std::bitset<10> getBallot() {
		std::bitset<10> result(0);
		for (auto i = 0; i < playerCount; i++) {
			result[i] = players[i].lastVote();
		}
		return result;
	}

	std::bitset<10> getTeams() {
		std::bitset<10> result(0);
		for (auto i = 0; i < playerCount; i++) {
			result[i] = players[i].team();
		}
		return result;
	}
};

#endif //SERVER_GAME_H
