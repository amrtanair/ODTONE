//==============================================================================
// Brief   : Mobile Node MIHD ID TLV
// Authors : Carlos Guimarães <cguimaraes@av.it.pt>
//------------------------------------------------------------------------------
// ODTONE - Open Dot Twenty One
//
// Copyright (C) 2009-2013 Universidade Aveiro
// Copyright (C) 2009-2013 Instituto de Telecomunicações - Pólo Aveiro
//
// This software is distributed under a license. The full license
// agreement can be found in the file LICENSE in this distribution.
// This software may not be copied, modified, sold or distributed
// other than expressed in the named license agreement.
//
// This software is distributed without any warranty.
//==============================================================================

package mih.tlvs;

import mih.types.basic.OctetString;
import mih.types.basic.UnsignedInteger8;

public class MobileNodeMihfIdTlv extends Tlv<OctetString> {

	public MobileNodeMihfIdTlv() {
		super(new UnsignedInteger8(52));
		value = new OctetString();
	}

	public MobileNodeMihfIdTlv(OctetString value) {
		super(new UnsignedInteger8(52), value);
	}
}