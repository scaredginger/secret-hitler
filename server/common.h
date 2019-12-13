//
// Created by ak on 12/12/19.
//

#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

enum GameType {
	FIVE = 5,
	SIX,
	SEVEN,
	EIGHT,
	NINE,
	TEN
};

enum Vote {
	NEIN = 0,
	JA = 1,
	ABSTAIN = 2
};

enum Team {
	FASCIST = 0,
	LIBERAL = 1
};

template <GameType type>
inline constexpr int getNoPlayers() {
	switch (type) {
		case FIVE:
			return 5;
		case SIX:
			return 6;
		case SEVEN:
			return 7;
		case EIGHT:
			return 8;
		case NINE:
			return 9;
		case TEN:
			return 10;
		default:
			return -1;
	}
}

#endif //SERVER_COMMON_H
