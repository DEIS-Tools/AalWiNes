//
// Created by dank on 4/6/20.
//

#ifndef AALWINES_TOPOLOGYZOOBUILDER_H
#define AALWINES_TOPOLOGYZOOBUILDER_H

#include <aalwines/model/Network.h>

#include <utility>

namespace aalwines {
    class TopologyZooBuilder {
    public:
        static Network parse(const std::string& gml);
    };
}


#endif //AALWINES_TOPOLOGYZOOBUILDER_H