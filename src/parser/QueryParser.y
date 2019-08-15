%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.5"
%defines
%define api.namespace {mpls2pda}
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
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
    //
    // Created by Peter G. Jensen on 10/15/18.
    //
    #include <string>
    #include <unordered_set>
    #include <memory>
    #include "pdaaal/model/NFA.h"
    #include "model/Query.h"
    #include "model/filter.h"

    namespace mpls2pda {
        class Builder;
        class Scanner;
    }
    
    using namespace pdaaal;
    using namespace mpls2pda;
}


%parse-param { Scanner& scanner  }
%parse-param { Builder& builder }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "QueryBuilder.h"
    #include "Scanner.h"
    #include "model/Query.h"
    #include "pdaaal/model/NFA.h"
    #include "model/filter.h"

    using namespace pdaaal;
    using namespace mpls2pda;

    #undef yylex
    #define yylex ([&](){\
        return Parser::symbol_type((Parser::token_type)scanner.yylex(), builder._location);\
        })
}

%token
        END  0    "end of file"
        LPAREN    "("
        RPAREN    ")"
        
        AND       "&"
        OR        "|"
        DOT       "."
        PLUS      "+"
        STAR      "*"
        QUESTION  "?"

        LSQBRCKT  "["
        RSQBRCKT  "]"
        LT        "<"
        GT        ">"       
        
        IDENTIFIER  "identifier"
        LITERAL     "literal"        
        NUMBER     "number"
        HEX        "hex"
        
        COMMA     ","
        HAT       "^"
        HASH      "#"
        COLON     ":"
        
        OVER      "OVER"
        UNDER     "UNDER"
        EXACT     "EXACT"
        ANYIP     "@"
        ANYMPLS   "¤"
;


// bison does not seem to like naked shared pointers :(
%type  <int64_t> number hexnumber;
%type  <Query> query;
%type  <NFA<Query::label_t>> regex cregex;
%type  <Query::mode_t> mode;
%type  <std::unordered_set<Query::label_t>> atom_list label ip4 ip6;
%type  <filter_t> atom identifier name;
%type  <std::string> literal;
//%printer { yyoutput << $$; } <*>;
%left AND
%left OR
%left DOT 
%left PLUS 
%left STAR 
%left QUESTION
%left COMMA
        
%%
%start query_list;
query_list
        : query_list query { builder._result.emplace_back(std::move($2)); }
        | query { builder._result.emplace_back(std::move($1)); }
        | END// empty 
        ;
query
    : LT { builder.label_mode(); } cregex 
      GT { builder.path_mode(); } cregex 
      LT { builder.label_mode(); } cregex GT number mode
    {
        $$ = Query(std::move($3), std::move($6), std::move($9), $11, $12);
    }
    ;

mode 
    : OVER { $$ = Query::OVER; }
    | UNDER { $$ = Query::UNDER; }
    | EXACT { $$ = Query::EXACT; }
    ;
    
cregex
    : regex cregex { $$ = std::move($1); $$.concat(std::move($2)); }
    | regex { $$ = std::move($1); }
    | { $$ = NFA<Query::label_t>(true); }
    ;
    
regex    
    : regex AND regex { $$ = std::move($1); $$.and_extend(std::move($3)); }
    | regex OR regex { $$ = std::move($1); $$.or_extend(std::move($3)); }
    | DOT { std::unordered_set<Query::label_t> empty; $$ = NFA(std::move(empty), true); }
    | regex PLUS { $$ = std::move($1); $$.plus_extend(); }
    | regex STAR { $$ = std::move($1); $$.star_extend(); }
    | regex QUESTION { $$ = std::move($1); $$.question_extend(); }    
    | LSQBRCKT atom_list RSQBRCKT { $$ = NFA<Query::label_t>(std::move($2), false); }
    | LSQBRCKT HAT atom_list RSQBRCKT { $$ = NFA<Query::label_t>(std::move($3), true); } // negated set
    | ANYIP { $$ = builder.any_ip(); }
    | ANYMPLS { $$ = builder.any_mpls(); }
    | LPAREN regex RPAREN { $$ = std::move($2); }
    ;

    
number
    : NUMBER {
        $$ = scanner.last_int;
    }
    ;
    
hexnumber
    : HEX {
        $$ = scanner.last_int;
    }
    | { $$ = 0; } // NONE
    ;
    
atom_list
    : atom COMMA atom_list { $$ = builder.filter_and_merge($1, $3); }
    | atom { $$ = builder.filter($1); }
    | label COMMA atom_list{ $$ = std::move($3); $$.insert($1.begin(), $1.end()); }
    | label { $$ = std::move($1); }
    ;
    
atom 
    : identifier HASH { builder.set_post(); } identifier { $$ = $1 && $4; builder.clear_post(); }// LINKS
    ;

identifier
    : name { $$ = std::move($1); }
    | name DOT { builder.set_link(); } name
    { $$ = $1 && $4; builder.clear_link(); }
    | "^" { $$ = builder.routing_id(); }
    | "!" { $$ = builder.discard_id(); }
    | DOT { $$ = filter_t{}; }
    ;

ip4
    : number DOT number DOT number DOT number "/" number { 
        $$ = builder.match_ip4($1, $3, $5, $7, $9);
    }
    | number DOT number DOT number DOT number {
        $$ = builder.match_ip4($1, $3, $5, $7, 0);
    }
    ;

ip6
    : hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber "/" number {
        $$ = builder.match_ip6($1, $3, $5, $7, $9, $11, $13, $15, $17);
    }
    | hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber { 
        $$ = builder.match_ip6($1, $3, $5, $7, $9, $11, $13, $15, 0);
    }
    ;

literal
    : LITERAL {
        $$ = scanner.last_string.substr(1, scanner.last_string.length()-2) ;
    }
    ;

    
name
    : IDENTIFIER { $$ = builder.match_exact(scanner.last_string); }
    | literal {  $$ = builder.match_re(std::move($1)); }
    ;

label
    : number "/" number  { $$ = builder.find_label($1, $3); }
    | number  { $$ = builder.find_label($1, 0); }
    | ip4
    | ip6
    ;

%%

void
mpls2pda::Parser::error (const location_type& l,
                          const std::string& m)
{
  builder.error(l, m);
}