%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.5"
%defines
%define api.namespace {adpp}
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    //
    // Created by Peter G. Jensen on 10/15/18.
    //
    #include <string>
    #include <unordered_map>
    #include <memory>
    #include "model/Context.h"

    namespace adpp {
        class Builder;
        class Scanner;
        class Assignable;
        class Action;
        class CostVar;
        class Arena;
        class BoolExpr;
        class Defender;
        class DispatchedAction;
    }
    using namespace adpp;
}


%parse-param { Scanner& scanner  }
%parse-param { Builder& builder }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "Builder.h"
    #include "Scanner.h"
    #include "Dispatcher.h"

    #include "model/Defender.h"
    #include "model/BoolExpr.h"
    #include "model/Arena.h"
    #include "model/CostVar.h"
    #include "model/Action.h"
    #include "model/Module.h"

    #include <engine/Classic.h>
    #include <engine/Refinement.h>
    #include <engine/Formats.h>
    #include <engine/Minimization.h>
    #include <engine/Show.h>

    using namespace adpp;
    #undef yylex
    #define yylex ([&](){\
        return Parser::symbol_type((Parser::token_type)scanner.yylex(), builder._location);\
        })
}

%token
  END  0    "end of file"
  TRUE      "true"
  FALSE     "false"
  EQUAL     "="
  LBRCKT    "{"
  RBRCKT    "}"
  LPAREN    "("
  RPAREN    ")"
  LSQBRCKT  "["
  RSQBRCKT  "]"
  AND       "&"
  OR        "|"
  NOT       "~"
  COMMA     ","
  SEMICOLON ";"
  COLON     ":"
  PLUS      "+"
  MINUS     "-"
  LTEQ      "<="
  DOT       "."
  ATOMICS               "Atomics"
  FOREST                "Forest"
  ARENA                 "Arena"
  ATTACKER              "Attacker"
  DEFENDER              "Defender"
  BUDGET                "Budget"
  COST                  "Cost"
  PROBABILITY           "Probability"
  DURATION              "Duration"
  DEPENDENCY            "Dependency"
  EXPIRATION            "Expiration"
  POLICY                "Policy"
  FOR                   "For"
  UNDER                 "under"
  AGAINST               "against"
  REFINES               "Refines"
  CLASSIC               "Classic"
  ATTACKERCOST          "AttackerCost"
  ATTACKERPROBABILITY   "AttackerProbability"
  SHOW                  "Show"
  MINIMISE              "Minimise"
  EXIT                  "Exit"
  IMPORT                "Import"
  AS                    "as"
  EXCLUDING             "excluding"
  MODULE                "Module"
  IDENTIFIER            "identifier"
  LITERAL               "literal"
  NUMBER                "number"
  FLOAT                 "float"
;


// bison does not seem to like naked shared pointers :(
%type  <Assignable*> assignable
%type  <Arena*> arena_id
%type  <std::unordered_map<Action*, std::pair<int32_t, int32_t>>> interval_list
%type  <std::unordered_map<CostVar*, int32_t>> budget_list cost_list
%type  <std::vector<Arena*>> inheritance_list inheritance
%type  <std::vector<std::string>> complex_id
%type  <std::string>  identifier literal alias
%type  <std::vector<Assignable*>> id_list
%type  <std::vector<Action*>> action_decl_list
%type  <Action*> action_decl_var
%type  <std::vector<CostVar*>> cost_var_list
%type  <CostVar*> cost_var_decl
%type  <BoolExpr*> bool_expr
%type  <int> number;
%type  <Defender*> defender_decl
%type  <std::vector<Defender*>> defender_id
%type  <std::shared_ptr<DispatchedAction>> action classic
%type  <Context> restricted_tree system
%type  <std::pair<int,int>> interval
%type  <double> float
%type  <std::unordered_map<Action*, bool>> bool_assign

//%printer { yyoutput << $$; } <*>;

%left AND OR EQUAL SHOW
%right NOT

%%
%start entry;
entry
    : command entry
    | identifier EQUAL assignable
    {
        if(!builder.dry_run())
        {
            builder.top_module()->declare_name($1, $3, builder._location, builder._warningstream);
            builder.top_module()->try_bind();
        }
    } entry
    | // empty
    ;

number
    : NUMBER {
        $$ = scanner.last_int;
    }
    ;

literal
    : LITERAL {
        $$ = scanner.last_string.substr(1, scanner.last_string.length()-2) ;
    }
    ;

identifier
    : IDENTIFIER {
        $$ = scanner.last_string;
    }
    ;

alias
    : literal { $$ = $1; }
    | identifier { $$ = $1; }
    ;

sub_routine
    : { if(!builder.dry_run()) builder.start_arena(builder._location); }
    ;

assignable
    : sub_routine arena {
        if(!builder.dry_run())
        {
            $$ = builder.current_arena(); builder.end_arena();
        }
    }
    | sub_routine FOREST LBRCKT
    {
        if(!builder.dry_run())
            builder.push_module(builder._location);
    }
    forest RBRCKT
    {
        if(!builder.dry_run())
        {
            $$ = builder.pop_module().get();
            builder.top_module()->try_bind(true);
        }
    }
    | sub_routine defender_decl { $$ = $2; }
    | sub_routine bool_expr {
        $$ = $2;
        if(!builder.dry_run())
        {
            builder.top_module()->try_bind(true);
        }
    }
    ;

defender_decl
    : DEFENDER LBRCKT id_list RBRCKT {
        if(!builder.dry_run())
        {
            auto el = std::make_shared<Defender>($3, builder._location);
            builder.add_var(el);
            $$ = el.get();
        }
    }
    ;

atomics_declarations
    : {}
    | ATTACKER LBRCKT action_decl_list RBRCKT atomics_declarations
    {
        if(!builder.dry_run(), true)
        {
            builder.current_arena()->add_atomics($3, true, builder._warningstream);
        }
    }
    | DEFENDER LBRCKT action_decl_list RBRCKT atomics_declarations
    {
        if(!builder.dry_run())
        {
            builder.current_arena()->add_atomics($3, false, builder._warningstream);
        }
    }
    | COST LBRCKT cost_var_list RBRCKT atomics_declarations
    {
        if(!builder.dry_run())
        {
            builder.current_arena()->add_costs($3, builder._warningstream);
        }
    }
    ;

action_decl_list
    : { if(!builder.dry_run()) $$ = {}; }// empty
    | action_decl_var action_decl_list
    {
        if(!builder.dry_run())
        {
            $2.push_back($1);
            $$ = $2;
        }
    }
    ;

action_decl_var
    : identifier EQUAL alias
    {
        if(!builder.dry_run())
        {
            auto el = std::make_shared<Action>($1, $3, builder._location);
            $$ = builder.add_var(el);
        }
    }
    ;

cost_var_list
    : { if(!builder.dry_run()) $$ = {}; }// empty
    | cost_var_decl cost_var_list
    {
        if(!builder.dry_run())
        {
            $2.push_back($1);
            $$ = $2;
        }
    }
    ;

cost_var_decl
    : identifier EQUAL alias
    {
        if(!builder.dry_run())
        {
            auto el = std::make_shared<CostVar>($1, $3, builder._location);
            $$ = builder.add_var(el);
        }
    }
    ;




arena
    : sub_arena { }
    | ARENA inheritance {} LBRCKT sub_arena_list RBRCKT {}
    ;

inheritance
    : { }
    | COLON inheritance_list { };

inheritance_list
    : complex_id PLUS
    {
        if(!builder.dry_run())
        {
            auto el = std::dynamic_pointer_cast<Arena>(
                builder.top_module()->unfold($1, builder._location));
            if(el == nullptr)
                builder.error(builder._location, "expected Arena");
            builder.current_arena()->merge(*el.get(), builder._location);
        }
    }
    inheritance_list {}
    | arena PLUS {} inheritance_list {}
    | {} // empty
    ;

sub_arena_list
    : {}
    | sub_arena_list {} sub_arena {}
    ;

sub_arena
    : COST LBRCKT cost_decl RBRCKT {  }
    | PROBABILITY LBRCKT probability_list RBRCKT {  }
    | DEPENDENCY LBRCKT dependency_list RBRCKT { }
    | POLICY LBRCKT policy_list RBRCKT {  }
    | ATOMICS LBRCKT atomics_declarations RBRCKT { }
    | BUDGET LBRCKT budget_list RBRCKT
    {
        if(!builder.dry_run())
        {
            builder.current_arena()->set_budget($3, builder._location);
        }
    }
    | DURATION LBRCKT interval_list RBRCKT {
        if(!builder.dry_run())
        {
            builder.current_arena()->set_duration($3);
        }
    }
    | EXPIRATION LBRCKT interval_list RBRCKT
    {
        if(!builder.dry_run())
        {
            builder.current_arena()->set_expiration($3);
        }
    }
    ;

policy_list
    : { }
    | policy_list bool_expr COLON LBRCKT bool_assign RBRCKT
    {
        if(!builder.dry_run())
            builder.current_arena()->set_policy($2, $5, builder._location);
    }
    ;

bool_assign
    : {} // empty
    | bool_assign complex_id COLON TRUE
    {
        if(!builder.dry_run())
        {
            auto var = builder.current_arena()->unfold($2, builder._location);
            auto action = std::dynamic_pointer_cast<Action>(var);
            if(action == nullptr)
                builder.terror(builder._location, "Type error: expected an Action");
            if($$.count(action.get()) > 0)
                builder._warningstream << "warning: Variable assigned more than once" << builder._location << std::endl;
            $$[action.get()] = true;
        }
    }
    | bool_assign complex_id COLON FALSE
    {
        if(!builder.dry_run())
        {
            auto var = builder.current_arena()->unfold($2, builder._location);
            auto action = std::dynamic_pointer_cast<Action>(var);
            if(action == nullptr)
                builder.terror(builder._location, "Type error: expected an Action");
            if($$.count(action.get()) > 0)
                builder._warningstream << "warning: Action assigned more than once" << builder._location << std::endl;
            $$[action.get()] = false;
        }
    }
    ;

cost_decl
    : {}
    | cost_decl complex_id COLON  LBRCKT cost_list RBRCKT
    {
        if(!builder.dry_run())
        {
            auto var = builder.current_arena()->unfold($2, builder._location);
            builder.current_arena()->set_cost(var.get(), $5, builder._location);
        }
    }
    ;

cost_list
    : { if(!builder.dry_run()) $$ = std::unordered_map<CostVar*, int32_t>(); } // empty
    | cost_list complex_id COLON number
    {
        if(!builder.dry_run())
        {
            auto var = builder.current_arena()->unfold($2, builder._location);
            auto cv = std::dynamic_pointer_cast<CostVar>(var);
            if(cv == nullptr)
                builder.terror(builder._location, "Type error: expected a Cost Variable");
            if($1.count(cv.get()) > 0)
                builder._warningstream << "warning: cost for " << $2 << " declared more than once " << builder._location << std::endl;
            $1[cv.get()] = $4;
            $$ = $1;
        }
    }
    ;

float
    : number { $$ = $1; }
    | FLOAT  { $$ = scanner.last_float; }
    ;

probability_list
    : probability_list complex_id COLON float
    {
        if(!builder.dry_run())
        {
            const Assignable::sptr& el = builder.current_arena()->unfold($2, builder._location);
            if($4 < 0 || $4 > 1.0)
            {
                throw adpp::type_error (builder._location, "probability is out of range: " + std::to_string($4) + " should be within [0,1]");
            }
            builder.current_arena()->set_probability(el.get(), $4 , builder._location);
        }
    }
    | {}
    ;

interval_list
    : interval_list complex_id COLON LSQBRCKT number COMMA number RSQBRCKT
    {
        if(!builder.dry_run())
        {
            $$ = std::move($1);
            if($5 < 0 || $7 < $5)
            {
                throw adpp::type_error (builder._location, "interval needs to be non-empty and positive, got: [" + std::to_string($5) + ", " + std::to_string($7) + "]");
            }
            const Assignable::sptr& el = builder.current_arena()->unfold($2, builder._location);
            auto act = std::dynamic_pointer_cast<Action>(el);
            if(act == nullptr)
                builder.terror(builder._location, "expected an Action");
            $$[act.get()] = std::make_pair($5,$7);
        }
    }
    | { }
    ;

dependency_list
    : dependency_list complex_id COLON bool_expr
    {
        if(!builder.dry_run())
        {
            const Assignable::sptr& el = builder.current_arena()->unfold($2, builder._location);
            builder.current_arena()->set_dependency(el.get(), $4, builder._location);
        }
    }
    | {}
    ;

budget_list
    : { if(!builder.dry_run()) $$ = std::unordered_map<CostVar*, int32_t>(); } // empty
    | budget_list complex_id COLON number
    {
        if(!builder.dry_run())
        {
            auto var = builder.current_arena()->unfold($2, builder._location);
            auto cv = std::dynamic_pointer_cast<CostVar>(var);
            if(cv == nullptr)
                builder.terror(builder._location, "expected a Cost Variable");
            if($1.count(cv.get()) > 0)
                builder._warningstream << "warning: budget for " << $2 << " declared more than once " << builder._location << std::endl;
            $1[cv.get()] = $4;
            $$ = $1;
        }
    }
    ;


forest
    : // empty
    | forest identifier EQUAL bool_expr
    {
        if(!builder.dry_run())
        {
            builder.top_module()->declare_name($2, $4, builder._location, builder._warningstream);
            builder.top_module()->try_bind(false);
        }
    }
    ;

bool_expr
    : TRUE {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolTrue>(builder._location));
    }
    | FALSE {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolFalse>(builder._location));
    }
    | bool_expr OR bool_expr  {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolOr>(*$1, *$3, builder._location));
    }
    | bool_expr AND bool_expr  {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolAnd>(*$1, *$3, builder._location));
    }
    | NOT bool_expr  {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolNegation>(*$2, builder._location));
    }
    | LPAREN AND id_list RPAREN  {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolAnd>($3, builder._location));
    }
    | LPAREN OR id_list RPAREN  {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolOr>($3, builder._location));
    }
    | LPAREN bool_expr RPAREN { if(!builder.dry_run()) $$ = $2; }
    | complex_id {
        if(!builder.dry_run())
        {
            try
            {
                auto el = builder.top_module()->unfold($1, builder._location);
                $$ = dynamic_cast<BoolExpr*>(el.get());
                if($$ == nullptr)
                {
                    $$ = builder.add_var(std::make_shared<BoolAlias>($1, builder._location));
                }
            }
            catch(undeclared_error& er)
            {
                $$ = builder.add_var(std::make_shared<BoolAlias>($1, builder._location));
            }
        }
    }
    | LSQBRCKT literal EQUAL bool_expr RSQBRCKT
    {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolAlias>($2, $4, builder._location));
    }
    | LSQBRCKT identifier EQUAL bool_expr RSQBRCKT
    {
        if(!builder.dry_run())
            $$ = builder.add_var(std::make_shared<BoolAlias>($2, $4, builder._location));
    }
    ;


complex_id
    : identifier {
        if(!builder.dry_run())
            $$ = {$1};
    }
    | identifier DOT complex_id
    {
        if(!builder.dry_run())
        {
            $3.push_back($1);
            $$ = $3;
        }
    }
    ;

id_list
    : complex_id id_list
    {
        if(!builder.dry_run())
        {
            auto el = builder.top_module()->unfold($1, builder._location).get();
            if(el == nullptr)
            {
                el = builder.add_var(std::make_shared<BoolAlias>($1, builder._location));
            }
            $2.insert($2.begin(), el);
            $$ = $2;
        }
    }
    | complex_id 
    { 
        if(!builder.dry_run())
        {
            auto el = builder.top_module()->unfold($1, builder._location).get();
            if(el == nullptr)
            {
                el = builder.add_var(std::make_shared<BoolAlias>($1, builder._location));
            }
            $$ = {el}; 
        };
    }

command
    : EXIT
    {
        if(!builder.dry_run())
            builder._dispatcher->cmd_exit();
    }
    | query
    | IMPORT import_statement
    | MODULE identifier
    {
        if(!builder.dry_run())
        {
            auto ot = builder.top_module();
            builder.push_module(builder._location);
            ot->declare_name($2, builder.top_module(), builder._location, builder._warningstream);
        }
    }
    LBRCKT entry RBRCKT
    {
        if(!builder.dry_run())
            builder.pop_module();
    }
    ;

import_statement
    : literal
    {
        if(!builder.dry_run())
        {
            builder.include_file($1, builder._location);
        }
    }
    ;


arena_id
    : complex_id {
        if(!builder.dry_run())
        {
            auto el = std::dynamic_pointer_cast<Arena>(
                        builder.top_module()->unfold($1, builder._location));
            if(el == nullptr)
                builder.error(builder._location, "expected Arena");
            $$ = el.get();
        }
    }
    ;

defender_id
    : id_list
    {
        if(!builder.dry_run())
        {
            for(auto* v : $1)
            {
                auto did = dynamic_cast<Defender*>(v);
                if(did == nullptr)
                    builder.terror(builder._location, "expected a Defender");
                $$.push_back(did);
            }
        }
    }
    | defender_decl
    {
        if(!builder.dry_run())
        {
            $$.push_back($1);
        }
    }
    ;

sys_decl
    : system AGAINST defender_id
    {
        if(!builder.dry_run()){
            builder.set_context($1);
            builder.set_defender($3);
        }
    }
    | system
    {
        if(!builder.dry_run()){
            builder.set_context($1);
            builder.set_defender({});
        }
    }
    ;

query
    : sys_decl COLON action
    {
        if(!builder.dry_run()){
            $3->set_context(builder.get_context());
            builder._dispatcher->cmd_action($3, builder.get_defender());
        }
    }
    ;

action
    : SHOW {
        if(!builder.dry_run())
        {
            auto el = std::make_shared<Show>();
            $$ = std::dynamic_pointer_cast<DispatchedAction>(el);
        }
    }
    | CLASSIC classic {
        $$ = $2;
    }
    | MINIMISE ATTACKERCOST LPAREN complex_id RPAREN interval {
        if(!builder.dry_run()) {
            auto cv = std::dynamic_pointer_cast<CostVar>(
                 builder.get_context()._arena->unfold($4, builder._location));
             if(cv == nullptr)
                 builder.error(builder._location, "expected Cost Variable");
             auto el = std::make_shared<Minimization>($6.first, $6.second, cv);
             $$ = std::dynamic_pointer_cast<DispatchedAction>(el);
         }
    }
    | ATTACKERCOST LPAREN complex_id RPAREN interval {
        if(!builder.dry_run()) {
            auto cv = std::dynamic_pointer_cast<CostVar>(
                builder.get_context()._arena->unfold($3, builder._location));
            if(cv == nullptr)
                builder.error(builder._location, "expected Cost Variable");
            auto el = std::make_shared<Formats>($5.first, $5.second, cv);
            $$ = std::dynamic_pointer_cast<DispatchedAction>(el);
        }
    }
    | ATTACKERPROBABILITY LSQBRCKT LTEQ number RSQBRCKT {
        if(!builder.dry_run()) {
            auto el = std::make_shared<Formats>($4, std::numeric_limits<int>::max(), nullptr);
            $$ = std::dynamic_pointer_cast<DispatchedAction>(el);
        }
    }
    | REFINES LPAREN system RPAREN {
        if(!builder.dry_run()) {
            auto el = std::make_shared<Refinement>($3);
            $$ = std::dynamic_pointer_cast<DispatchedAction>(el);
        }
    }
    ;

system
    : FOR restricted_tree UNDER arena_id
    {
        if(!builder.dry_run())
        {
            $$ = $2;
            $$._arena = $4->shared();
        }
    }
    | FOR restricted_tree { $$ = $2; }
    | restricted_tree { $$ = $1; }
    ;

restricted_tree
    : bool_expr EXCLUDING LBRCKT id_list RBRCKT
    {
        if(!builder.dry_run())
        {
            $$._tree = $1->shared();
            for(auto* as : $4)
            {
                auto action = dynamic_cast<Action*>(as);
                if(action == nullptr)
                    builder.terror(builder._location, "expected a Action");
                $$._excludes.push_back(action);
            }
        }
    }
    | bool_expr {
        if(!builder.dry_run())
        {
            $$._tree = $1->shared();
        }
    }
    ;

classic
    : ATTACKERCOST LPAREN complex_id RPAREN
    {
        if(!builder.dry_run())
        {
            auto el = std::dynamic_pointer_cast<CostVar>(
                builder.top_module()->unfold($3, builder._location));
            if(el == nullptr)
                builder.error(builder._location, "expected Cost Variable");
            auto op = std::make_shared<Classic>(el);
            $$ = std::dynamic_pointer_cast<DispatchedAction>(op);
        }
    }
    | ATTACKERCOST
    {
        if(!builder.dry_run())
        {
            auto op = std::make_shared<Classic>(nullptr);
            $$ = std::dynamic_pointer_cast<DispatchedAction>(op);
        }
    }
    | ATTACKERPROBABILITY
    {
        if(!builder.dry_run())
        {
            auto op = std::make_shared<Classic>();
            $$ = std::dynamic_pointer_cast<DispatchedAction>(op);
        }
    }
    ;

interval
    : LSQBRCKT LTEQ number COMMA number RSQBRCKT
    {
        $$ = std::make_pair($3,$5);
    }
    ;

%%

void
adpp::Parser::error (const location_type& l,
                          const std::string& m)
{
  builder.error(l, m);
}