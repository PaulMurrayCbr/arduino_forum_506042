
//#define DEBUG

#ifdef DEBUG
#define LOG(s) Serial.println(s)
#define LOGN(s) Serial.print(s)
#else
#define LOG(s)
#define LOGN(s)
#endif



/**
   A simple debounce. We only really need this for the "PRESS" button.
*/

class DebounceButton {
    const byte pin;
    byte state;
    byte prevState;
    uint32_t debounce_ms;
  public:
    DebounceButton(byte pinAttach) : pin(pinAttach) {}
    void setup() {
      pinMode(pin, INPUT_PULLUP);
    }
    void loop() {
      prevState = state;
      if (millis() - debounce_ms > 50) {
        state = digitalRead(pin);
      }
      if (state != prevState) {
        debounce_ms = millis();
        LOGN(rising() ? 'R' : '-');
        LOGN(high() ? 'H' : '-');
        LOGN(falling() ?  'F' : '-');
        LOGN(low() ? 'L' : '-');
        LOG();
      }
    }
    boolean high() {
      return state != LOW;
    }
    boolean low() {
      return state == LOW;
    }
    boolean rising() {
      return state != LOW && prevState == LOW;
    }
    boolean falling() {
      return state == LOW && prevState != LOW;
    }
};

/**
   The only purpose of this class id to bundle a pair of variables together
*/

class SimpleButton {
    const byte pin;
    byte state;
  public:
    SimpleButton(byte pinAttach) : pin(pinAttach) {}
    void setup() {
      pinMode(pin, INPUT_PULLUP);
    }
    void loop() {
      state = digitalRead(pin);
    }
    boolean high() {
      return state != LOW;
    }
    boolean low() {
      return state == LOW;
    }
};



/**
   the purpose of this class is to encapsulate the output pins
   and guarantee that they will *never* be both HIGH at the same time
*/
class RetractExtend {
    const byte retractPin, extendPin;

    enum State { IDLE, EXTEND, RETRACT } state = IDLE;

  public:
    RetractExtend(byte retractAttach, byte extendAttach) : retractPin(retractAttach), extendPin(extendAttach) {}

    void setup() {
      pinMode(retractPin, OUTPUT);
      pinMode(extendPin, OUTPUT);
      off();
    }

    void retract() {

      if (state == EXTEND) {
        digitalWrite(extendPin, LOW);
        delay(10);
      }
      if (state != RETRACT) {
        LOG("retracting actuator");
        digitalWrite(retractPin, HIGH);
        state = RETRACT;
      }
    }

    void extend() {
      if (state == RETRACT) {
        digitalWrite(retractPin, LOW);
        delay(10);
      }
      
      if (state != EXTEND) {
        LOG("extending actuator");
        digitalWrite(extendPin, HIGH);
        state = EXTEND;
      }
    }

    void off() {
      // do this unconditionally, just in case
      digitalWrite(retractPin, LOW);
      digitalWrite(extendPin, LOW);

      if (state != IDLE) {
        LOG("stopping actuator");
        state = IDLE;
      }
    }
};


enum State {
  IDLE, PRESS_EXTEND, PRESS_RETRACT
} state = IDLE;

/**
   PINOUT

   Testing this with some buttons plugged into the board
*/

SimpleButton retractBtn(2), extendBtn(4), setBtn(3);
DebounceButton stampBtn(5);
RetractExtend actuator(10, 11);

int retractedPosition;
int extendedPosition;
int position;


/**
   This function needs to be replaced with soemthing that reads the digital encoder.
   For now, I will simulate it with a potentiometer on A0, which I will operate by hand.

   I am assuming that the EXTENDED position is GREATER than the RETRACTED position, and that the actuator
   EXTEND pin extends the acuator and causes the position to INCREASE over time.
*/

void readPosition() {
  position = analogRead(A0);
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);

  LOG("Beginning sketch");
#endif

  retractBtn.setup();
  extendBtn.setup();
  setBtn.setup();
  stampBtn.setup();
  actuator.setup();
}

void loop() {
  retractBtn.loop();
  extendBtn.loop();
  setBtn.loop();
  stampBtn.loop();
  readPosition();

  switch (state) {
    case IDLE:
      if (stampBtn.falling()) {
        if (retractBtn.low() || extendBtn.low() || setBtn.low()) break;
        if (position < extendedPosition && retractedPosition < extendedPosition) {
          LOG("COMMENCING STAMP");
          actuator.extend();
          state = PRESS_EXTEND;
        }
      }
      else if (setBtn.low()) {
        if (retractBtn.low() != extendBtn.low()) {
          if (retractBtn.low()) {
            retractedPosition = position;
            LOGN("retractedPosition is now ");
            LOG(retractedPosition);
          }
          else  {
            extendedPosition = position;
            LOGN("extendedPosition is now ");
            LOG(extendedPosition);
          }
        }
      }
      else {
        if (retractBtn.low() != extendBtn.low()) {
          if (retractBtn.low()) {
            actuator.retract();
          }
          else  {
            actuator.extend();
          }
        }
        else {
          actuator.off();
        }
      }
      break;

    case PRESS_EXTEND:
      if (retractBtn.low() || extendBtn.low() || setBtn.low() || stampBtn.falling() ) {
        LOG("CANCELLING PRESS EXTENSION");
        actuator.off();
        state = IDLE;
        break;
      }
      else if (position >= extendedPosition) {
        LOG("END OF RANGE, COMMENCING RETRACT");
        actuator.retract();
        state = PRESS_RETRACT;
      }
      break;

    case PRESS_RETRACT:
      if (retractBtn.low() || extendBtn.low() || setBtn.low() || stampBtn.falling() ) {
        LOG("CANCELLING PRESS RETRACTION");
        actuator.off();
        state = IDLE;
        break;
      }
      else if (position <= retractedPosition) {
        LOG("END OF RANGE, CYCLE COMPLETE");
        actuator.off();
        state = IDLE;
      }
      break;
  }

}



