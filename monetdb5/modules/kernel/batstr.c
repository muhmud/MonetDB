/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

/*
 *  M.L. Kersten
 * String multiplexes
 * [TODO: property propagations]
 * The collection of routines provided here are map operations
 * for the atom string primitives.
 *
 * In line with the batcalc module, we assume that if two bat operands
 * are provided that they are aligned.
 */
#include "monetdb_config.h"
#include "gdk.h"
#include <ctype.h>
#include <string.h>
#include "mal_exception.h"
#include "str.h"

#define prepareOperand(X,Y,Z)									\
	if( (X= BATdescriptor(*Y)) == NULL )						\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);
#define prepareOperand2(X,Y,A,B,Z)								\
	if( (X= BATdescriptor(*Y)) == NULL )						\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);	\
	if( (A= BATdescriptor(*B)) == NULL ){						\
		BBPunfix(X->batCacheid);								\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);	\
	}
#define prepareOperand3(X,Y,A,B,I,J,Z)							\
	if( (X= BATdescriptor(*Y)) == NULL )						\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);	\
	if( (A= BATdescriptor(*B)) == NULL ){						\
		BBPunfix(X->batCacheid);								\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);	\
	}															\
	if( (I= BATdescriptor(*J)) == NULL ){						\
		BBPunfix(X->batCacheid);								\
		BBPunfix(A->batCacheid);								\
		throw(MAL, Z, SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);	\
	}
#define prepareResult(X,Y,T,Z)							\
	X= COLnew((Y)->hseqbase,T,BATcount(Y), TRANSIENT);	\
	if( X == NULL){										\
		BBPunfix(Y->batCacheid);						\
		throw(MAL, Z, SQLSTATE(HY013) MAL_MALLOC_FAIL);	\
	}													\
	X->tsorted=false;									\
	X->trevsorted=false;
#define prepareResult2(X,Y,A,T,Z)						\
	X= COLnew((Y)->hseqbase,T,BATcount(Y), TRANSIENT);	\
	if( X == NULL){										\
		BBPunfix(Y->batCacheid);						\
		BBPunfix(A->batCacheid);						\
		throw(MAL, Z, SQLSTATE(HY013) MAL_MALLOC_FAIL);	\
	}													\
	X->tsorted=false;									\
	X->trevsorted=false;
#define finalizeResult(X,Y,Z)								\
	(Y)->theap.dirty |= BATcount(Y) > 0;					\
	*X = (Y)->batCacheid;									\
	BBPkeepref(*(X));										\
	BBPunfix(Z->batCacheid);

static str
do_batstr_int(bat *ret, const bat *l, const char *name, str (*func)(int *, const str *))
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x;
	int y;
	str msg = MAL_SUCCEED;

	prepareOperand(b, l, name);
	prepareResult(bn, b, TYPE_int, name);

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		x = (str) BUNtvar(bi, p);
		if (strNil(x)) {
			y = int_nil;
			bn->tnonil = false;
			bn->tnil = true;
		} else if ((msg = (*func)(&y, &x)) != MAL_SUCCEED) {
			goto bunins_failed;
		}
		if (bunfastappTYPE(int, bn, &y) != GDK_SUCCEED)
			goto bunins_failed;
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;
bunins_failed:
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

static str
STRbatLength(bat *ret, const bat *l)
{
	return do_batstr_int(ret, l, "batstr.Length", STRLength);
}

static str
STRbatBytes(bat *ret, const bat *l)
{
	return do_batstr_int(ret, l, "batstr.Bytes", STRBytes);
}

static str
do_batstr_str(bat *ret, const bat *l, const char *name, str (*func)(str *, const str *))
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand(b, l, name);
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l' and a constant string 's2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_conststr_str(bat *ret, const bat *l, const str *s2, const char *name, str (*func)(str *, const str *, const str *))
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand(b, l, name);
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x, s2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: two BATs of strings 'l' and 'l2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_batstr_str(bat *ret, const bat *l, const bat *l2, const char *name, str (*func)(str *, const str *, const str *))
{
	BATiter bi, bi2;
	BAT *bn, *b, *b2;
	BUN p, q;
	str x, x2, y;
	str msg = MAL_SUCCEED;

	prepareOperand2(b, l, b2, l2, name);
	if(BATcount(b) != BATcount(b2)) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		throw(MAL, name, ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn, b, b2, TYPE_str, name);

	bi = bat_iterator(b);
	bi2 = bat_iterator(b2);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		x2 = (str) BUNtvar(bi2, p);
		if (!strNil(x) &&
			!strNil(x2) &&
			(msg = (*func)(&y, &x, &x2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(b2->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l' and a constant int 'n'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_constint_str(bat *ret, const bat *l, const int *n, const char *name, str (*func)(str *, const str *, const int *))
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand(b, l, name);
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x, n)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l' and a BAT of integers 'n'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_batint_str(bat *ret, const bat *l, const bat *n, const char *name, str (*func)(str *, const str *, const int *))
{
	BATiter bi, bi2;
	BAT *bn, *b, *b2;
	BUN p, q;
	int nn;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand2(b, l, b2, n, name);
	if(BATcount(b) != BATcount(b2)) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		throw(MAL, name, ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn, b, b2, TYPE_str, name);

	bi = bat_iterator(b);
	bi2 = bat_iterator(b2);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		nn = *(int *)BUNtloc(bi2, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x, &nn)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(b2->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l', a constant int 'n' and a constant str 's2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_constint_conststr_str(bat *ret, const bat *l, const int *n, const str *s2, const char *name, str (*func)(str *, const str *, const int *, const str *))
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand(b, l, name);
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x, n, s2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l', a BAT of integers 'n' and a constant str 's2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_batint_conststr_str(bat *ret, const bat *l, const bat *n, const str *s2, const char *name, str (*func)(str *, const str *, const int *, const str *))
{
	BATiter bi, bi2;
	BAT *bn, *b, *b2;
	BUN p, q;
	int nn;
	str x, y;
	str msg = MAL_SUCCEED;

	prepareOperand2(b, l, b2, n, name);
	if(BATcount(b) != BATcount(b2)) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		throw(MAL, name, ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);
	bi2 = bat_iterator(b2);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		nn = *(int *)BUNtloc(bi2, p);
		if (!strNil(x) &&
			(msg = (*func)(&y, &x, &nn, s2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(b2->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l', a constant int 'n' and a BAT of strings 'l2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_constint_batstr_str(bat *ret, const bat *l, const int *n, const bat *l2, const char *name, str (*func)(str *, const str *, const int *, const str *))
{
	BATiter bi, bi2;
	BAT *bn, *b, *b2;
	BUN p, q;
	str x, x2, y;
	str msg = MAL_SUCCEED;

	prepareOperand2(b, l, b2, l2, name);
	if(BATcount(b) != BATcount(b2)) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		throw(MAL, name, ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult(bn, b, TYPE_str, name);

	bi = bat_iterator(b);
	bi2 = bat_iterator(b2);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		x2 = (str) BUNtvar(bi2, p);
		if (!strNil(x) &&
			!strNil(x2) &&
			(msg = (*func)(&y, &x, n, &x2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(b2->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

/* Input: a BAT of strings 'l', a BAT of int 'n' and a BAT of strings 'l2'
 * Output type: str (a BAT of strings)
 */
static str
do_batstr_batint_batstr_str(bat *ret, const bat *l, const bat *n, const bat *l2, const char *name, str (*func)(str *, const str *, const int *, const str *))
{
	BATiter bi, bi2, bi3;
	BAT *bn, *b, *b2, *b3;
	BUN p, q;
	int nn;
	str x, x2, y;
	str msg = MAL_SUCCEED;


	prepareOperand3(b, l, b2, n, b3, l2, name);
	if (BATcount(b) != BATcount(b2) || BATcount(b) != BATcount(b3)) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		BBPunfix(b3->batCacheid);
		throw(MAL, name, ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	bn = COLnew(b->hseqbase, TYPE_str, BATcount(b), TRANSIENT);
	if (bn == NULL) {
		BBPunfix(b->batCacheid);
		BBPunfix(b2->batCacheid);
		BBPunfix(b3->batCacheid);
		throw(MAL, name, SQLSTATE(HY013) MAL_MALLOC_FAIL);
	}
	bn->tsorted=false;
	bn->trevsorted=false;

	bi = bat_iterator(b);
	bi2 = bat_iterator(b2);
	bi3 = bat_iterator(b3);

	BATloop(b, p, q) {
		y = NULL;
		x = (str) BUNtvar(bi, p);
		nn = *(int *)BUNtloc(bi2, p);
		x2 = (str) BUNtvar(bi3, p);
		if (!strNil(x) &&
			!strNil(x2) &&
			(msg = (*func)(&y, &x, &nn, &x2)) != MAL_SUCCEED)
			goto bailout;
		if (y == NULL)
			y = (str) str_nil;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED) {
			if (y != str_nil)
				GDKfree(y);
			goto bailout;
		}
		if (y == str_nil) {
			bn->tnonil = false;
			bn->tnil = true;
		} else
			GDKfree(y);
	}
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;

  bailout:
	BBPunfix(b->batCacheid);
	BBPunfix(b2->batCacheid);
	BBPunfix(b3->batCacheid);
	BBPunfix(bn->batCacheid);
	if (msg != MAL_SUCCEED)
		return msg;
	throw(MAL, name, OPERATION_FAILED " During bulk operation");
}

static str
STRbatLower(bat *ret, const bat *l)
{
	return do_batstr_str(ret, l, "batstr.Lower", STRLower);
}

static str
STRbatUpper(bat *ret, const bat *l)
{
	return do_batstr_str(ret, l, "batstr.Upper", STRUpper);
}

static str
STRbatStrip(bat *ret, const bat *l)
{
	return do_batstr_str(ret, l, "batstr.Strip", STRStrip);
}

static str
STRbatLtrim(bat *ret, const bat *l)
{
	return do_batstr_str(ret, l, "batstr.Ltrim", STRLtrim);
}

static str
STRbatRtrim(bat *ret, const bat *l)
{
	return do_batstr_str(ret, l, "batstr.Rtrim", STRRtrim);
}

static str
STRbatStrip2_const(bat *ret, const bat *l, const str *s2)
{
	return do_batstr_conststr_str(ret, l, s2, "batstr.Strip", STRStrip2);
}

static str
STRbatLtrim2_const(bat *ret, const bat *l, const str *s2)
{
	return do_batstr_conststr_str(ret, l, s2, "batstr.Ltrim", STRLtrim2);
}

static str
STRbatRtrim2_const(bat *ret, const bat *l, const str *s2)
{
	return do_batstr_conststr_str(ret, l, s2, "batstr.Rtrim", STRRtrim2);
}

static str
STRbatStrip2_bat(bat *ret, const bat *l, const bat *l2)
{
	return do_batstr_batstr_str(ret, l, l2, "batstr.Strip", STRStrip2);
}

static str
STRbatLtrim2_bat(bat *ret, const bat *l, const bat *l2)
{
	return do_batstr_batstr_str(ret, l, l2, "batstr.Ltrim", STRLtrim2);
}

static str
STRbatRtrim2_bat(bat *ret, const bat *l, const bat *l2)
{
	return do_batstr_batstr_str(ret, l, l2, "batstr.Rtrim", STRRtrim2);
}

static str
STRbatLpad_const(bat *ret, const bat *l, const int *n)
{
	return do_batstr_constint_str(ret, l, n, "batstr.Lpad", STRLpad);
}

static str
STRbatRpad_const(bat *ret, const bat *l, const int *n)
{
	return do_batstr_constint_str(ret, l, n, "batstr.Rpad", STRRpad);
}

static str
STRbatLpad_bat(bat *ret, const bat *l, const bat *n)
{
	return do_batstr_batint_str(ret, l, n, "batstr.Lpad", STRLpad);
}

static str
STRbatRpad_bat(bat *ret, const bat *l, const bat *n)
{
	return do_batstr_batint_str(ret, l, n, "batstr.Rpad", STRRpad);
}

static str
STRbatLpad2_const_const(bat *ret, const bat *l, const int *n, const str *s2)
{
	return do_batstr_constint_conststr_str(ret, l, n, s2, "batstr.Lpad", STRLpad2);
}

static str
STRbatRpad2_const_const(bat *ret, const bat *l, const int *n, const str *s2)
{
	return do_batstr_constint_conststr_str(ret, l, n, s2, "batstr.Rpad", STRRpad2);
}

static str
STRbatLpad2_bat_const(bat *ret, const bat *l, const bat *n, const str *s2)
{
	return do_batstr_batint_conststr_str(ret, l, n, s2, "batstr.Lpad", STRLpad2);
}

static str
STRbatRpad2_bat_const(bat *ret, const bat *l, const bat *n, const str *s2)
{
	return do_batstr_batint_conststr_str(ret, l, n, s2, "batstr.Rpad", STRRpad2);
}

static str
STRbatLpad2_const_bat(bat *ret, const bat *l, const int *n, const bat *l2)
{
	return do_batstr_constint_batstr_str(ret, l, n, l2, "batstr.Lpad", STRLpad2);
}

static str
STRbatRpad2_const_bat(bat *ret, const bat *l, const int *n, const bat *l2)
{
	return do_batstr_constint_batstr_str(ret, l, n, l2, "batstr.Rpad", STRRpad2);
}

static str
STRbatLpad2_bat_bat(bat *ret, const bat *l, const bat *n, const bat *l2)
{
	return do_batstr_batint_batstr_str(ret, l, n, l2, "batstr.Lpad", STRLpad2);
}

static str
STRbatRpad2_bat_bat(bat *ret, const bat *l, const bat *n, const bat *l2)
{
	return do_batstr_batint_batstr_str(ret, l, n, l2, "batstr.Rpad", STRRpad2);
}

/*
 * A general assumption in all cases is the bats are synchronized on their
 * head column. This is not checked and may be mis-used to deploy the
 * implementation for shifted window arithmetic as well.
 */

static str
STRbatPrefix(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	bit v;

	prepareOperand2(left,l,right,r,"batstr.startsWith");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.startsWith", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_bit,"batstr.startsWith");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		str tr = (str) BUNtvar(righti,p);
		STRPrefix(&v, &tl, &tr);
		if (bunfastappTYPE(bit, bn, &v) != GDK_SUCCEED)
			goto bunins_failed;
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;

bunins_failed:
	BBPunfix(left->batCacheid);
	BBPunfix(right->batCacheid);
	BBPunfix(*ret);
	throw(MAL, "batstr.startsWith", OPERATION_FAILED " During bulk operation");
}

static str
STRbatPrefixcst(bat *ret, const bat *l, const str *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	bit v;

	prepareOperand(left,l,"batstr.startsWith");
	prepareResult(bn,left,TYPE_bit,"batstr.startsWith");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		STRPrefix(&v, &tl, cst);
		if (bunfastappTYPE(bit, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.startsWith",
				  OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatSuffix(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	bit v;

	prepareOperand2(left,l,right,r,"batstr.endsWith");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.endsWith", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_bit,"batstr.endsWith");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		str tr = (str) BUNtvar(righti,p);
		STRSuffix(&v, &tl, &tr);
		if (bunfastappTYPE(bit, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(right->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.endsWith", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatSuffixcst(bat *ret, const bat *l, const str *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	bit v;

	prepareOperand(left,l,"batstr.endsWith");
	prepareResult(bn,left,TYPE_bit,"batstr.endsWith");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		STRSuffix(&v, &tl, cst);
		if (bunfastappTYPE(bit, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.endsWith", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatstrSearch(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	int v;

	prepareOperand2(left,l,right,r,"batstr.search");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.search", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_int,"batstr.search");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		str tr = (str) BUNtvar(righti,p);
		STRstrSearch(&v, &tl, &tr);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(right->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.search", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatstrSearchcst(bat *ret, const bat *l, const str *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	int v;

	prepareOperand(left,l,"batstr.search");
	prepareResult(bn,left,TYPE_int,"batstr.search");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		STRstrSearch(&v, &tl, cst);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.search", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatRstrSearch(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	int v;

	prepareOperand2(left,l,right,r,"batstr.r_search");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.r_search", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_int,"batstr.r_search");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		str tr = (str) BUNtvar(righti,p);
		STRReverseStrSearch(&v, &tl, &tr);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(right->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.r_search", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatRstrSearchcst(bat *ret, const bat *l, const str *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	int v;

	prepareOperand(left,l,"batstr.r_search");
	prepareResult(bn,left,TYPE_int,"batstr.r_search");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		STRReverseStrSearch(&v, &tl, cst);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.r_search", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatTail(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	str v;
	str msg = MAL_SUCCEED;

	prepareOperand2(left,l,right,r,"batstr.string");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.string", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_str,"batstr.string");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		int *tr = (int *) BUNtloc(righti,p);
		if ((msg = STRTail(&v, &tl, tr)) != MAL_SUCCEED)
			goto bunins_failed;
		if (bunfastappVAR(bn, v) != GDK_SUCCEED)
			goto bunins_failed;
		GDKfree(v);
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;

bunins_failed:
	BBPunfix(left->batCacheid);
	BBPunfix(right->batCacheid);
	BBPunfix(*ret);
	if (msg)
		return msg;
	GDKfree(v);
	throw(MAL, "batstr.string" , OPERATION_FAILED " During bulk operation");
}

static str
STRbatTailcst(bat *ret, const bat *l, const int *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	str v;
	str msg = MAL_SUCCEED;

	prepareOperand(left,l,"batstr.string");
	prepareResult(bn,left,TYPE_str,"batstr.string");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		if ((msg = STRTail(&v, &tl, cst)) != MAL_SUCCEED)
			goto bunins_failed;
		if (bunfastappVAR(bn, v) != GDK_SUCCEED)
			goto bunins_failed;
		GDKfree(v);
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;

bunins_failed:
	BBPunfix(left->batCacheid);
	BBPunfix(*ret);
	if (msg)
		return msg;
	GDKfree(v);
	throw(MAL, "batstr.string", OPERATION_FAILED " During bulk operation");
}

static str
STRbatWChrAt(bat *ret, const bat *l, const bat *r)
{
	BATiter lefti, righti;
	BAT *bn, *left, *right;
	BUN p,q;
	int v;

	prepareOperand2(left,l,right,r,"batstr.unicodeAt");
	if(BATcount(left) != BATcount(right)) {
		BBPunfix(left->batCacheid);
		BBPunfix(right->batCacheid);
		throw(MAL, "batstr.unicodeAt", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}
	prepareResult2(bn,left,right,TYPE_int,"batstr.unicodeAt");

	lefti = bat_iterator(left);
	righti = bat_iterator(right);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		ptr tr = BUNtail(righti,p);
		STRWChrAt(&v, &tl, tr);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(right->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.unicodeAt", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	BBPunfix(right->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatWChrAtcst(bat *ret, const bat *l, const int *cst)
{
	BATiter lefti;
	BAT *bn, *left;
	BUN p,q;
	int v;

	prepareOperand(left,l,"batstr.unicodeAt");
	prepareResult(bn,left,TYPE_int,"batstr.unicodeAt");

	lefti = bat_iterator(left);

	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		STRWChrAt(&v, &tl, cst);
		if (bunfastappTYPE(int, bn, &v) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(*ret);
			throw(MAL, "batstr.unicodeAt", OPERATION_FAILED " During bulk operation");
		}
	}
	bn->tnonil = false;
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

static str
STRbatSubstitutecst(bat *ret, const bat *l, const str *arg2, const str *arg3, const bit *rep)
{
	BATiter bi;
	BAT *bn, *b;
	BUN p, q;
	str x;
	str y;
	str err = MAL_SUCCEED;

	prepareOperand(b, l, "subString");
	prepareResult(bn, b, TYPE_int, "subString");

	bi = bat_iterator(b);

	BATloop(b, p, q) {
		y = (str) str_nil;
		x = (str) BUNtvar(bi, p);
		if (!strNil(x) &&
			(err = STRSubstitute(&y, &x, arg2, arg3, rep)) != MAL_SUCCEED)
			goto bunins_failed;
		if (bunfastappVAR(bn, y) != GDK_SUCCEED)
			goto bunins_failed;
		if (y != str_nil)
			GDKfree(y);
	}
	bn->tnonil = false;
	finalizeResult(ret, bn, b);
	return MAL_SUCCEED;
bunins_failed:
	if (err == MAL_SUCCEED && y != str_nil)
		GDKfree(y);
	BBPunfix(b->batCacheid);
	BBPunfix(bn->batCacheid);
	if (err)
		return err;
	throw(MAL, "batstr.subString", OPERATION_FAILED " During bulk operation");
}

/*
 * The substring functions require slightly different arguments
 */
static str
STRbatsubstringcst(bat *ret, const bat *bid, const int *start, const int *length)
{
	BATiter bi;
	BAT *b,*bn;
	BUN p, q;
	str res;
	char *msg = MAL_SUCCEED;

	if( (b= BATdescriptor(*bid)) == NULL)
		throw(MAL, "batstr.substring", SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);
	bn= COLnew(b->hseqbase, TYPE_str, BATcount(b)/10+5, TRANSIENT);
	if (bn == NULL) {
		BBPunfix(b->batCacheid);
		throw(MAL, "batstr.substring", SQLSTATE(HY013) MAL_MALLOC_FAIL);
	}
	bn->tsorted = b->tsorted;
	bn->trevsorted = b->trevsorted;

	bi = bat_iterator(b);
	BATloop(b, p, q) {
		str t =  (str) BUNtvar(bi, p);

		if ((msg = STRsubstring(&res, &t, start, length)) != MAL_SUCCEED ||
			BUNappend(bn, (ptr)res, false) != GDK_SUCCEED) {
			BBPunfix(b->batCacheid);
			BBPunfix(bn->batCacheid);
			if (msg != MAL_SUCCEED)
				return msg;
			GDKfree(res);
			throw(MAL, "batstr.substring", SQLSTATE(HY013) MAL_MALLOC_FAIL);
		}
		GDKfree(res);
	}

	bn->tnonil = false;
	*ret = bn->batCacheid;
	BBPkeepref(bn->batCacheid);
	BBPunfix(b->batCacheid);
	return msg;
}

static str
STRbatsubstring(bat *ret, const bat *l, const bat *r, const bat *t)
{
	BATiter lefti, starti, lengthi;
	BAT *bn, *left, *start, *length;
	BUN p,q;
	str v;

	if( (left= BATdescriptor(*l)) == NULL )
		throw(MAL, "batstr.substring", SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);
	if( (start= BATdescriptor(*r)) == NULL ){
		BBPunfix(left->batCacheid);
		throw(MAL, "batstr.substring", SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);
	}
	if( (length= BATdescriptor(*t)) == NULL ){
		BBPunfix(left->batCacheid);
		BBPunfix(start->batCacheid);
		throw(MAL, "batstr.substring", SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);
	}
	if (BATcount(left) != BATcount(start) ||
		BATcount(left) != BATcount(length)) {
		BBPunfix(left->batCacheid);
		BBPunfix(start->batCacheid);
		BBPunfix(length->batCacheid);
		throw(MAL, "batstr.substring", ILLEGAL_ARGUMENT " Requires bats of identical size");
	}

	bn= COLnew(left->hseqbase, TYPE_str,BATcount(left), TRANSIENT);
	if( bn == NULL){
		BBPunfix(left->batCacheid);
		BBPunfix(start->batCacheid);
		BBPunfix(length->batCacheid);
		throw(MAL, "batstr.substring", SQLSTATE(HY013) MAL_MALLOC_FAIL);
	}

	bn->tsorted=false;
	bn->trevsorted=false;

	lefti = bat_iterator(left);
	starti = bat_iterator(start);
	lengthi = bat_iterator(length);
	BATloop(left, p, q) {
		str tl = (str) BUNtvar(lefti,p);
		int *t1 = (int *) BUNtloc(starti,p);
		int *t2 = (int *) BUNtloc(lengthi,p);
		str msg;
		if ((msg = STRsubstring(&v, &tl, t1, t2)) != MAL_SUCCEED ||
			BUNappend(bn, v, false) != GDK_SUCCEED) {
			BBPunfix(left->batCacheid);
			BBPunfix(start->batCacheid);
			BBPreclaim(bn);
			if (msg)
				return msg;
			GDKfree(v);
			throw(MAL, "batstr.substring", SQLSTATE(HY013) MAL_MALLOC_FAIL);
		}
		GDKfree(v);
	}
	bn->tnonil = false;
	BBPunfix(start->batCacheid);
	BBPunfix(length->batCacheid);
	finalizeResult(ret,bn,left);
	return MAL_SUCCEED;
}

#include "mel.h"
mel_func batstr_init_funcs[] = {
 command("batstr", "length", STRbatLength, false, "Return the length of a string.", args(1,2, batarg("",int),batarg("s",str))),
 command("batstr", "nbytes", STRbatBytes, false, "Return the string length in bytes.", args(1,2, batarg("",int),batarg("s",str))),
 command("batstr", "toLower", STRbatLower, false, "Convert a string to lower case.", args(1,2, batarg("",str),batarg("s",str))),
 command("batstr", "toUpper", STRbatUpper, false, "Convert a string to upper case.", args(1,2, batarg("",str),batarg("s",str))),
 command("batstr", "trim", STRbatStrip, false, "Strip whitespaces around a string.", args(1,2, batarg("",str),batarg("s",str))),
 command("batstr", "ltrim", STRbatLtrim, false, "Strip whitespaces from start of a string.", args(1,2, batarg("",str),batarg("s",str))),
 command("batstr", "rtrim", STRbatRtrim, false, "Strip whitespaces from end of a string.", args(1,2, batarg("",str),batarg("s",str))),
 command("batstr", "trim", STRbatStrip2_const, false, "Strip characters in the second string around the first strings.", args(1,3, batarg("",str),batarg("s",str),arg("s2",str))),
 command("batstr", "ltrim", STRbatLtrim2_const, false, "Strip characters in the second string from start of the first strings.", args(1,3, batarg("",str),batarg("s",str),arg("s2",str))),
 command("batstr", "rtrim", STRbatRtrim2_const, false, "Strip characters in the second string from end of the first strings.", args(1,3, batarg("",str),batarg("s",str),arg("s2",str))),
 command("batstr", "trim", STRbatStrip2_bat, false, "Strip characters in the second strings around the first strings.", args(1,3, batarg("",str),batarg("s",str),batarg("s2",str))),
 command("batstr", "ltrim", STRbatLtrim2_bat, false, "Strip characters in the second strings from start of the first strings.", args(1,3, batarg("",str),batarg("s",str),batarg("s2",str))),
 command("batstr", "rtrim", STRbatRtrim2_bat, false, "Strip characters in the second strings from end of the first strings.", args(1,3, batarg("",str),batarg("s",str),batarg("s2",str))),
 command("batstr", "lpad", STRbatLpad_const, false, "Prepend whitespaces to the strings to reach the given length. Truncate the strings on the right if their lengths is larger than the given length.", args(1,3, batarg("",str),batarg("s",str),arg("n",int))),
 command("batstr", "rpad", STRbatRpad_const, false, "Append whitespaces to the strings to reach the given length. Truncate the strings on the right if their lengths is larger than the given length.", args(1,3, batarg("",str),batarg("s",str),arg("n",int))),
 command("batstr", "lpad", STRbatLpad_bat, false, "Prepend whitespaces to the strings to reach the given lengths. Truncate the strings on the right if their lengths is larger than the given lengths.", args(1,3, batarg("",str),batarg("s",str),batarg("n",int))),
 command("batstr", "rpad", STRbatRpad_bat, false, "Append whitespaces to the strings to reach the given lengths. Truncate the strings on the right if their lengths is larger than the given lengths.", args(1,3, batarg("",str),batarg("s",str),batarg("n",int))),
 command("batstr", "lpad", STRbatLpad2_const_const, false, "Prepend the second string to the first strings to reach the given length. Truncate the first strings on the right if their lengths is larger than the given length.", args(1,4, batarg("",str),batarg("s",str),arg("n",int),arg("s2",str))),
 command("batstr", "rpad", STRbatRpad2_const_const, false, "Append the second string to the first strings to reach the given length. Truncate the first strings on the right if their lengths is larger than the given length.", args(1,4, batarg("",str),batarg("s",str),arg("n",int),arg("s2",str))),
 command("batstr", "lpad", STRbatLpad2_bat_const, false, "Prepend the second string to the first strings to reach the given lengths. Truncate the first strings on the right if their lengths is larger than the given lengths.", args(1,4, batarg("",str),batarg("s",str),batarg("n",int),arg("s2",str))),
 command("batstr", "rpad", STRbatRpad2_bat_const, false, "Append the second string to the first strings to reach the given lengths. Truncate the first strings on the right if their lengths is larger than the given lengths.", args(1,4, batarg("",str),batarg("s",str),batarg("n",int),arg("s2",str))),
 command("batstr", "lpad", STRbatLpad2_const_bat, false, "Prepend the second strings to the first strings to reach the given length. Truncate the first strings on the right if their lengths is larger than the given length.", args(1,4, batarg("",str),batarg("s",str),arg("n",int),batarg("s2",str))),
 command("batstr", "rpad", STRbatRpad2_const_bat, false, "Append the second strings to the first strings to reach the given length. Truncate the first strings on the right if their lengths is larger than the given length.", args(1,4, batarg("",str),batarg("s",str),arg("n",int),batarg("s2",str))),
 command("batstr", "lpad", STRbatLpad2_bat_bat, false, "Prepend the second strings to the first strings to reach the given lengths. Truncate the first strings on the right if their lengths is larger than the given lengths.", args(1,4, batarg("",str),batarg("s",str),batarg("n",int),batarg("s2",str))),
 command("batstr", "rpad", STRbatRpad2_bat_bat, false, "Append the second strings to the first strings to reach the given lengths. Truncate the first strings on the right if their lengths is larger than the given lengths.", args(1,4, batarg("",str),batarg("s",str),batarg("n",int),batarg("s2",str))),
 command("batstr", "startsWith", STRbatPrefix, false, "Prefix check.", args(1,3, batarg("",bit),batarg("s",str),batarg("prefix",str))),
 command("batstr", "startsWith", STRbatPrefixcst, false, "Prefix check.", args(1,3, batarg("",bit),batarg("s",str),arg("prefix",str))),
 command("batstr", "endsWith", STRbatSuffix, false, "Suffix check.", args(1,3, batarg("",bit),batarg("s",str),batarg("suffix",str))),
 command("batstr", "endsWith", STRbatSuffixcst, false, "Suffix check.", args(1,3, batarg("",bit),batarg("s",str),arg("suffix",str))),
 command("batstr", "search", STRbatstrSearch, false, "Search for a substring. Returns position, -1 if not found.", args(1,3, batarg("",int),batarg("s",str),batarg("c",str))),
 command("batstr", "search", STRbatstrSearchcst, false, "Search for a substring. Returns position, -1 if not found.", args(1,3, batarg("",int),batarg("s",str),arg("c",str))),
 command("batstr", "r_search", STRbatRstrSearch, false, "Reverse search for a substring. Returns position, -1 if not found.", args(1,3, batarg("",int),batarg("s",str),batarg("c",str))),
 command("batstr", "r_search", STRbatRstrSearchcst, false, "Reverse search for a substring. Returns position, -1 if not found.", args(1,3, batarg("",int),batarg("s",str),arg("c",str))),
 command("batstr", "string", STRbatTail, false, "Return the tail s[offset..n] of a string s[0..n].", args(1,3, batarg("",str),batarg("b",str),batarg("offset",int))),
 command("batstr", "string", STRbatTailcst, false, "Return the tail s[offset..n] of a string s[0..n].", args(1,3, batarg("",str),batarg("b",str),arg("offset",int))),
 command("batstr", "substring", STRbatsubstring, false, "Substring extraction using [start,start+length]", args(1,4, batarg("",str),batarg("s",str),batarg("start",int),batarg("index",int))),
 command("batstr", "substring", STRbatsubstringcst, false, "Substring extraction using [start,start+length]", args(1,4, batarg("",str),batarg("s",str),arg("start",int),arg("index",int))),
 command("batstr", "unicodeAt", STRbatWChrAt, false, "get a unicode character (as an int) from a string position.", args(1,3, batarg("",int),batarg("s",str),batarg("index",int))),
 command("batstr", "unicodeAt", STRbatWChrAtcst, false, "get a unicode character (as an int) from a string position.", args(1,3, batarg("",int),batarg("s",str),arg("index",int))),
 command("batstr", "substitute", STRbatSubstitutecst, false, "Substitute first occurrence of 'src' by\n'dst'.  Iff repeated = true this is\nrepeated while 'src' can be found in the\nresult string. In order to prevent\nrecursion and result strings of unlimited\nsize, repeating is only done iff src is\nnot a substring of dst.", args(1,5, batarg("",str),batarg("s",str),arg("src",str),arg("dst",str),arg("rep",bit))),
 { .imp=NULL }
};
#include "mal_import.h"
#ifdef _MSC_VER
#undef read
#pragma section(".CRT$XCU",read)
#endif
LIB_STARTUP_FUNC(init_batstr_mal)
{ mal_module("batstr", NULL, batstr_init_funcs); }
