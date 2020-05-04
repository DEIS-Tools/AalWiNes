# AalWiNes (Aalborg Wien Network Suite)
[![DOI](https://zenodo.org/badge/241874332.svg)](https://zenodo.org/badge/latestdoi/241874332)
## Compilation
Requirements for compilation:

```
build-essential flex bison cmake libboost-all-dev
```

For Ubuntu 19.10 (ssh keys must be already configured for github):

```bash
# install prerequisites 
sudo apt update
sudo apt upgrade
sudo apt install build-essential flex bison cmake libboost-all-dev


# get aalwines (P-Rex v2) and compile
git clone git@github.com:DEIS-Tools/aalwines.git
cd aalwines
mkdir build
cd build
cmake ..
make 
#export MOPED_PATH=`pwd`/../bin/moped
# binary will be in build/bin
```

Option for static linkage -> binary will not need libboost and other libs installed on target system:

```bash
cmake -DAALWINES_BuildBundle=ON ..
```

## Usage Examples

This will run queryfile `query.txt` and weightfile `weight.json` over the network defined in the P-Rex data files, using Post* implementation as engine and producing a trace:
    
```bash
cd bin
./aalwines --topology ../../example_net/Agis-topo.xml --routing ../../example_net/Agis-routing.xml -w ../../example_net/Agis-weight.json -q ../../example_net/Agis-query.q -t -e 2
```

An example `query.txt` (syntax see below):
```
<.> [.#Stockton] .* [Santa_Clara#.] <.> 0 DUAL
<.> [.#Chicago] [^.#Los_Angeles]* [Santa_Clara#.] <.> 1 DUAL
```

An example `weight.json` (syntax see below):
```
[
        [
                {"atom": "tunnels"}
        ],
        [
                {"atom": "failures"},
                {"atom": "hops", "factor": 2}
        ]
]
```

will produce following output:
```json
{
        "network-parsing-time":0.591031, "query-parsing-time":0.0021856,
        "answers":{
        "Q1" : {
                "result":    true,
                "engine":    "Post*",
                "mode":      "OVER",
                "reduction": [75810, 75810],
                "trace":[
                        {"router":"Stockton","stack":["1569^"]},
                        {"ingoing":"iStockton","pre":"1569^","rule":{"weight":0, "via":"Santa_Clara", "ops":[{"swap":"1570"}]}},
                        {"router":"Santa_Clara","stack":["1570"]},
                        {"ingoing":"Stockton","pre":"1570","rule":{"weight":0, "via":"Los_Angeles", "ops":[{"swap":"1571"}]}},
                        {"router":"Los_Angeles","stack":["1571"]}
                ],
                "compilation-time":0.330325,
                "reduction-time":0.0010753,
                "verification-time":0.618661
        },
        "Q2" : {
                "result":    true,
                "engine":    "Post*",
                "mode":      "OVER",
                "reduction": [134673, 134673],
                "trace":[
                        {"router":"Chicago","stack":["2634^"]},
                        {"ingoing":"iChicago","pre":"2634^","rule":{"weight":1, "via":"Seattle", "ops":[{"swap":"2635"}, {"push":"3452"}]}},
                        {"router":"Seattle","stack":["3452","2635"]},
                        {"ingoing":"Chicago","pre":"3452","rule":{"weight":0, "via":"San_Francisco", "ops":[{"swap":"3451"}]}},
                        {"router":"San_Francisco","stack":["3451","2635"]},
                        {"ingoing":"Seattle","pre":"3451","rule":{"weight":0, "via":"Santa_Clara", "ops":[{"swap":"3450"}]}},
                        {"router":"Santa_Clara","stack":["3450","2635"]},
                        {"ingoing":"San_Francisco","pre":"3450","rule":{"weight":0, "via":"Los_Angeles", "ops":["pop"]}},
                        {"router":"Los_Angeles","stack":["2635"]}
                ],
                "compilation-time":0.526041,
                "reduction-time":0.0024805,
                "verification-time":1.95766
        }
        }
}
```

## Query Syntax

A query file contains one or more queries. They can be separated by space or new line. Each query consists out of following parts:

```
<preCondition> path <postCondition> linkFailures mode
```
| part           | type       | description |
| -------------: | ---------- | ----------- |
| `preCondition` | regex-list | labels before first router |
| `path`         | regex-list | path through the network |
| `postCondition`| regex-list | labels after last router |
| `linkFailures` | number     | maximum failed links |
| `mode`         | enum       | simulation mode: one out of OVER, UNDER, DUAL, EXACT |

The type regex-list is a space separated list of regular expressions (syntax see below).
For `preCondition` and `postCondition` it defines the labels on the stack of the packet. Packets must have at least 1 label when entering or leaving the network. For `path` it defines the interfaces(routers) the packet must, can or must not follow.

The `mode` can be OVER or UNDER. DUAL is a combination of OVER and UNDER. EXACT is not supported yet.

## Weight Syntax
 
A weight file contains a priority in the outer array. The inner array contains the linear combination of atoms. 

```
[
        [
                {"atom": ATOM, "factor": NUM},
                ...
        ], 
        ...
]
```

ATOM = {`hops`, `failures`, `tunnels`, `distance` <!-- , `latency`, `zero` --> }

NUM = {0,1,2,...} factor is optional and NUM default is 1.

The different priority groups, of linear combinations, represent the order of which the weights are compared. The second priority group will be considered if two similar weighted traces in the first linear combination are equal.

`hops` minimize the amount of links in the trace.

`failures` minimize the amount of failures in the trace.

`tunnels` minimize the amount of push operations in the trace.

`distance` minimize the accumulated distance between routers estimated from router coordinates.

## Regular Expression Syntax (regex)

Every regular expression in the regex-list is built out of following components:

| syntax          | description |
| --------------: | ----------- |
| regex `&` regex | AND: both regex must be fulfilled |
| regex `\|` regex | OR: one or both regex must be fulfilled |
| `.`             | matches everything |
| regex`+`        | multiple: regex must match once or multiple times |
| regex`*`        | optional multiple: regex must match zero, one or multiple times |
| regex`?`        | optional: regex must match zero or one time |
| `[`atom-list`]` | matches the atom_list (see below) |
| `[^`atom-list`]`| matches everything except the atom_list |
| `ip`            | matches any ip address |
| `mpls`          | matches any non-sticky mpls label |
| `smpls`         | matches any sticky mpls label |
| `(`regex-list`)`| must match the given sub regex-list |

The atom-list contains a comma separated list of atoms (for `path`) or labels(for `preCondition` and `postCondition`).

Only labels are allowed in `preCondition` and `postCondition` and only `Atoms` in the `path`.

The semantics of an atom-list is that of union (OR) and the negation-symbol `^` is intepreted as complement of the following `atom-list`.

## Atom Syntax
An atom defines a hop between routers via their exit and entry interfaces. An atom-list with one entry looks like:

`[`exit_if`#`entry_if`]`

with multiple entries the syntax is:

`[`exit_if1`#`entry_if1`,`exit_if2`#`entry_if2`,`...`]`

Following possibilities can be used for entry_if and exit_if:

| syntax          | description |
| --------------: | ----------- |
| name            | the router name (matches any interface of that router) |
| name`.`name     | specific interface of router |
| `.`             | any interface on any router |

name itself can be either an identifier (starting with a character; defining an exact interface) or a literal (a double-quoted regular expression matching interfaces `"`regex`"`). 
The regex-matching of `name` is implemented via `boost::basic_regex` and follows this semantics.

## Label Syntax
The syntax of a list with labels is:

| syntax          | description |
| --------------: | ----------- |
| `[$`label`]`        | A sticky label. Can be used with all following types |
| `[`label1`,`label2`]`        | A list of labels. Can be used with all following types |
| `[`number`]`        | A mpls label with number |
| `[`number`/`mask`]`   | A range of mpls labels, specified by number and a mask in bits |
| `[`ip`]`            | An ip label. Can be an ipv4 or an ipv6 address |
| `[`ip`/`mask`]`   | An ip network. An ipv4 or an ipv6 address together with a mask in bits |
