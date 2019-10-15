/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Camels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Camels.  If not, see <https://www.gnu.org/licenses/>.
 *
 * © Tom Rodgers notaraptor@gmail.com 2017-2019
 */

#ifndef NET_H
#define NET_H

#include <vector>

#include "settings.hpp"

class Node {
    double value = 0;
    std::vector<double> weights;
public:
    Node(size_t nLSz);
    double getValue() const { return value; }
    double output(size_t idx) const { return value * weights[idx]; }
    void setValue(double vl) { value = vl; }
};

class Net {
    std::vector<std::vector<Node>> layers;
public:
    Net(const std::vector<unsigned int> &szs);
};

#endif // NET_H
