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
