//
// Created by steven on 11/18/25.
//

#pragma once
#include "symbol.h"

namespace front::grammar {
    enum ParseAction {
        Reduction, Move, Accept, Error
    };

    struct ParseStep {
        Symbol top;
        Symbol lookahead;
        ParseAction action{Error};
    };
}
