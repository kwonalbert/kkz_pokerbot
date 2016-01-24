#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <time.h>
#include <math.h>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include "pokerstove/peval/PokerHandEvaluator.h"
#include "cppitertools/combinations.hpp"

using namespace iter;
using namespace std;
namespace po = boost::program_options;
using namespace pokerstove;

int rand_val(int max) {
        return rand() % max;
}

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
            for (int i = 0; i < _hands.size(); i++)
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

vector<std::string> all_cards = {
        "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "Ts", "Js", "Qs", "Ks", "As",
        "2c", "3c", "4c", "5c", "6c", "7c", "8c", "9c", "Tc", "Jc", "Qc", "Kc", "Ac",
        "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "Th", "Jh", "Qh", "Kh", "Ah",
        "2d", "3d", "4d", "5d", "6d", "7d", "8d", "9d", "Td", "Jd", "Qd", "Kd", "Ad"
};

//elaborate all boards
int main(int argc, char *argv[]) {
        srand(time(NULL));

        ofstream flop_avgs, flop_devs;
        ofstream turn_avgs, turn_devs;
        ofstream river_avgs, river_devs;;

        flop_avgs.open("flop_avgs.data");
        turn_avgs.open("turn_avgs.data");
        river_avgs.open("river_avgs.data");

        flop_devs.open("flop_devs.data");
        turn_devs.open("turn_devs.data");
        river_devs.open("river_devs.data");

        for (auto&& f : combinations(all_cards,3)) { //flop
                vector<double> flop_strs;
                double flop_total = 0;
                vector<string> avail_turns = all_cards;
                vector<string> flop;
                for (auto&& i : f) {
                        flop.push_back(i);
                        avail_turns.erase(std::remove(avail_turns.begin(), avail_turns.end(), i), avail_turns.end());
                }
                std::sort(flop.begin(), flop.end());

                int flop_num = 0;
                for (auto&& t : avail_turns) {
                        vector<double> turn_strs;
                        double turn_total = 0;
                        vector<string> turn = flop;
                        turn.push_back(t); std::sort(turn.begin(), turn.end());

                        vector<string> avail_rivers = avail_turns;
                        avail_rivers.erase(std::remove(avail_rivers.begin(), avail_rivers.end(), t), avail_rivers.end());
                        int turn_num = 0;
                        for (auto&& r : avail_rivers) {
                                vector<double> river_strs;
                                double river_total = 0;
                                vector<string> river = turn;
                                river.push_back(r); std::sort(river.begin(), river.end());
                                string board = "";
                                for (int i = 0; i < 5; i++)
                                        board += river[i];

                                vector<string> avail_hands = avail_rivers;
                                avail_hands.erase(std::remove(avail_hands.begin(), avail_hands.end(), r), avail_hands.end());
                                int river_num = 0;

                                auto all_hands = combinations(avail_hands, 4);
                                vector<int> samples;
                                for (int i = 0; i < 2; i++) {
                                        samples.push_back(rand_val(178365));
                                }

                                for (int i = 0; i < samples.size(); i++) {
                                        auto h = all_hands.begin();
                                        advance(h, samples[i]);

                                        vector<string> hand;
                                        for (auto i : *h) {
                                                hand.push_back(i);
                                        }

                                        EvalDriver driver("O", hand, board);
                                        double strength = (double) driver.postRiverStrengthAnalysis("O", hand, board);

                                        river_total += strength; river_strs.push_back(strength);
                                        turn_total += strength; turn_strs.push_back(strength);
                                        flop_total += strength; flop_strs.push_back(strength);

                                        river_num++;
                                        turn_num++;
                                        flop_num++;
                                }
                                river_avgs << board << ' ' << river_total/river_num << '\n';

                                double river_dev_sum = 0;
                                for (int i = 0; i < river_strs.size(); i++)
                                        river_dev_sum += river_strs[i];
                                river_devs << board << ' ' << sqrt(river_dev_sum/(river_strs.size()-1)) << '\n';
                        }
                        string board = "";
                        for (int i = 0; i < 4; i++)
                                board += turn[i];
                        turn_avgs << board << ' ' << turn_total/turn_num << '\n';

                        double turn_dev_sum = 0;
                        for (int i = 0; i < turn_strs.size(); i++)
                                turn_dev_sum += turn_strs[i];
                        turn_devs << board << ' ' << sqrt(turn_dev_sum/(turn_strs.size()-1)) << '\n';
                }
                string board = "";
                for (int i = 0; i < 3; i++)
                        board += flop[i];
                flop_avgs << board << ' ' << flop_total/flop_num << '\n';

                double flop_dev_sum = 0;
                for (int i = 0; i < flop_strs.size(); i++)
                        flop_dev_sum += flop_strs[i];
                flop_devs << board << ' ' << sqrt(flop_dev_sum/(flop_strs.size()-1)) << '\n';
                std::cout << board << '\n';
        }

        flop_avgs.close();
        turn_avgs.close();
        river_avgs.close();


        flop_devs.close();
        turn_devs.close();
        river_devs.close();

        return 0;
}
