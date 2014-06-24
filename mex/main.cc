//===================================================================
// File:        main.cc
// Author:      Drahoslav Zan
// Date:        Mar 03 2013
// Comments:    Matlab MEX function
// Project:     Pro'sKit MT1820 Multimeter Probe (PMP)
//-------------------------------------------------------------------
// Copyright (C) 2013 Drahoslav Zan
//
// This file is part of PMP.
//
// PMP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PMP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PMP. If not, see <http://www.gnu.org/licenses/>.
//===================================================================
// vim: set nowrap sw=2 ts=2


#include "../proskit.h"

#include <cstdlib>
#include <cmath>

#include <libusb-1.0/libusb.h>
#include <mex.h>


// Data
libusb_device **list;
libusb_context *ctx;
libusb_device_handle *dh;	

bool active = false;
bool attached = false;

MTMsg msg;


bool isDevice(libusb_device *dev)
{
	libusb_device_descriptor desc;

	libusb_get_device_descriptor(dev, &desc);
 
	if(desc.idVendor == VENDOR_PROSKIT && desc.idProduct == PRODUCT_MT1820)
		return true;
 
	return false;
}

void cleanup()
{
	if(!active)
		return;

	if(attached)
	{
		libusb_attach_kernel_driver(dh, 0);

		attached = false;
	}

	libusb_close(dh);

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	active = false;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	mexAtExit(&cleanup);

	if(nrhs)
		mexErrMsgTxt("Too much params");

	if(!active)
	{
		libusb_init(&ctx);
		libusb_set_debug(ctx, 3); 

		ssize_t cnt = libusb_get_device_list(ctx, &list);

		ssize_t i = 0;

		if(cnt < 0)
		{
			libusb_free_device_list(list, 1);
			libusb_exit(ctx);

			mexErrMsgTxt("No USB devices found");
		}

		libusb_device *dev = NULL;

		for(i = 0; i < cnt; i++)
		{
			libusb_device *device = list[i];

			if(isDevice(device))
			{
				dev = device;

				break;
			}
		}

		if(dev == NULL)
		{
			libusb_free_device_list(list, 1);
			libusb_exit(ctx);

			mexErrMsgTxt("Device not found");
		}

		if(libusb_open(dev, &dh))
		{
			libusb_free_device_list(list, 1);
			libusb_exit(ctx);

			mexErrMsgTxt("Cannot open device");
		}

		if(libusb_kernel_driver_active(dh, 0))
		{
			libusb_detach_kernel_driver(dh, 0);
			attached = true;
		}

		libusb_claim_interface(dh, 0);

		active = true;
	}
	
	unsigned char data[2 * MTMsg::SIZE];
	int sz;

	int e = libusb_interrupt_transfer(dh, EPIN_MT1820,
			data, sizeof(data), &sz, 0);

	if(e)
	{
		cleanup();

		mexErrMsgTxt("libusb_interrupt_transfer()");
	}

	msg(data, sz);

	float v = msg.value();
	float f = msg.factor();

	plhs[0] = mxCreateNumericMatrix(1, 3, mxSINGLE_CLASS, mxREAL);

	float * d = (float *) mxGetData(plhs[0]);

	d[0] = v;
	d[1] = powf(10, f);

	switch(msg.unit())
	{
		case MTMsg::UNIT_HFE:
			d[2] = 1;
			break;
		case MTMsg::UNIT_V:
			d[2] = 2;
			break;
		case MTMsg::UNIT_OHM:
			d[2] = 3;
			break;
		case MTMsg::UNIT_F:
			d[2] = 4;
			break;
		case MTMsg::UNIT_HZ:
			d[2] = 5;
			break;
		case MTMsg::UNIT_C:
			d[2] = 6;
			break;
		case MTMsg::UNIT_A:
			d[2] = 7;
			break;
	}
}

