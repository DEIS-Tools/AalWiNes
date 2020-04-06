//
// Created by Morten on 06-04-2020.
//

#ifndef AALWINES_COORDINATE_H
#define AALWINES_COORDINATE_H

namespace aalwines {

    class Coordinate {
    public:
        explicit Coordinate(std::pair<double, double> pair) : _latitude(pair.first), _longitude(pair.second) { };
        Coordinate(double latitude, double longitude) : _latitude(latitude), _longitude(longitude) {};

        [[nodiscard]] double distance_to(Coordinate other) const {
            // TODO: Implement Haversine function
            return 0.0;
        }
        [[nodiscard]] double latitude() const {
            return _latitude;
        }
        [[nodiscard]] double longitude() const {
            return _longitude;
        }
        void write_xml_attributes(std::ostream& s) const {
            s << " latitude=\"" << _latitude << "\" longitude=\"" << _longitude << "\" ";
        }

    private:
        double _latitude;
        double _longitude;
    };
}

#endif //AALWINES_COORDINATE_H
