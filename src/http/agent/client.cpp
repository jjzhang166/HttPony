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

#include "httpony/http/agent/client.hpp"
#include "httpony/http/formatter.hpp"
#include "httpony/http/parser.hpp"

namespace httpony {


ClientStatus Client::get_response(const std::shared_ptr<io::Connection>& connection, Request& request, Response& response) const
{
    request.connection = connection;
    return get_response_attempt(0, request, response);
}

ClientStatus Client::get_response_attempt(int attempt, Request& request, Response& response) const
{
    response = Response();
    response.connection = request.connection;

    if ( !request.connection )
        return "client not connected";

    process_request(request);
    auto ostream = request.connection->send_stream();
    http::Http1Formatter().request(ostream, request);
    if ( !ostream.send() )
        return "connection error";

    auto istream = request.connection->receive_stream();
    ClientStatus status = Http1Parser().response(istream, response);

    if ( istream.timed_out() )
        return "timeout";

    if ( status.error() )
        return status;

    if ( response.body.has_data() )
        request.connection->input_buffer().expect_input(response.body.content_length());

    return on_attempt(request, response, attempt);
}

ClientStatus Client::on_attempt(Request& request, Response& response, int attempt) const
{
    /// \todo Try again on 408 (Request Timeout)
    /// \todo Handle 426 (Upgrade Required) for known protocol versions
    /// \todo Also handle on async client
    if ( response.status.type() == StatusType::Redirection && response.headers.contains("Location") )
    {
        if ( attempt > _max_redirects )
            return "too many redirects";

        Uri target = response.headers["Location"];
        if ( request.url.authority.host != target.authority.host ||
            request.url.authority.port != target.authority.port )
        {
            ClientStatus status;
            request.connection = connect(target, status);
            if ( status.error() )
                return status;
        }

        request.url = target;
        /// \todo Handle 307 differently (keeping POST)
        if ( request.method == "POST" )
            request.method = "GET";
        request.body.stop_output();

        if ( _max_redirects > 0 )
            return get_response_attempt(attempt + 1, request, response);
    }

    return {};
}

} // namespace httpony