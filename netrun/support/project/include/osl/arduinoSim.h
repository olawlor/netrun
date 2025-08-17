/*
 Outcome-oriented simulator for Arduino.
 
 Pin definitions taken from Arduino SdFat library: ./libraries/SD/src/utility/Sd2PinMap.h
 
 See also: ./hardware/tools/avr/avr/include/avr/iom168.h
 ./hardware/arduino/avr/variants/standard/pins_arduino.h
*/
#ifndef __ARDUINO_SIM_H
#define __ARDUINO_SIM_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


typedef enum {
    INPUT=0,
    OUTPUT=1,
    INPUT_PULLUP=2,
    pinMode_invalid=3
} pinMode_t;


typedef unsigned char uint8_t;


/** struct for mapping digital pins */
class ArduinoPin {
public:
  volatile uint8_t* ddr; // data direction register  (0=INPUT, 1=OUTPUT)
  volatile uint8_t* port; // write ports (DDR==1)
  volatile uint8_t* pin; // read pins (DDR==0)
  uint8_t bit;
  inline uint8_t mybit(void) const { return 1<<bit; }
  
  pinMode_t mode;
  void pinMode(pinMode_t newmode) { mode=newmode; }
  
  void digitalWrite(uint8_t value) {
    if (value)
        *port |= mybit(); // turn output on
    else
        *port &= ~mybit(); // turn output off
  }
  
  int digitalRead(void) {
    return (*pin & mybit())!=0;
  }
  
  // Used by context object:
  int getDigitalWrite(void) {
    return (*port & mybit())!=0;
  }
  void setDigitalRead(uint8_t value) {
    if (value)
        *pin |= mybit(); // turn input on
    else
        *pin &= ~mybit(); // turn input off
  }
};


// This represents the circuit and other hardware sending stuff to the Arduino
class ArduinoContext {
protected:
    // These are all the I/O pins (for manually doing getDigitalWrite / setDigitalRead)
    ArduinoPin *pins;

public:
    // Called by the ArduinoSim when it's initializing, used to set up initial pin values
    virtual void setPins(ArduinoPin *newPins) 
    {
        pins=newPins;
    }

    // Simulation time, in microseconds
    unsigned long long timeUS;
    virtual void delay(unsigned long us) {
        timeUS+=us;
    }
    
    virtual int analogRead(int pin) { return 327; }
    
    // Any simulator housekeeping happens here.  If this returns false, the simulation is over.
    virtual bool simComplete() {
        return timeUS>10*1000*1000;
    }
    

    // A serial port is printing this stuff:
    virtual size_t printString(const char *buf) {
        return printf("%s",buf);
    }
    virtual size_t printInt(int v) {
        return printf("%d",v);
    }
    
    // Error out
    virtual void error(const char *why,int number) {
        fprintf(stderr,"ERROR: %s %d\n", why, number);
        exit(1);
    }

    ArduinoContext() { timeUS=0; }
};

// User-visible API to Arduino serial port
class HardwareSerial {
    ArduinoContext &ctx;
    bool begun; // sim-only flag to detect use without a ".begin"
    void check() {
        if (!begun) ctx.error("Serial error: missing Serial.begin before I/O ",0);
    }
public:
    HardwareSerial(ArduinoContext &ctx_) 
        :ctx(ctx_), begun(false)  {}
    void begin(unsigned long baudrate) { begun=true; }
    
// From: ./hardware/arduino/avr/cores/arduino/Stream.h
    size_t print(int v) { 
        return ctx.printInt(v);
    }
    size_t print(const char *arr) { 
        return ctx.printString(arr);
    }
    
    // Our implementation of println just tacks on a newline:
    template <class T> size_t println(T t) {
        size_t a=print(t);
        size_t b=print("\n");
        return a+b;
    }
    
    // FIXME: figure out an available / read interface to the ctx
    
    void end() { begun=false; }
};

// User-visible API to Arduino itself
class ArduinoSim {
public:
    ArduinoContext &ctx;

    HardwareSerial Serial;



// From: Arduino/hardware/arduino/avr/cores/arduino/Arduino.h
#define HIGH 0x1
#define LOW  0x0


#define bit(b) (1UL << (b))


// Definitions directly from: Arduino/hardware/arduino/avr/variants/standard/pins_arduino.h
// 

#define LED_BUILTIN 13

#define PIN_A0   (14)
#define PIN_A1   (15)
#define PIN_A2   (16)
#define PIN_A3   (17)
#define PIN_A4   (18)
#define PIN_A5   (19)
#define PIN_A6   (20)
#define PIN_A7   (21)

static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;
static const uint8_t A6 = PIN_A6;
static const uint8_t A7 = PIN_A7;


// From: ./libraries/SD/src/utility/Sd2PinMap.h
// 168 and 328 Arduinos -- Uno


// Two Wire (aka I2C) ports
uint8_t const SDA_PIN = 18;
uint8_t const SCL_PIN = 19;

// SPI port
uint8_t const SS_PIN = 10;
uint8_t const MOSI_PIN = 11;
uint8_t const MISO_PIN = 12;
uint8_t const SCK_PIN = 13;

//  https://www.arduino.cc/en/Reference/PortManipulation
// On the real arduino, these are memory mapped I/O devices:
uint8_t DDRD, DDRB, DDRC; // data direction (0=INPUT, 1=OUTPUT)
uint8_t PORTD, PORTB, PORTC; // write ports (DDR==1)
uint8_t PIND, PINB, PINC; // read pins (DDR==0)



enum {MAXPIN=22};
ArduinoPin digitalPinMap[MAXPIN] = {
  {&DDRD, &PIND, &PORTD, 0, pinMode_invalid},  // D0  0
  {&DDRD, &PIND, &PORTD, 1, pinMode_invalid},  // D1  1
  {&DDRD, &PIND, &PORTD, 2, pinMode_invalid},  // D2  2
  {&DDRD, &PIND, &PORTD, 3, pinMode_invalid},  // D3  3
  {&DDRD, &PIND, &PORTD, 4, pinMode_invalid},  // D4  4
  {&DDRD, &PIND, &PORTD, 5, pinMode_invalid},  // D5  5
  {&DDRD, &PIND, &PORTD, 6, pinMode_invalid},  // D6  6
  {&DDRD, &PIND, &PORTD, 7, pinMode_invalid},  // D7  7
  {&DDRB, &PINB, &PORTB, 0, pinMode_invalid},  // B0  8
  {&DDRB, &PINB, &PORTB, 1, pinMode_invalid},  // B1  9
  {&DDRB, &PINB, &PORTB, 2, pinMode_invalid},  // B2 10
  {&DDRB, &PINB, &PORTB, 3, pinMode_invalid},  // B3 11
  {&DDRB, &PINB, &PORTB, 4, pinMode_invalid},  // B4 12
  {&DDRB, &PINB, &PORTB, 5, pinMode_invalid},  // B5 13
  {&DDRC, &PINC, &PORTC, 0, pinMode_invalid},  // C0 14
  {&DDRC, &PINC, &PORTC, 1, pinMode_invalid},  // C1 15
  {&DDRC, &PINC, &PORTC, 2, pinMode_invalid},  // C2 16
  {&DDRC, &PINC, &PORTC, 3, pinMode_invalid},  // C3 17
  {&DDRC, &PINC, &PORTC, 4, pinMode_invalid},  // C4 18
  {&DDRC, &PINC, &PORTC, 5, pinMode_invalid},  // C5 19
  {&DDRC, &PINC, &PORTC, 6, pinMode_invalid},  // C6 20
  {&DDRC, &PINC, &PORTC, 7, pinMode_invalid}   // C7 21
};

// Port simulation
    void pinMode(uint8_t pin, pinMode_t mode) {
        delayMicroseconds(2);
        if (pin<MAXPIN) digitalPinMap[pin].pinMode(mode);
        else ctx.error("Call to pinMode for bogus pin ",(int)pin);
    }
    void digitalWrite(uint8_t pin, uint8_t value) {
        delayMicroseconds(3);
        if (pin<MAXPIN) {
            if (digitalPinMap[pin].mode!=OUTPUT)
                ctx.error("Invalid call to digitalWrite: not in OUTPUT state for pin",(int)pin);
            digitalPinMap[pin].digitalWrite(value);
        }
        else ctx.error("Call to digitalWrite for bogus pin ",(int)pin);
    }
    int digitalRead(uint8_t pin) {
        delayMicroseconds(3);
        if (pin<MAXPIN) {
            if (digitalPinMap[pin].mode!=INPUT && digitalPinMap[pin].mode!=INPUT_PULLUP)
                ctx.error("Invalid call to digitalRead: not in INPUT state for pin",(int)pin);
            return digitalPinMap[pin].digitalRead();
        }
        else ctx.error("Invalid call to digitalRead for pin ",(int)pin);
        return 0;
    }
    
    int analogRead(uint8_t pin) {
        delayMicroseconds(112);
        if (pin>=A0 && pin<MAXPIN) return ctx.analogRead(pin-A0);
        else ctx.error("Invalid call to analogRead for pin ",(int)pin);
        return 0;
    }
    /*
    void analogWrite(uint8_t pin, int pwm) {
        ctx.analogWrite(pin,pwm);
    }
    */

// Time simulation
    inline unsigned long millis(void) { return ctx.timeUS/1000; }
    inline unsigned long micros(void) { return ctx.timeUS; }
    inline void delay(unsigned long ms) { ctx.delay(ms*1000); }
    inline void delayMicroseconds(unsigned int us) { ctx.delay(us); }


    // User writes these functions:
    virtual void setup()=0;
    virtual void loop()=0;

    // Run the simulation
    void simRun() {
        setup();
        while (!ctx.simComplete()) {
            loop();
            delayMicroseconds(100); // <- about 1.8 us per loop() for a real Uno, longer here for faster idle in simulation
        }
    }
    ArduinoSim(ArduinoContext &ctx_)
        :ctx(ctx_), Serial(ctx)
    {
        ctx.setPins(digitalPinMap);
    }
};



// Utility class for simulating CS 241 HW0 cubesat:
class CubesatPhysicsCtx : public ArduinoContext {
public:
    double physicsTime=0; // simulated time, in seconds
    double physicsdT=0.25; // timestep size
    int physicsStep=0; // integer step number
    double heater=0.0; // is heater currently on?
    double temperature=270.0; // propellant temperature in K
    
    int heaterOn;
    int heaterCallTime;
    CubesatPhysicsCtx(int heaterOn_,double heaterCallTime_)
        :heaterOn(heaterOn_),heaterCallTime(heaterCallTime_)
    {
    }
    
    void setPins(ArduinoPin *newPins)
    {
        ArduinoContext::setPins(newPins);
        pins[12].setDigitalRead(1); // vacuum sensor initially high
        pins[8].setDigitalRead(1); // heater call line initially high
    }
    
    void delay(unsigned long us) {
        ArduinoContext::delay(us);
        while (physicsTime<timeUS/1.0e6) takePhysicsStep();
    }
    void takePhysicsStep() 
    {
        // Apply the current physics
        heater=pins[3].mode==OUTPUT && pins[3].getDigitalWrite()==heaterOn;
        if (heater) temperature+=5.0*physicsdT; // heating is on
        temperature-=0.5*physicsdT; // cooldown to space ambient
        
        bool thrust=pins[11].mode==OUTPUT && pins[11].getDigitalWrite()==1;
        if (thrust) temperature+=30.0*physicsdT; // big heating rate from thruster
        
        // Log the current state
        printf("time=%.2f seconds, heater=%.0f, %stemperature=%.0f K\n",
            physicsTime, heater, thrust?"THRUST, ":"", temperature);
        
        // Move the loop forward
        if (physicsTime>=4.0) exit(0); // we're done
        physicsTime+=physicsdT;
        physicsStep++;
        if (physicsTime>=1.5 && pins[12].mode==INPUT_PULLUP)
            pins[12].setDigitalRead(0); // vacuum sensor ready
        
        bool heaterCall=(physicsTime>=heaterCallTime)&&(physicsTime<3.5);
        pins[8].setDigitalRead(heaterCall?0:1); // heater call ready
    }
};






#endif


