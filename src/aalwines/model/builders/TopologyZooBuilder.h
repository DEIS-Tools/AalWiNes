//
// Created by dank on 4/6/20.
//

#ifndef AALWINES_TOPOLOGYZOOBUILDER_H
#define AALWINES_TOPOLOGYZOOBUILDER_H

#include <aalwines/model/Network.h>

#include <utility>

namespace aalwines {
    class parse_router {
    public:
        parse_router(size_t id, std::string name, std::pair<double,double> coordinates) : _id(id), _name(std::move(name)), _coordinates(std::move(coordinates)){};
        size_t get_id() { return _id; };

    private:
        size_t _id;
        std::string _name;
        std::pair<double,double> _coordinates;
    };
    class parse_link {
    public:
        parse_link(size_t f_id, size_t t_id) : _t_id(t_id), _f_id(f_id) {};
    private:
        size_t _t_id;
        size_t _f_id;
    };
    class TopologyZooBuilder {
    public:
        static Network parse(const std::string& gml, std::ostream& warnings);
    };
}


#endif //AALWINES_TOPOLOGYZOOBUILDER_H