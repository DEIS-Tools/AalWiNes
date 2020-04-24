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
        location _location;
        base_parser_error(const location &l, std::string m)
        : base_error(m), _location(l) {
        }

        explicit base_parser_error(std::string m)
        : base_error(std::move(m)) {
        }


        const char *what() const noexcept override {
            return _message.c_str();
        }

        void print_json(std::ostream& os) const override
        {
            start(os);
            if(_location.begin.filename)
                os << R"(,"inputName":")" << _location.begin.filename;
            os << R"(,"lineStart":")" << _location.begin.line;
            os << R"(,"lineEnd":")" << _location.end.line;
            os << R"(,"columnStart":")" << _location.begin.column;
            os << R"(,"columnEnd":")" << _location.end.column;
            finish(os);
        }

    protected:
        [[nodiscard]] std::string type() const override {
            return "parser_error";
        }
    };


    struct syntax_error : public base_parser_error {
        using base_parser_error::base_parser_error;
    protected:
        [[nodiscard]] std::string type() const override {
            return "syntax_error";
        }
    };

    struct type_error : public base_parser_error {
        using base_parser_error::base_parser_error;
    protected:
        [[nodiscard]] std::string type() const override {
            return "type_error";
        }
    };

    struct no_match_error : public base_parser_error {
        using base_parser_error::base_parser_error;
    protected:
        [[nodiscard]] std::string type() const override {
            return "no_match_error";
        }
    };

}

#endif //PROJECT_ERRORS_H
