/*
 * DataSaver.h
 *
 *  Created on: Aug. 31, 2026
 *      Author: Danny
 */

#ifndef DCCOMMON_DATASAVER_H_
#define DCCOMMON_DATASAVER_H_

#include "FileSys.h"


class DataSaver {
public:
    DataSaver(void* struct_ptr, size_t struct_size, const char* filename);

    bool write();
    bool read();

private:
    void* ptr;
    size_t size;
    const char* fname;
};



#endif /* DCCOMMON_DATASAVER_H_ */
