// The contents of this file are subject to the MonetDB Public License
// Version 1.1 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://monetdb.cwi.nl/Legal/MonetDBLicense-1.1.html
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License.
//
// The Original Code is the MonetDB Database System.
//
// The Initial Developer of the Original Code is CWI.
// Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
// Copyright August 2008-2011 MonetDB B.V.
// All Rights Reserved.

#include <monetdb_config.h>
#include "any_arg.h"
#include "language.h"
#include <string.h>


AnyArg::AnyArg(int t, char *n, int nr, const char *s) : Arg(t,n)
{
	_nr = nr;
	_bound = anynr_find(nr, this);
	strcpy(typestr, s);
}

const char *
AnyArg::toString() const
{
        if (_nr > 0) {
                char *buf = new char[80];
                sprintf(buf, "any::%d", _nr);
		return buf;
        } else {
		return "any";
        }
}

int
AnyArg::nr() const
{
   	return _nr;
}
 
AnyArg *
AnyArg::bound() const
{
        return _bound;
}

ostream &
AnyArg::print(language *l, ostream &o) const
{
	return l->gen_any_arg(o, *this);
}

 
#define MAX_ANYNR	100
AnyArg* anynr[MAX_ANYNR] = { NULL };
 
void
anynr_clear()
{
	memset((void *) anynr, 0, MAX_ANYNR * sizeof(AnyArg *));
}
 
AnyArg *
anynr_find(int nr, AnyArg*d)
{
        if (nr >  0 && nr < MAX_ANYNR) {
                if (anynr[nr])
			return anynr[nr];
                anynr[nr] = d;
        }
        return NULL;
}
