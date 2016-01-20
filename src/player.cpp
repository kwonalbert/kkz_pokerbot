#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include "player.hpp"
#include "lib.hpp"
#include <ctime>
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

    int postFlopStrengthAnalysis(const string& game, const vector<string>& hand, const string& board)  //Only for the flop. Not for the turn/river
    {//this method determines the game ending hands you possess based on your hand and partial board information
    //This is accomplished by filling in the rest of the board with all possible cards and evaluating the showdown
    //hand using PokerHandEvaluator.
        clock_t begin = clock();
        string outString = "pout.txt";
        //ofstream pout(outString.c_str());

        int strength = 0;
        int avgStrength = 0;
        int fiveCardHand = 0;
        int remainingCards = 52-7;
        int combos = remainingCards*(remainingCards-1)/2;
        vector<std::string> possibleCards = {"2","3","4","5","6","7","8","9","T","J","Q","K","A"}; //length  = 13.
        vector<std::string> possibleSuits = {"c","d","h","s"};
        vector<std::string> possibleHands = {"high card","one pair","two pair","trips","straight","flush","full house","quads","str8 flush"};

        std::string boardSuit,boardNum,handNum,handSuit;
        int handSuitCount[] = {0,0,0,0};       //stores a count for each index in possibleSuits
        int boardSuitCount[] = {0,0,0,0};
        int handNumsCount[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        int boardNumsCount[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        int indexHandSuit[4];       //stores the index in possibleSuits that corresponds to the suit of each card
        int indexBoardSuit[3];
        int indexHandNum[4];
        int indexBoardNum[3];

        //organize the information of the hand and board
        for (int i = 0; i<3; i++)
        {
            boardSuit = board.substr(1+i*2,1);        //should add just the suit of the into one string
            boardNum = board.substr(0+i*2,1);
            for (int s = 0; s < 4; s++)
            {
                if (boardSuit == possibleSuits[s])
                {
                    boardSuitCount[s]++;
                    indexBoardSuit[i] = s;
                }
            }
            for (int c = 0; c < 13; c++)
            {
                if (boardNum == possibleCards[c])
                {
                    boardNumsCount[c]++;
                    indexBoardNum[i] = c;
                }
            }
        }


        for (int i = 0; i<4; i++)
        {
            handSuit = hand[i].substr(1,1);        //should add just the suit of card  into one string
            handNum = hand[i].substr(0,1);
            for (int s = 0; s < 4; s++)
            {
                if (handSuit == possibleSuits[s])
                {
                    handSuitCount[s]++;
                    indexHandSuit[i] = s;
                }
            }
            for (int c = 0; c < 13; c++)
            {
                if (handNum == possibleCards[c])
                {
                    handNumsCount[c]++;
                    indexHandNum[i] = c;
                }
            }
        }

        int possibleEmptyFlushSuits[4]; //There needs to be at least two of these
        int emptySuitIndex[] = {0,0,0,0};
        int eSuits = 0;
        int possibleOneCardFlushSuits[4]; //Need two same suit cards on turn and river to get a flush

        for(int l = 0; l < 4; l++) //Count flush opportunities independently
        {
            if(handSuitCount[l] >= 2 && boardSuitCount[l] > 0)
            {
                if(boardSuitCount[l] == 1) //need both of the final cards to land the flush
                {//both of the last two cards have to be the same suit; handIndex for flush is 5
                    avgStrength += (5*13+7)*(10-handSuitCount[l])*(9-handSuitCount[l]);       // 5 for flush 7 for average flush high card,the other multiplyers are to account for all flush combos
                    possibleOneCardFlushSuits[l] = 1;
                    fiveCardHand += (5*13+7)*(10-handSuitCount[l])*(9-handSuitCount[l]);
                }
                if(boardSuitCount[l] == 2) //Only need one card to land a flush
                {//only one of the last two cards have to be the same suit
                    avgStrength += (5*13+7)*(9-handSuitCount[l])*(remainingCards);       // 5 for flush 7 for average flush high card,the other multiplyers are to account for all flush combos
                    fiveCardHand += (5*13+7)*(9-handSuitCount[l])*(remainingCards);
                }
                if(boardSuitCount[l] > 2)
                {//we have a fluhs!
                    avgStrength += (5*13+7)*combos;     //just boost the whole fucking thing up to this level. This is a great hand.
                    fiveCardHand += (5*13+7)*combos;
                }
            }
            else
            {
                possibleEmptyFlushSuits[l] = 1;
                emptySuitIndex[eSuits] = l;
                eSuits++;
                //pout<<"eSuits"<<eSuits<<'\n';
            }
        }

        //Build a vector of cards that we can safely use as turn and river cards; they can't be the same as any card already in the hand/board.
        vector<std::string> turnCards;
        vector<std::string> riverCards;  //use only when the number is the same as the river
        int turnCardNum[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};            //0 if no turn, 1 if only a turn, 2 if turn and river exist for this number.
        int riverCardNum[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        int knownSuits[] = {0,0,0,0};
        int localEmptySuit1 = -1;
        int localEmptySuit2 = -1;
        int turnCounter = 0;
        int riverCounter = 0;

        for (int i = 0; i < 13; i++)
        {
            if(handNumsCount[i] > 0 || boardNumsCount[i] > 0)//find open suits Its ok if I create/double count a few flushes as long as I still check for full houses too
            {
                if(handNumsCount[i] > 0)       //Take note of which cards of number 'i' are shown already in your hand and in the flop
                {
                    for (int j = 0; j < 4; j++)
                    {
                        if(indexHandNum[j] == i) //a card with the number 'i' is found
                        {
                            knownSuits[indexHandSuit[j]] = 1; //knownSuits stores a value of one for the suit in question
                        }
                    }
                }
                if(boardNumsCount[i] > 0)
                {
                    for(int j = 0; j < 3; j++)
                    {
                        if(indexBoardNum[j] == i)
                        {
                            knownSuits[indexBoardSuit[j]] = 1;

                        }
                    }
                }
                for (int s = 0; s<4; s++)
                {
                    if (knownSuits[s] == 1) //if the suit in question already exists in the hand then don't do anything
                    {
                        knownSuits[s] = 0;
                    }
                    else
                    {
                        if(s == emptySuitIndex[0]) {localEmptySuit1 = s;}
                        else if(s == emptySuitIndex[1]) {localEmptySuit2 = s;}
                        else if (localEmptySuit1 == -1) {localEmptySuit1 = s;}      //I may double count some flushes... this would be a strong hand anyway
                        else if(localEmptySuit2 == -1) {localEmptySuit2 = s;}
                    }
                }
                if(localEmptySuit1 > -1 && localEmptySuit2 > -1)
                    {
                        turnCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit1]);
                        turnCardNum[turnCounter] = i;
                        turnCounter++;
                        riverCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit2]);
                        riverCardNum[riverCounter] = i;
                        riverCounter++;
                    }
                if(localEmptySuit1 > -1 && localEmptySuit2 == -1) //we need to use the same card in both and check for this in our if loop b/c only one card is left w/this number
                    {
                        turnCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit1]);
                        turnCardNum[turnCounter] = i;
                        turnCounter++;
                        riverCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit1]);
                        riverCardNum[riverCounter] = i;
                        riverCounter++;
                    }
                if(localEmptySuit2 > -1 && localEmptySuit1 == -1) //we need to use the same card in both and check for this in our if loop
                    {
                        turnCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit2]);
                        turnCardNum[turnCounter] = i;
                        turnCounter++;
                        riverCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit2]);
                        riverCardNum[riverCounter] = i;
                        riverCounter++;
                    }
            }
            else
            {
                turnCards.push_back(possibleCards[i]+possibleSuits[emptySuitIndex[0]]);
                riverCards.push_back(possibleCards[i]+possibleSuits[emptySuitIndex[1]]);
                turnCardNum[turnCounter] = i;
                riverCardNum[riverCounter] = i;
                turnCounter++;
                riverCounter++;
            }
            localEmptySuit1 = -1;
            localEmptySuit2 = -1;

        }
        //pout<<"Hand: ";
        for (int i =0; i < 4; i++)
        {
            //pout<<hand[i]<<' ';
        }
        //pout<<'\n';
        //pout <<"Turn cards: ";
        for (int i = 0; i < turnCounter; i++)
        {
            //pout<<turnCards[i]<<' ';
        }
       //pout <<"River cards: ";
        for (int i = 0; i < riverCounter; i++)
        {
            //pout<<riverCards[i]<<' ';
        }
        //pout<<riverCounter<<'\n';

        //pout<<'\n';

        std::string result,showdown,highCard;
        std::string delimiter1 = ":";

        int handI = 0;
        int highCardI = 12;
        int degeneracy;
        int degeneracyCount = 0;
        string newBoard;

        for (int i = 0; i < turnCounter; i++) //Complete the board and run evaluate
        {
            for (int j = 0; j < turnCounter && riverCardNum[j] <= turnCardNum[i]; j++)
            {
                if(j < riverCounter && riverCards[j] != turnCards[i])  // make sure we are only doing river cards that are up to the turn card and that the turn and river cards are not the same card
                {
                    newBoard = board + turnCards[i] + riverCards[j];
                    //pout<<newBoard<<'\n';
                    EvalDriver trial(game, hand, newBoard);
                    //pout<<turnCards[i]<<riverCards[j]<<'\n';
                    //pout<<newBoard<<'\n';
                    trial.evaluate();      //creates a private variable _result that is of type PokerHandEvaluation. catgeorize this as a poker hand that is used to calculate strength
                    result = trial.str();
                    showdown = result.substr(0,result.find(delimiter1));
                    highCard = result.substr(15,1);

                    //pout<<"Showdown: "<<showdown<<'\n';
                    //pout<<"highCard: "<<highCard<<'\n';
                    handI = 0;
                    while (showdown != possibleHands[handI]) {handI++;}
                    highCardI = 12;
                    while (highCard != possibleCards[highCardI]) {highCardI--;}
                    //pout<<"showdownI: "<<handI<<'\n';
                    //pout<<"highCardI: "<<highCardI<<'\n';

                    //determine the degeneracy
                    if(riverCardNum[j] == turnCardNum[i])
                    {
                        degeneracy = (4-handNumsCount[i]-boardNumsCount[i])*(3-handNumsCount[i]-boardNumsCount[i])/2;
                    }
                    else
                    {
                        degeneracy = (4-handNumsCount[i]-boardNumsCount[i])*(4-handNumsCount[j]-boardNumsCount[j]);
                    }
                    //pout<<"Degeneracy: "<<degeneracy<<'\n';
                    avgStrength += degeneracy*(handI*13 + highCardI);         //roughly 13 of each set (straights are the exception) when only considering the high card. This just gives us a jump between trips and straights
                    //pout<<"Hand strength: "<< degeneracy*(handI*13 + highCardI) <<'\n';

                    if(handI > 3) //we have a 5 carder at least
                    {
                        fiveCardHand += degeneracy*(handI*13 + highCardI);
                    }
                    degeneracyCount += degeneracy;
                }
            }
        }
        //pout<< "DegeneracyCount: "<<degeneracyCount <<'\n';
        strength  = avgStrength/combos;        //the average strength is a scale from 0 to 103 that tells you the average outcome of your current hand*/
        //pout<<"Total strength: "<< strength <<'\n';
        fiveCardHand = fiveCardHand/combos;
        //pout<<"High Hand strength: "<< fiveCardHand <<'\n';
        clock_t end = clock();
        //pout << "Time Elapsed: "<< double(end-begin) / CLOCKS_PER_SEC;
        //pout.close();

        return strength;
    }

    int postTurnStrengthAnalysis(const string& game, const vector<string>& hand, const string& board)  //Only for the flop. Not for the turn/river
    {//this method determines the game ending hands you possess based on your hand and partial board information
    //This is accomplished by filling in the rest of the board with all possible cards and evaluating the showdown
    //hand using PokerHandEvaluator.
        clock_t begin = clock();
        string outString = "pout.txt";
        //ofstream pout(outString.c_str());

        int strength = 0;
        int avgStrength = 0;
        int fiveCardHand = 0;
        int remainingCards = 52-8;
        int combos = remainingCards;
        vector<std::string> possibleCards = {"2","3","4","5","6","7","8","9","T","J","Q","K","A"}; //length  = 13.
        vector<std::string> possibleSuits = {"c","d","h","s"};
        vector<std::string> possibleHands = {"high card","one pair","two pair","trips","straight","flush","full house","quads","str8 flush"};

        std::string boardSuit,boardNum,handNum,handSuit;
        int handSuitCount[] = {0,0,0,0};       //stores a count for each index in possibleSuits
        int boardSuitCount[] = {0,0,0,0};
        int handNumsCount[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        int boardNumsCount[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
        int indexHandSuit[4];       //stores the index in possibleSuits that corresponds to the suit of each card
        int indexBoardSuit[4];
        int indexHandNum[4];
        int indexBoardNum[4];

        //organize the information of the hand and board
        for (int i = 0; i<4; i++)
        {
            boardSuit = board.substr(1+i*2,1);        //should add just the suit of the into one string
            boardNum = board.substr(0+i*2,1);
            for (int s = 0; s < 4; s++)
            {
                if (boardSuit == possibleSuits[s])
                {
                    boardSuitCount[s]++;
                    indexBoardSuit[i] = s;
                }
            }
            for (int c = 0; c < 13; c++)
            {
                if (boardNum == possibleCards[c])
                {
                    boardNumsCount[c]++;
                    indexBoardNum[i] = c;
                }
            }
        }


        for (int i = 0; i<4; i++)
        {
            handSuit = hand[i].substr(1,1);        //should put just the suit of card  into a string
            handNum = hand[i].substr(0,1);
            for (int s = 0; s < 4; s++)
            {
                if (handSuit == possibleSuits[s])
                {
                    handSuitCount[s]++;
                    indexHandSuit[i] = s;
                }
            }
            for (int c = 0; c < 13; c++)
            {
                if (handNum == possibleCards[c])
                {
                    handNumsCount[c]++;
                    indexHandNum[i] = c;
                }
            }
        }

        int possibleEmptyFlushSuits[4]; //There needs to be at least two of these
        int emptySuitIndex[] = {0,0,0,0};
        int eSuits = 0;
        int possibleOneCardFlushSuits[4]; //Need two same suit cards on turn and river to get a flush

        for(int l = 0; l < 4; l++) //Count flush opportunities independently
        {
            if(handSuitCount[l] >= 2 && boardSuitCount[l] > 1)
            {
                if(boardSuitCount[l] == 2) //Only need one card to land a flush
                {//the last card has to be the same suit
                    avgStrength += (5*13+7)*(9-handSuitCount[l]);       // 5 for flush 7 for average flush high card,the other multiplyers are to account for all flush combos
                    fiveCardHand += (5*13+7)*(9-handSuitCount[l]);
                }
                if(boardSuitCount[l] > 2)
                {//we have a flush!
                    avgStrength += (5*13+7)*combos;     //just boost the whole fucking thing up to this level. This is a great hand.
                    fiveCardHand += (5*13+7)*combos;
                }
            }
            else
            {
                possibleEmptyFlushSuits[l] = 1;
                emptySuitIndex[eSuits] = l;
                eSuits++;
                //pout<<"eSuits"<<eSuits<<'\n';
            }
        }

        //Build a vector of cards that we can safely use as turn and river cards; they can't be the same as any card already in the hand/board.
        vector<std::string> riverCards;  //use only when the number is the same as the river
        int riverCardNum[13];
        int knownSuits[] = {0,0,0,0};
        int localEmptySuit1 = -1;
        int riverCounter = 0;

        for (int i = 0; i < 13; i++)
        {
            if(handNumsCount[i] > 0 || boardNumsCount[i] > 0)//find open suits Its ok if I create/double count a few flushes as long as I still check for full houses too
            {
                if(handNumsCount[i] > 0)       //Take note of which cards of number 'i' are shown already in your hand and in the flop
                {
                    for (int j = 0; j < 4; j++)
                    {
                        if(indexHandNum[j] == i) //a card with the number 'i' is found
                        {
                            knownSuits[indexHandSuit[j]] = 1; //knownSuits stores a value of one for the suit in question
                        }
                    }
                }
                if(boardNumsCount[i] > 0)
                {
                    for(int j = 0; j < 4; j++)
                    {
                        if(indexBoardNum[j] == i)
                        {
                            knownSuits[indexBoardSuit[j]] = 1;
                        }
                    }
                }
                for (int s = 0; s<4; s++)
                {
                    if (knownSuits[s] == 1) //if the suit in question already exists in the hand then don't do anything
                    {
                        knownSuits[s] = 0;
                    }
                    else
                    {
                        if(s == emptySuitIndex[0]) {localEmptySuit1 = s;}
                        else if(s == emptySuitIndex[1]) {localEmptySuit1 = s;}
                        else if (localEmptySuit1 == -1) {localEmptySuit1 = s;}      //I may double count some flushes... this would be a strong hand anyway
                    }
                }

                if(localEmptySuit1 > -1)
                    {
                        riverCards.push_back(possibleCards[i]+possibleSuits[localEmptySuit1]);
                        riverCardNum[riverCounter] = i;
                        riverCounter++;
                    }
            }
            else
            {
                riverCards.push_back(possibleCards[i]+possibleSuits[emptySuitIndex[1]]);
                riverCardNum[riverCounter] = i;
                riverCounter++;
            }
            localEmptySuit1 = -1;
        }
        //pout<<"Hand: ";
        for (int i =0; i < 4; i++)
        {
            //pout<<hand[i]<<' ';
        }
        //pout <<"River cards: ";
        for (int i = 0; i < riverCounter; i++)
        {
            //pout<<riverCards[i]<<' ';
        }
        //pout<<'\n';

        std::string result,showdown,highCard;
        std::string delimiter1 = ":";

        int handI = 0;
        int highCardI = 12;
        int degeneracy;
        int degeneracyCount = 0;
        string newBoard;
        for (int i = 0; i < riverCards.size(); i++) //Complete the board and run evaluate
        {
            newBoard = board + riverCards[i];
            EvalDriver trial(game, hand, newBoard);

            //pout<<riverCards[i]<<'\n';
            // std::cout<<newBoard<<'\n';
            // std::cout << riverCards[i] << '\n';
            trial.evaluate();      //creates a private variable _result that is of type PokerHandEvaluation. catgeorize this as a poker hand that is used to calculate strength
            result = trial.str();
            showdown = result.substr(0,result.find(delimiter1));
            highCard = result.substr(15,1);

            //pout<<"Showdown: "<<showdown<<'\n';
            //pout<<"highCard: "<<highCard<<'\n';
            handI = 0;
            while (showdown != possibleHands[handI]) {handI++;}
            highCardI = 12;
            while (highCard != possibleCards[highCardI]) {highCardI--;}
            //pout<<"showdownI: "<<handI<<'\n';
            //pout<<"highCardI: "<<highCardI<<'\n';

            //determine the degeneracy
            degeneracy = (4-handNumsCount[i]-boardNumsCount[i]);

            //pout<<"Degeneracy: "<<degeneracy<<'\n';
            avgStrength += degeneracy*(handI*13 + highCardI);         //roughly 13 of each set (straights are the exception) when only considering the high card. This just gives us a jump between trips and straights
            //pout<<"Hand strength: "<< degeneracy*(handI*13 + highCardI) <<'\n';

            if(handI > 3) //we have a 5 carder at least
            {
                fiveCardHand += degeneracy*(handI*13 + highCardI);
            }
            degeneracyCount += degeneracy;
        }
        strength = avgStrength/combos;        //the average strength is a scale from 0 to 103 that tells you the average outcome of your current hand
        fiveCardHand = fiveCardHand/combos;
        clock_t end = clock();

        return strength;
    }

    int postRiverStrengthAnalysis(const string& game, const vector<string>& hand, const string& board)  //Only for the flop. Not for the turn/river
    {//this method determines the game ending hands you possess based on your hand and partial board information
    //This is accomplished by filling in the rest of the board with all possible cards and evaluating the showdown
    //hand using PokerHandEvaluator.
        clock_t begin = clock();
        string outString = "pout.txt";
        //ofstream pout(outString.c_str());

        int strength = 0;
        int avgStrength = 0;

        vector<std::string> possibleCards = {"2","3","4","5","6","7","8","9","T","J","Q","K","A"}; //length  = 13.
        vector<std::string> possibleSuits = {"c","d","h","s"};
        vector<std::string> possibleHands = {"high card","one pair","two pair","trips","straight","flush","full house","quads","str8 flush"};

        std::string result,showdown,highCard;
        std::string delimiter1 = ":";

        int handI = 0;
        int highCardI = 12;
        EvalDriver trial(game, hand, board);

        trial.evaluate();      //creates a private variable _result that is of type PokerHandEvaluation. catgeorize this as a poker hand that is used to calculate strength
        result = trial.str();
        showdown = result.substr(0,result.find(delimiter1));
        highCard = result.substr(15,1);

        //pout<<"Showdown: "<<showdown<<'\n';
        //pout<<"highCard: "<<highCard<<'\n';
        handI = 0;
        while (showdown != possibleHands[handI]) {handI++;}
        highCardI = 12;
        while (highCard != possibleCards[highCardI]) {highCardI--;}
        //pout<<"showdownI: "<<handI<<'\n';
        //pout<<"highCardI: "<<highCardI<<'\n';
        strength = (handI*13 + highCardI);        //the average strength is a scale from 0 to 103 that tells you the average outcome of your current hand
        //pout.close();
        return strength;
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

double pre_fold_thresh = 0.065;
double pre_raise_thresh = 0.14;

double flop_avg = 43;
double flop_dev = 11;

void raise(tcp::iostream &stream, int val) {
        stream << "RAISE:" << val << "\n";
}

int raise_max(tcp::iostream &stream, getaction *ga) {
        int max = -1;
        for (int i = 0; i < ga->num_legal_actions; i++) {
                if (ga->legal_actions[i].t == L_RAISE) {
                        max = ga->legal_actions[i].max;
                }
        }
        if (max != -1) {
                raise(stream, max);
                return 1;
        } else {
                return 0;
        }
}

void bet(tcp::iostream &stream, int val) {
        stream << "BET:" << val << "\n";
}

int bet_max(tcp::iostream &stream, getaction *ga) {
        int max = -1;
        for (int i = 0; i < ga->num_legal_actions; i++) {
                if (ga->legal_actions[i].t == L_BET) {
                        max = ga->legal_actions[i].max;
                }
        }
        if (max != -1) {
                bet(stream, max);
                return 1;
        } else {
                return 0;
        }
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
        vector<std::string> cur_hand;   //each string in vector is a card in the hole
        bool button;
        bool pre;
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
                        button = nh.button;
                } else if (strncmp("GETACTION", splits[0].begin(), 9) == 0) {
                        getaction ga = parse_getaction(splits);

                        pre = ga.num_board_cards == 0;

                        std::string board = "";
                        for (int i = 0; i < ga.num_board_cards; i++)
                                board += ga.board_cards[i];

                        std::string hand = "";
                        for (int i = 0; i < 4; i++) {
                                hand += cur_hand[i];
                        }

                        performedaction last_action = ga.last_actions[ga.num_last_actions-1];

                        EvalDriver driver(game, cur_hand, board);
                        double strength;
                        if (ga.num_board_cards == 0) {
                                strength = (double) preflop[hand]/1000000;
                        } else if (ga.num_board_cards == 3) {
                                strength = (double) driver.postFlopStrengthAnalysis(game, cur_hand, board);
                        } else if (ga.num_board_cards == 4) {
                                strength = (double) driver.postTurnStrengthAnalysis(game, cur_hand, board);
                        } else if (ga.num_board_cards == 5) {
                                strength = (double) driver.postRiverStrengthAnalysis(game, cur_hand, board);
                        }
                        if (last_action.t == BET) {
                                if (ga.num_board_cards >= 3) { //flop + turn + river
                                        if (button) { //second
                                                if (strength < flop_avg - 1.5*flop_dev) {
                                                        stream << "FOLD\n";
                                                } else if (strength > flop_avg + flop_dev) {
                                                        if (rand_val() > .5) {
                                                                if (!raise_max(stream, &ga))
                                                                        stream << "CALL\n";
                                                        } else {
                                                                stream << "CALL\n";
                                                        }
                                                } else {
                                                        if (rand_val() < (strength/(flop_avg + 1.5*flop_dev))) {
                                                                stream << "CALL\n";
                                                        } else {
                                                                stream << "FOLD\n";
                                                        }
                                                }
                                        } else { //first
                                                if (strength < flop_avg - 1.5*flop_dev) {
                                                        stream << "FOLD\n";
                                                } else if (strength > flop_avg + 1.5*flop_dev) {
                                                        if (rand_val() > .5) {
                                                                if (!raise_max(stream, &ga))
                                                                        stream << "CALL\n";
                                                        } else {
                                                                stream << "CALL\n";
                                                        }
                                                } else {
                                                        if (rand_val() < (strength/(flop_avg + 1.5*flop_dev))) {
                                                                stream << "CALL\n";
                                                        } else {
                                                                stream << "FOLD\n";
                                                        }
                                                }
                                        }
                                }
                        } else if (last_action.t == CALL) { // if (pre && !button)
                                if (strength > pre_raise_thresh) {
                                        if (!bet_max(stream, &ga))
                                                stream << "CALL\n";
                                } else if (strength < pre_fold_thresh) {
                                        stream << "CHECK\n";
                                } else {
                                        double bet_prob = ((strength-pre_fold_thresh) / pre_raise_thresh);
                                        if (rand_val() > bet_prob) {
                                                stream << "CHECK\n";
                                        } else {
                                                if (!bet_max(stream, &ga))
                                                        stream << "CALL\n";
                                        }
                                }
                        } else if (last_action.t == CHECK) {
                                if (ga.num_board_cards >= 3) { //flop, turn, river
                                        if (strength > flop_avg + 1.5*flop_dev) {
                                                if (!bet_max(stream, &ga))
                                                        stream << "CALL\n";
                                        } else if (strength < flop_avg - 1.5*flop_dev) {
                                                stream << "CHECK\n";
                                        } else {
                                                if (rand_val() > (strength/(flop_avg+1.5*flop_dev))) {
                                                        stream << "CHECK\n";
                                                } else {
                                                        if (!bet_max(stream, &ga))
                                                                stream << "CALL\n";
                                                }
                                        }
                                }
                        } else if (last_action.t == DEAL) {
                                if (ga.num_board_cards >= 3) { //flop, turn, river
                                        if (strength > flop_avg + 1.5*flop_dev) {
                                                if (rand_val() < (strength-flop_avg)/600) {
                                                        if (!bet_max(stream, &ga))
                                                                stream << "CALL\n";
                                                } else {
                                                        stream << "CHECK\n";
                                                }
                                        } else if (strength < flop_avg - 1.5*flop_dev) {
                                                stream << "CHECK\n";
                                        } else {
                                                if (rand_val() < 0.015) {
                                                        if (!bet_max(stream, &ga))
                                                                stream << "CALL\n";
                                                } else {
                                                        stream << "CHECK\n";
                                                }
                                        }
                                }
                        } else if (last_action.t == POST) { // if (pre && button)
                                if (strength < pre_fold_thresh) {
                                        stream << "FOLD\n";
                                } else if (strength > pre_raise_thresh) {
                                        if (!raise_max(stream, &ga))
                                                stream << "CALL\n";
                                } else {
                                        double bet_prob = (strength-pre_fold_thresh) / pre_raise_thresh;
                                        if (ga.last_actions[ga.num_last_actions-1].t == BET || rand_val() > bet_prob) {
                                                stream << "CALL\n";
                                        } else {
                                                if (!raise_max(stream, &ga))
                                                        stream << "CALL\n";
                                        }
                                }
                        } else if (last_action.t == RAISE) {
                                if (pre) {
                                        if (strength > pre_raise_thresh) {
                                                if (rand_val() > .5) {
                                                        if (!raise_max(stream, &ga))
                                                                stream << "CALL\n";
                                                } else {
                                                        stream << "CALL\n";
                                                }
                                        } else {
                                                stream << "CALL\n";
                                        }
                                } else {
                                        if (strength > flop_avg + 1.5*flop_dev) {
                                                if (rand_val() > .5) {
                                                        if (!raise_max(stream, &ga))
                                                                stream << "CALL\n";
                                                } else {
                                                        stream << "CALL\n";
                                                }
                                        } else {
                                                if (rand_val() < (strength/(flop_avg + 1.5*flop_dev))) {
                                                        stream << "CALL\n";
                                                } else {
                                                        stream << "FOLD\n";
                                                }
                                        }
                                }
                        } else {
                                stream << "CALL\n";
                        }
                } else if (strncmp("HANDOVER", splits[0].begin(), 8) == 0) {
                        handover ho = parse_handover(splits);
                } else if (strncmp("REQUESTKEYVALUES", splits[0].begin(), 16) == 0) {
                        // FINISh indicates no more keyvalue pairs to store.
                        stream << "FINISH\n";
                }
        }

        std::cout << "Gameover, engine disconnected.\n";
}
