//
// Copyright (c) 2007-2009 2009 Universidade Aveiro - Instituto de
// Telecomunicacoes Polo Aveiro
// This file is part of ODTONE - Open Dot Twenty One.
//
// This software is distributed under a license. The full license
// agreement can be found in the file LICENSE in this distribution.
// This software may not be copied, modified, sold or distributed
// other than expressed in the named license agreement.
//
// This software is distributed without any warranty.
//
// Author:     Simao Reis <sreis@av.it.pt>
//

///////////////////////////////////////////////////////////////////////////////
#include <odtone/base.hpp>
#include <odtone/mih/message.hpp>
#include <odtone/buffer.hpp>

#include <boost/asio.hpp>
///////////////////////////////////////////////////////////////////////////////

using namespace boost::asio;

namespace odtone { namespace mihf {

class session {
public:
	session(io_service &io);

	ip::tcp::socket &socket();

	void start();

	void handle_read(const boost::system::error_code& error, size_t rbytes);

private:
	odtone::buffer<uint8> _buff;
	ip::tcp::socket _sock;
};

class tcp_server {
public:
	tcp_server(io_service &io, ip::tcp ipv, const char* ip, uint16 port);

	void start();

	void handle_accept(session *s, const boost::system::error_code &e);

	void send(mih::message_ptr &msg, const char *ip, uint16 port);

private:
	io_service &_io;
	ip::tcp::acceptor _acceptor;
};

} /* namespace mifh */ } /* namespace odtone */