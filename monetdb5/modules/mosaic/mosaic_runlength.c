/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

/*
 * authors Martin Kersten, Aris Koning
 * Run-length encoding framework for a single chunk
 */

#include "monetdb_config.h"
#include "mosaic.h"
#include "mosaic_runlength.h"
#include "mosaic_private.h"

bool MOStypes_runlength(BAT* b) {
	switch(b->ttype){
	case TYPE_bit: return true; // Will be mapped to bte
	case TYPE_bte: return true;
	case TYPE_sht: return true;
	case TYPE_int: return true;
	case TYPE_lng: return true;
	case TYPE_oid: return true;
	case TYPE_flt: return true;
	case TYPE_dbl: return true;
#ifdef HAVE_HGE
	case TYPE_hge: return true;
#endif
	default:
		if (b->ttype == TYPE_date) {return true;} // Will be mapped to int
		if (b->ttype == TYPE_daytime) {return true;} // Will be mapped to lng
		if (b->ttype == TYPE_timestamp) {return true;} // Will be mapped to lng
	}

	return false;
}

#define METHOD runlength
#define METHOD_TEMPLATES_INCLUDE MAKE_TEMPLATES_INCLUDE_FILE(METHOD)

#define COMPRESSION_DEFINITION
#define TPE bte
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#define TPE sht
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#define TPE int
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#define TPE lng
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#define TPE flt
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#define TPE dbl
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#ifdef HAVE_HGE
#define TPE hge
#include METHOD_TEMPLATES_INCLUDE
#undef TPE
#endif
#undef COMPRESSION_DEFINITION

#define TPE bte
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#define TPE sht
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#define TPE int
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#define TPE lng
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#define TPE flt
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#define TPE dbl
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#ifdef HAVE_HGE
#define TPE hge
#include "mosaic_select_template.h"
#include "mosaic_projection_template.h"
#undef TPE
#endif

#define outer_loop_runlength(HAS_NIL, NIL_MATCHES, TPE, LEFT_CI_NEXT, RIGHT_CI_NEXT) \
do {\
	const TPE lval = GET_VAL_runlength(task, TPE);\
	if (HAS_NIL && !NIL_MATCHES) {\
		if (IS_NIL(TPE, lval)) { break;}\
	}\
    for (oid lo = canditer_peekprev(task->ci); !is_oid_nil(lo) && lo < last; lo = LEFT_CI_NEXT(task->ci)) {\
		INNER_LOOP_UNCOMPRESSED(HAS_NIL, TPE, RIGHT_CI_NEXT);\
	}\
} while (0)

MOSjoin_COUI_DEF(runlength, bte)
MOSjoin_COUI_DEF(runlength, sht)
MOSjoin_COUI_DEF(runlength, int)
MOSjoin_COUI_DEF(runlength, lng)
MOSjoin_COUI_DEF(runlength, flt)
MOSjoin_COUI_DEF(runlength, dbl)
#ifdef HAVE_HGE
MOSjoin_COUI_DEF(runlength, hge)
#endif

#undef METHOD_TEMPLATES_INCLUDE
#undef METHOD
