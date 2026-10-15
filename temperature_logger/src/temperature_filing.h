/*
 * temperature_filing.h
 *
 *  Created on: Sep. 26, 2021
 *      Author: Danny
 */

#ifndef TEMPERATURE_FILING_H_
#define TEMPERATURE_FILING_H_

#include <tuple>

struct qentry {
  unsigned long ptime;
  float         temp;
  unsigned char flags;
};


bool write_bytes_to_file_store(unsigned char *b, int len);

std::tuple<int, int, int> gatherData(qentry *qearr, int dstart, int nmax, int nskip);

#endif /* TEMPERATURE_FILING_H_ */
