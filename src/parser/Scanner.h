//
// Created by Peter G. Jensen on 10/25/18.
//

#ifndef PROJECT_SCANNER_H
#define PROJECT_SCANNER_H

#if !defined(yyFlexLexerOnce)

#include <FlexLexer.h>

#endif

#include "generated_parser.hh"
#include "location.hh"
#include "Builder.h"

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
        int last_int;
        std::string last_string;
    };

}

#endif //PROJECT_SCANNER_H
