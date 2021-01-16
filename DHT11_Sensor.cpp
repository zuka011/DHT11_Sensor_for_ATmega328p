#include "Arduino.h"
#include "DHT11_Sensor.h"

//------------------------------------------------------------------------//
// DHTMeasurement struct method implementations

DHTMeasurement::DHTMeasurement(float temperature, float humidity, TempScale scale, int averageCount) {
    
    this->temperature = temperature;
    this->humidity = humidity;  
    this->scale = scale;
    this->averageCount = averageCount;
}

void DHTMeasurement::convertScale(TempScale newScale) {
 
    if(scale == newScale) return;

    if(scale != Celsius) {
        
        if(scale == Fahrenheit) fahrenToCelsius();
        else if(scale == Kelvin) kelvinToCelsius();
    }

    if(newScale != Celsius) {

        if(newScale == Fahrenheit) celsiusToFahren();
        else if(newScale == Kelvin) celsiusToKelvin();
    }
}

String DHTMeasurement::toString() {

    String str = temperatureToString() + "\t" + humidityToString();
    return str;        
}

String DHTMeasurement::temperatureToString() {

    String str = "Temperature: " + String(temperature); 

    switch(scale) {
        case Celsius: str += " C"; break;
        case Fahrenheit: str += " F"; break;
        case Kelvin: str += " K"; break;
        default: str += " (scale error)"; break;
    }
    return str;
}

String DHTMeasurement::humidityToString() {
    
    String str = "Humidity: " + String(humidity) + "%";
    return str;
}

void DHTMeasurement::fahrenToCelsius() {
    
    scale = Celsius;
    temperature = (temperature - 32)/1.8;
}

void DHTMeasurement::kelvinToCelsius() {
    
    scale = Celsius;
    temperature = temperature - 273;
}

void DHTMeasurement::celsiusToFahren() {
    
    scale = Fahrenheit;
    temperature = temperature*1.8 + 32;
}

void DHTMeasurement::celsiusToKelvin() {

    scale = Kelvin;
    temperature = temperature + 273;
}

void DHTMeasurement::operator+=(DHTMeasurement newMeasurement) {
    
    if(scale != newMeasurement.scale) {
     
        convertScale(Celsius);
        newMeasurement.convertScale(Celsius);
    }  

    temperature = (temperature * averageCount + newMeasurement.temperature * newMeasurement.averageCount)/(averageCount + newMeasurement.averageCount);
    humidity = (humidity * averageCount + newMeasurement.humidity * newMeasurement.averageCount)/(averageCount + newMeasurement.averageCount);
    averageCount++;
}

//------------------------------------------------------------------------//
// DHT class method implementations 

DHT11::DHT11(byte dataPin) {  
    this->dataPin = dataPin;
}

float DHT11::getTemperature(TempScale scale) {

    getData();
    return parseTemperature(scale);
}

float DHT11::getHumidity() {

    getData();
    return parseHumidity();
}

DHTMeasurement DHT11::getMeasurements(TempScale scale) {
    
    getData();
    return DHTMeasurement(parseTemperature(scale),  parseHumidity());    
}

void DHT11::getData() {

    getTimerMasks();

    bool success = false;
    while (!success) {
        
        requestData();
        success = receiveData();
    }  
    enableInterrupts();
}

void DHT11::requestData() {

    // Procedure for requesting data from the DHT sensor
    static const int PULL_DOWN_TIME = 20, PULL_UP_TIME_MICRO = 40, FAIL_DELAY = 20;

    do {

        enableInterrupts(); 

        // A small delay before the next attempt, in case the sensor didn't receive the previous request
        delay(FAIL_DELAY);

        pinMode(dataPin, OUTPUT);

        digitalWrite(dataPin, LOW);
        delay(PULL_DOWN_TIME);

        disableInterrupts();

        digitalWrite(dataPin, HIGH);
        delayMicroseconds(PULL_UP_TIME_MICRO);

    } while (!confirmRequest());
}

bool DHT11::confirmRequest() {

    // The sensor should pull low for 54u seconds if it received the data request.
    static const int MIN_LOW_DURATION = 30;
    static const int MAX_HIGH_DURATION = 100;
    static const int MIN_HIGH_DURATION = 40;

    pinMode(dataPin, INPUT);

    long timer = micros();
    while (!digitalRead(dataPin));
    if (micros() - timer < MIN_LOW_DURATION) return false;

    // After pulling low, the sensor will pull high for around 80u seconds.
    timer = micros();
    while (digitalRead(dataPin)) if(micros() - timer > MAX_HIGH_DURATION) return false;

    return micros() - timer > MIN_HIGH_DURATION;
}

bool DHT11::receiveData() {

    // A '0' bit is represented as a 24u second high, a '1' bit is represented as a 70u second high.
    static const int MIN_HIGH_BIT_DURATION = 47;
    static const int N_DATA_BITS = N_DATA_BYTES * 8;
    static const int MAX_HIGH_BIT_DURATION = 100;

    pinMode(dataPin, INPUT);

    for (int i = 0; i < N_DATA_BYTES; i++) data[i] = 0;    
    
    for (int i = 0; i < N_DATA_BITS; i++) {

        // There's a 54u second low between every bit.
        while (!digitalRead(dataPin));

        long timer = micros();
        while (digitalRead(dataPin)) if(micros() - timer > MAX_HIGH_BIT_DURATION) return false;

        bool currBit = (micros() - timer) > MIN_HIGH_BIT_DURATION;

        //Data is received MSB, so every bit is shifted left, except for the last one.
        data[i / 8] += currBit;
        if (i % 8 != 7) data[i / 8] = int(data[i / 8]) << 1;
    }

    uint8_t sum = 0;
    for (int i = 0; i < N_DATA_BYTES - 1; i++) sum += data[i];

    return sum == data[4];
}

float DHT11::parseTemperature(TempScale scale) {

    float value = int(data[2]) + int(data[3]) / 10.0;
    switch(scale) {
        case Celsius:  return value;
        case Fahrenheit: return value * 1.8 + 32;
        case Kelvin: return value + 273;
    }
}

float DHT11::parseHumidity() {

    return int(data[0]) + int(data[1]) / 10.0;
}

void DHT11::getTimerMasks() {

    timer1Mask = TIMSK1;
    timer2Mask = TIMSK2;
}

void DHT11::disableInterrupts() {

    // disabling all Timer interrupts (except for Timer0, 
    // because that one is responsible for measuring time).
    // This is required to get the sensor data.
    getTimerMasks();
    TIMSK1 &= B11111000;
    TIMSK2 &= B11111000; 
}

void DHT11::enableInterrupts() {

    // Re-enabling all the timer interrupts.
    TIMSK1 = timer1Mask;
    TIMSK2 = timer2Mask;  
}