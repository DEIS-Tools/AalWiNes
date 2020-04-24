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
 * File:   errors.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on May 10, 2019, 9:46 AM
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <exception>
#include <string>
#include <ostream>
#include <utility>

struct base_error : public std::exception {
    std::string _message;

    explicit base_error(std::string m)
            : _message(std::move(m)) {
    }

    const char *what() const noexcept override {
        return _message.c_str();
    }

    virtual void print_json(std::ostream &os) const {
        start(os);
        finish(os);
    }


    friend std::ostream &operator<<(std::ostream &os, const base_error &el) {
        el.print_json(os);
        return os;
    }
protected:
    void start(std::ostream& os) const
    {
        os << R"({"type":")" << type() << R"(","message":")" << what() << '"';
    }

    void finish(std::ostream& os) const
    {
        os << "}";
    }

    [[nodiscard]] virtual std::string type() const
    {
        return "base";
    }
};

struct file_not_found_error : public base_error {
    std::string _path;

    file_not_found_error(std::string message, std::string path)
            : base_error(std::move(message)), _path(std::move(path)) {
    }

    void print_json(std::ostream &os) const override {
        start(os);
        os << R"(", "path":")" + _path << "\"";
        finish(os);
    }

    [[nodiscard]] std::string type() const override
    {
        return "file_not_found";
    }
};

struct illegal_option_error : public base_error {
    std::string _option;
    std::string _value;

    illegal_option_error(std::string message, std::string option, std::string value)
            : base_error(std::move(message)), _option(std::move(option)), _value(std::move(value)) {
    }

    void print_json(std::ostream &os) const override {
        os << R"(", "option":")" + _option + R"(","value":")" + _value + "\"";
    }

    [[nodiscard]] std::string type() const override
    {
        return "illegal_option_error";
    }
};

#endif /* ERRORS_H */

