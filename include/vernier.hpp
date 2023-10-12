#ifndef VernierLib_h
#define VernierLib_h
#include <cmath>
#endif
#define VERNLIB32_VERSION "1.0.0"

/* Custom library based on Vernier's VernierLib.
   Compatible with Artila Matrix-310.
*/

class Vernier {
  protected:
    float Vin = 5.0;
    float slope;
    float intercept;
    float responeTime;
    char sensorUnit[7];

  public:
    virtual ~Vernier() = 0;
    // Return sensor's current unit of measurement.
    char* getSensorUnit()      { return sensorUnit; }
    // Return sensor's respone time in seconds.
    float getResponeTime()     { return responeTime; }
    // Calculate the sensor value from measured voltage.
    float readSensor(float voltage);
    // Calculate the sensor value from ADC count.
    float readSensor(int rawADC);

    void setVin(float voltage) { Vin = voltage; }
    void calibrate(float slope, float intercept);
};

// Stainless Steel Temperature sensor.
class SSTempSensor : public Vernier {
  private:
    // Steinhart-Hart coefficients:
    const double K0 = 0.00102119;
    const double K1 = 0.000222468;
    const double K2 = 0.000000133342;
    // Sensor thermistor:
    const int therm = 20000;
    // Schematic:
    // [GND] -- [Thermistor] ----- | ----- [Resistor] --[Vcc (5v)]
    //                             |
    //                       Analog input
    //
    // Vernier's BTA protoboard builtin resistor:
    float divider = 15000;

  public:
    SSTempSensor();
    // Return balance resistance.
    int getDividerResistance()       { return divider; };
    // Set balance resistor value.
    void setDividerResistance(int r) { divider = r; };
    //  Switch between Celcius and Fahrenheit, default is Celcius.
    void switchUnit();
    // Return temperature value using Steinhart-Hart equation
    float calculateTemp(float resistance);

    float readSensor(float voltage);
    float readSensor(int rawADC);
};

// Optical Dissolved Oxygen sensor.
class ODOSensor : public Vernier {
  public:
    ODOSensor();
    //  Switch between mg/L, default is mg/L.
    void switchUnit();
};

// Tris-compatible Flat pH sensor.
class FPHSensor : public Vernier {
  public:
    FPHSensor();
};

// END OF FILE