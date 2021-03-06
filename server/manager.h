#ifndef SERVER_COMMUNICATION_MANAGER_H
#define SERVER_COMMUNICATION_MANAGER_H
#include <cinttypes>
#include <list>
#include <algorithm>
#include <App.h> // uWebSockets
#include "common.h"
#include "game.h"

// using WebSocket = uWS::WebSocket<true, true>;
using WebSocket = uWS::WebSocket<false, true>;

struct Message {
	int length;
	char data[256];
	Message() = default;
};

class Manager;
struct UserData {
	WebSocket *socket;
	Manager *manager;
	std::list<Message> queue;
	int playerId;
};

class Client {
private:
	WebSocket *socket = nullptr;
	std::string name;

	WebSocket *getSocket() {
		// we are using the lower 2 bits of socket for flags
		return reinterpret_cast<WebSocket *>(reinterpret_cast<uintptr_t>(socket) & ~static_cast<uintptr_t>(3));
	}

public:
	Client() = default;

	explicit Client(WebSocket *ws) : socket(ws) {}

	Client(Client &&other) : name(std::move(other.name)) {
		auto s = getSocket();
		if (s) {
			s->end(4000);
		}
		socket = other.socket;
		other.socket = nullptr;
	}

	Client &operator=(Client &&other) {
		name = std::move(other.name);
		auto s = getSocket();
		if (s) {
			s->end(4000);
		}
		socket = other.socket;
		other.socket = nullptr;
		return *this;
	}

	~Client() {
		auto s = getSocket();
		if (s) {
			s->end(4000);
			socket = nullptr;
		}
	}

	bool ready() {
		return (reinterpret_cast<uintptr_t>(socket) & 1) == 1;
	}

	void ready(bool value) {
		socket = reinterpret_cast<WebSocket *>(reinterpret_cast<uintptr_t>(socket) | value);
	}

	bool voted() {
		return (reinterpret_cast<uintptr_t>(socket) & 1) == 1;
	}

	void voted(bool value) {
		socket = reinterpret_cast<WebSocket *>((reinterpret_cast<uintptr_t>(socket) & ~1) | value);
	}

	bool connected() {
		return getSocket() != nullptr;
	}

	void safeSend(std::string_view view) {
		auto s = getSocket();
		if (s) {
			send(view);
		}
	}

	void safeSend(char *buf, int i) {
		std::string_view view(buf, i);
		safeSend(view);
	}

	void send(std::string_view view) {
		auto *s = getSocket();
		auto data = reinterpret_cast<UserData *>(s->getUserData());
		auto &q = data->queue;
		while (!q.empty()) {
			if (s->send(std::string_view(q.front().data, q.front().length))) {
				q.pop_front();
			} else {
				break;
			}
		}
		if (q.empty()) {
			if (s->send(view, uWS::OpCode::BINARY, true)) {
			} else {
				q.emplace_back();
				auto &x = q.back();
				x.length = view.copy(x.data, 256);
			}
		} else {
			q.emplace_back();
			auto &x = q.back();
			x.length = view.copy(x.data, 256);
		}
	}

	void send(char *buf, int i) {
		std::string_view view(buf, i);
		socket->send(view, uWS::OpCode::BINARY, true);
	}

	void setName(const std::string_view newName) {
		name = newName;
	}

	void setId(int id) {
		static_cast<UserData *>(getSocket()->getUserData())->playerId = id;
	}

	const auto &getName() const {
		return name;
	}

	void onDisconnect() {
		socket = nullptr;
	}

	void safeUncleanEnd() {
		auto s = getSocket();
		if (s) {
			s->end(4001);
			socket = nullptr;
		}
	}
};

class Manager {
private:
	using Game = GenericGame<Manager>;

	Game game;
	std::array<Client, 10> clients;
	std::function<void()> deleter;
	int clientCount = 0;
	char sendBuffer[256];

	static constexpr int MAX_NAME_SIZE = sizeof(sendBuffer) - 1;

public:
	Manager() : game(*this) {
	}

	void setDeleter(std::function<void()> f) {
		deleter = f;
	}

	int addClient(WebSocket *ws) {
		if (clientCount >= 10) {
			return -1;
		}
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		int i;
		for (i = 0; i < 10 && clients[i].connected(); i++) {}
		ptr[0] = i;
		ws->send(std::string_view(sendBuffer, 1));
		for (int j = 0; j < 10; j++) {
			auto &c = clients[j];
			if (c.connected()) {
				ptr[0] = (j << 4) | NAME;
				c.getName().copy(sendBuffer + 1, sizeof(sendBuffer) - 1);
				ws->send(std::string_view(sendBuffer, c.getName().length() + 1));
			}
		}
		clientCount++;
		clients[i] = Client(ws);
		return i;
	}

	void onDisconnect(int id, int code) {
		if (code >= 4000) return;
		clients[id].onDisconnect();
		switch(game.getState()) {
			case Game::NOT_STARTED:
				announceDisconnect(id);
				break;
			case Game::LIBERAL_POLICY_WIN:
			case Game::LIBERAL_HITLER_WIN:
			case Game::FASCIST_POLICY_WIN:
			case Game::FASCIST_HITLER_WIN:
				break;
			default:
				announceDisconnect(id);
				return destroyGame();
		}
		clientCount--;
		if (clientCount == 0) {
			return destroyGameClean();
		}
	}

	void announceDisconnect(int id) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = DISCONNECT | (id << 4);
		safeBroadcast(std::string_view(sendBuffer, 1));
	}

	void destroyGame() {
		for (auto &c : clients) {
			c.safeUncleanEnd();
		}
		deleter();
	}

	void destroyGameClean() {
		deleter();
	}

private:
	void tryToStartGame() {
		if (clientCount < 5) {
			return;
		}
		for (auto &client : clients) {
			if (client.connected() && !client.ready()) {
				return;
			}
		}
		startGame();
	}

	void startGame() {
		removeNulls();
		for (auto &c : clients) {
			c.voted(false);
		}
		game.init();
		sendTeams();
		game.start();
	}

	void sendTeams() {
		std::string_view libMessage(sendBuffer, 1);
		std::string_view fascMessage(sendBuffer + 1, 2);
		auto hitler = game.getHitler();
		auto teams = game.getTeams();
		auto teamFlags = teams.to_ulong();
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = TEAM;
		int fasc = -1;

		std::bitset<10> mask(0x3ff);
		teams.flip();
		unsigned hitlerNumber = (teams.count() - (teams >> hitler).count()) & 3;
		ptr[1] = TEAM | (hitlerNumber << 4) | ((teamFlags & 3) << 6);
		ptr[2] = (teamFlags >> 2) & 255;

		for (auto i = 0; i < hitler; i++) {
			if (teams[i]) {
				clients[i].send(fascMessage);
				fasc = i;
			} else {
				clients[i].send(libMessage);
			}
		}
		for (auto i = hitler + 1; i < clientCount; i++) {
			if (teams[i]) {
				clients[i].send(fascMessage);
				fasc = i;
			} else {
				clients[i].send(libMessage);
			}
		}
		switch (clientCount) {
			case FIVE:
			case SIX:
				ptr[0] = TEAM | ((fasc + 1) << 4);
				clients[hitler].send(std::string_view(sendBuffer, 1));
				break;
			case SEVEN:
			case EIGHT:
			case NINE:
			case TEN:
				ptr[0] = TEAM | (15 << 4);
				clients[hitler].send(std::string_view(sendBuffer, 1));
				break;
		}
	}

	// TODO: make sure this works (should probably write tests, too)
	void removeNulls() {
		int i = 0;
		int j = clients.size() - 1;
		while (i < j) {
			while (i < j && clients[i].connected()) {
				i++;
			}
			while (i < j && !clients[j].connected()) {
				j--;
			}
			if (i < j) {
				clients[i] = std::move(clients[j]);
				clients[i].setId(i);
				announceReassign(j, i);
			}
		}
	}

	void announceReassign(int oldId, int newId) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REASSIGN;
		ptr[1] = ((oldId & 15) << 4) | (newId & 15);
		std::string_view message(sendBuffer, 2);
		safeBroadcast(message);
	}

	public:
	void successfulElection() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		auto ballot = game.getBallot().to_ulong();
		ptr[0] = BALLOT | 16 | ((ballot & 3) << 6);
		ptr[1] = (ballot >> 2) & 255;
		std::string_view message(sendBuffer, 2);
		broadcast(message);
	}

	void announceDeath(int id) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = DEATH | ((id & 15) << 4);
		std::string_view message(sendBuffer, 1);
		broadcast(message);
	}

	void failedElection() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		auto ballot = game.getBallot().to_ulong();
		ptr[0] = BALLOT | ((ballot & 3) << 6);
		ptr[1] = (ballot >> 2) & 255;
		std::string_view message(sendBuffer, 2);
		broadcast(message);
	}

	private:
	void announceName(int id) {
		auto &c = clients[id];
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = (id << 4) | NAME;
		c.getName().copy(sendBuffer + 1, sizeof(sendBuffer) - 1);
		std::string_view message(sendBuffer, c.getName().size() + 1);
		for (auto i = 0; i < id; i++) {
			auto &k = clients[i];
			k.safeSend(message);
		}
		for (auto i = id + 1; i < 10; i++) {
			auto &k = clients[i];
			k.safeSend(message);
		}
	}

	void setName(int id, std::string_view name) {
		if (name.size() > MAX_NAME_SIZE) {
			return;
		}
		clients[id].setName(name);
		announceName(id);
	}

	void setReady(int id, bool value) {
		if (clients[id].ready() != value) {
			announceReadyState(id, value);
		}
		clients[id].ready(value);
		if (value) {
			tryToStartGame();
		}
	}

	void announceReadyState(int id, bool value) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = value ? READY_TO_START : NOT_READY;
		ptr[0] |= id << 4;
		std::string_view message(sendBuffer, 1);
		broadcast(message);
	}

	void eliminatePolicy(int id, int choice) {
		if (game.getState() == game.AWAITING_PRESIDENT_POLICY && id == game.getPresidentId()) {
			game.removePresidentPolicy(static_cast<typename Game::PolicyChoice>(choice));
		} else if (game.getState() == game.AWAITING_CHANCELLOR_POLICY && id == game.getChancellorId()) {
			game.removeChancellorPolicy(static_cast<typename Game::PolicyChoice>(choice));
		} else if (game.getState() == game.AWAITING_CHANCELLOR_POLICY_NO_VETO && id == game.getChancellorId()) {
			if (choice != game.VETO) {
				game.removeChancellorPolicy(static_cast<typename Game::PolicyChoice>(choice));
			}
		} else {
			return;
		}
	}

	bool presidentChoiceCheck(int id, int choice) {
		return choice >= 0 && choice < clientCount && id == game.getPresidentId();
	}

	void selectChancellor(int id, int choice) {
		if (!presidentChoiceCheck(id, choice)) return;
		if (game.getState() != game.AWAITING_CHANCELLOR_NOMINATION) {
			return;
		}
		game.nominateChancellor(choice);
	}

	void reveal(int id, int choice) {
		if (!presidentChoiceCheck(id, choice)) return;
		if (game.getState() != game.AWAITING_ALLEGIENCE_PEEK_CHOICE) {
			return;
		}
		game.revealLoyalty(choice);
	}

	void kill(int id, int choice) {
		if (!presidentChoiceCheck(id, choice)) return;
		if (game.getState() != game.AWAITING_KILL_CHOICE) {
			return;
		}
		game.killPlayer(choice);
	}

	void selectPresident(int id, int choice) {
		if (!presidentChoiceCheck(id, choice)) return;
		if (game.getState() != game.AWAITING_SPECIAL_PRESIDENT_CHOICE) {
			return;
		}
		game.useSpecialPresident(choice);
	}

	void castVote(int id, Vote vote) {
		if (clients[id].voted()) return;
		clients[id].voted(true);
		announceVoteReceived(id);
		game.addVote(id, vote);
	}

	void respondToVeto(int id, bool accept) {
		if (id != game.getPresidentId()) {
			return;
		}
		game.presidentVeto(accept);
	}

	void safeBroadcast(std::string_view msg) {
		for (auto &c : clients) {
			c.safeSend(msg);
		}
	}

	void broadcast(std::string_view msg) {
		for (auto i = 0; i < clientCount; i++) {
			clients[i].send(msg);
		}
	}

	void announceVoteReceived(int id) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = VOTE_RECEIVED | (id << 4);
		std::string_view message(sendBuffer, 1);
		broadcast(message);
	}

public:
	void sendGameKey(int id, uint32_t key) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = GAME_KEY;
		ptr[1] = (key >> 24) & 255;
		ptr[2] = (key >> 16) & 255;
		ptr[3] = (key >> 8) & 255;
		ptr[4] = key & 255;
		std::string_view message(sendBuffer, 5);
		clients[id].send(message);
	}

	enum MessageCode {
		ANNOUNCE_ELECTION = 0,
		REQUEST_PRESIDENT_POLICY_CHOICE,
		REQUEST_CHANCELLOR_POLICY_CHOICE,
		REQUEST_INVESTIGATION,
		REQUEST_KILL,
		SEND_LOYALTY,
		TOP_CARDS,
		VOTE_RECEIVED,
		BALLOT,
		DISCONNECT,
		READY_TO_START,
		NOT_READY,
		TEAM,
		NAME,
		DEATH,
		EXTENDED
	};

	static_assert(EXTENDED < 16);

	enum ExtendedMessageCodes {
		REQUEST_PRESIDENT_VETO = 0 * 16 | EXTENDED,
		LIBERAL_POLICY_WIN = 1 * 16 | EXTENDED,
		LIBERAL_HITLER_WIN = 2 * 16 | EXTENDED,
		FASCIST_POLICY_WIN = 3 * 16 | EXTENDED,
		FASCIST_HITLER_WIN = 4 * 16 | EXTENDED,
		REQUEST_SPECIAL_NOMINATION = 5 * 16 | EXTENDED,
		REASSIGN = 6 * 16 | EXTENDED,
		REGULAR_FASCIST_POLICY = 7 * 16 | EXTENDED,
		CHAOTIC_FASCIST_POLICY = 8 * 16 | EXTENDED,
		REGULAR_LIBERAL_POLICY = 9 * 16 | EXTENDED,
		CHAOTIC_LIBERAL_POLICY = 10 * 16 | EXTENDED,
		REQUEST_CHANCELLOR_NOMINATION = 11 * 16 | EXTENDED,
		GAME_KEY = 12 * 16 | EXTENDED
	};

	void announceElection() {
		for (auto &c : clients) {
			c.voted(false);
		}
		auto chancellor = game.getChancellorId();
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = ANNOUNCE_ELECTION | (chancellor << 4);
		broadcast(std::string_view(sendBuffer, 1));
	}

	void chaoticFascistPolicy() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = CHAOTIC_FASCIST_POLICY;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void regularFascistPolicy() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REGULAR_FASCIST_POLICY;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void chaoticLiberalPolicy() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = CHAOTIC_LIBERAL_POLICY;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void regularLiberalPolicy() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REGULAR_LIBERAL_POLICY;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void fascistHitlerWin() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = FASCIST_HITLER_WIN;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void fascistPolicyWin() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = FASCIST_POLICY_WIN;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void liberalPolicyWin() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = LIBERAL_POLICY_WIN;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void liberalHitlerWin() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = LIBERAL_HITLER_WIN;
		broadcast(std::string_view(sendBuffer, 1));
	}

	void requestChancellorNomination() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REQUEST_CHANCELLOR_NOMINATION;
		auto v = game.getEligibleChancellors().to_ulong();
		ptr[1] = game.getPresidentId() | ((v & 3) << 6);
		ptr[2] = (v >> 2) & 255;
		broadcast(std::string_view(sendBuffer, 3));
	}

	void sendPresidentPolicyChoice() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REQUEST_PRESIDENT_POLICY_CHOICE;
		std::string_view message(sendBuffer, 1);
		for (int i = 0; i < game.getPresidentId(); i++) {
			clients[i].send(message);
		}
		for (int i = game.getPresidentId() + 1; i < clientCount; i++) {
			clients[i].send(message);
		}
		ptr[0] |= (game.getFirstPolicy() << 5) | (game.getSecondPolicy() << 6) | (game.getThirdPolicy() << 7);
		clients[game.getPresidentId()].send(message);
	}

	void sendChancellorPolicyChoice() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REQUEST_CHANCELLOR_POLICY_CHOICE;
		std::string_view message(sendBuffer, 1);
		for (int i = 0; i < game.getChancellorId(); i++) {
			clients[i].send(message);
		}
		for (int i = game.getChancellorId() + 1; i < clientCount; i++) {
			clients[i].send(message);
		}
		bool canVeto = game.getState() == game.AWAITING_CHANCELLOR_POLICY && game.getFascistPolicies() == 5;
		ptr[0] |= (game.getFirstPolicy() << 5) | (game.getSecondPolicy() << 6) | (canVeto << 7);
		clients[game.getChancellorId()].send(message);
	}

	void sendPresidentVetoOption() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REQUEST_PRESIDENT_VETO;
		std::string_view message(sendBuffer, 1);
		broadcast(message);
	}

	void requestInvestigation() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		auto eligible = game.eligibleForInvestigation().to_ulong();
		ptr[0] = REQUEST_INVESTIGATION | ((eligible & 3) << 6);
		ptr[1] = (eligible >> 2) & 255;
		std::string_view message(sendBuffer, 2);
		broadcast(message);
	}

	void sendLoyalty(int id, Team team) {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = SEND_LOYALTY | (id << 4);
		std::string_view message(sendBuffer, 1);
		for (int i = 0; i < game.getPresidentId(); i++) {
			clients[i].send(message);
		}
		for (int i = game.getPresidentId() + 1; i < clientCount; i++) {
			clients[i].send(message);
		}
		ptr[1] = team;
		clients[game.getPresidentId()].send(std::string_view(sendBuffer, 2));
	}

	void sendTopCards() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = TOP_CARDS;
		std::string_view message(sendBuffer, 1);
		for (int i = 0; i < game.getPresidentId(); i++) {
			clients[i].send(message);
		}
		for (int i = game.getPresidentId() + 1; i < clientCount; i++) {
			clients[i].send(message);
		}
		auto [a, b, c] = game.peekTopCards();
		ptr[0] |= 16 | (a << 5) | (b << 6) | (c << 7);
		clients[game.getPresidentId()].send(message);
	}

	void requestSpecialPresidentNomination() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		ptr[0] = REQUEST_SPECIAL_NOMINATION;
		std::string_view message(sendBuffer, 1);
		broadcast(message);
	}

	void requestKill() {
		auto *ptr = reinterpret_cast<unsigned char *>(sendBuffer);
		auto alive = game.alive().to_ulong();
		ptr[0] = REQUEST_KILL | ((alive & 3) << 6);
		ptr[1] = (alive >> 2) & 255;
		std::string_view message(sendBuffer, 2);
		broadcast(message);
	}

public:
	void handleMessage(int id, std::string_view message) {
		if (message.size() < 1) {
			return;
		}
		unsigned firstByte = static_cast<unsigned char>(message.data()[0]);
		switch(firstByte % 8) {
			case 0:
				selectChancellor(id, firstByte / 8);
				return;
			case 1:
				eliminatePolicy(id, firstByte / 8);
				return;
			case 2:
				reveal(id, firstByte / 8);
				return;
			case 3:
				kill(id, firstByte / 8);
				return;
			case 4:
				selectPresident(id, firstByte / 8);
				return;
			case 7:
				break;
			default:
				return;
		}
		switch(firstByte / 8) {
			case 0:
				castVote(id, JA);
				return;
			case 1:
				castVote(id, NEIN);
				return;
			case 2:
				respondToVeto(id, true);
				return;
			case 3:
				respondToVeto(id, false);
				return;
			case 4:
				setName(id, message.substr(1));
				return;
			case 5:
				setReady(id, true);
				return;
			case 6:
				setReady(id, false);
				return;
			default:
				break;
		}
	}

	int getClientCount() const {
		return clientCount;
	}
};

#endif //SERVER_COMMUNICATION_MANAGER_H
