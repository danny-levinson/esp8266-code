/*
 * CommonGlobals.cpp
 *
 *  Created on: Oct. 13, 2026
 *      Author: Danny
 */

#include "CommonGlobals.h"

#include "DataSaver.h"

int cg_tzone = 0, cg_dst = 0, cg_offset = 0;

const static char *cgfilename = "globals.dat";

CommonGlobals common_globals;

CommonGlobals::CommonGlobals() :
	globalSaver(&cglobals, sizeof(cglobals), cgfilename) {
}

bool CommonGlobals::save() {
	cglobals = { cg_tzone, cg_dst, cg_offset };
	return globalSaver.write();
}

bool CommonGlobals::restore() {
	if (! globalSaver.read()) return false;
	cg_tzone = cglobals.tzone;
	cg_dst = cglobals.dst;
	cg_offset = cglobals.offset;
	return true;
}
