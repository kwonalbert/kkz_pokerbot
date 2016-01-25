#ifndef __LIB_HPP__
#define __LIB_HPP__

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <vector>

using namespace std;

struct newgame {
        std::string your_name;
        std::string opp_name;
        int stack_size;
        int bb;
        int num_hands;
        double time_bank;
};

struct newhand {
        int hand_id;
        bool button;
        vector<std::string> holecards;
        int my_bank;
        int other_bank;
        double time_bank;
};

enum laction_type { L_BET, L_CALL, L_CHECK, L_FOLD, L_RAISE };
enum paction_type { BET, CALL, CHECK, DEAL, FOLD, POST, RAISE, REFUND,
                    SHOW, TIE, WIN };

// thought about super action class, but doesn't seem to help much here
struct legalaction {
        laction_type t;
        int amount;
        int min;
        int max;
};

struct performedaction {
        paction_type t;

        //optional for some
        int amount;
        int min;
        int max;
        std::string actor; //overloaded for street for DEAL

        //only useful for SHOW
        std::string card1;
        std::string card2;
        //seems like documentation is wrong, and only reveals 2 cards
        //std::string card3;
        //std::string card4;
};

struct getaction {
        int pot_size;
        int num_board_cards;
        vector<std::string> board_cards;
        int num_last_actions;
        vector<performedaction> last_actions;
        int num_legal_actions;
        vector<legalaction> legal_actions;
        double time_bank;
};


struct handover {
        int my_bank;
        int other_bank;
        int num_board_cards;
        vector<std::string> board_cards;
        int num_last_actions;
        vector<performedaction> last_actions;
        double time_bank;
};

class StringRef {
private:
        char const*     begin_;
        int             size_;

public:
        int size() const { return size_; }
        char const* begin() const { return begin_; }
        char const* end() const { return begin_ + size_; }

        StringRef( char const* const begin, int const size )
                : begin_(begin)
                , size_(size)
        {}
};

vector<StringRef> split(string const& str, char delimiter = ' ') {
        vector<StringRef> result;

        enum State { inSpace, inToken };

        State state = inSpace;
        char const* pTokenBegin = 0;
        for( auto it = str.begin(); it != str.end(); ++it ) {
                State const newState = (*it == delimiter? inSpace : inToken);
                if( newState != state ) {
                        switch( newState ) {
                        case inSpace:
                                result.push_back(StringRef(pTokenBegin, &*it - pTokenBegin));
                                break;
                        case inToken:
                                pTokenBegin = &*it;
                        }
                }
                state = newState;
        }
        if( state == inToken ) {
                result.push_back(StringRef(pTokenBegin, &*str.end() - pTokenBegin));
        }
        return result;
}

int atoi(const char* str, const int size)
{
        int val = 0;
        int count = 0;
        bool neg = false;
        while(*str && count < size) {
                if (*str == '-') {
                        neg = true;
                } else {
                        val = val*10 + (*str - '0');
                }
                str++; count++;
        }
        if (neg)
                return -val;
        else
                return val;
}

newgame parse_newgame(vector<StringRef> splits) {
        int count = 1; //count = 0 is NEWGAME
        std::string your_name(splits[count].begin(), splits[count].size()); count++;
        std::string opp_name(splits[count].begin(), splits[count].size()); count++;
        int stack_size = atoi(splits[count].begin(), splits[count].size()); count++;
        int bb = atoi(splits[count].begin(), splits[count].size()); count++;
        int num_hands = atoi(splits[count].begin(), splits[count].size()); count++;
        std::string time_str(splits[count].begin(), splits[count].size());
        float time_bank = boost::lexical_cast<double>(time_str);
        newgame ng;
        ng.your_name = your_name; ng.opp_name = opp_name; ng.stack_size = stack_size;
        ng.bb = bb; ng.num_hands = num_hands; ng.time_bank = time_bank;
        return ng;
}

newhand parse_newhand(vector<StringRef> splits) {
        int count = 1; //count = 0 is NEWHAND
        int hand_id = atoi(splits[count].begin(), splits[count].size()); count++;
        bool button;
        if (splits[count++].begin()[0] == 'f')
                button = false;
        else
                button = true;
        vector<std::string> holecards;
        for (int i = 0; i < 4; i++) {
                std::string holecard(splits[count].begin(), splits[count].size()); count++;
                holecards.push_back(holecard);
        }
        int my_bank = atoi(splits[count].begin(), splits[count].size()); count++;
        int other_bank = atoi(splits[count].begin(), splits[count].size()); count++;
        std::string time_str(splits[count].begin(), splits[count].size());
        float time_bank = boost::lexical_cast<double>(time_str);
        newhand nh;
        nh.hand_id = hand_id; nh.button = button; nh.holecards = holecards;
        nh.my_bank = my_bank; nh.other_bank = other_bank; nh.time_bank = time_bank;
        return nh;
}

legalaction parse_legalaction(string const& str) {
        vector<StringRef> raw = split(str, ':');

        legalaction action;
        if (strncmp("BET", raw[0].begin(), 3) == 0) {
                action.t = L_BET;
                action.min = atoi(raw[1].begin(), raw[1].size());
                action.max = atoi(raw[2].begin(), raw[2].size());
        } else if (strncmp("CALL", raw[0].begin(), 4) == 0) {
                action.t = L_CALL;
                //seems like the specification is wrong, and there's no amount
        } else if (strncmp("CHECK", raw[0].begin(), 5) == 0) {
                action.t = L_CHECK;
        } else if (strncmp("FOLD", raw[0].begin(), 4) == 0) {
                action.t = L_FOLD;
        } else if (strncmp("RAISE", raw[0].begin(), 5) == 0) {
                action.t = L_RAISE;
                action.min = atoi(raw[1].begin(), raw[1].size());
                action.max = atoi(raw[2].begin(), raw[2].size());
        }
        return action;
}

performedaction parse_performedaction(string const& str) {
        vector<StringRef> raw = split(str, ':');

        performedaction action;
        if (strncmp("BET", raw[0].begin(), 3) == 0) {
                action.t = BET;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                if (raw.size() == 3) {
                        std::string actor(raw[2].begin(), raw[2].size());
                        action.actor = actor;
                }
        } else if (strncmp("CALL", raw[0].begin(), 4) == 0) {
                action.t = CALL;
                if (raw.size() == 2) {
                        std::string actor(raw[1].begin(), raw[1].size());
                        action.actor = actor;
                }
        } else if (strncmp("CHECK", raw[0].begin(), 5) == 0) {
                action.t = CHECK;
                if (raw.size() == 2) {
                        std::string actor(raw[1].begin(), raw[1].size());
                        action.actor = actor;
                }
        } else if (strncmp("DEAL", raw[0].begin(), 4) == 0) {
                action.t = DEAL;
                std::string street(raw[1].begin(), raw[1].size());
                action.actor = street;
        } else if (strncmp("FOLD", raw[0].begin(), 4) == 0) {
                action.t = FOLD;
                if (raw.size() == 2) {
                        std::string actor(raw[1].begin(), raw[1].size());
                        action.actor = actor;
                }
        } else if (strncmp("POST", raw[0].begin(), 4) == 0) {
                action.t = POST;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                std::string actor(raw[2].begin(), raw[2].size());
                action.actor = actor;
        } else if (strncmp("RAISE", raw[0].begin(), 5) == 0) {
                action.t = RAISE;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                if (raw.size() == 3) {
                        std::string actor(raw[2].begin(), raw[2].size());
                        action.actor = actor;
                }
        } else if (strncmp("REFUND", raw[0].begin(), 6) == 0) {
                action.t = REFUND;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                std::string actor(raw[2].begin(), raw[2].size());
                action.actor = actor;
        } else if (strncmp("SHOW", raw[0].begin(), 4) == 0) {
                action.t = SHOW;
                std::string card1(raw[1].begin(), raw[1].size());
                std::string card2(raw[2].begin(), raw[2].size());
                //std::string card3(raw[3].begin(), raw[3].size());
                //std::string card4(raw[4].begin(), raw[4].size());
                std::string actor(raw[3].begin(), raw[3].size());
                action.card1 = card1; action.card2 = card2;
                //action.card3 = card3; action.card4 = card4;
                action.actor = actor;
        } else if (strncmp("TIE", raw[0].begin(), 3) == 0) {
                action.t = TIE;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                std::string actor(raw[2].begin(), raw[2].size());
                action.actor = actor;
        } else if (strncmp("WIN", raw[0].begin(), 3) == 0) {
                action.t = WIN;
                action.amount = atoi(raw[1].begin(), raw[1].size());
                std::string actor(raw[2].begin(), raw[2].size());
                action.actor = actor;
        }
        return action;
}

getaction parse_getaction(vector<StringRef> splits) {
        int count = 1;
        int pot_size = atoi(splits[count].begin(), splits[count].size()); count++;
        int num_board_cards = atoi(splits[count].begin(), splits[count].size()); count++;
        vector<std::string> board_cards;
        for (int i = 0; i < num_board_cards; i++) {
                std::string str(splits[count].begin(), splits[count].size()); count++;
                board_cards.push_back(str);
        }
        int num_last_actions = atoi(splits[count].begin(), splits[count].size()); count++;
        vector<performedaction> last_actions;
        for (int i = 0; i < num_last_actions; i++) {
                std::string str(splits[count].begin(), splits[count].size()); count++;
                performedaction pa = parse_performedaction(str);
                last_actions.push_back(pa);
        }
        int num_legal_actions = atoi(splits[count].begin(), splits[count].size()); count++;
        vector<legalaction> legal_actions;
        for (int i = 0; i < num_legal_actions; i++) {
                std::string str(splits[count].begin(), splits[count].size()); count++;
                legalaction la = parse_legalaction(str);
                legal_actions.push_back(la);
        }
        std::string time_str(splits[count].begin(), splits[count].size());
        float time_bank = boost::lexical_cast<double>(time_str);

        getaction ga;
        ga.pot_size = pot_size;
        ga.num_board_cards = num_board_cards; ga.board_cards = board_cards;
        ga.num_last_actions = num_last_actions; ga.last_actions = last_actions;
        ga.num_legal_actions = num_legal_actions; ga.legal_actions = legal_actions;
        ga.time_bank = time_bank;
        return ga;
}

handover parse_handover(vector<StringRef> splits) {
        int count = 1;
        int my_bank = atoi(splits[count].begin(), splits[count].size()); count++;
        int other_bank = atoi(splits[count].begin(), splits[count].size()); count++;
        int num_board_cards = atoi(splits[count].begin(), splits[count].size()); count++;
        vector<std::string> board_cards;
        for (int i = 0; i < num_board_cards; i++) {
                std::string str(splits[count].begin(), splits[count].size()); count++;
                board_cards.push_back(str);
        }
        int num_last_actions = atoi(splits[count].begin(), splits[count].size()); count++;
        vector<performedaction> last_actions;
        for (int i = 0; i < num_last_actions; i++) {
                std::string str(splits[count].begin(), splits[count].size()); count++;
                performedaction pa = parse_performedaction(str);
                last_actions.push_back(pa);
        }
        std::string time_str(splits[count].begin(), splits[count].size());
        float time_bank = boost::lexical_cast<double>(time_str);

        handover ho;
        ho.my_bank = my_bank; ho.other_bank = other_bank;
        ho.num_board_cards = num_board_cards; ho.board_cards = board_cards;
        ho.num_last_actions = num_last_actions; ho.last_actions = last_actions;
        ho.time_bank = time_bank;
        return ho;
}

double rand_val() {
        return ((double) rand() / (RAND_MAX));
}

#endif  // __LIB_HPP__
