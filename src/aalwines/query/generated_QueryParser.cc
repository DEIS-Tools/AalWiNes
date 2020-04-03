// A Bison parser, made by GNU Bison 3.4.1.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2019 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.





#include "generated_QueryParser.hh"


// Unqualified %code blocks.
#line 58 "aalwines/query/QueryParser.y"

    #include "QueryBuilder.h"
    #include "Scanner.h"
    #include "aalwines/model/Query.h"
    #include <pdaaal/NFA.h>
    #include "aalwines/model/filter.h"

    using namespace pdaaal;
    using namespace aalwines;

    #undef yylex
    #define yylex ([&](){\
        return Parser::symbol_type((Parser::token_type)scanner.yylex(), builder._location);\
        })

#line 61 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Suppress unused-variable warnings by "using" E.
#define YYUSE(E) ((void) (E))

// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 4 "aalwines/query/QueryParser.y"
namespace aalwines {
#line 156 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"


  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  Parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  Parser::Parser (Scanner& scanner_yyarg, Builder& builder_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      scanner (scanner_yyarg),
      builder (builder_yyarg)
  {}

  Parser::~Parser ()
  {}

  Parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/



  // by_state.
  Parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  Parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  Parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  Parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  Parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  Parser::symbol_number_type
  Parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  Parser::stack_symbol_type::stack_symbol_type ()
  {}

  Parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 40: // cregex
      case 41: // regex
        value.YY_MOVE_OR_COPY< NFA<Query::label_t> > (YY_MOVE (that.value));
        break;

      case 35: // query
        value.YY_MOVE_OR_COPY< Query > (YY_MOVE (that.value));
        break;

      case 39: // mode
        value.YY_MOVE_OR_COPY< Query::mode_t > (YY_MOVE (that.value));
        break;

      case 45: // atom
      case 47: // identifier
      case 52: // name
        value.YY_MOVE_OR_COPY< filter_t > (YY_MOVE (that.value));
        break;

      case 42: // number
      case 43: // hexnumber
        value.YY_MOVE_OR_COPY< int64_t > (YY_MOVE (that.value));
        break;

      case 51: // literal
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      case 44: // atom_list
      case 49: // ip4
      case 50: // ip6
      case 53: // slabel
      case 55: // label
        value.YY_MOVE_OR_COPY< std::unordered_set<Query::label_t> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  Parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 40: // cregex
      case 41: // regex
        value.move< NFA<Query::label_t> > (YY_MOVE (that.value));
        break;

      case 35: // query
        value.move< Query > (YY_MOVE (that.value));
        break;

      case 39: // mode
        value.move< Query::mode_t > (YY_MOVE (that.value));
        break;

      case 45: // atom
      case 47: // identifier
      case 52: // name
        value.move< filter_t > (YY_MOVE (that.value));
        break;

      case 42: // number
      case 43: // hexnumber
        value.move< int64_t > (YY_MOVE (that.value));
        break;

      case 51: // literal
        value.move< std::string > (YY_MOVE (that.value));
        break;

      case 44: // atom_list
      case 49: // ip4
      case 50: // ip6
      case 53: // slabel
      case 55: // label
        value.move< std::unordered_set<Query::label_t> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 40: // cregex
      case 41: // regex
        value.move< NFA<Query::label_t> > (that.value);
        break;

      case 35: // query
        value.move< Query > (that.value);
        break;

      case 39: // mode
        value.move< Query::mode_t > (that.value);
        break;

      case 45: // atom
      case 47: // identifier
      case 52: // name
        value.move< filter_t > (that.value);
        break;

      case 42: // number
      case 43: // hexnumber
        value.move< int64_t > (that.value);
        break;

      case 51: // literal
        value.move< std::string > (that.value);
        break;

      case 44: // atom_list
      case 49: // ip4
      case 50: // ip6
      case 53: // slabel
      case 55: // label
        value.move< std::unordered_set<Query::label_t> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  Parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  Parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  Parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  Parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  Parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  Parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  Parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  Parser::debug_level_type
  Parser::debug_level () const
  {
    return yydebug_;
  }

  void
  Parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  Parser::state_type
  Parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  Parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  Parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  Parser::operator() ()
  {
    return parse ();
  }

  int
  Parser::parse ()
  {
    // State.
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << yystack_[0].state << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex ());
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      goto yydefault;

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", yyn, YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 40: // cregex
      case 41: // regex
        yylhs.value.emplace< NFA<Query::label_t> > ();
        break;

      case 35: // query
        yylhs.value.emplace< Query > ();
        break;

      case 39: // mode
        yylhs.value.emplace< Query::mode_t > ();
        break;

      case 45: // atom
      case 47: // identifier
      case 52: // name
        yylhs.value.emplace< filter_t > ();
        break;

      case 42: // number
      case 43: // hexnumber
        yylhs.value.emplace< int64_t > ();
        break;

      case 51: // literal
        yylhs.value.emplace< std::string > ();
        break;

      case 44: // atom_list
      case 49: // ip4
      case 50: // ip6
      case 53: // slabel
      case 55: // label
        yylhs.value.emplace< std::unordered_set<Query::label_t> > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2:
#line 132 "aalwines/query/QueryParser.y"
    { builder._result.emplace_back(std::move(yystack_[0].value.as < Query > ())); }
#line 712 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 3:
#line 133 "aalwines/query/QueryParser.y"
    { builder._result.emplace_back(std::move(yystack_[0].value.as < Query > ())); }
#line 718 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 5:
#line 137 "aalwines/query/QueryParser.y"
    { builder.label_mode(); builder.invert(true) ; builder.expand(false); }
#line 724 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 6:
#line 138 "aalwines/query/QueryParser.y"
    { builder.path_mode();  builder.invert(false); builder.expand(false); }
#line 730 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 7:
#line 139 "aalwines/query/QueryParser.y"
    { builder.label_mode(); builder.invert(false); builder.expand(true) ; }
#line 736 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 8:
#line 140 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < Query > () = Query(std::move(yystack_[9].value.as < NFA<Query::label_t> > ()), std::move(yystack_[6].value.as < NFA<Query::label_t> > ()), std::move(yystack_[3].value.as < NFA<Query::label_t> > ()), yystack_[1].value.as < int64_t > (), yystack_[0].value.as < Query::mode_t > ());
    }
#line 744 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 9:
#line 146 "aalwines/query/QueryParser.y"
    { yylhs.value.as < Query::mode_t > () = Query::OVER; }
#line 750 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 10:
#line 147 "aalwines/query/QueryParser.y"
    { yylhs.value.as < Query::mode_t > () = Query::UNDER; }
#line 756 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 11:
#line 148 "aalwines/query/QueryParser.y"
    { yylhs.value.as < Query::mode_t > () = Query::DUAL; }
#line 762 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 12:
#line 149 "aalwines/query/QueryParser.y"
    { yylhs.value.as < Query::mode_t > () = Query::EXACT; }
#line 768 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 13:
#line 153 "aalwines/query/QueryParser.y"
    { 
        if(builder.inverted())
        {
            yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[0].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().concat(std::move(yystack_[1].value.as < NFA<Query::label_t> > ()));
        }
        else
        {
            yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[1].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().concat(std::move(yystack_[0].value.as < NFA<Query::label_t> > ())); 
        }
    }
#line 783 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 14:
#line 163 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[0].value.as < NFA<Query::label_t> > ()); }
#line 789 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 15:
#line 164 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = NFA<Query::label_t>(true); }
#line 795 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 16:
#line 168 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[2].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().and_extend(std::move(yystack_[0].value.as < NFA<Query::label_t> > ())); }
#line 801 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 17:
#line 169 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[2].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().or_extend(std::move(yystack_[0].value.as < NFA<Query::label_t> > ())); }
#line 807 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 18:
#line 170 "aalwines/query/QueryParser.y"
    { std::unordered_set<Query::label_t> empty; yylhs.value.as < NFA<Query::label_t> > () = NFA(std::move(empty), true); }
#line 813 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 19:
#line 171 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[1].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().plus_extend(); }
#line 819 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 20:
#line 172 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[1].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().star_extend(); }
#line 825 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 21:
#line 173 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[1].value.as < NFA<Query::label_t> > ()); yylhs.value.as < NFA<Query::label_t> > ().question_extend(); }
#line 831 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 22:
#line 174 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = NFA<Query::label_t>(std::move(yystack_[1].value.as < std::unordered_set<Query::label_t> > ()), false); }
#line 837 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 23:
#line 175 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = NFA<Query::label_t>(std::move(yystack_[1].value.as < std::unordered_set<Query::label_t> > ()), true); }
#line 843 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 24:
#line 176 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = builder.any_ip(); }
#line 849 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 25:
#line 177 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = builder.any_mpls(); }
#line 855 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 26:
#line 178 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = builder.any_sticky(); }
#line 861 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 27:
#line 179 "aalwines/query/QueryParser.y"
    { yylhs.value.as < NFA<Query::label_t> > () = std::move(yystack_[1].value.as < NFA<Query::label_t> > ()); }
#line 867 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 28:
#line 184 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < int64_t > () = scanner.last_int;
    }
#line 875 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 29:
#line 190 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < int64_t > () = scanner.last_int;
    }
#line 883 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 30:
#line 193 "aalwines/query/QueryParser.y"
    { yylhs.value.as < int64_t > () = 0; }
#line 889 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 31:
#line 197 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.filter_and_merge(yystack_[2].value.as < filter_t > (), yystack_[0].value.as < std::unordered_set<Query::label_t> > ()); }
#line 895 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 32:
#line 198 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.filter(yystack_[0].value.as < filter_t > ()); }
#line 901 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 33:
#line 199 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = std::move(yystack_[0].value.as < std::unordered_set<Query::label_t> > ()); yylhs.value.as < std::unordered_set<Query::label_t> > ().insert(yystack_[2].value.as < std::unordered_set<Query::label_t> > ().begin(), yystack_[2].value.as < std::unordered_set<Query::label_t> > ().end()); }
#line 907 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 34:
#line 200 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = std::move(yystack_[0].value.as < std::unordered_set<Query::label_t> > ()); }
#line 913 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 35:
#line 204 "aalwines/query/QueryParser.y"
    { builder.set_post(); }
#line 919 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 36:
#line 204 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = yystack_[3].value.as < filter_t > () && yystack_[0].value.as < filter_t > (); builder.clear_post(); }
#line 925 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 37:
#line 208 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = std::move(yystack_[0].value.as < filter_t > ()); }
#line 931 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 38:
#line 209 "aalwines/query/QueryParser.y"
    { builder.set_link(); }
#line 937 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 39:
#line 210 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = yystack_[3].value.as < filter_t > () && yystack_[0].value.as < filter_t > (); builder.clear_link(); }
#line 943 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 40:
#line 211 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = builder.routing_id(); }
#line 949 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 41:
#line 212 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = builder.discard_id(); }
#line 955 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 42:
#line 213 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = filter_t{}; }
#line 961 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 43:
#line 217 "aalwines/query/QueryParser.y"
    { 
        yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.match_ip4(yystack_[8].value.as < int64_t > (), yystack_[6].value.as < int64_t > (), yystack_[4].value.as < int64_t > (), yystack_[2].value.as < int64_t > (), yystack_[0].value.as < int64_t > ());
    }
#line 969 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 44:
#line 220 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.match_ip4(yystack_[6].value.as < int64_t > (), yystack_[4].value.as < int64_t > (), yystack_[2].value.as < int64_t > (), yystack_[0].value.as < int64_t > (), 0);
    }
#line 977 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 45:
#line 226 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.match_ip6(yystack_[16].value.as < int64_t > (), yystack_[14].value.as < int64_t > (), yystack_[12].value.as < int64_t > (), yystack_[10].value.as < int64_t > (), yystack_[8].value.as < int64_t > (), yystack_[6].value.as < int64_t > (), yystack_[4].value.as < int64_t > (), yystack_[2].value.as < int64_t > (), yystack_[0].value.as < int64_t > ());
    }
#line 985 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 46:
#line 229 "aalwines/query/QueryParser.y"
    { 
        yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.match_ip6(yystack_[14].value.as < int64_t > (), yystack_[12].value.as < int64_t > (), yystack_[10].value.as < int64_t > (), yystack_[8].value.as < int64_t > (), yystack_[6].value.as < int64_t > (), yystack_[4].value.as < int64_t > (), yystack_[2].value.as < int64_t > (), yystack_[0].value.as < int64_t > (), 0);
    }
#line 993 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 47:
#line 235 "aalwines/query/QueryParser.y"
    {
        yylhs.value.as < std::string > () = scanner.last_string.substr(1, scanner.last_string.length()-2) ;
    }
#line 1001 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 48:
#line 242 "aalwines/query/QueryParser.y"
    { yylhs.value.as < filter_t > () = builder.match_exact(scanner.last_string); }
#line 1007 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 49:
#line 243 "aalwines/query/QueryParser.y"
    {  yylhs.value.as < filter_t > () = builder.match_re(std::move(yystack_[0].value.as < std::string > ())); }
#line 1013 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 50:
#line 247 "aalwines/query/QueryParser.y"
    { builder.set_sticky(); }
#line 1019 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 51:
#line 247 "aalwines/query/QueryParser.y"
    { builder.unset_sticky(); yylhs.value.as < std::unordered_set<Query::label_t> > () = yystack_[0].value.as < std::unordered_set<Query::label_t> > (); }
#line 1025 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 52:
#line 248 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = yystack_[0].value.as < std::unordered_set<Query::label_t> > (); }
#line 1031 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 53:
#line 252 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.find_label(yystack_[2].value.as < int64_t > (), yystack_[0].value.as < int64_t > ()); }
#line 1037 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 54:
#line 253 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = builder.find_label(yystack_[0].value.as < int64_t > (), 0); }
#line 1043 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 55:
#line 254 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = yystack_[0].value.as < std::unordered_set<Query::label_t> > (); }
#line 1049 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;

  case 56:
#line 255 "aalwines/query/QueryParser.y"
    { yylhs.value.as < std::unordered_set<Query::label_t> > () = yystack_[0].value.as < std::unordered_set<Query::label_t> > (); }
#line 1055 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"
    break;


#line 1059 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yyterror_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = yyn;
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  Parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  Parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (!yyla.empty ())
      {
        int yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char Parser::yypact_ninf_ = -47;

  const signed char Parser::yytable_ninf_ = -41;

  const signed char
  Parser::yypact_[] =
  {
      10,   -47,   -47,    55,   -47,    14,   -47,   -47,    14,   -47,
      56,   -47,   -47,   -47,   -10,    51,     4,   -47,   -47,   -47,
     -47,   -47,    -2,   -47,   -47,    -4,     8,    15,    20,    29,
     -47,   -47,   -47,    59,    50,   -47,   -47,    14,    14,   -47,
     -47,   -47,   -47,   -47,   -47,    58,    -6,    60,    60,    64,
     -47,    68,   -47,   -47,    68,    14,    83,    92,   -47,   -47,
      91,   -47,    80,   -47,    17,    30,   -47,    93,    60,    64,
     -47,   -47,   -47,    97,    82,    14,    60,    64,    94,    75,
      86,    60,    60,    64,    70,   -47,    87,   -47,   -47,   -47,
     -47,   -47,    64,    88,    64,    89,    64,    81,    60,   -47
  };

  const unsigned char
  Parser::yydefact_[] =
  {
       0,     4,     5,     0,     3,    15,     1,     2,    15,    18,
      30,    24,    25,    26,     0,    14,     0,    42,    48,    47,
      28,    29,    30,    50,    41,    54,     0,     0,    32,     0,
      55,    56,    49,    37,    34,    52,     6,     0,     0,    19,
      20,    21,    13,    27,    40,     0,    30,     0,     0,    30,
      22,    30,    35,    38,    30,    15,    16,    17,    23,    51,
       0,    53,     0,    31,     0,     0,    33,     0,     0,    30,
      36,    39,     7,     0,     0,    15,     0,    30,     0,    44,
       0,     0,     0,    30,     0,    43,     0,     9,    10,    11,
      12,     8,    30,     0,    30,     0,    30,    46,     0,    45
  };

  const signed char
  Parser::yypgoto_[] =
  {
     -47,   -47,   111,   -47,   -47,   -47,   -47,    -8,    27,   -46,
     -43,   -13,   -47,   -47,    52,   -47,   -47,   -47,   -47,    53,
     -47,   -47,    69
  };

  const signed char
  Parser::yydefgoto_[] =
  {
      -1,     3,     4,     5,    55,    75,    91,    14,    15,    25,
      26,    27,    28,    64,    29,    65,    30,    31,    32,    33,
      34,    46,    35
  };

  const signed char
  Parser::yytable_[] =
  {
      16,    60,    61,    47,    36,    17,    62,    42,    43,    45,
       1,    20,    21,    18,    19,    20,    21,     8,    44,   -40,
      23,     9,    73,     2,    17,    10,    74,    50,    48,    24,
      79,    49,    18,    19,    80,    84,    85,    44,    63,    51,
      86,    66,    11,    12,    13,    18,    19,    67,    24,    93,
      52,    95,    99,    97,     8,     6,    37,    38,     9,    39,
      40,    41,    10,    17,    56,    57,    53,    78,     2,    54,
      58,    18,    19,    20,    21,    17,    22,    20,    23,    11,
      12,    13,    21,    18,    19,    20,    21,    24,    44,    38,
      23,    39,    40,    41,    87,    88,    89,    90,    68,    24,
      39,    40,    41,    69,    76,    77,    72,    82,    81,    83,
      92,    94,    96,    98,     7,    59,    70,     0,    71
  };

  const signed char
  Parser::yycheck_[] =
  {
       8,    47,    48,     7,    14,     7,    49,    15,     4,    22,
       0,    17,    18,    15,    16,    17,    18,     3,    20,    21,
      22,     7,    68,    13,     7,    11,    69,    12,    32,    31,
      76,    23,    15,    16,    77,    81,    82,    20,    51,    19,
      83,    54,    28,    29,    30,    15,    16,    55,    31,    92,
      21,    94,    98,    96,     3,     0,     5,     6,     7,     8,
       9,    10,    11,     7,    37,    38,     7,    75,    13,    19,
      12,    15,    16,    17,    18,     7,    20,    17,    22,    28,
      29,    30,    18,    15,    16,    17,    18,    31,    20,     6,
      22,     8,     9,    10,    24,    25,    26,    27,     7,    31,
       8,     9,    10,    23,     7,    23,    13,    32,    14,    23,
      23,    23,    23,    32,     3,    46,    64,    -1,    65
  };

  const unsigned char
  Parser::yystos_[] =
  {
       0,     0,    13,    34,    35,    36,     0,    35,     3,     7,
      11,    28,    29,    30,    40,    41,    40,     7,    15,    16,
      17,    18,    20,    22,    31,    42,    43,    44,    45,    47,
      49,    50,    51,    52,    53,    55,    14,     5,     6,     8,
       9,    10,    40,     4,    20,    44,    54,     7,    32,    23,
      12,    19,    21,     7,    19,    37,    41,    41,    12,    55,
      42,    42,    43,    44,    46,    48,    44,    40,     7,    23,
      47,    52,    13,    42,    43,    38,     7,    23,    40,    42,
      43,    14,    32,    23,    42,    42,    43,    24,    25,    26,
      27,    39,    23,    43,    23,    43,    23,    43,    32,    42
  };

  const unsigned char
  Parser::yyr1_[] =
  {
       0,    33,    34,    34,    34,    36,    37,    38,    35,    39,
      39,    39,    39,    40,    40,    40,    41,    41,    41,    41,
      41,    41,    41,    41,    41,    41,    41,    41,    42,    43,
      43,    44,    44,    44,    44,    46,    45,    47,    48,    47,
      47,    47,    47,    49,    49,    50,    50,    51,    52,    52,
      54,    53,    53,    55,    55,    55,    55
  };

  const unsigned char
  Parser::yyr2_[] =
  {
       0,     2,     2,     1,     1,     0,     0,     0,    12,     1,
       1,     1,     1,     2,     1,     0,     3,     3,     1,     2,
       2,     2,     3,     4,     1,     1,     1,     3,     1,     1,
       0,     3,     1,     3,     1,     0,     4,     1,     0,     4,
       1,     1,     1,     9,     7,    17,    15,     1,     1,     1,
       0,     3,     1,     3,     1,     1,     1
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const Parser::yytname_[] =
  {
  "\"end of file\"", "error", "$undefined", "\"(\"", "\")\"", "\"&\"",
  "\"|\"", "\".\"", "\"+\"", "\"*\"", "\"?\"", "\"[\"", "\"]\"", "\"<\"",
  "\">\"", "\"identifier\"", "\"literal\"", "\"number\"", "\"hex\"",
  "\",\"", "\"^\"", "\"#\"", "\"$\"", "\":\"", "\"OVER\"", "\"UNDER\"",
  "\"DUAL\"", "\"EXACT\"", "\"ip\"", "\"mpls\"", "\"smpls\"", "\"!\"",
  "\"/\"", "$accept", "query_list", "query", "$@1", "$@2", "$@3", "mode",
  "cregex", "regex", "number", "hexnumber", "atom_list", "atom", "$@4",
  "identifier", "$@5", "ip4", "ip6", "literal", "name", "slabel", "$@6",
  "label", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned char
  Parser::yyrline_[] =
  {
       0,   132,   132,   133,   134,   137,   138,   139,   137,   146,
     147,   148,   149,   153,   163,   164,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   184,   190,
     193,   197,   198,   199,   200,   204,   204,   208,   209,   209,
     211,   212,   213,   217,   220,   226,   229,   235,   242,   243,
     247,   247,   248,   252,   253,   254,   255
  };

  // Print the state stack on the debug stream.
  void
  Parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << i->state;
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  Parser::yy_reduce_print_ (int yyrule)
  {
    unsigned yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 4 "aalwines/query/QueryParser.y"
} // aalwines
#line 1513 "/home/dank/SoftwareProjects/AalWiNes/src/aalwines/query/generated_QueryParser.cc"

#line 258 "aalwines/query/QueryParser.y"


void
aalwines::Parser::error (const location_type& l,
                          const std::string& m)
{
  builder.error(l, m);
}
