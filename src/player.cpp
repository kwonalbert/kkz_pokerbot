#include <iostream>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include "player.hpp"
#include "peval/PokerHandEvaluator.h"

using namespace std;
namespace po = boost::program_options;
using namespace pokerstove;

class EvalDriver
{
public:
    EvalDriver(const string& game,
               const vector<string>& hands,
               const string& board)
        : _peval(PokerHandEvaluator::alloc (game))
        , _hands(hands)
        , _board(board)
    {
    }

    void evaluate()
    {
        for (auto it=_hands.begin(); it!=_hands.end(); it++)
        {
            string& hand = *it;
            _results[hand] = _peval->evaluate(hand, _board);
        }
    }

    string str() const
    {
        string ret;
        for (auto it=_hands.begin(); it!=_hands.end(); it++)
        {
            const string& hand = *it;
            ret += boost::str(boost::format("%10s: %s\n")
                              % hand
                              % _results.at(hand).str());
        }
        return ret;
    }

private:
    boost::shared_ptr<PokerHandEvaluator> _peval;
    vector<string> _hands;
    string _board;
    map<string,PokerHandEvaluation> _results;
};

Player::Player() {
}

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
  while (std::getline(stream, line)) {
    // For now, just print out whatever date is read in.
    std::cout << line << "\n";

    std::string getaction("GETACTION");
    std::string request_keyvalue_action("REQUESTKEYVALUES");
    std::string first_word = line.substr(0, line.find_first_of(' '));
    if (getaction.compare(first_word) == 0) {
      // Respond with CHECK when playing, you'll want to change this.
      stream << "CALL\n";
    } else if (request_keyvalue_action.compare(first_word) == 0) {
      // FINISh indicates no more keyvalue pairs to store.
      stream << "FINISH\n";
    }
  }

  std::cout << "Gameover, engine disconnected.\n";
}
