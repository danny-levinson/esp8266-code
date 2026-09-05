/*
 * FileSys.cpp
 *
 *  Created on: Oct. 1, 2024
 *      Author: Danny
 */

#include "FileSys.h"

#include "SerialDebugHelper.h"


void safestrncpy(char *dst, const char *src, int maxlen) {
	strncpy(dst, src, maxlen-1);
	dst[maxlen-1] = '\0';
}

static bool fs_is_started = false;

void setup_file_system() { // Start the file system and (maybe) list all contents
	if (! fs_is_started) {
		if (FileSys.begin()) {                         // Start the File System (SPIFFS or LittleFS)
			fs_is_started = true;
		}
	}
}

bool file_system_is_running() {
	return fs_is_started;
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
	if (bytes < 1024) {
		return String(bytes) + "B";
	} else if (bytes < (1024 * 1024)) {
		return String(bytes / 1024.0) + "KB";
	} else if (bytes < (1024 * 1024 * 1024)) {
		return String(bytes / 1024.0 / 1024.0) + "MB";
	}
	return "?";
}

void print_files_to_serial(bool listall) {
	int filecount = 0;
	Dir dir = FileSys.openDir("/");
	while (dir.next()) filecount++;                 // Count the file system contents
	SPRNTF(1, "File system started. %d files in /\r\n", filecount);
	if (! listall) return;

	dir = FileSys.openDir("/");
	while (dir.next()) {                      // List the file system contents
		String fileName = dir.fileName();
		size_t fileSize = dir.fileSize();
		SPRNTF(1, "\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
	}
	SPRNTF(1, "\n");
}



