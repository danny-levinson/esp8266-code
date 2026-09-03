// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _morse_sender_H_
#define _morse_sender_H_
#include "Arduino.h"


class MorseSignaller {
public:
	MorseSignaller(int chan);
	void signal(char c);
	static void update();
private:
	int channel;
};


#endif /* _morse_sender_H_ */
