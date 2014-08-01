/*
 * libfreelan - A C++ library to establish peer-to-peer virtual private
 * networks.
 * Copyright (C) 2010-2011 Julien KAUFFMANN <julien.kauffmann@freelan.org>
 *
 * This file is part of libfreelan.
 *
 * libfreelan is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * libfreelan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 * If you intend to use libfreelan in a commercial software, please
 * contact me : we may arrange this for a small fee or no fee at all,
 * depending on the nature of your project.
 */

/**
 * \file server.cpp
 * \author Julien KAUFFMANN <julien.kauffmann@freelan.org>
 * \brief The freelan server class.
 */

#include "server.hpp"

#include <boost/lexical_cast.hpp>

#include <cassert>

namespace freelan
{
	namespace
	{
		class session_type : public mongooseplus::generic_session, public mongooseplus::basic_session_type
		{
			public:
				session_type(const std::string& session_id, const std::string& _username) :
					mongooseplus::generic_session(session_id),
					basic_session_type(_username)
				{
				}
		};

		class authentication_handler : public mongooseplus::basic_authentication_handler
		{
			public:

				authentication_handler() :
					mongooseplus::basic_authentication_handler("freelan")
				{}

			protected:

				bool authenticate_from_username_and_password(mongooseplus::connection& conn, const std::string& username, const std::string& password) const override
				{
					if ((username != "test") || (password != "password"))
					{
						return false;
					}

					const auto session = conn.get_session<session_type>();

					if (!session || (session->username() != username))
					{
						conn.set_session<session_type>(username);
					}

					return true;
				}
		};
	}

	web_server::web_server(logger& _logger, const freelan::server_configuration& configuration) :
		m_logger(_logger)
	{
		m_logger(LL_DEBUG) << "Web server's listen endpoint set to " << configuration.listen_on << ".";
		set_option("listening_port", boost::lexical_cast<std::string>(configuration.listen_on));

		// Routes
		register_route("/", [this](mongooseplus::connection& conn) {
			m_logger(LL_DEBUG) << "Requested root.";

			conn.send_header("content-type", "application/json");
			conn.send_data("{\"a\": 1, \"b\": [2, 3]}");
			return request_result::handled;
		}).set_authentication_handler<authentication_handler>();
	}

	web_server::request_result web_server::handle_request(mongooseplus::connection& conn)
	{
		if (m_logger.level() <= LL_DEBUG)
		{
			m_logger(LL_DEBUG) << "Web server - Received " << conn.request_method() << " request from " << conn.remote() << " for " << conn.uri() << " (" << conn.content_size() << " byte(s) content).";
			m_logger(LL_DEBUG) << "--- Headers follow ---";

			for (auto&& header : conn.get_headers())
			{
				m_logger(LL_DEBUG) << header.key() << ": " << header.value();
			}

			m_logger(LL_DEBUG) << "--- End of headers ---";
		}

		return mongooseplus::routed_web_server::handle_request(conn);
	}

	web_server::request_result web_server::handle_http_error(mongooseplus::connection& conn)
	{
		m_logger(LL_WARNING) << "Web server - Sending back " << conn.status_code() << " to " << conn.remote() << ".";

		return mongooseplus::routed_web_server::handle_http_error(conn);
	}
}
