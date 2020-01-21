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

/*
 *  Copyright Peter G. Jensen
 */

/* 
 * File:   JuniperParser.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on August 16, 2019, 11:38 AM
 */

#include "JuniperBuilder.h"
#include "model/Network.h"

#include <boost/filesystem/path.hpp>

#include <fstream>
#include <sstream>
#include <map>

namespace mpls2pda
{
    Network JuniperBuilder::parse(const std::string& network, std::ostream& warnings, bool skip_pfe)
    {
        std::vector<std::unique_ptr<Router> > routers;
        std::vector<const Interface*> interfaces;
        ptrie::map<Router*> mapping;
        
        // lets start by creating empty router-objects for all the alias' we have
        using tp = std::tuple<std::string, std::string, std::string>;
        std::vector<tp> configs;
        std::string line;
        std::ifstream data(network);
        auto wd = boost::filesystem::path(network).parent_path();
        if (!data.is_open()) {
            std::cerr << "Could not open " << network << std::endl;
            exit(-1);
        }

        while (getline(data, line)) {
            if (line.size() == 0)
                continue;
            size_t en = 0;
            for (; en < line.length(); ++en) if (line[en] == ':') break;
            std::string alias = line.substr(0, en);
            {
                auto id = routers.size();
                routers.emplace_back(std::make_unique<Router>(id));
                std::string tmp;
                bool some = false;
                std::istringstream ss(alias);
                while (std::getline(ss, tmp, ',')) {
                    if (tmp.size() == 0) continue;
                    some = true;
                    auto res = mapping.insert((unsigned char*) tmp.c_str(), tmp.size());
                    if (!res.first) {
                        auto oid = mapping.get_data(res.second)->index();
                        if (oid != id) {
                            std::cerr << "error: Duplicate definition of \"" << tmp << "\", previously found in entry " << oid << std::endl;
                            exit(-1);
                        }
                        else {
                            warnings << "Warning: entry " << id << " contains the duplicate alias \"" << tmp << "\"" << std::endl;
                            continue;
                        }
                    }
                    mapping.get_data(res.second) = routers.back().get();
                    routers.back()->add_name(tmp);
                }
                if (!some) {
                    std::cerr << "error: Empty name-string declared for entry " << id << std::endl;
                    exit(-1);
                }
                configs.emplace_back();
                if (en + 1 < line.size()) {
                    // TODO: cleanup this pasta.
                    ++en;
                    size_t config = en;
                    for (; config < line.size(); ++config) if (line[config] == ':') break;
                    if (en < config)
                        std::get<0>(configs.back()) = line.substr(en, (config - en));
                    ++config;
                    en = config;
                    for (; config < line.size(); ++config) if (line[config] == ':') break;
                    if (en < config)
                        std::get<1>(configs.back()) = line.substr(en, (config - en));
                    ++config;
                    en = config;
                    for (; config < line.size(); ++config) if (line[config] == ':') break;
                    if (en < config)
                        std::get<2>(configs.back()) = line.substr(en, (config - en));

                    if (std::get<0>(configs.back()).empty() || std::get<1>(configs.back()).empty()) {
                        std::cerr << "error: Either no configuration files are specified, or both adjacency and mpls is specified." << std::endl;
                        std::cerr << line << std::endl;
                        exit(-1);
                    }
                    if (!std::get<2>(configs.back()).empty() && (std::get<1>(configs.back()).empty() || std::get<0>(configs.back()).empty())) {
                        std::cerr << "error: next-hop-table requires definition of other configuration-files." << std::endl;
                        std::cerr << line << std::endl;
                        exit(-1);
                    }
                }
            }
        }
        std::unordered_map<const Interface*, uint32_t> ipmap;
        for (size_t i = 0; i < configs.size(); ++i) {
            if (std::get<0>(configs[i]).empty()) {
                warnings << "warning: No adjacency info for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
            }
            else {
                auto path = wd;
                path.append(std::get<0>(configs[i]));
                std::ifstream stream(path.string());
                if (!stream.is_open()) {
                    std::cerr << "error: Could not open adjacency-description for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
                    exit(-1);
                }
                try {
                    router_parse_adjacency(*routers[i], stream, routers, mapping, interfaces, warnings, ipmap);
                }
                catch (base_error& ex) {
                    std::cerr << ex.what() << "\n";
                    std::cerr << "while parsing : " << std::get<0>(configs[i]) << std::endl;
                    stream.close();
                    exit(-1);
                }
                stream.close();
            }
            if (std::get<1>(configs[i]).empty()) {
                warnings << "warning: No routingtables for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
            }
            else {
                auto path = wd;
                path.append(std::get<1>(configs[i]));
                std::ifstream stream(path.string());
                if (!stream.is_open()) {
                    std::cerr << "error: Could not open routing-description for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
                    exit(-1);
                }
                std::ifstream id;
                if (!std::get<2>(configs[i]).empty() && !skip_pfe) {
                    auto path = wd;
                    path.append(std::get<2>(configs[i]));
                    id.open(path.string());
                    if(!id.is_open())
                    {
                        std::cerr << "error: Could not open PFE-description for index " << i << " (i.e. router " << routers[i]->name() << ", file " << path.string() << ")" << std::endl;
                        exit(-1);                        
                    }
                }
                try {
                    router_parse_routing(*routers[i], stream, id, interfaces, warnings, skip_pfe);
                }
                catch (base_error& ex) {
                    std::cerr << ex.what() << "\n";
                    std::cerr << "while parsing : " << std::get<1>(configs[i]) << std::endl;
                    stream.close();
                    exit(-1);
                }
                stream.close();
            }
        }
        
        for (auto& r : routers) {
            r->pair_interfaces(interfaces, [&ipmap](const Interface* i1, const Interface* i2) {
                auto ip1 = ipmap[i1];
                auto ip2 = ipmap[i2];
                auto diff = std::max(ip1, ip2) - std::min(ip1, ip2);
                return diff == 1 && i1->target() == i2->source() && i2->target() == i1->source();
            });
        }
        
        Router::add_null_router(routers, interfaces, mapping);
   
        return Network(std::move(mapping), std::move(routers), std::move(interfaces));
    }

    void JuniperBuilder::router_parse_adjacency(Router& router, std::istream& data, std::vector<std::unique_ptr<Router> >& routers, ptrie::map<Router*>& mapping, std::vector<const Interface*>& all_interfaces, std::ostream& warnings, std::unordered_map<const Interface*, uint32_t>& ipmap)
    {
        rapidxml::xml_document<> doc;
        std::vector<char> buffer((std::istreambuf_iterator<char>(data)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
        if (!doc.first_node()) {
            std::stringstream e;
            e << "Ill-formed XML-document for " << router.name() << std::endl;
            throw base_error(e.str());
        }
        auto info = doc.first_node()->first_node("isis-adjacency-information");
        if (!info) {
            std::stringstream e;
            e << "Missing an \"isis-adjacency-information\"-tag for " << router.name() << std::endl;
            throw base_error(e.str());
        }
        auto n = info->first_node("isis-adjacency");
        if (n) {
            do {
                // we look up in order of ipv6, ipv4, name
                Router* next = nullptr;
                auto state = n->first_node("adjacency-state");
                if (state && strcmp(state->value(), "Up") != 0)
                    continue;
                auto addv6 = n->first_node("ipv6-address");
                if (addv6) {
                    auto res = mapping.exists((unsigned char*) addv6->value(), strlen(addv6->value()));
                    if (!res.first) {
                        warnings << "warning: Could not find ipv6 " << addv6->value() << " in adjacency of " << router.name() << std::endl;
                    }
                    else next = mapping.get_data(res.second);

                }
                auto addv4 = n->first_node("ip-address");
                if (addv4) {
                    auto res = mapping.exists((unsigned char*) addv4->value(), strlen(addv4->value()));
                    if (!res.first) {
                        warnings << "warning: Could not find ipv4 " << addv4->value() << " in adjacency of " << router.name() << std::endl;
                    }
                    else if (next) {
                        auto o = mapping.get_data(res.second);
                        if (next != o) {
                            std::stringstream e;
                            e << "Mismatch between ipv4 and ipv6 in adjacency of " << router.name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                            throw base_error(e.str());
                        }
                    }
                    else next = mapping.get_data(res.second);
                }
                auto name_node = n->first_node("system-name");
                if (name_node) {
                    auto res = mapping.exists((unsigned char*) name_node->value(), strlen(name_node->value()));
                    if (!res.first) {
                        warnings << "warning: Could not find name " << name_node->value() << " in adjacency of " << router.name() << std::endl;
                    }
                    else if (next) {
                        auto o = mapping.get_data(res.second);
                        if (next != o) {
                            std::stringstream e;
                            e << "Mismatch between ipv4 or ipv6 and name in adjacency of " << router.name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                            throw base_error(e.str());
                        }
                    }
                    else next = mapping.get_data(res.second);
                }

                if (next == nullptr) {
                    warnings << "warning: no matches router for adjecency-list of " << router.name() << " creating external node" << std::endl;
                    auto nnid = routers.size();
                    routers.emplace_back(std::make_unique<Router>(nnid));
                    next = routers.back().get();
                    if (addv4) {
                        auto res = mapping.insert((unsigned char*) addv4->value(), strlen(addv4->value()));
                        mapping.get_data(res.second) = next;
                        std::string nn = addv4->value();
                        next->add_name(nn);
                        warnings << "\t" << nn << std::endl;
                    }
                    if (addv6) {
                        auto res = mapping.insert((unsigned char*) addv6->value(), strlen(addv6->value()));
                        mapping.get_data(res.second) = next;
                        std::string nn = addv6->value();
                        next->add_name(nn);
                        warnings << "\t" << nn << std::endl;
                    }
                    if (name_node) {

                        auto res = mapping.insert((unsigned char*) name_node->value(), strlen(name_node->value()));
                        mapping.get_data(res.second) = next;
                        std::string nn = name_node->value();
                        next->add_name(nn);
                        warnings << "\t" << nn << std::endl;
                    }

                }

                auto iname = n->first_node("interface-name");
                if (!iname) {
                    std::stringstream e;
                    e << "name not given to interface on adjacency of " << router.name() << std::endl;
                    throw base_error(e.str());
                }
                std::string iface = iname->value();
                auto res = router.get_interface(all_interfaces, iface, next);
                if (res == nullptr) {
                    std::stringstream e;
                    e << "Could not find interface " << iface << std::endl;
                    throw base_error(e.str());
                }
                const char* str = nullptr;
                if (auto ip = n->first_node("ip-address"))
                    str = ip->value();

                ipmap[res] = parse_ip4(str);
            }
            while ((n = n->next_sibling("isis-adjacency")));
        }
        else {
            std::stringstream e;
            e << "No isis-adjacency tags in " << router.name() << std::endl;
            throw base_error(e.str());
        }
    }

    void JuniperBuilder::router_parse_routing(Router& router, std::istream& data, std::istream& indirect, std::vector<const Interface*>& all_interfaces, std::ostream& warnings, bool skip_pfe)
    {
        rapidxml::xml_document<> doc;
        std::vector<char> buffer((std::istreambuf_iterator<char>(data)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
        if (!doc.first_node()) {
            std::stringstream e;
            e << "Ill-formed XML-document for " << router.name() << std::endl;
            throw base_error(e.str());
        }
        auto info = doc.first_node()->first_node("forwarding-table-information");
        if (!info) {
            std::stringstream e;
            e << "Missing an \"forwarding-table-information\"-tag for " << router.name() << std::endl;
            throw base_error(e.str());
        }
        auto n = info->first_node("route-table");
        if (n == nullptr) {
            std::stringstream e;
            e << "Found no \"route-table\"-entries" << std::endl;
            throw base_error(e.str());
        }
        else {
            ptrie::map<std::pair < std::string, std::string>> indir;
            if (indirect.good() && indirect.peek() != EOF) {
                std::string line;
                while (std::getline(indirect, line)) {
                    size_t i = 0;
                    for (; i < line.size(); ++i)
                        if (line[i] != ' ') break;
                    size_t j = i;
                    for (; j < line.size(); ++j) {
                        if (!std::isdigit(line[j]))
                            break;
                    }
                    // TODO: a lot of pasta here too.
                    if (j <= i) continue;
                    auto rn = line.substr(i, (j - i));
                    i = j;
                    // find type
                    for (; i < line.size(); ++i)
                        if (line[i] != ' ') break;
                    // skip type
                    for (; i < line.size(); ++i)
                        if (line[i] == ' ') break;
                    // find eth
                    for (; i < line.size(); ++i)
                        if (line[i] != ' ') break;
                    j = i;
                    for (; i < line.size(); ++i)
                        if (line[i] == ' ') break;
                    if (i <= j) {
                        warnings << "warning; no iface for " << rn << std::endl;
                        continue;
                    }
                    auto iface = line.substr(j, (i - j));
                    for (; i < line.size(); ++i)
                        if (line[i] != ' ') break;
                    for (; i < line.size(); ++i)
                        if (line[i] == ' ') break;
                    if (i == line.size()) {
                        // next line
                        if (!std::getline(indirect, line)) {
                            std::stringstream e;
                            e << "unexpected end of file" << std::endl;
                            throw base_error(e.str());
                        }
                        i = 0;
                    }
                    for (; i < line.size(); ++i)
                        if (line[i] != ' ') break;

                    j = i;
                    for (; i < line.size(); ++i)
                        if (line[i] == ' ') break;
                    auto conv = line.substr(j, (i - j));
                    if (conv.empty()) {
                        warnings << "warning: skip rule without protocol" << std::endl;
                        warnings << "\t" << line << std::endl;
                    }

                    auto res = indir.insert((const unsigned char*) rn.c_str(), rn.size());
                    auto& data = indir.get_data(res.second);
                    if (!res.first) {
                        if (data.first != iface) {
                            warnings << "warning: duplicate rule " << rn << " expected \"" << data.first << ":" << data.second << "\" got \"" << iface << "\"" << std::endl;
                        }
                    }
                    else {
                        data.first.swap(iface);
                    }

                    if (!res.first) {
                        if (data.second != conv) {
                            warnings << "warning: duplicate rule expected \"" << data.first << ":" << data.second << "\" got \"" << conv << "\"" << std::endl;
                        }
                    }
                    else {
                        data.second.swap(conv);
                    }
                }
            }
            while (n) {
                try {
                    auto rt = parse_table(n, indir, &router, all_interfaces, warnings, skip_pfe);
                    for (auto& inf : router.interfaces()) {
                        // add table to all interfaces (needed as some routers can have rules matching
                        // both TOS and interface
                        if(!inf->table().merge(rt, *inf, warnings))
                            warnings << "warning: nondeterministic routing discovered for " << router.name() << " in table " << n->first_node("table-name")->value() << std::endl;    
                        for(auto& e : inf->table().entries())
                            assert(e._ingoing == inf.get());
                    }
                }
                catch (base_error& ex) {
                    std::stringstream e;
                    e << ex._message << " :: in router " << router.name() << std::endl;
                    throw base_error(e.str());
                }
                n = n->next_sibling("route-table");
            }
        }
    }

    RoutingTable JuniperBuilder::parse_table(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string, std::string> >& indirect, 
                                             Router* parent, std::vector<const Interface*>& all_interfaces, std::ostream& warnings, bool skip_pfe)
    {
        RoutingTable nr;
        if (node == nullptr)
            return nr;


        std::string name = "";
        if (auto nn = node->first_node("table-name"))
            name = nn->value();

        if (auto tn = node->first_node("address-family")) {
            if (strcmp(tn->value(), "MPLS") != 0) {
                std::stringstream e;
                e << "Not MPLS-type address-family routing-table (\"" << name << "\")" << std::endl;
                throw base_error(e.str());
            }
        }

        auto rule = node->first_node("rt-entry");
        if (rule == nullptr) {
            std::stringstream e;
            e << "no entries in routing-table \"" << name << "\"" << std::endl;
            throw base_error(e.str());
        }

        while (rule) {
            std::string tl = rule->first_node("rt-destination")->value();
            auto pos = tl.find("(S=0)");
            RoutingTable::entry_t& entry = nr.push_entry();

            int sticky = std::numeric_limits<int>::max();
            if (pos != std::string::npos) {
                if (pos != tl.size() - 5) {
                    std::stringstream e;
                    e << "expect only (S=0) notation as postfix of <rt-destination> in table " << name << " of router " << parent->name() << std::endl;
                    throw base_error(e.str());
                }
                entry._sticky_label = false;
                tl = tl.substr(0, pos);
            }
            else
            {
                entry._sticky_label = true;
                sticky = 1; 
            }
            if (std::all_of(std::begin(tl), std::end(tl), [](auto& c) {return std::isdigit(c);})) 
            {
                entry._top_label.set_value(entry._sticky_label ? Query::STICKY_MPLS : Query::MPLS, atoll(tl.c_str()), 0);
            }
            else if (tl == "default") {
                // we ignore these! (I suppose, TODO, check)
                rule = rule->next_sibling("rt-entry");
                nr.pop_entry();
                continue;
            }
            else {
                auto inf = parent->get_interface(all_interfaces, tl);
                entry._ingoing = inf;
                entry._top_label = Query::label_t::any_ip;
                sticky = 0;
            }

            auto nh = rule->first_node("nh");
            if (nh == nullptr) {
                std::stringstream e;
                e << "no \"nh\" entries in routing-table \"" << name << "\" for \"" << entry._top_label << "\"" << std::endl;
                throw base_error(e.str());
            }
            int cast = 0;
            std::map<size_t, size_t> weights;
            do {
                entry._rules.emplace_back();
                auto& r = entry._rules.back();
                r._weight = parse_weight(nh);
                auto ops = nh->first_node("nh-type");
                bool skipvia = true;
                rapidxml::xml_node<>* nhid = nullptr;
                if (ops) {
                    auto val = ops->value();
                    if (strcmp(val, "unilist") == 0) {
                        if (cast != 0) {
                            std::stringstream e;
                            e << "already in cast" << std::endl;
                            throw base_error(e.str());
                        }
                        cast = 1;
                        entry._rules.pop_back();
                        continue;
                    }
                    else if (strcmp(val, "discard") == 0) {
                        r._type = RoutingTable::DISCARD; // Drop
                    }
                    else if (strcmp(val, "receive") == 0) // check.
                    {
                        r._type = RoutingTable::RECEIVE; // Drops out of MPLS?
                    }
                    else if (strcmp(val, "table lookup") == 0) {
                        r._type = RoutingTable::ROUTE; // drops out of MPLS?
                    }
                    else if (strcmp(val, "indirect") == 0) {
                        if(skip_pfe)
                        {
                            entry._rules.pop_back(); // So is the P-rex semantics?
                            continue;
                        }
                        else
                        {
                            skipvia = false;
                            // lookup in indirect
                            nhid = nh->first_node("nh-index");
                            if (!nhid) {
                                std::stringstream e;
                                e << "expected nh-index of indirect";
                                throw base_error(e.str());
                            }
                        }
                    }
                    else if (strcmp(val, "unicast") == 0) {
                        skipvia = false;
                        // no ops, though.
                    }
                    else {
                        std::string ostr = ops->value();
                        parse_ops(r, ostr, sticky);
                        skipvia = false;
                    }
                }
                auto via = nh->first_node("via");
                if (via && strlen(via->value()) > 0) {
                    if (skipvia) {
                        warnings << "warning: found via \"" << via->value() << "\" in \"" << name << "\" for \"" << entry._top_label << "\"" << std::endl;
                        warnings << "\t\tbut got type expecting no via: " << ops->value() << std::endl;
                    }

                    r._via = parse_via(parent, via, all_interfaces);
                }
                else if (!skipvia && indirect.size() > 0) {
                    if (nhid) {
                        auto val = nhid->value();
                        auto alt = indirect.exists((const unsigned char*) val, strlen(val));
                        if (!alt.first) {
                            std::stringstream e;
                            e << "Could not lookup indirect : " << val << std::endl;
                            e << "\ttype : " << ops->value() << std::endl;
                            throw base_error(e.str());
                        }
                        else {
                            auto& d = indirect.get_data(alt.second);
                            r._via = parent->get_interface(all_interfaces, d.first);
                        }
                    }
                    else {
                        warnings << "warning: found no via in \"" << name << "\" for \"" << entry._top_label << "\"" << std::endl;
                        warnings << "\t\tbut got type: " << ops->value() << std::endl;
                        warnings << std::endl;
                    }
                }
                else if(r._type == RoutingTable::MPLS)
                {
                    std::stringstream e;
                    e << "No target found for : " << name  << " from " << tl << std::endl;
                    e << "\ttype : " << ops->value() << std::endl;
                    if(nhid)
                        e << "\tindirect " << nhid->value() << std::endl;
                    throw base_error(e.str());                    
                }
                assert(r._via || r._type != RoutingTable::MPLS);
                weights[r._weight] = 0;
            }
            while ((nh = nh->next_sibling("nh")));
            
            // align weights of rules
            size_t wcnt = 0;
            for(auto& w : weights)
            {
                w.second = wcnt;
                ++wcnt;
            }
            for(auto& r : entry._rules)
            {
                r._weight = weights[r._weight];
            }
            
            rule = rule->next_sibling("rt-entry");
        }
        nr.sort();
        return nr;
    }

    void JuniperBuilder::parse_ops(RoutingTable::forward_t& f, std::string& ostr, int sticky)
    {
        auto pos = ostr.find("(top)");
        if (pos != std::string::npos) {
            if (pos != ostr.size() - 5) {
                std::stringstream e;
                e << "expected \"(top)\" predicate at the end of <nh-type> only." << std::endl;
                throw base_error(e.str());
            }
            ostr = ostr.substr(0, pos);
        }
        // parse ops
        bool parse_label = false;
        f._ops.emplace_back();
        for (size_t i = 0; i < ostr.size(); ++i) {
            if (ostr[i] == ' ') continue;
            if (ostr[i] == ',') continue;
            if (!parse_label) {
                if (ostr[i] == 'S') {
                    f._ops.back()._op = RoutingTable::SWAP;
                    i += 4;
                    parse_label = true;
                }
                else if (ostr[i] == 'P') {
                    if (ostr[i + 1] == 'u') {
                        f._ops.back()._op = RoutingTable::PUSH;
                        parse_label = true;
                        i += 4;
                    }
                    else if (ostr[i + 1] == 'o') {
                        f._ops.back()._op = RoutingTable::POP;
                        i += 2;
                        f._ops.emplace_back();
                        continue;
                    }
                }

                if (parse_label) {
                    while (i < ostr.size() && ostr[i] == ' ') {
                        ++i;
                    }
                    if (i != ostr.size() && std::isdigit(ostr[i])) {
                        size_t j = i;
                        while (j < ostr.size() && std::isdigit(ostr[j])) {
                            ++j;
                        }
                        auto n = ostr.substr(i, (j - i));
                        auto olabel = std::atoi(n.c_str());
                        f._ops.back()._op_label.set_value(Query::MPLS, olabel, 0);
                        i = j;
                        parse_label = false;
                        f._ops.emplace_back();
                        continue;
                    }
                }
                std::stringstream e;
                e << "unexpected operation type \"" << (&ostr[i]) << "\"." << std::endl;
                throw base_error(e.str());
            }
        }
        f._ops.pop_back();
        assert(!f._ops.empty());
        //std::reverse(f._ops.begin(), f._ops.end());
        if(sticky >= 0)
        {
            int depth = sticky;
            for(RoutingTable::action_t& op : f._ops)
            {
                switch(op._op)
                {
                case RoutingTable::PUSH:
                    if(depth == 0)
                        op._op_label.set_type(Query::STICKY_MPLS);
                    ++depth;
                    break;
                case RoutingTable::POP:
                    --depth;
                    if(depth < 0) 
                    {
                        std::stringstream e;
                        e << "unexpected pop below stack-size : " << ostr << std::endl;
                        throw base_error(e.str());                            
                    }
                    break;
                case RoutingTable::SWAP:
                    if(depth == 1)
                        op._op_label.set_type(Query::STICKY_MPLS);
                    break;
                }
            }
        }
    }
    
    Interface* JuniperBuilder::parse_via(Router* parent, rapidxml::xml_node<char>* via, std::vector<const Interface*>& all_interfaces)
    {
        std::string iname = via->value();
        for (size_t i = 0; i < iname.size(); ++i) {
            if (iname[i] == ' ') {
                iname = iname.substr(0, i);
                break;
            }
        }
        if (iname.find("lsi.") == 0) {
            // self-looping interface
            auto inf = parent->get_interface(all_interfaces, iname, parent);
            assert(inf);
            assert(inf->source() == inf->target());
            return inf;
        }
        else {
            return parent->get_interface(all_interfaces, iname);
        }
    }

    int JuniperBuilder::parse_weight(rapidxml::xml_node<char>* nh)
    {
        auto nhweight = nh->first_node("nh-weight");
        if (nhweight) {
            auto val = nhweight->value();
            auto len = strlen(val);
            if (len > 1 && val[0] == '0' && val[1] == 'x')
                return std::stoull(&(val[2]), nullptr, 16);
            else
                return atoll(val);
        }
        return 0;
    }

}
