//==============================================================================
// Brief   : Discover Service
// Authors : Carlos Guimarães <cguimaraes@av.it.pt>
//------------------------------------------------------------------------------
// ODTONE - Open Dot Twenty One
//
// Copyright (C) 2009-2012 Universidade Aveiro
// Copyright (C) 2009-2012 Instituto de Telecomunicações - Pólo Aveiro
//
// This software is distributed under a license. The full license
// agreement can be found in the file LICENSE in this distribution.
// This software may not be copied, modified, sold or distributed
// other than expressed in the named license agreement.
//
// This software is distributed without any warranty.
//==============================================================================

///////////////////////////////////////////////////////////////////////////////
#include "discover_service.hpp"
#include "mihfid.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <odtone/mih/request.hpp>
#include <odtone/mih/response.hpp>
#include <odtone/mih/indication.hpp>
#include <odtone/mih/confirm.hpp>
#include <odtone/mih/types/odtone.hpp>
#include <odtone/mih/tlv_types.hpp>

#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////

namespace odtone { namespace mihf {

discover_service::discover_service(io_service &io,
								   local_transaction_pool &lpool,
								   address_book &address_abook,
								   user_book &user_abook,
								   transmit &t,
								   bool enable_unsolicited)
	: _io(io),
	  _lpool(lpool),
	  _abook(address_abook),
	  _user_abook(user_abook),
	  _transmit(t)
{
	_enable_unsolicited = enable_unsolicited;
}

void discover_service::request(meta_message_ptr& in, meta_message_ptr& out)
{
	ODTONE_LOG(1, "(discovery) Received a request to discover a new PoS");
	in->destination(mih::id(_user_abook.discovery_user().get()));
	in->opcode(mih::operation::indication);
	utils::forward_request(in, _lpool, _transmit);
}

void discover_service::response(meta_message_ptr& in, meta_message_ptr& out)
{
	bool unsolicited;

	ODTONE_LOG(1, "(discovery) Received a response with the PoS discovery");

	if(!_lpool.set_user_tid(in)) {
		if(!_enable_unsolicited) {
			ODTONE_LOG(1, "(discovery) Unsolicited discovery not allowed");
			return;
		} else {
			ODTONE_LOG(1, "(discovery) this is an unsolicited discovery");
			out->source(mihfid);
			unsolicited = true;
		}
	} else {
		unsolicited = false;
		out->source(in->destination());
	}

	mih::status									st;
	boost::optional<mih::net_type_addr_list>	capabilities_list_net_type_addr;
	boost::optional<mih::event_list>			capabilities_event_list;
	boost::optional<mih::command_list>			capabilities_cmd_list;
	boost::optional<mih::iq_type_list>			capabilities_query_type;
	boost::optional<mih::transport_list>		capabilities_trans_list;
	boost::optional<mih::mbb_ho_supp_list>		capabilities_mbb_ho_supp;
	boost::optional<mih::mos_dscv>				mos_dscv;

	*in >> mih::response(mih::response::capability_discover)
			& mih::tlv_status(st)
			& mih::tlv_net_type_addr_list(capabilities_list_net_type_addr)
			& mih::tlv_event_list(capabilities_event_list)
			& mih::tlv_command_list(capabilities_cmd_list)
			& mih::tlv_query_type_list(capabilities_query_type)
			& mih::tlv_transport_option_list(capabilities_trans_list)
			& mih::tlv_mbb_ho_supp_list(capabilities_mbb_ho_supp)
			& mih::tlv_mos_dscv(mos_dscv);

	address_entry mihf_cap = _abook.get(mihfid_t::instance()->to_string());
	*out << mih::request(mih::request::capability_discover)
			& mih::tlv_net_type_addr_list(mihf_cap.capabilities_list_net_type_addr)
			& mih::tlv_event_list(mihf_cap.capabilities_event_list)
			& mih::tlv_command_list(mihf_cap.capabilities_cmd_list)
			& mih::tlv_query_type_list(mihf_cap.capabilities_query_type)
			& mih::tlv_transport_option_list(mihf_cap.capabilities_trans_list)
			& mih::tlv_mbb_ho_supp_list(mihf_cap.capabilities_mbb_ho_supp);

	out->destination(mih::id(""));
	out->tid(in->tid());

	if(st == mih::status_success && mos_dscv) {
		bool all = true;
		mih::mos_dscv missing;
		std::vector<mih::mos_info> sent;

		BOOST_FOREACH(mih::mos_info pos, mos_dscv->is) {
			std::vector<mih::mos_info>::iterator it;
			it = std::find(sent.begin(), sent.end(), pos);

			if(it == sent.end()) {
				if(pos.ip.address().compare("") == 0) {
					missing.is.push_back(pos);
					all = false;
				} else {
					request_pos_capabilities(out, pos, unsolicited);
					sent.push_back(pos);
				}
			}
		}

		BOOST_FOREACH(mih::mos_info pos, mos_dscv->cs) {
			std::vector<mih::mos_info>::iterator it;
			it = std::find(sent.begin(), sent.end(), pos);

			if(it == sent.end()) {
				if(pos.ip.address().compare("") == 0) {
					missing.cs.push_back(pos);
					all = false;
				} else {
					request_pos_capabilities(out, pos, unsolicited);
					sent.push_back(pos);
				}
			}
		}

		BOOST_FOREACH(mih::mos_info pos, mos_dscv->es) {
			std::vector<mih::mos_info>::iterator it;
			it = std::find(sent.begin(), sent.end(), pos);

			if(it == sent.end()) {
				if(pos.ip.address().compare("") == 0) {
					missing.es.push_back(pos);
					all = false;
				} else {
					request_pos_capabilities(out, pos, unsolicited);
					sent.push_back(pos);
				}
			}
		}

		if(!all) {
			// Check the next discovery mechanism to use
			mih::octet_string next_disc_user;
			std::map<mih::octet_string, user_entry> user_map = _user_abook.get_discovery_users();
			std::map<mih::octet_string, user_entry>::iterator src = user_map.find(in->source().to_string());

			std::map<mih::octet_string, user_entry>::iterator it;
			for(it = user_map.begin() ; it != user_map.end(); ++it) {
				if(it->second.priority == (src->second.priority + 1)) {
					next_disc_user = it->first;
					break;
				}
			}

			if(next_disc_user.size() != 0) {
				ODTONE_LOG(1, "(discovery) Using a complementar mechanism ",
						   "to discover the remaining PoS");
				*out << mih::indication(mih::indication::capability_discover)
					& mih::tlv_mos_dscv(missing);
				out->destination(mih::id(next_disc_user));
				if(!unsolicited)
					utils::forward_request(out, _lpool, _transmit);
				else
					_transmit(out);
			} else {
				// Broadcast request
				ODTONE_LOG(1, "(discovery) There are pending discovers of PoS. ",
						   "Sending a broadcast Capability Discover request");
				out->destination(mih::id(""));
				if(!unsolicited)
					utils::forward_request(out, _lpool, _transmit);
				else
					_transmit(out);
			}
		}
	} else {
		// Check the next discovery mechanism to use
		mih::octet_string next_disc_user;
		std::map<mih::octet_string, user_entry> user_map = _user_abook.get_discovery_users();
		std::map<mih::octet_string, user_entry>::iterator src = user_map.find(in->source().to_string());

		std::map<mih::octet_string, user_entry>::iterator it;
		for(it = user_map.begin() ; it != user_map.end(); ++it) {
			if(it->second.priority == (src->second.priority + 1)) {
				next_disc_user = it->first;
				break;
			}
		}

		if(next_disc_user.size() != 0) {
			ODTONE_LOG(1, "(discovery) The discover mechanism cannot discover any PoS.",
					   " Trying ", next_disc_user, " discover mechanims.");
			*out << mih::indication(mih::indication::capability_discover);
			out->destination(mih::id(next_disc_user));
			if(!unsolicited)
				utils::forward_request(out, _lpool, _transmit);
			else
				_transmit(out);
		} else {
			// Broadcast request
			ODTONE_LOG(1, "(discovery) None of the configured discovery mechanism had results.",
					   " Sending a broadcast Capability Discover request");
			out->source(in->destination());
			out->destination(mih::id(""));
			if(!unsolicited)
				utils::forward_request(out, _lpool, _transmit);
			else
				_transmit(out);
		}
	}
}

void discover_service::request_pos_capabilities(meta_message_ptr& out, mih::mos_info &pos, bool unsolicited)
{
	ODTONE_LOG(1, "(discovery) Requesting capabilities of a discovered PoS: ", pos.id.to_string());
	out->ip(pos.ip.address());
	out->port(pos.port);
	out->destination(pos.id);
	if(!unsolicited)
		utils::forward_request(out, _lpool, _transmit);
	else
		_transmit(out);
}

} /* namespace mihf */ } /* namespace odtone */
