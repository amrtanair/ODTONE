//=============================================================================
// Brief   : Link SAP
// Authors : Bruno Santos <bsantos@av.it.pt>
//
//
// Copyright (C) 2009 Universidade Aveiro - Instituto de Telecomunicacoes Polo Aveiro
//
// This file is part of ODTONE - Open Dot Twenty One.
//
// This software is distributed under a license. The full license
// agreement can be found in the file LICENSE in this distribution.
// This software may not be copied, modified, sold or distributed
// other than expressed in the named license agreement.
//
// This software is distributed without any warranty.
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
#include "link_sap.hpp"
#include <odtone/debug.hpp>
#include <odtone/mih/message.hpp>
#include <odtone/mih/indication.hpp>
#include <odtone/mih/request.hpp>
#include <odtone/mih/response.hpp>
#include <odtone/mih/tlv_types.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

///////////////////////////////////////////////////////////////////////////////
link_sap::link_sap(const odtone::mih::config& cfg, boost::asio::io_service& io)
	: _mihf(cfg, io, boost::bind(&link_sap::default_handler, this, _1))
{
}

link_sap::~link_sap()
{
}

void link_sap::update(interface* it)
{
	interface& ifi = _ifmap.find(it->index());
	bool update = false;

	if (ifi == nullref) {
		_ifmap.insert(*it);
		update = true;

	} else {
		boost::logic::tribool prev = ifi.up(it->up());

		update = (prev != it->up());

		delete it;
		it = &ifi;
	}

	if (update) {
		odtone::mih::message msg;
		odtone::mih::link_tuple_id lid;
		odtone::mih::link_dn_reason rc(odtone::mih::link_dn_reason_explicit_disconnect);

		lid.type = it->type();
		lid.addr = it->link_addr();

		if (it->up())
			msg << odtone::mih::indication(odtone::mih::indication::link_up)
				& odtone::mih::tlv_link_identifier(lid);

		else if (!it->up())
			msg << odtone::mih::indication(odtone::mih::indication::link_down)
				& odtone::mih::tlv_link_identifier(lid)
				& odtone::mih::tlv_link_dn_reason(rc);
		else
			return;

		_mihf.async_send(msg, boost::bind(&link_sap::cleanup_handler, this, boost::ref(msg), _1));
	}
}

void link_sap::default_handler(odtone::mih::message& msg)
{
	odtone::mih::status st;

	switch (msg.mid()) {
	case odtone::mih::request::capability_discover:
		{
			odtone::mih::message m;
			odtone::mih::net_type_addr_list ll;
			odtone::mih::event_list el;

			st = odtone::mih::status_success;
			el.set(odtone::mih::link_up);
			el.set(odtone::mih::link_down);

			//TODO: fill the net_type_addr_list

			m << odtone::mih::response(odtone::mih::response::capability_discover)
				& odtone::mih::tlv_status(st)
				& odtone::mih::tlv_net_type_addr_list(ll)
				& odtone::mih::tlv_event_list(el);

			_mihf.async_send(m, boost::bind(&link_sap::cleanup_handler, this, boost::ref(m), _1));
		}
		break;

	default:
		{
			odtone::mih::message m;

			st = odtone::mih::status_failure;

			m << odtone::mih::response(odtone::mih::response::capability_discover)
				& odtone::mih::tlv_status(st);

			_mihf.async_send(m, boost::bind(&link_sap::cleanup_handler, this, boost::ref(m), _1));
		}
	}
}

void link_sap::cleanup_handler(odtone::mih::message& /*msg*/, const boost::system::error_code& /*ec*/)
{
}

// EOF ////////////////////////////////////////////////////////////////////////
