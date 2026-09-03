/*
 * FileSys.h
 *
 *  Created on: Oct. 1, 2024
 *      Author: Danny
 */

#ifndef FILESYS_H_
#define FILESYS_H_


#include <FS.h>

//#define USE_SPIFFS X

#ifdef USE_SPIFFS
#define FileSys SPIFFS
#else
#include <LittleFS.h>
#define FileSys LittleFS
#endif


void setup_file_system();

String formatBytes(size_t bytes);

void print_files_to_serial(bool listall);			// for debugging

void safestrncpy(char *dst, const char *src, int maxlen);

#endif /* FILESYS_H_ */
