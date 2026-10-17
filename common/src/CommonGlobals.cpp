/*
 * CommonGlobals.cpp
 *
 *  Created on: Oct. 13, 2026
 *      Author: Danny
 */

#include "CommonGlobals.h"

#include "DataSaver.h"
#include "SerialDebugHelper.h"

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


extern g_data_struct g_data_with_names[];

static bool data_was_initialised = false;

#define DFNAME "/gdata"
static const char *newdatafilename = DFNAME ".new";
static const char *bkpdatafilename = DFNAME ".bkp";
static const char *curdatafilename = DFNAME ".dat";


bool CommonGlobals::dataWasInitialised() {
	return data_was_initialised;
}

bool CommonGlobals::restoreDataFromFile() {
    File f = FileSys.open(curdatafilename, "r");
    if (! f) {
    	SPRTLN(1, "can't open data file for reading"); // @suppress("Method cannot be resolved")
    	data_was_initialised = false;
    	return false;
    }
    while (f.available() > 0) {
    	static int gdptr = 0;
    	String s = f.readStringUntil('\0');
    	int d = 0;
    	f.readBytes((char*)&d, sizeof(int));
    	//SPRNTF(1, "got '%s' = %d (%d)\r\n", s.c_str(), d, f.available());
		if (s.equals(g_data_with_names[gdptr].uiname)) {
			*g_data_with_names[gdptr++].gvar = d;		// this should usually work
		} else {
			// otherwise, it's linear search
			for (gdptr=0; g_data_with_names[gdptr].gvar != NULL; gdptr++) {
				if (s.equals(g_data_with_names[gdptr].uiname)) {
					*g_data_with_names[gdptr++].gvar = d;
					break;
				}
			}
		}
		if (g_data_with_names[gdptr].gvar == NULL) gdptr = 0;
    }
    data_was_initialised = true;
    common_globals.restore();			// override values of time modifiers from common repository
    SPRNTF(1, "restored data from file, offset %d, tzone: %d, dst: %d\n", cg_offset, cg_tzone, cg_dst); // @suppress("Method cannot be resolved")
   	return true;
}

void CommonGlobals::saveDataToFile() {
    File f = FileSys.open(newdatafilename, "w");
	for (int i=0; g_data_with_names[i].gvar != NULL; i++) {
		f.write((char*)g_data_with_names[i].uiname);
		f.write((char)'\0');		// write doesn't null terminate strings so we write a (char)0 [not (char*)]
		f.write((char*)(&*g_data_with_names[i].gvar), sizeof(int));
	}
	f.close();
	if (FileSys.exists(bkpdatafilename)) FileSys.remove(bkpdatafilename);
	if (FileSys.exists(curdatafilename)) FileSys.rename(curdatafilename, bkpdatafilename);
	FileSys.rename(newdatafilename, curdatafilename);
	data_was_initialised = true;
    SPRNTF(1, "saved data to file, offset %d, tzone: %d, dst: %d\n", cg_offset, cg_tzone, cg_dst);
	SPRTLN(1, "data saved to file");
	common_globals.save();				// save time mods, etc, to the common repository
	restoreDataFromFile();
}


// we don't delete the .bkp file though
void CommonGlobals::deleteDataFromFile() {
	if (FileSys.exists(curdatafilename)) {
		FileSys.remove(curdatafilename);
	}
	data_was_initialised = false;
}

void CommonGlobals::showGVars() {
	char outstr[120] = "", *p = outstr;
	for (int i=0; g_data_with_names[i].gvar != NULL; i++) {
		sprintf(p, "%s=%d, ", g_data_with_names[i].uiname, *g_data_with_names[i].gvar);
		p += strlen(p);
		if (p - outstr > 50) {
			SPRTLN(1, outstr); p = outstr; outstr[0] = '\0';
		}
	}
	if (p != outstr) SPRTLN(1, outstr);
	saveDataToFile();
}
