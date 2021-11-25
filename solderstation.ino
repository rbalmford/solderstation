////////////////////////////////
////  solder station  v1.1  ////
////  (c) 2021 R. Balmford  ////
////////////////////////////////

//#define DEBUG

#define SET_TEMP_init  375        // initial ON temperatire
#define SET_TEMP_stby  150        // initial IDLE temperatire
#define SET_POWER_open 0          // initial OPEN power
#define TMAX           450
#define TMIN           25
#define BL_INT         1          // backlight intensity (max 10)
#define STATEi         1

#define STATE_ON       1
#define STATE_STBY     2
#define STATE_MAX      2

#define PIN_TX         A3         // to LCD serial
#define PIN_RX         A2         // not used, any spare pin
#define PIN_PWM        9          // timer1 OC1A
#define PIN_BUTA       4          //  \.
#define PIN_BUTB       3          //  |
#define PIN_BUTC       2          //  | set pins according
#define PIN_tcDO       7          //  | to your wiring
#define PIN_tcCS       6          //  |
#define PIN_tcCK       5          //  /

#define TC_SCALING     0.73       // thermocouple scaling factor
#define DEBOUNCE_DELAY 50         // ms
#define LONGPRESS      1000       // ms

#include <max6675.h>
MAX6675  thermocouple(PIN_tcCK, PIN_tcCS, PIN_tcDO);

#include       <SoftwareSerial.h>
#include       <RB_LCD_HC.h>             // lib for HobbyComponents serial backpack
SoftwareSerial lcdSS(PIN_RX, PIN_TX);    // only TX used
RB_LCD_BP      lcdBP(lcdSS);             // for serial backpack
//#include       <Wire.h>
//#include       <SmartLCDI2C.h>           // lib for HobbyComponents I2C backpack
//SmartLCD       lcdBP(0x27);              // for I2C backpack

//// variables

volatile byte newSample = false;
char          str_toprow[17];
char          str_botrow[17] = "P               ";

////////////////////////////////////////

ISR (TIMER1_OVF_vect) {        // clears overflow flag
    newSample = true;
}

////////////////////////////////////////

void setup() {

#ifdef DEBUG
    Serial.begin(57600);
#endif
    pinMode(PIN_PWM, OUTPUT);
    pinMode(PIN_BUTA, INPUT_PULLUP);
    pinMode(PIN_BUTB, INPUT_PULLUP);
    pinMode(PIN_BUTC, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    lcdSS.begin(9600);
//    lcdSS.begin(19200);
//    lcdBP.setBaud9600();
//    lcdBP.setBaud19200();
//    Wire.begin();

    delay(1500);
    lcdBP.set16x2();
    lcdBP.Clear();
    lcdBP.Backlight(BL_INT);

    cli();

    // initialise timer1 to generate PWM for iron drive
    // and interrupt to read temperature
    TCCR1A = 0;
    TCCR1B = 0;
    bitSet(TCCR1A, COM1A1);    // clear OC1A (pin 9) on Compare Match
    bitSet(TCCR1B, WGM13);
    bitSet(TCCR1A, WGM11);     // Phase Correct PWM, TOP = ICR1
    bitSet(TCCR1B, CS12);
    bitSet(TCCR1B, CS10);      // set 1024 prescaler
    TIMSK1 = bit(TOIE1);       // enable overflow interrupt (to clear flag)
    ICR1   = 0xF3C;            // 2Hz PWM

    sei();

}  // setup

////////////////////////////////////////

void loop() {

    static int16_t  setTemp = SET_TEMP_init;
    static int16_t  pint;
    static int16_t  power;
    static byte     state = STATEi;
    static byte     valid = false;
    static byte     updateDisp = true;

    float           thermo;
    int16_t         curTemp;
    float           dTemp;

    const byte      dTfullPower = 15;        // C

    if (newSample) {
        newSample = false;

        thermo = thermocouple.readCelsius() * TC_SCALING;

        valid = (thermo == thermo);        // valid reading
        if (valid) {
            curTemp = (int16_t) thermo;
            static float lastThermo = thermo;
            dTemp = thermo - lastThermo;
            lastThermo = thermo;
        }
        updateDisp = true;
    }

    if (updateDisp) {
        updateDisp = false;
        if (state > STATE_MAX) {
            if (valid) {
                sprintf(str_toprow, "  OPEN  temp:%3d", curTemp);
            }
            else {
                sprintf(str_toprow, "  OPEN  temp:OOR");
            }
        }
        else {
            if (valid) {
                int16_t terror;
                if (state == STATE_STBY) {
                    sprintf(str_toprow, "  STBY  temp:%3d", curTemp);
                    const byte stbyTemp = SET_TEMP_stby;
                    terror = stbyTemp - curTemp;
                    pint = stbyTemp / 100;
Serial.print("set: ");
Serial.print(stbyTemp);
                }
                else if (state == STATE_ON) {
                    sprintf(str_toprow, "set:%3d temp:%3d", setTemp, curTemp);
                    terror = setTemp - curTemp;
                    pint = setTemp / 100;
Serial.print("set: ");
Serial.print(setTemp);
                }
                int16_t pprop = 15 * terror / dTfullPower;
                int16_t pdiff = (dTemp > 0) ?
                                    (int16_t) (dTemp * 1.0 + 0.5)
                                :
                                    (int16_t) (dTemp * ((terror >= 0) ? 4.0 : 2.0) - 0.5)
                                ;
                power = pprop - pdiff + pint;
                if (power > 15) {
                    power = 15;
                }
                else if (power < 0) {
                    power = 0;
                }
Serial.print(" power: ");
Serial.print(power * 10);
Serial.print(" cur: ");
Serial.print(curTemp);
Serial.println();
            }
            else {
                if (state == STATE_STBY) {
                    sprintf(str_toprow, "  STBY  temp:OOR");
                }
                else if (state == STATE_ON) {
                    sprintf(str_toprow, "set:%3d temp:OOR", setTemp);
                }
                power = 0;
            }
        }

        for (byte i=0; i<15; i++) {
            str_botrow[i+1] = (i < power) ? '>' : ' ';
        }
        lcdBP.CurPos(0,0);
        lcdBP.Print(str_toprow);
        lcdBP.CurPos(1,0);
        lcdBP.Print(str_botrow);

        OCR1A = power * 0x0104;    // ICR1 / 15
    }

    unsigned long        currentTime   = millis();
    static unsigned long butAstartTime = currentTime;
    static unsigned long butBstartTime = currentTime;
    static unsigned long butCstartTime = currentTime;
    static byte          longPressed   = false;

    byte        butAstate     = !digitalRead(PIN_BUTA);
    static byte lastbutAstate = butAstate;
    static byte butAactivity  = false;
    if (butAstate != lastbutAstate) {
        butAactivity = true;
        lastbutAstate = butAstate;
        butAstartTime = currentTime;
    }
    if (butAactivity) {
        if (butAstate) {
            if (!longPressed) {
                if (currentTime - butAstartTime > LONGPRESS) {
                    longPressed = true;
                    butAactivity = false;
                    if (state > STATE_MAX) {
                        state -= 10;
                    }
                    else {
                        state += 10;
                        power = SET_POWER_open;
                    }
                    updateDisp = true;
                }
            }
        }
        else {
           if (currentTime - butAstartTime > DEBOUNCE_DELAY) {
                butAactivity = false;
                if (!longPressed) {
                    if (state <= STATE_MAX) {
                        state = (state < STATE_MAX) ? state+1 : 1;
                        updateDisp = true;
                    }
                }
                else {
                    longPressed = false;
                }
            }
        }
    }

    byte        butBstate     = !digitalRead(PIN_BUTB);
    static byte lastbutBstate = butBstate;
    static byte butBactivity  = false;
    if (butBstate != lastbutBstate) {
        butBactivity = true;
        lastbutBstate = butBstate;
        butBstartTime = currentTime;
    }
    if (butBactivity) {
        if (currentTime - butBstartTime > DEBOUNCE_DELAY) {
            butBactivity = false;
            if (butBstate) {
                if (state == STATE_ON) {
                    setTemp = (setTemp > TMIN) ? setTemp - 25 : setTemp;
                    updateDisp = true;
                }
                else if (state > STATE_MAX) {
                    power = (power == 15) ? 8 : power / 2;
                    updateDisp = true;
                }
            }
        }
    }

    byte        butCstate     = !digitalRead(PIN_BUTC);
    static byte lastbutCstate = butCstate;
    static byte butCactivity  = false;
    if (butCstate != lastbutCstate) {
        butCactivity = true;
        lastbutCstate = butCstate;
        butCstartTime = currentTime;
    }
    if (butCactivity) {
        if (currentTime - butCstartTime > DEBOUNCE_DELAY) {
            butCactivity = false;
            if (butCstate) {
                if (state == STATE_ON) {
                    setTemp = (setTemp < TMAX) ? setTemp + 25 : setTemp;
                    updateDisp = true;
                }
                else if (state > STATE_MAX) {
                    power = (power == 0) ? 1 : (power == 8) ? 15 : (power == 15) ? power : power * 2;
                    updateDisp = true;
                }
            }
        }
    }

}  // loop
