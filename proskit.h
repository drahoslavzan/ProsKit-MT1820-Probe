//===================================================================
// File:        proskit.h
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


#ifndef _PROSKIT_H_
#define _PROSKIT_H_


#include <cstdlib>


const int VENDOR_PROSKIT  = 0x1244;
const int PRODUCT_MT1820  = 0xd237;

const int EPIN_MT1820     = 0x81;


class MTMsg
{
public:
	enum { SIZE = 14 };
private:
	float val;
	int scl;
	char un;
	int mtr;
private:
	void decipher(unsigned char *data);
public:
	inline float value() const { return val; }
	inline int scale() const { return scl; }
	inline char unit() const { return un; }
	inline int meter() const { return mtr; }

	bool operator()(unsigned char *data, size_t sz);
};


#endif /* _PROSKIT_H_ */
