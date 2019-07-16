%{ /* -*- C++ -*- */
//
// Created by Peter G. Jensen on 10/15/18.
//

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include "generated_parser.hh"
#include "Builder.h"
#include "errors.h"
#include "Scanner.h"
#undef  YY_DECL
#define YY_DECL int adpp::Scanner::yylex()
using token = adpp::Parser::token;
%}

%option noyywrap nounput batch debug noinput interactive c++
%option yyclass="adpp::Scanner"
%option debug
%x comment

id    [a-zA-Z][a-zA-Z_0-9_$#\-]*
int   [0-9]+
float {int}*\.{int}+
blank [ \t]
name  \"(\\.|[^\\"])*\"

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  builder._location.columns (yyleng);
  # define MAX_INCLUDE_DEPTH 10
  size_t depth = 0;
%}

%%

%{
  // Code run each time yylex is called.
  builder._location.step();
%}

{blank}+   builder._location.step ();
[\n\r]+    builder._location.lines(yyleng); builder._location.step();
"//".*     ;

"/*"                    BEGIN(comment);

<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>"*"+"/"        BEGIN(INITIAL);
<comment><<EOF>>        {

                            adpp::Parser::syntax_error
                                       (builder._location, "Unterminated multiline comment: " + std::string(yytext));
                        }

"true"  return token::TRUE;
"false" return token::FALSE;
"="     return token::EQUAL;
"{"     return token::LBRCKT;
"}"     return token::RBRCKT;
"("     return token::LPAREN;
")"     return token::RPAREN;
"["     return token::LSQBRCKT;
"]"     return token::RSQBRCKT;
"&&"    return token::AND;
"&"     return token::AND;
"AND"   return token::AND;
"and"   return token::AND;
"/\\"   return token::AND;
"||"    return token::OR;
"|"     return token::OR;
"OR"    return token::OR;
"or"    return token::OR;
"\\/"   return token::OR;
"!"     return token::NOT;
"~"     return token::NOT;
"NOT"   return token::NOT;
"not"   return token::NOT;
","     return token::COMMA;
";"     return token::SEMICOLON;
":"     return token::COLON;
"+"     return token::PLUS;
"-"     return token::MINUS;
"<="    return token::LTEQ;
"."     return token::DOT;

"Atomics"           return token::ATOMICS;
"Forest"            return token::FOREST;
"Arena"             return token::ARENA;

"Attacker"          return token::ATTACKER;
"Defender"          return token::DEFENDER;

"Budget"            return token::BUDGET;
"Cost"              return token::COST;
"Probability"       return token::PROBABILITY;
"Duration"          return token::DURATION;
"Dependency"        return token::DEPENDENCY;
"Expiration"        return token::EXPIRATION;
"Policy"            return token::POLICY;


"For"                   return token::FOR;
"under"                 return token::UNDER;
"in"                    return token::UNDER;
"against"               return token::AGAINST;
"Refines"               return token::REFINES;
"Classic"               return token::CLASSIC;
"AttackerCost"          return token::ATTACKERCOST;
"AttackerProbability"   return token::ATTACKERPROBABILITY;
"Minimise"              return token::MINIMISE;
"Minimize"              return token::MINIMISE;
"as"                    return token::AS;
"excluding"             return token::EXCLUDING;

"Exit"                  return token::EXIT;
"Show"                  return token::SHOW;
"Import"                return token::IMPORT;
"Module"                return token::MODULE;

{name}   {
    last_string = yytext;
    return token::LITERAL;
}
{id} {
    last_string = yytext;
    return token::IDENTIFIER;
}
{int}      {
  errno = 0;
  long n = strtol (yytext, NULL, 10);
  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
    throw adpp::syntax_error (builder._location, "integer is out of range: " + std::string(yytext));

  last_int = n;
  return token::NUMBER;
}

{float}    {

    std::string data(yytext);
    std::istringstream i(data);
    i >> last_float;
    return token::FLOAT;
}
