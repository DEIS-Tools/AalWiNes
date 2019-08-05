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

// originates from the verifypn-project.

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <ctime>
#include <sstream>


class stopwatch {
    bool _running = false;
    clock_t _start;
    clock_t _stop;

public:
    double started() const { return _start; }
    double stopped() const { return _stop; }
    bool running() const { return _running; }
    stopwatch(bool r = true) : _running(r), _start(clock()), _stop(clock()){}
    void start() {
        if(!_running)
        {
            _running = true;
            _start = clock() - (_stop - _start);
        }
    }
    void stop() {
        if(_running)
        {
            _stop = clock();
            _running = false;
        }
    }
    double duration() const { return ( (double(_stop - _start))*1000)/CLOCKS_PER_SEC; }

    std::ostream &operator<<(std::ostream &os){
        os << duration() << " ms";
        return os;
    }
};

#endif // STOPWATCH_H
