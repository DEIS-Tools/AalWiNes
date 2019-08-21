/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Peter G. Jensen
 */

//
// Created by Peter G. Jensen on 10/25/18.
//

#ifndef PROJECT_SCANNER_H
#define PROJECT_SCANNER_H

#if !defined(yyFlexLexerOnce)

#include <FlexLexer.h>

#endif

#include "generated_QueryParser.hh"
#include "location.hh"
#include "QueryBuilder.h"

namespace mpls2pda {

    class Scanner : public yyFlexLexer {
    public:
        Builder &builder;

        Scanner(std::istream *in, Builder &bld)
        : yyFlexLexer(in), builder(bld) {
        };
        using FlexLexer::yylex;

        virtual
        int yylex();

        double last_float;
        int64_t last_int;
        std::string last_string;
    };

}

#endif //PROJECT_SCANNER_H
