// Do not remove the include below
#include "MorseSender.h"

#include <pgmspace.h>
#include "SerialDebugHelper.h"

#define DEBUG 1
#define MORSE_MAX_MSG_LEN 15                   // buffer size, must be greater than MAXCHANNELS below

class MorseSender {
public:
    MorseSender(int pin, bool activeLow = false,
                unsigned charWPM = 20, unsigned effWPM = 15)
        : pin(pin), activeLow(activeLow)
    {
        DOT = 60000UL / (50UL * charWPM);
        DASH = DOT * 3;
        GAP_SYMBOL = DOT;

        float factor = (float)charWPM / (float)effWPM;
        GAP_LETTER = (unsigned)(DOT * 3 * factor);
        GAP_WORD   = (unsigned)(DOT * 7 * factor);

        msgLen = 0;
        index = 0;
        currentCode = nullptr;
        codePos = 0;
    }

    void begin() {
        pinMode(pin, OUTPUT);
        writeLED(false);
    }

    // note: immediately interrupts any message currently being sent
    void setMessage(const char* msg) {
        msgLen = 0;
        while (msg[msgLen] && msgLen < MORSE_MAX_MSG_LEN) {
            message[msgLen] = msg[msgLen];
            msgLen++;
        }
        message[msgLen < MORSE_MAX_MSG_LEN ? msgLen : MORSE_MAX_MSG_LEN - 1] = '\0';

        index = 0;
        nextChange = millis();
        ledOn = false;
        currentCode = nullptr;
        codePos = 0;
        writeLED(false);
    }

    void update() {
        unsigned long now = millis();

        if (ledOn && now >= offTime) {
            ledOn = false;
            writeLED(false);
        }

        if (now < nextChange) return;

        // If we are in the middle of a Morse pattern, continue it
        if (currentCode != nullptr) {
            continueCode();
            return;
        }

        char c;
        // Otherwise, fetch next character
        if (index >= msgLen) {
            index = 0;
            c = '.';
        }
        else c = toupper(message[index++]);

        if (c == ' ' || c == '.') {
            writeLED(false);
            nextChange = now + (c == ' ' ? GAP_WORD : 2 * GAP_WORD);
            SPRTLN(2, nextChange);
            SPRTLN(2, "end word");
            return;
        }

        currentCode = getMorseCode(c);
        codePos = 0;
        //Serial.println(currentCode);

        continueCode();
    }

private:
    int pin;
    bool activeLow;
    bool ledOn = false;
    unsigned long offTime = 0;

    unsigned DOT, DASH, GAP_SYMBOL, GAP_LETTER, GAP_WORD;

    char message[MORSE_MAX_MSG_LEN];
    unsigned msgLen;
    unsigned long nextChange = 0;
    unsigned index = 0;

    const char* currentCode;
    unsigned codePos;

    void writeLED(bool on) {
        digitalWrite(pin, activeLow ? !on : on);
    }

    void turnOnFor(unsigned duration) {
        ledOn = true;
        writeLED(true);
        offTime = millis() + duration;
    }

    void continueCode() {
        unsigned long now = millis();

        char symbol = pgm_read_byte(currentCode + codePos);

        if (symbol == '.') {
            turnOnFor(DOT);
            nextChange = now + DOT + GAP_SYMBOL;
            codePos++;
            return;
        }

        if (symbol == '-') {
            turnOnFor(DASH);
            nextChange = now + DASH + GAP_SYMBOL;
            codePos++;
            return;
        }

        // End of pattern
        currentCode = nullptr;
        codePos = 0;
        writeLED(false);
        nextChange = now + GAP_LETTER;
    }

    // PROGMEM table (unchanged)
    static const char A_[] PROGMEM;
    static const char B_[] PROGMEM;
    static const char C_[] PROGMEM;
    static const char D_[] PROGMEM;
    static const char E_[] PROGMEM;
    static const char F_[] PROGMEM;
    static const char G_[] PROGMEM;
    static const char H_[] PROGMEM;
    static const char I_[] PROGMEM;
    static const char J_[] PROGMEM;
    static const char K_[] PROGMEM;
    static const char L_[] PROGMEM;
    static const char M_[] PROGMEM;
    static const char N_[] PROGMEM;
    static const char O_[] PROGMEM;
    static const char P_[] PROGMEM;
    static const char Q_[] PROGMEM;
    static const char R_[] PROGMEM;
    static const char S_[] PROGMEM;
    static const char T_[] PROGMEM;
    static const char U_[] PROGMEM;
    static const char V_[] PROGMEM;
    static const char W_[] PROGMEM;
    static const char X_[] PROGMEM;
    static const char Y_[] PROGMEM;
    static const char Z_[] PROGMEM;

    static const char* const table[] PROGMEM;

    const char* getMorseCode(char c) {
        if (c < 'A' || c > 'Z') return nullptr;
        return (const char*)pgm_read_ptr(&table[c - 'A']);
    }
};


// -----------------------------
// PROGMEM definitions
// -----------------------------

const char MorseSender::A_[] PROGMEM = ".-";
const char MorseSender::B_[] PROGMEM = "-...";
const char MorseSender::C_[] PROGMEM = "-.-.";
const char MorseSender::D_[] PROGMEM = "-..";
const char MorseSender::E_[] PROGMEM = ".";
const char MorseSender::F_[] PROGMEM = "..-.";
const char MorseSender::G_[] PROGMEM = "--.";
const char MorseSender::H_[] PROGMEM = "....";
const char MorseSender::I_[] PROGMEM = "..";
const char MorseSender::J_[] PROGMEM = ".---";
const char MorseSender::K_[] PROGMEM = "-.-";
const char MorseSender::L_[] PROGMEM = ".-..";
const char MorseSender::M_[] PROGMEM = "--";
const char MorseSender::N_[] PROGMEM = "-.";
const char MorseSender::O_[] PROGMEM = "---";
const char MorseSender::P_[] PROGMEM = ".--.";
const char MorseSender::Q_[] PROGMEM = "--.-";
const char MorseSender::R_[] PROGMEM = ".-.";
const char MorseSender::S_[] PROGMEM = "...";
const char MorseSender::T_[] PROGMEM = "-";
const char MorseSender::U_[] PROGMEM = "..-";
const char MorseSender::V_[] PROGMEM = "...-";
const char MorseSender::W_[] PROGMEM = ".--";
const char MorseSender::X_[] PROGMEM = "-..-";
const char MorseSender::Y_[] PROGMEM = "-.--";
const char MorseSender::Z_[] PROGMEM = "--..";

const char* const MorseSender::table[] PROGMEM = {
    A_, B_, C_, D_, E_, F_, G_, H_, I_, J_, K_, L_, M_,
    N_, O_, P_, Q_, R_, S_, T_, U_, V_, W_, X_, Y_, Z_
};

// ---------------------------------------------------------------------


#define MAXCHANNELS 4				// limits the number of channels

class MorseSignalController {
public:
	MorseSignalController(int pin, bool activeLow = false,
            unsigned charWPM = 20, unsigned effWPM = 15)
		{
			morseSender = new MorseSender(pin, activeLow, charWPM, effWPM);
			morseSender->begin();
			SignalChannels[0] = 'a';
			for (int i=1; i<MAXCHANNELS; i++) SignalChannels[i] = 'z';		// uninitialized channels
			SignalChannels[MAXCHANNELS] = '\0';
			setMessage();
		}

	void setChannel(int chan, char c) {
		if (chan <= 0 || chan > MAXCHANNELS) {	// channel 0 is for internal errors
			SignalChannels[0] = 'b';			// indicate internal error
		} else {
			SignalChannels[chan] = c;
		}
		setMessage();
	}

	void update() {
		morseSender->update();
	}

private:
	MorseSender *morseSender;
	char SignalChannels[MAXCHANNELS+1];

	void setMessage() {
		morseSender->setMessage(SignalChannels);
	}
};

// ---------------------------------------------------------------------

static MorseSignalController *theMorseSignalController = new MorseSignalController(LED_BUILTIN, true, 12, 4);

// create a new morse signaller assigned to a particular channel
MorseSignaller::MorseSignaller(int chan) : channel(chan) {
	signal('a');
}

// send a signal on this signaller's channel
void MorseSignaller::signal(char c) {
	if (theMorseSignalController != 0) {
		theMorseSignalController->setChannel(channel, c);
	}
}

// static
void MorseSignaller::update() {
	theMorseSignalController->update();
}

#if 0
// Stand-alone test of MorseSignaller

void setup() {
	Serial.begin(115200);
	Serial.println("Start");
}

void loop() {
	theMorseSignalController->update();

	static MorseSignaller *siggie = 0;
	static char lastsig = 0;
	static unsigned long last = 0;
    unsigned long now = millis();
    if (siggie == 0) {
    	siggie = new MorseSignaller(1);
    	lastsig = 'a';
    	siggie->signal(lastsig);
    	last = now;
    }
    if (now - last > 30000) {		// at 30 second intervals
    	lastsig += 1;
    	if (lastsig > 'e') lastsig = 'a';
    	siggie->signal(lastsig);
    	last = now;
    	Serial.println("signal changed");
    }
}
#endif

#if 0
// Stand-alone test of MorseSender class:

// 12 WPM characters, 6 WPM Farnsworth spacing
MorseSender morse(LED_BUILTIN, true, 12, 6);

void setup() {
	Serial.begin(115200);
	Serial.println("Start");

    morse.begin();
    morse.setMessage("HELLO WORLD");
    Serial.println("end setup");
}

void loop() {
    morse.update();
}
#endif
