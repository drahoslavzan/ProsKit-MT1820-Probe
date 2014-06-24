//===================================================================
// File:        main.cc
// Author:      Drahoslav Zan
// Date:        Mar 03 2013
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


#include <iostream>
#include <cstdlib>
#include <cmath>

#include <unistd.h>
#include <cstdio>

#include <libusb-1.0/libusb.h>

#include "proskit.h"


using namespace std;


static const char *progName;


void dump(FILE *stream, unsigned char *data, size_t sz)
{
	fprintf(stream, "(%u) ", (unsigned)sz);

	for(size_t i = 0; i < sz; ++i)
		fprintf(stream, "%.2x  ", data[i]);

	fputs("\n", stream);

	fflush(stream);
}


bool isDevice(libusb_device *dev)
{
	libusb_device_descriptor desc;

	libusb_get_device_descriptor(dev, &desc);
 
	if(desc.idVendor == VENDOR_PROSKIT && desc.idProduct == PRODUCT_MT1820)
		return true;
 
	return false;
}


int main(int argc, char **argv)
{
	progName = argv[0];

	libusb_device **list;
	libusb_context *ctx = NULL;

	libusb_init(&ctx);
	libusb_set_debug(ctx, 3); 

	ssize_t cnt = libusb_get_device_list(ctx, &list);

	ssize_t i = 0;

	if(cnt < 0)
	{
		cerr << "WARNING: No USB devices found" << endl;

		return 0;
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

	if(dev != NULL)
	{
		libusb_device_handle *dh;	

		if(libusb_open(dev, &dh))
			cerr << "ERROR: Cannot open device" << endl;
		else
		{
			bool attached = false;

			if(libusb_kernel_driver_active(dh, 0))
			{
				libusb_detach_kernel_driver(dh, 0);
				attached = true;
			}

			libusb_claim_interface(dh, 0);

			//fprintf(stderr, "     0   1   2   3   4   5   6   7   8   9   10  11  12  13\n");
			//fprintf(stderr, "===========================================================\n");

			for(;;)
			{
				unsigned char data[2 * MTMsg::SIZE];
				int sz;

				int e = libusb_interrupt_transfer(dh, EPIN_MT1820,
						data, sizeof(data), &sz, 0);

				if(e)
				{
					cerr << "ERROR: libusb_interrupt_transfer(): " << e << endl;

					break;
				}

				MTMsg msg;

				if(msg(data, sz))
				{
					float v = msg.value();
					int s = msg.scale();

					const char *fs = "";
					const char *us = "";

					switch(s)
					{
						case -6:
							fs = "u";
							break;
						case -3:
							fs = "m";
							break;
						case 3:
							fs = "k";
							break;
						case 6:
							fs = "M";
							break;
						default:
							v *= pow(10, s);
					}

					switch(msg.unit())
					{
						case 'h':
							us = "hFE";
							break;
						case 'V':
							us = "V";
							break;
						case 'O':
							us = "Ohm";
							break;
						case 'F':
							us = "F";
							break;
						case 'H':
							us = "Hz";
							break;
						case 'C':
							us = "°C";
							break;
						case 'A':
							us = "A";
							break;
						default:
							us = "?";
					}

					//cout << v << " " << fs << us << endl;
				}
				
				if(!data[0]) continue;

				//data[0] = data[1] = data[2] = data[3] = data[4] = data[6] = data[11] = 0;
				
				dump(stderr, data, sz);

				//sleep(2);
				//getchar();
			}

			if(attached)
			{
				libusb_attach_kernel_driver(dh, 0);

				attached = false;
			}

			libusb_close(dh);
		}
	}
	else
		cerr << "WARNING: Device not found" << endl;

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	return 0;
}
