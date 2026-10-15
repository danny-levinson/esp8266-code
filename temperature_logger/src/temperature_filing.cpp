/*
 * tempfiles.cpp
 *
 *  Created on: Sep. 26, 2021
 *      Author: Danny
 */



#include "FixedQueueArray.h"
#include "MsgLogHandler.h"

#include "FileSys.h"

#include "temperature_filing.h"
#include "temperature_logger.h"


#define MAXTFILES 15              // 15 files at 60000 bytes each ~= 900,000 < 1M
#define MAXFILESIZE 60000         // 3.47 days @ 12 bytes/record, 4 days is 4*24*60*12 = 69120

#define FNAMEPFX "tdatafile_"		// for SPIFFS use "/tdatafile_"!


static bool Stopped = false;		// queue handling stopped because of a fatal error so as not to make things worse

void exit_handler() {
	Stopped = true;         // stop any further processing of queues
	MSGE("!! fixed queue handling has broken !!");
}

static void get_file_name(char *nbuf, int idx) {
	//if (idx < 10) sprintf(nbuf, "%s%d", FNAMEPFX,idx); else // causes first file in empty store to be tdatafile_1 instead of tdatafile_00001
	sprintf(nbuf, "%s%05d", FNAMEPFX, idx);
}

struct fqentry {
	unsigned long firsttime, lasttime;
	int fidx;
};
FixedSizeQueueArray<fqentry> fidxstore(MAXTFILES);
fqentry currentfile = {0,0,0};                          // so we can change lasttime as we add to it

static int get_latest_fs_file_num() {
  return currentfile.fidx;
}

static bool fileStoreIsEmpty() {
	bool r = fidxstore.isEmpty() && currentfile.fidx == 0;
	MSGV("calling fileStoreIsEmpty (result: %s)", r ? "true" : "false");
	return r;
}

static bool isAllDigits(String &s) {
	for (byte i=0; i<s.length(); i++) {
		if (! isDigit(s.charAt(i))) return false;
	}
	return true;
}

// called as needed from write_bytes_to_store() etc.
// typically called the first time bytes need to be written to a file


static void setup_file_queue() {
	MSGV("setting up file queue");
//	fidxstore.setPrinter(ErrorHandler);
	fidxstore.setExitHandler(exit_handler);
	Dir dir = FileSys.openDir("/");
	int minidx = -1, maxidx = -1;
	while (dir.next()) {                      // find first & last data files by suffix order
		String fileName = dir.fileName();
		size_t fileSize = dir.fileSize();
		if (! fileName.startsWith(FNAMEPFX)) continue;
		String suffix = fileName.substring(strlen(FNAMEPFX));
		if (! isAllDigits(suffix)) { MSGW("bad filename '%s'", fileName.c_str()); continue; }
		int idx = atoi(suffix.c_str());
		if (idx <= 0) { MSGW("bad filename '%s'", fileName.c_str()); continue; }
		if (fileSize == 0)  { MSGW("file '%s' is zero size", fileName.c_str()); continue; }
		if (fileSize % sizeof(fqentry) != 0)  { MSGW("bad size %d for file '%s'", fileSize, fileName.c_str()); continue; }
		if (minidx == -1 || idx < minidx) minidx = idx;
		if (maxidx == -1 || idx > maxidx) maxidx = idx;
	}
	MSGV("Found files between %d and %d", minidx, maxidx);
	//dir. closeDir();  // no way to close dir
	if (minidx == -1 || maxidx == -1) { MSGW("no data files found"); return; }
	// there are files, so iterate over them and populate fidxstore
	for (int idx = minidx; idx <= maxidx; idx++) {
		char fnm[40];
		get_file_name(fnm, idx);
		File f = FileSys.open(fnm, "r");
		if (! f) { MSGW("can't open file '%s' (missing from sequence?)", fnm); continue; }
		// add start & end times in file idx to fbuf entry
		fqentry fbuf;
		fbuf.fidx = idx;
		qentry buf;
		int bytesread = f.readBytes((char*)&buf, sizeof(qentry));
		if (bytesread != sizeof(qentry)) { MSGW("can't read from start of file '%s'", fnm); continue; }
		fbuf.firsttime = buf.ptime;
		f.seek(sizeof(qentry), SeekEnd);
		bytesread = f.readBytes((char*)&buf, sizeof(qentry));
		if (bytesread != sizeof(qentry)) { MSGW("can't read from end of file '%s'", fnm); continue; }
		fbuf.lasttime = buf.ptime;
		if (idx == maxidx) {
			currentfile = fbuf;
		} else {
			if (! Stopped) fidxstore.enqueue(fbuf);
		}
	}
}

// called from tempstore.cpp

bool write_bytes_to_file_store(unsigned char *b, int len) {
	//MSGV("writing %d bytes to store", len);
	char fnm[40];
	if (fileStoreIsEmpty()) {                // initialize the system
		setup_file_queue();
	}
	//MSGV("after first setup");
	if (fileStoreIsEmpty()) {                 // create the first file
		get_file_name(fnm, 1);
		MSGV("store still empty - writing '%s'", fnm);
		File f = FileSys.open(fnm, "w");
		if (! f) { MSGW("can't open first file '%s' for writing", fnm); return false; }
		f.write(b, len);
		f.close();
		MSGV("reading files again");
		setup_file_queue();
		return true;
	}
	//MSGV("after sys init check");
	get_file_name(fnm, currentfile.fidx);
	File f = FileSys.open(fnm, "a");
	if (! f) { MSGW("can't open last file '%s' for appending", fnm); return false; }
	int fsz = f.size();
	if (fsz > MAXFILESIZE) {                // time to open a new one & perhaps drop an old   -- should loop till < MAXTFILES
		f.close();
		if (! Stopped && fidxstore.count() >= MAXTFILES) {
			fqentry oldest = fidxstore.dequeue();                // drop oldest
			get_file_name(fnm, oldest.fidx);                                    // and delete the contents
			FileSys.remove(fnm);                                                // !!! should check for errors
			MSGV("removed old file '%s'", fnm);
		}
		// next we open a new file, add it to the store, mark where it begins
		if (! Stopped) fidxstore.enqueue(currentfile);
		currentfile.firsttime = ((qentry*)&b)->ptime;
		currentfile.fidx += 1;                                  // create new file with next name
		get_file_name(fnm, currentfile.fidx);
		f = FileSys.open(fnm, "w");
		if (! f) { MSGW("can't open new file '%s' for writing", fnm); return false; }
	}
	// now calculate when the file ends (for all cases) from b+len-sizeof(qentry)
	// update currentfile, and finally we add the bytes to the file
	currentfile.lasttime = ((qentry*)(&b+len-sizeof(qentry)))->ptime;
	int w = f.write(b, len);
	if (w != len) { MSGW("only wrote %d bytes to '%s', %d wanted", w, fnm, len); return false; }
	f.close();
	return true;
}


//void test_queue() {
//  FixedSizeQueueArray<char> testqueue;
//  testqueue.enqueue('a');
//  testqueue.enqueue('b');
//  testqueue.enqueue('c');
//  char y = testqueue.peek();
//  char x = testqueue.dequeue();
//  MSG("queue test: added a, b, c, peek: " + String(y) + " dequeued: " + x);
//}

// ================================= data gathering ==================================


static std::tuple<int, int> find_starting_point(int latest_idx, int fstart) {
	int start_idx = latest_idx;
	while (1) {
		char fnm[40];
		get_file_name(fnm, start_idx);
		File f = FileSys.open(fnm, "r");
		if (! f) {
			MSGE("can't open file '%s' for reading", fnm);
			return std::make_tuple(start_idx + 1, 0);
		}
		int fsz = f.size();
		f.close();
		int entries_in_file = fsz / sizeof(qentry);
		if (fstart <= entries_in_file) {
			return std::make_tuple(start_idx, entries_in_file - fstart);
		}
		fstart -= entries_in_file;
		start_idx -= 1;
	}
}

// put into qearr nentries data records from files starting fstart records back
// returns number of entries put into the array, plus the number of entries represented including skips

static std::tuple<int, int> read_last_entries_from_files(qentry *qearr, int fstart, int nentries, int nskip) {
	MSGV("reading last entries");
	if (fileStoreIsEmpty()) {               // initialize the system
		setup_file_queue();
	}
	MSGV("file store ready");
	int latest_file_idx = get_latest_fs_file_num();
	if (latest_file_idx == 0) return std::make_tuple(0, 0);              // no files or not initialised!!!
	MSGV("finding start point");
	std::tuple<int, int> start_point = find_starting_point(latest_file_idx, fstart);
	int findex = std::get<0>(start_point);
	int foffset = std::get<1>(start_point);
	char fnm[40];
	get_file_name(fnm, findex);
	MSGV("opening file %s", fnm);
	File f = FileSys.open(fnm, "r");
	MSGV("found starting point in file #%d, entries: %d at offset %d", findex, f.size()/sizeof(qentry), foffset);
	if (! f) {
		MSGE("can't open file '%s' for reading", fnm);
		return std::make_tuple(0, 0); // @suppress("Invalid arguments")
	}
	f.seek(foffset*sizeof(qentry), SeekSet);
	int entries_from_file = f.size() / sizeof(qentry) - foffset;
	int qpos = 0;               // position of next entry in qearr, also count of entries in qearr
	//int entries_to_read = min(entries_from_file, nentries);
	int skipcnt = 1, entries_represented = 0;
	MSGV("looping");
	while (1) {
		int tpos = 0;           // count of entries added to qearr for just this file
		MSGV("getting from file: %d, up to: %d entries, skips: %d", findex, entries_from_file, nskip);
		for (int i=0; i<entries_from_file; i++) {
			qentry qe;
			int bytesread = f.readBytes((char*)&qe, sizeof(qentry));
			if (bytesread != sizeof(qentry)) {
				MSGE("error reading from file '%s'", fnm);
				return std::make_tuple(qpos, entries_represented);
			}
			entries_represented += 1;
			if (skipcnt == 1) { qearr[qpos++] = qe; tpos++; }
			if (skipcnt++ >= nskip) skipcnt = 1;
			if (nentries <= tpos) break;
		}
		f.close();
		nentries -= tpos;
		if (nentries <= 0) break;
		findex += 1;
		if (findex > latest_file_idx) break;
		get_file_name(fnm, findex);
		f = FileSys.open(fnm, "r");
		if (! f) {
			MSGE("can't open next file '%s' for reading", fnm);
			return std::make_tuple(qpos, entries_represented);
		}
		entries_from_file = f.size() / sizeof(qentry);
		//entries_to_read = min(entries_from_file, nentries);
	}
	MSGV("returning %d entries", qpos);
	return std::make_tuple(qpos, entries_represented);
}


/*
 * Used to create temperature graphs by webgraph (under sockets control) & possibly webhandler
 *
 * returns number of entries used to populate qearr & number represented including skipped entries
 * puts data in qearr from both the local buffer & from files as needed
 *
 * parameters:
 * 		qearr  - array of up to nmax qentry's
 * 		dstart - number of filed datapoints back to start
 * 		nmax   - max data points to return
 * 		nskip  - number of points to skip between each returned point
 *
 * returned results:
 * 		actually_read - number of data points actually returned
 * 		e_represented - number of points in the file these represent
 * 		at_end		  - whether at the end
 */

std::tuple<int, int, int> gatherData(qentry *qearr, int dstart, int nmax, int nskip) {
	TemperatureStore &theStore = the_temperature_logger.getStore();
	int localcnt = theStore.availableInStore();
	MSGV("localcnt is %d", localcnt);
	int fstart = dstart - localcnt;
	MSGV("starting point in files is %d entries back, wanted=%d, local=%d", fstart, nmax, localcnt);
	int actually_read = 0, e_represented = 0;
	if (fstart > 0) {
		MSGV("fstart is > 0");
		MSGV("calling read_last_entries_from_files");
		std::tuple<int, int> rdata = read_last_entries_from_files(qearr, fstart, nmax, nskip);
		MSGV("called read_last_entries_from_files");
		actually_read = std::get<0>(rdata);
		e_represented = std::get<1>(rdata);
	}
	MSGV("done start");
	int at_end = 0;
	int still_needed = 0;
	if (actually_read < nmax) {
		still_needed = nmax - actually_read;
		if (still_needed > localcnt) still_needed = localcnt;
		MSGV("getting from buffer, entries: %d, with no skips", still_needed);
		theStore.copyFromStore(&qearr[actually_read], still_needed);    // don't bother with skip for last few
		if (still_needed == localcnt) at_end = 1;
	}
	actually_read += still_needed;
	e_represented += still_needed;
	MSGV("actually read: %d, representing: %d entries", actually_read, e_represented);
	return std::make_tuple(actually_read, e_represented, at_end);
}

