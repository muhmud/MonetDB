/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "monetdb_config.h"
#include "msabaoth.h"
#include "merovingian.h"
#include "mapi.h"
#include "snapshot.h"

static err validate_destination(const char *dest);

/* Create a snapshot of database dbname to file dest.
 * TODO: verify that dest is a safe location.
 * TODO: Make it work for databases without monetdb/monetdb root account.
 */
err
snapshot_adhoc(char *dbname, char *dest)
{
	err e = NO_ERR;
	sabdb *stats = NULL;
	int port = -1;
	Mapi conn = NULL;
	MapiHdl handle = NULL;


	/* First look up the database in our administration. */
	e = msab_getStatus(&stats, dbname);
	if (e != NO_ERR) {
		goto bailout;
	}
	if (!stats) {
		e = newErr("No such database: '%s'", dbname);
		goto bailout;
	}

	/* Do not overwrite random files on the system. */
	e = validate_destination(dest);
	if (e != NO_ERR) {
		goto bailout;
	}

	/* Connect. This is a dirty hack, making two assumptions:
	 * 1. we're listening on localhost
	 * 2. the database has a root user 'monetdb' with password 'monetdb'.
	 * We'll improve this when have figured out how to authenticate.
	 */
	port = getConfNum(_mero_props, "port");
	conn = mapi_connect("localhost", port, "monetdb", "monetdb", "sql", dbname);
	if (conn == NULL || mapi_error(conn)) {
		e = newErr("connection error: %s", mapi_error_str(conn));
		goto bailout;
	}

	/* Trigger the snapshot */
	handle = mapi_prepare(conn, "CALL sys.hot_snapshot(?)");
	if (handle == NULL || mapi_error(conn)) {
		e = newErr("prepare failed: %s", mapi_error_str(conn));
		goto bailout;
	}
	if (mapi_param_string(handle, 0, 12, dest, NULL) != MOK) {
		e = newErr("internal error: mapi_param_string: %s", mapi_error_str(conn));
		goto bailout;
	}
	if (mapi_execute(handle) != MOK) {
		e = newErr("internal error: execute failed: %s", mapi_result_error(handle));
		goto bailout;
	}

	/* everything seems ok. Leave e == NO_ERR */
bailout:
	if (handle != NULL)
		mapi_close_handle(handle);
	if (conn != NULL)
		mapi_destroy(conn);
	if (stats != NULL)
		msab_freeStatus(&stats);
	return e;
}

static err
validate_destination(const char *dest)
{
	err e = NO_ERR;
	/* these are malloc'ed: */
	char *resolved_snapdir = NULL;
	char *dest_dup = NULL;
	char *resolved_destination = NULL;
	/* and these are not: */
	char *configured_snapdir = NULL;
	char *dest_dir = NULL;

	/* Get the canonical path of the snapshot directory */
	configured_snapdir = getConfVal(_mero_props, "snapshotdir");
	if (configured_snapdir == NULL || configured_snapdir[0] == '\0') {
		e = newErr("Snapshot target file not allowed because no 'snapshotdir' has been configured");
		goto bailout;
	}
	if (configured_snapdir[0] != '/') {
		e = newErr("Setting 'snapshotdir' should be an absolute path");
		goto bailout;
	}
	resolved_snapdir = realpath(configured_snapdir, NULL);
	if (resolved_snapdir == NULL) {
		e = newErr("Cannot resolve configured snapshot directory '%s': %s",
			configured_snapdir, strerror(errno));
		goto bailout;
	}

	/* resolve the directory the snapshot is about to be written to */
	dest_dup = strdup(dest);
	dest_dir = dirname(dest_dup);

	resolved_destination = realpath(dest_dir, NULL);
	if (resolved_destination == NULL) {
		e = newErr("Cannot resolve snapshot target directory '%s': %s",
			dest_dir, strerror(errno));
		goto bailout;
	}

	/* snapdir must be a prefix of destination */
	if (
		strlen(resolved_snapdir) > strlen(resolved_destination)
		|| 0 != strncmp(
			resolved_snapdir,
			resolved_destination,
			strlen(resolved_snapdir))
	) {
		e = newErr("Snapshot target '%s' is not inside configured snapshot directory '%s'",
			dest, resolved_snapdir);
		goto bailout;
	}



bailout:
	free(resolved_snapdir);
	free(dest_dup);
	free(resolved_destination);

	return e;
}