/*
 * CommonGlobals.h
 *
 *  Created on: Oct. 13, 2026
 *      Author: Danny
 */

#ifndef COMMON_SRC_COMMONGLOBALS_H_
#define COMMON_SRC_COMMONGLOBALS_H_

#include "DataSaver.h"

/*
 * Delay time is kept in DelayTimings.cpp rather than here
 * as it does not need to be visible except for tuning
 */

extern int cg_offset, cg_tzone, cg_dst;


class CommonGlobals
{
	struct {
		int tzone, dst, offset;
	} cglobals;
	DataSaver globalSaver;
public:
	CommonGlobals();
	bool save();
	bool restore();

// utilities
	bool dataWasInitialised();
	bool restoreDataFromFile();
	void saveDataToFile();
	void deleteDataFromFile();
	void showGVars();
};

extern CommonGlobals common_globals;


struct g_data_struct {
	int *gvar;
	char *uiname;
};

extern g_data_struct g_data_with_names[];


#endif /* COMMON_SRC_COMMONGLOBALS_H_ */
