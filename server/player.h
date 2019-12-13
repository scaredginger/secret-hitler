//
// Created by ak on 12/12/19.
//

#ifndef SERVER_PLAYER_H
#define SERVER_PLAYER_H

#include "common.h"

class Player {
private:
	enum Flags {
		TEAM = 1,
		VOTED = TEAM << 1,
		ALIVE = VOTED << 1,
		INVESTIGATED = ALIVE << 1,
		LAST_VOTE = INVESTIGATED << 1
	};
	unsigned flags = 0;

public:
	bool loyalty() {
		return (TEAM & flags) == TEAM;
	}

	void loyalty(Team team) {
		flags &= ~TEAM;
		flags |= team * TEAM;
	}

	bool alive() {
		return (ALIVE & flags) == ALIVE;
	}

	void alive(bool isAlive) {
		flags &= ~ALIVE;
		flags |= isAlive * ALIVE;
	}

	bool hasVoted() {
		return (VOTED & flags) == VOTED;
	}

	void hasVoted(bool voted) {
		flags &= ~VOTED;
		flags |= voted * VOTED;
	}

	Vote lastVote() {
		return static_cast<Vote>((flags / LAST_VOTE) & 1);
	}

	void lastVote(Vote lastVote) {
		flags &= ~LAST_VOTE;
		flags |= lastVote * LAST_VOTE;
	}

	bool investigated() {
		return (INVESTIGATED & flags) == INVESTIGATED;
	}

	void investigated(bool voted) {
		flags &= ~INVESTIGATED;
		flags |= voted * INVESTIGATED;
	}
};


#endif //SERVER_PLAYER_H
