#ifndef _DHT11_Sensor_h
#define _DHT11_Sensor_h

#include "Arduino.h"

enum TempScale {Celsius, Fahrenheit, Kelvin};

/** 
 * DHTMeasurement:
 * ------------------------------------------------------------------------------------------------
 * This struct holds two data values: Temperature and Humidity. You can switch the temperature  
 * scale by calling convertScale([newScale]). Supports a + operator override as an average of
 * two measurements (Scale Conversion is automatic).
 */
struct DHTMeasurement {

  float temperature;
  float humidity;
  TempScale scale;
  int averageCount; // Keeps track of how many measurements were averaged to get this one.

  DHTMeasurement(float temperature = 20, float humidity = 40, TempScale scale = Celsius, int averageCount = 1);
  
  void convertScale(TempScale newScale);
  void operator+=(DHTMeasurement m2);

  String toString();
  String temperatureToString();
  String humidityToString();

private:

  void fahrenToCelsius();
  void kelvinToCelsius();
  void celsiusToFahren();
  void celsiusToKelvin();
};


class DHT11 {
public:

    /** 
     * Constructor:
     * Creates a DHT11 object that is associated with a DHT11 temperature & humidity sensor by specifying
     * the data pin used to communicate with the device.
     */
    DHT11(uint8_t dataPin);
    
    /**
     * getTemperature()/getHumidity():
     * ------------------------------------------------------------------------------------------------
     * Returns the current temperature/humidity as a floating point value. The temperature scale can
     * be specified.
     */
    float getTemperature(TempScale scale = Celsius);
    float getHumidity();

     /**
     * getMeasurements():
     * ------------------------------------------------------------------------------------------------
     * Returns the current temperature and humidity as a DHTMeasurement struct. The temperature scale can
     * be specified.
     */
    DHTMeasurement getMeasurements(TempScale scale = Celsius);

private:

    static const int N_DATA_BYTES = 5;
    uint8_t timer1Mask, timer2Mask;

    uint8_t *data[N_DATA_BYTES];
    uint8_t dataPin;


    void getData();
    void requestData();
    bool confirmRequest();
    bool receiveData();

    float parseTemperature(TempScale scale);
    float parseHumidity();

    void getTimerMasks();
    void disableInterrupts();
    void enableInterrupts();
};

#endif