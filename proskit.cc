//===================================================================
// File:        proskit.cc
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


/***********************************************************************
	Protocol description:
	======================================
	SIZE: 14 bytes

	BYTE:
		00       Sign +-
		01 - 04  Value, ie. 4 digits from left to right as in display
		05       ?
		06       Decimal point
		07 - 08  Flags 08 only in Farrad maybe belong to unit
		09 - 10  Unit, byte is 09 scale factor (u,m,M) of unit on byte 10
		11       Meter at bottom of display, signed value
		12 - 13  ?

	======================================
	09 - 10:
		UNIT_hFE = 0x0010,
		UNIT_mV = 0x4080,
		UNIT_V = 0x0080,
		UNIT_Ohm = 0x0020,
		UNIT_kOhm = 0x2020,
		UNIT_MOhm = 0x1020,
		UNIT_DIODE_V = 0x0480,
		UNIT_F = 0x0004,
		UNIT_Hz = 0x0008,
		UNIT_DUTY_Hz = 0x0200,
		UNIT_C = 0x0002,
		UNIT_uA = 0x8040,
		UNIT_mA = 0x4040,
		UNIT_A = 0x0040

***********************************************************************/


#include "proskit.h"

#include <limits>


// Messages
const unsigned B00_POS         = 0x2b;
const unsigned B00_NEG         = 0x2d;
const unsigned B01_04_OVERFLOW = 0x3a;
// const unsigned B05
const unsigned B06_1E0         = 0x00;
const unsigned B06_1E3         = 0x31;
const unsigned B06_1E2         = 0x32;
const unsigned B06_1E1         = 0x34;
// const unsigned B07_08
const unsigned B09_S1Em6       = 0x80;
const unsigned B09_S1Em3       = 0x40;
const unsigned B09_S1E3        = 0x20;
const unsigned B09_S1E6        = 0x10;
const unsigned B09_FDUTY       = 0x02;
const unsigned B09_FDIODE      = 0x04;
const unsigned B10_UhFE        = 0x10;
const unsigned B10_UV          = 0x80;
const unsigned B10_UOHM        = 0x20;
const unsigned B10_UF          = 0x04;
const unsigned B10_Hz          = 0x00;
const unsigned B10_C           = 0x02;
const unsigned B10_A           = 0x40;
// const unsigned B12
// const unsigned B13

const unsigned A_B01_04_SUB    = 0x30;
const unsigned B_B09_SMASK     = 0xF0;
const unsigned B_B09_FMASK     = 0x0F;


inline static unsigned char reverse(unsigned char b)
{
	return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}


void MTMsg::decipher(unsigned char *data)
{
	unsigned char n[SIZE];

	n[0 ] = data[1 ] + 0x91;
	n[1 ] = data[10] + 0x95;
	n[2 ] = data[3 ] + 0x9b;
	n[3 ] = data[9 ] + 0x9d;
	n[4 ] = data[6 ] + 0x8b;
	n[5 ] = data[7 ] + 0x92;
	n[6 ] = data[5 ] + 0x88;
	n[7 ] = data[0 ] + 0x96;
	n[8 ] = data[2 ] + 0x9c;
	n[9 ] = data[12] + 0x97;
	n[10] = data[8 ] + 0x97;
	n[11] = data[4 ] + 0x92;
	n[12] = data[13] + 0x9f;
	n[13] = data[11] + 0x88;

	for(size_t i = 0; i < SIZE; ++i)
    data[i] = reverse(n[i]);
}

bool MTMsg::operator()(unsigned char *data, size_t sz)
{
	if(sz != SIZE)
		return false;

	if(!data[0])
		return false;

	decipher(data);

	// Overflow
	if(data[1] == B01_04_OVERFLOW || data[2] == B01_04_OVERFLOW
			|| data[3] == B01_04_OVERFLOW || data[4] == B01_04_OVERFLOW)
	{
		val = std::numeric_limits<float>::infinity();
	}
	else
	{
		// Value
		data[1] -= A_B01_04_SUB;
		data[2] -= A_B01_04_SUB;
		data[3] -= A_B01_04_SUB;
		data[4] -= A_B01_04_SUB;

		float d;

		// Floating point
		switch(data[6])
		{
			case B06_1E0:
				d = 1;
				break;
			case B06_1E3:
				d = 1000;
				break;
			case B06_1E2:
				d = 100;
				break;
			case B06_1E1:
				d = 10;
				break;
			default:
				return false;
		}

		val = (1000 * data[1] + 100 * data[2] + 10 * data[3] + data[4]) / d;
	}

	// Value sign
	if(data[0] == B00_NEG)
		val *= -1;

	// Unit scale
	switch(data[9] & B_B09_SMASK)
	{
		case B09_S1Em6: // u
			scl = -6;
			break;
		case B09_S1Em3: // m
			scl = -3;
			break;
		case B09_S1E3:  // k
			scl = 3;
			break;
		case B09_S1E6:  // M
			scl = 6;
			break;
		default:
			scl = 0;
	}

	// Unit
	switch(data[10])
	{
		case B10_UhFE:
			un = 'h';
			break;
		case B10_UV:
			un = 'V';
			break;
		case B10_UOHM:
			un = 'O';
			break;
		case B10_UF:
			un = 'F';
			break;
		case B10_Hz:
			un = 'H';
			break;
		case B10_C:
			un = 'C';
			break;
		case B10_A:
			un = 'A';
			break;
	}

	// Meter
	mtr = data[11];

	return true;
}
