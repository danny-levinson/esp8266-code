/*
 * gdata.h
 *
 *  Created on: Sep. 23, 2019
 *      Author: Danny
 */

#ifndef GDATA_H_
#define GDATA_H_

// ui state data, defined in websocket.cpp, initialised from file system file

extern int g_offset, g_tzone, g_dst, g_manual, g_damperopen;


struct g_data_struct {
	int *gvar;
	char *uiname;
};

extern g_data_struct g_data_with_names[];



#endif /* GDATA_H_ */
