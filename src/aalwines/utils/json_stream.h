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
 *  Copyright Morten K. Schou
 */

/*
 * File:   json_stream.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on October 9, 2020
 */

#ifndef AALWINES_JSON_STREAM_H
#define AALWINES_JSON_STREAM_H

#include <json.hpp>
#include <string>
#include <iostream>

using json = nlohmann::json;

class json_stream {
public:
    explicit json_stream(size_t indent_size = 4, std::ostream &out = std::cout) : indent_size(indent_size), out(out) { };
    virtual ~json_stream() {
        close();
    }

    void begin_object(const std::string& key) {
        start_entry(key);
        started = false;
    }
    void end_object(){
        if (!started) {
            out << "{}";
            started = true;
        } else {
            out << std::endl;
            indent--;
            do_indent();
            out << "}";
        }
    }

    void entry(const std::string& key, const json& value) {
        start_entry(key);
        out << value.dump();
    }

    void close() {
        while (indent > 0) {
            end_object();
        }
        started = false;
    }

private:
    void do_indent() {
        for (size_t i = 0; i < indent * indent_size; ++i) {
            out << ' ';
        }
    }
    void start_entry(const std::string& key){
        if (!started) {
            out << "{" << std::endl;
            started = true;
            indent++;
        } else {
            out << ',' << std::endl;
        }
        do_indent();
        out << "\"" << key << "\" : ";
    }

    bool started = false;
    size_t indent = 0;
    size_t indent_size;
    std::ostream &out;
};


#endif //AALWINES_JSON_STREAM_H
