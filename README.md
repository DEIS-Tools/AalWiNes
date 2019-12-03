# mpls2pda
## Compilation
Requirements for compilation:

    build-essential flex bison cmake libboost-all-dev

For Ubuntu 19.10 (ssh keys already configured for github):

    sudo apt update
    sudo apt install build-essential flex bison cmake libboost-all-dev
    git clone git@github.com:P-RexMPLS/P-Rex.git
    export MOPED_PATH=`pwd`/P-Rex/bin/moped
    git clone git@github.com:petergjoel/mpls2pda.git
    cd mpls2pda
    git submodule init
    git submodule update
    cmake .
    make

## Usage

## Query Syntax
