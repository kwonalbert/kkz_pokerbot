#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include "player.hpp"
#include "lib.hpp"
#include "pokerstove/peval/PokerHandEvaluator.h"

using namespace std;
namespace po = boost::program_options;
using namespace pokerstove;

class EvalDriver {
public:
    EvalDriver(const string& game,
               const vector<string>& hands,
               const string& board)
        : _peval(PokerHandEvaluator::alloc (game))
        , _hands(hands)
        , _board(board)
        {}

    void evaluate() {
            CardSet hand = CardSet();
            for (int i = 0; i < 4; i++)
                    hand.insert(CardSet(_hands[i]));
            _result = _peval->evaluate(hand, _board);
    }

    string str() const {
        string ret;
        ret = boost::str(boost::format("%s\n")
                         % _result.str());
        return ret;
    }

private:
    boost::shared_ptr<PokerHandEvaluator> _peval;
    vector<string> _hands;
    string _board;
    PokerHandEvaluation _result;
};

Player::Player() {
}

std::map<std::string, int> preflop;

/**
 * Simple example pokerbot, written in C++.
 *
 * This is an example of a bare bones pokerbot. It only sets up the socket
 * necessary to connect with the engine (using a Boost iostream for
 * convenience) and then always returns the same action.  It is meant as an
 * example of how a pokerbot should communicate with the engine.
 */
void Player::run(tcp::iostream &stream) {
        std::string line;
        vector<std::string> cur_hand;
        std::string game("O");
        while (std::getline(stream, line)) {
                // For now, just print out whatever date is read in.
                std::cout << line << "\n";
                vector<StringRef> splits = split(line);

                if (strncmp("NEWGAME", splits[0].begin(), 7) == 0) {
                        newgame ng = parse_newgame(splits);
                } else if (strncmp("NEWHAND", splits[0].begin(), 7) == 0) {
                        newhand nh = parse_newhand(splits);
                        cur_hand = nh.holecards;
                        std::sort(cur_hand.begin(), cur_hand.end());
                } else if (strncmp("GETACTION", splits[0].begin(), 9) == 0) {
                        getaction ga = parse_getaction(splits);

                        // std::string board = "";
                        // for (int i = 0; i < ga.num_board_cards; i++)
                        //         board += ga.board_cards[i];
                        std::string board = "4d9dAh9s3c";

                        std::cout << "HAND:";
                        std::string hand = "";
                        for (int i = 0; i < 4; i++) {
                                std::cout << cur_hand[i];
                                hand += cur_hand[i];
                        }
                        std::cout << "\nSCORE:";
                        std::cout << preflop[hand];
                        std::cout << "\nBOARD:";
                        std::cout << board << "\n";

                        // vector<std::string> new_hand;
                        // new_hand.push_back("9c");
                        // new_hand.push_back("9h");

                        EvalDriver driver(game, cur_hand, board);
                        driver.evaluate();
                        // std::cout << driver.str() << "\n";

                        stream << "CALL\n";
                } else if (strncmp("HANDOVER", splits[0].begin(), 8) == 0) {
                        handover ho = parse_handover(splits);
                } else if (strncmp("REQUESTKEYVALUES", splits[0].begin(), 16) == 0) {
                        // FINISh indicates no more keyvalue pairs to store.
                        stream << "FINISH\n";
                }
        }

        std::cout << "Gameover, engine disconnected.\n";
}
