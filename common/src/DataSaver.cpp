/*
 * DataSaver.cpp
 *
 *  Created on: Aug. 31, 2026
 *      Author: Danny
 */


#include "DataSaver.h"

DataSaver::DataSaver(void* struct_ptr, size_t struct_size, const char* filename)
				: ptr(struct_ptr), size(struct_size), fname(filename) {}

bool DataSaver::write() {
	File f = FileSys.open(fname, "w");
	if (!f) return false;
	size_t written = f.write(reinterpret_cast<uint8_t*>(ptr), size);
	f.close();
	return written == size;
}

bool DataSaver::read() {
	File f = FileSys.open(fname, "r");
	if (!f) return false;
	size_t read_bytes = f.read(reinterpret_cast<uint8_t*>(ptr), size);
	f.close();
	return read_bytes == size;
}
