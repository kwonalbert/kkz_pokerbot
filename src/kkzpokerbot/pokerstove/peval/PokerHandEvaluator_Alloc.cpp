/**
 * Copyright (c) 2012 Andrew Prock. All rights reserved.
 * $Id: PokerHandEvaluator_Alloc.cpp 2649 2012-06-30 04:53:24Z prock $
 */
#include <stdexcept>
#include "OmahaHighHandEvaluator.h"
//#include "LowballA5HandEvaluator.h"
//#include "ThreeCardPokerHandEvaluator.h"

using namespace std;
using namespace pokerstove;

boost::shared_ptr<PokerHandEvaluator> PokerHandEvaluator::alloc(const string& strid)
{
    boost::shared_ptr<PokerHandEvaluator> ret;
    switch (strid[0])
    {
        case 'h':       //     hold'em
        case 'O':       //     omaha high
            ret.reset(new OmahaHighHandEvaluator);
            break;
    }

    ret->_subclassID = strid;

    return ret;
}
