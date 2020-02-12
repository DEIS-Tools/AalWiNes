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
 * File:   TestPDAFactory.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 03-01-2020.
 */

#ifndef PDAAAL_TESTPDAFACTORY_H
#define PDAAAL_TESTPDAFACTORY_H

#include "pdaaal/model/SimplePDAFactory.h"

namespace pdaaal {

    class TestPDAFactory : public SimplePDAFactory<char> {
    public:
        TestPDAFactory();

    protected:
        const std::vector<size_t>& states() override;
        std::vector<rule_t> rules(size_t) override;

    private:
        std::vector<size_t> _states;
    };
}


#endif //PDAAAL_TESTPDAFACTORY_H
