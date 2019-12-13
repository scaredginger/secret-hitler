#include <gtest/gtest.h>
#include "../game.h"

#include <fmt/core.h>

#define Mock(method) \
int method##_calls = 0;\
void method() { method##_calls++; }

namespace {
	class TestCommunicationManager5 {
	public:
		Game<FIVE, TestCommunicationManager5> game;

		TestCommunicationManager5() : game(*this, 100) {}

		Mock(announceVote)
		Mock(fascistHitlerWin)
		Mock(fascistPolicyWin)
		Mock(liberalPolicyWin)
		Mock(requestChancellorNomination)
		Mock(sendPresidentPolicyChoice)
		Mock(sendChancellorPolicyChoice)
		Mock(sendPresidentVetoOption)
		Mock(requestInvestigation)
		Mock(sendTopCards)
		Mock(requestSpecialPresidentNomination)
		Mock(requestKill)

		void initState() {
			EXPECT_LT(game.getPresidentId(), game.noPlayers);
			EXPECT_LT(game.getPresidentCounter(), game.noPlayers);
			EXPECT_LT(game.getHitler(), game.noPlayers);
			EXPECT_GE(game.getPresidentId(), 0);
			EXPECT_GE(game.getHitler(), 0);
			EXPECT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			EXPECT_EQ(game.getElectionTracker(), 0);
			EXPECT_EQ(game.getPreviousChancellorId(), -1);
			EXPECT_EQ(game.getPreviousPresidentId(), -1);
		}

		void round0() {
			game.nominateChancellor(3);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);

			game.nominateChancellor(1);
			ASSERT_EQ(announceVote_calls, 1);
			ASSERT_EQ(game.getState(), game.VOTING);

			game.addVote(0, JA);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(1, NEIN);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(2, NEIN);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(3, JA);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(4, NEIN);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			ASSERT_EQ(game.getElectionTracker(), 1);
			ASSERT_EQ(game.getPresidentId(), 4);
		}

		void round1() {
			game.nominateChancellor(1);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(0, JA);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(1, JA);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(2, NEIN);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(3, JA);
			ASSERT_EQ(game.getState(), game.VOTING);
			game.addVote(4, NEIN);
			ASSERT_EQ(game.getState(), game.AWAITING_PRESIDENT_POLICY);
			ASSERT_EQ(sendPresidentPolicyChoice_calls, 1);
			ASSERT_EQ(game.getElectionTracker(), 1);

			game.removePresidentPolicy(game.FIRST);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_POLICY);
			ASSERT_EQ(sendChancellorPolicyChoice_calls, 1);

			game.removeChancellorPolicy(game.SECOND);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			ASSERT_EQ(game.getPresidentId(), 0);
			ASSERT_EQ(game.getElectionTracker(), 0);
			ASSERT_EQ(game.getFascistPolicies(), 1);
		}

		// fail three elections
		void round2() {
			game.nominateChancellor(4);
			game.addVote(0, JA);
			game.addVote(1, NEIN);
			game.addVote(2, NEIN);
			game.addVote(3, JA);
			game.addVote(4, NEIN);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			ASSERT_EQ(game.getElectionTracker(), 1);

			game.nominateChancellor(4);
			game.addVote(0, NEIN);
			game.addVote(1, JA);
			game.addVote(2, NEIN);
			game.addVote(3, JA);
			game.addVote(4, NEIN);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			ASSERT_EQ(game.getElectionTracker(), 2);

			game.nominateChancellor(4);
			game.addVote(0, JA);
			game.addVote(1, JA);
			game.addVote(2, NEIN);
			game.addVote(3, NEIN);
			game.addVote(4, NEIN);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
			ASSERT_EQ(game.getElectionTracker(), 0);
			ASSERT_EQ(game.getLiberalPolicies() + game.getFascistPolicies(), 2);

			ASSERT_EQ(game.getLiberalPolicies(), 0);
			ASSERT_EQ(game.getFascistPolicies(), 2);
		}

		// we should have powers now
		void round3() {
			game.nominateChancellor(0);
			game.addVote(0, JA);
			game.addVote(1, JA);
			game.addVote(2, NEIN);
			game.addVote(3, JA);
			game.addVote(4, NEIN);

			ASSERT_EQ(game.getState(), game.AWAITING_PRESIDENT_POLICY);
			ASSERT_EQ(sendPresidentPolicyChoice_calls, 2);
			ASSERT_EQ(game.getElectionTracker(), 0);

			ASSERT_EQ(game.getFirstPolicy(), LIBERAL);
			ASSERT_EQ(game.getSecondPolicy(), FASCIST);
			ASSERT_EQ(game.getSecondPolicy(), FASCIST);
			game.removePresidentPolicy(game.SECOND);

			ASSERT_EQ(game.getFirstPolicy(), LIBERAL);
			ASSERT_EQ(game.getSecondPolicy(), FASCIST);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_POLICY);
			ASSERT_EQ(sendChancellorPolicyChoice_calls, 2);

			game.removeChancellorPolicy(game.FIRST);
			ASSERT_EQ(game.getFascistPolicies(), 3);
			ASSERT_EQ(game.getLiberalPolicies(), 0);
			ASSERT_EQ(sendTopCards_calls, 1);
			ASSERT_EQ(game.getState(), game.AWAITING_CHANCELLOR_NOMINATION);
		}

		// do two liberal policies
		void round4() {
			game.nominateChancellor(1);
			game.addVote(0, JA);
			game.addVote(1, JA);
			game.addVote(2, NEIN);
			game.addVote(3, JA);
			game.addVote(4, NEIN);

			ASSERT_EQ(game.getState(), game.FASCIST_HITLER_WIN);
			ASSERT_EQ(fascistHitlerWin_calls, 1);
		}
	};

	TEST(FivePeople, FascistHitlerWinGame) {
		TestCommunicationManager5 manager;
		manager.initState();
		manager.round0();
		manager.round1();
		manager.round2();
		manager.round3();
		manager.round4();
	}
}
