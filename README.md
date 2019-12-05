# mpls2pda
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

# get P-Rex v1 for moped binary and sample data
#   maybe you want to add MOPED_PATH to your .bashrc
git clone git@github.com:P-RexMPLS/P-Rex.git
export MOPED_PATH=`pwd`/P-Rex/bin/moped

# get mpls2pda (P-Rex v2) and compile
git clone --recurse-submodules git@github.com:petergjoel/mpls2pda.git
cd mpls2pda
mkdir build
cd build
cmake ..
make
# binary will be in build/bin
```

Option for static linkage -> binary will not need libboost and other libs installed on target system:

```bash
cmake -DMPLS2PDA_BuildBundle=ON ..
```

## Usage Examples

This will run queryfile `query.txt` over the network defined in the P-Rex data files, using moped as external engine and producing a trace:
    
```bash
cd bin
./mpls2pda --topology ../../../P-Rex/res/nestable/topo.xml --routing ../../../P-Rex/res/nestable/routing.xml -e 1 -q query.txt -t
```

An example `query.txt` (syntax see below):
```
<.> .* [s2#.] .* <.> 0 OVER
<.> .* [s2#.] .* <.> 0 UNDER
```

will produce following output:
```json
{
        "network-parsing-time":0.0192828, "query-parsing-time":0.0011643,
        "answers":{
        "Q1" : {
                "result":true,
                "reduction":[1179, 1179],
                "trace":[
                        {"router":"s2","stack":["10"]},
                        {"pre":"10","rule":{"weight":0, "via":"s4"}},
                        {"router":"s4","stack":["10"]}
                ],
                "compilation-time":0.0387264,
                "reduction-time":1.07e-05,
                "verification-time":0.0340714
        },
        "Q2" : {
                "result":true,
                "reduction":[1179, 1179],
                "trace":[
                        {"router":"s2","stack":["10"]},
                        {"pre":"10","rule":{"weight":0, "via":"s4"}},
                        {"router":"s4","stack":["10"]}
                ],
                "compilation-time":0.038969,
                "reduction-time":8.4e-06,
                "verification-time":0.0349733
        }

}}
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
For `preCondition` and `postCondition` it defines the labels on the stack of the packet. For `path` it defines the interfaces(routers) the packet must, can or must not follow.

The `mode` can be OVER or UNDER. DUAL is a combination of OVER and UNDER. EXACT is not supported yet.

## Regular Expression Syntax (regex)

Every regular expression in the regex-list is built out of following components:

| syntax          | description |
| --------------: | ----------- |
| regex `&` regex | AND: both regex must be fulfilled |
| regex `|` regex | OR: one or both regex must be fulfilled |
| `.`             | matches everything |
| regex`+`        | multiple: regex must match once or multiple times |
| regex`*`        | optional multiple: regex must match zero, one or multiple times |
| regex`?`        | optional: regex must match zero or one time |
| `[`atom_list`]` | matches the atom_list (see below) |
| `[^`atom_list`]`| matches everything except the atom_list |
| `ip`            | matches any ip address |
| `mpls`          | matches any mpls label |
| `smpls`         | matches any sticky mpls label |
| `(`regex-list`)`| must match the given sub regex-list |


