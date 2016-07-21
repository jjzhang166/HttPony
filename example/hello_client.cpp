/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "httpony.hpp"

int main(int argc, char** argv)
{
    httpony::Client client;

    // This starts the client on a separate thread
    client.queue_request("example.com");
    client.start();
    std::cout << "Client started, hit enter to quit\n";
    // Pause the main thread listening to standard input
    std::cin.get();
    std::cout << "Client stopped\n";

    // The client destructor will stop it and join the thread
    return 0;
}