/*
 * debug.h
 *
 *  Created on: Sep. 24, 2021
 *      Author: Danny
 */

#ifndef DEBUG_H_
#define DEBUG_H_


#define DEBUG 1


#if DEBUG
#define SBEGIN(a) Serial.begin(a)
#define SPRINT(n, a) if (DEBUG >= n) Serial.print(a);
#define SPRTLN(n, a) if (DEBUG >= n) Serial.println(a);
#define SPRNTF(n, ...) if (DEBUG >= n) Serial.printf(__VA_ARGS__);
#else
#define SBEGIN(a)
#define SPRINT(n, a)
#define SPRTLN(n, a)
#define SPRNTF(n, ...)
#endif




#endif /* DEBUG_H_ */
