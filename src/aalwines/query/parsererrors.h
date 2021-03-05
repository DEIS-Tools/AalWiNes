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

//
// Created by Peter G. Jensen on 10/18/18.
//

#ifndef PROJECT_ERRORS_H
#define PROJECT_ERRORS_H

#include "location.hh"

#include <string>
#include <stdexcept>
#include <sstream>
#include "aalwines/utils/errors.h"

namespace aalwines {

    struct base_parser_error : public base_error {

        base_parser_error(const location &l, std::string m)
        : base_error(m), _location() {
            std::stringstream ss;
            ss << "[" << l << "]";
            _location = ss.str();
        }

        explicit base_parser_error(std::string m)
        : base_error(std::move(m)) {
        }

        std::string _location;

        const char *what() const noexcept override {
            return _message.c_str();
        }

        virtual std::string prefix() const {
            return "base error ";
        }

        void print(std::ostream &os) const override {
            os << prefix() << " " << _location << " : " << what() << std::endl;
        }

        friend std::ostream &operator<<(std::ostream &os, const base_parser_error &el) {
            el.print(os);
            return os;
        }
    };

    struct file_not_found_error : public base_parser_error {
        using base_parser_error::base_parser_error;

        std::string prefix() const override {
            return "file not found error";
        }
    };

    struct syntax_error : public base_parser_error {
        using base_parser_error::base_parser_error;

        std::string prefix() const override {
            return "syntax error";
        }
    };

    struct type_error : public base_parser_error {
        using base_parser_error::base_parser_error;

        std::string prefix() const override {
            return "type error";
        }
    };

    struct undeclared_error : public base_parser_error {

        undeclared_error(const location &l, const std::string &name, const std::string &m)
        : base_parser_error("variable " + name + " " + m) {
        }

        std::string prefix() const override {
            return "undeclared error";
        }
    };

    struct redeclare_error : public base_parser_error {

        redeclare_error(const location &l,
                const std::string &name,
                const std::string &message,
                const location &ol)
        : base_parser_error("redeclaration of " + name + " " + message), _other_location(ol) {
        };

        std::string prefix() const override {
            return "redeclare error";
        }

        location _other_location;
    };
}

#endif //PROJECT_ERRORS_H
