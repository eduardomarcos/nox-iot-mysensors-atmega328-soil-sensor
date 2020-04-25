#define MY_DEBUG
#define MY_RADIO_RF24

#define MY_RF24_PA_LEVEL RF24_PA_MIN
#define MY_RF24_CHANNEL (72)

#define MY_NODE_ID 2

#include <MySensors.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Update interval to send data (milliseconds)
static const uint16_t UPDATE_INTERVAL = 3000;
static const uint16_t MAX_CYCLES_WITHOUT_SENDING = 10;

// SOIL MEASUREMENT CONFIGURATION
static const uint16_t SOIL_READS_DELAY = 100;
static const uint16_t SOIL_DRY_MAX = 1016;
static const uint8_t SOIL_WET_MAX = 41;
static const uint8_t SOIL_READS = 10;
static const uint8_t SOIL_POWER_PIN = 3;

#define DALLAS_PIN 2
#define SOIL_PIN A3

#define CHILD_ID_TEMP 1
#define CHILD_ID_SOIL 2

OneWire oneWire(DALLAS_PIN);
DallasTemperature sensors(&oneWire);

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgSoil(CHILD_ID_SOIL, V_TRIPPED);

float temperatureLastRead = 0;
uint8_t temperatureCyclesWithoutSending = 0;
int soilLastRead = 0;
uint8_t soilCyclesWithoutSending = 0;

void readTemperatureAndSoilMoisture();
float roundMeasurement(float val);
void processTemperature(float temperature);
void processSoilValue();

long soilReads[SOIL_READS];

void presentation()
{
  sendSketchInfo("SoilSensor", "1.0");
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_SOIL, S_MOISTURE);
}

void before()
{
  sensors.begin();
}

void setup()
{
  // analogReference(INTERNAL);
  pinMode(SOIL_POWER_PIN, OUTPUT);
  sensors.setWaitForConversion(false);
}

void loop()
{
  digitalWrite(SOIL_POWER_PIN, HIGH);
  readTemperatureAndSoilMoisture();
  digitalWrite(SOIL_POWER_PIN, LOW);
  sleep(UPDATE_INTERVAL);
}

void readTemperatureAndSoilMoisture()
{
  sensors.requestTemperatures();
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  sleep(conversionTime);
  processTemperature(sensors.getTempCByIndex(0));

  for (int i = 0; i < SOIL_READS; i++)
  {
    int soilRead = analogRead(SOIL_PIN);
    #ifdef MY_DEBUG
      Serial.print("Soil Read: ");
      Serial.println(soilRead);
    #endif
    soilReads[i] = soilRead;
    delayMicroseconds(SOIL_READS_DELAY);
  }
  processSoilValue();
}

void processTemperature(float temperature)
{
  if (isnan(temperature) || temperature < -10)
  {
    Serial.println("Failed reading temperature!");
  }
  else
  {
    temperature = roundMeasurement(temperature);
    if (++temperatureCyclesWithoutSending > MAX_CYCLES_WITHOUT_SENDING || temperature != temperatureLastRead)
    {
      send(msgTemp.set(temperature, 1));
      temperatureLastRead = temperature;
      temperatureCyclesWithoutSending = 0;
    }
  }
}

void processSoilValue()
{
  uint16_t sum = 0;
  for (int i = 0; i < SOIL_READS; i++)
  {
    uint16_t readValue = soilReads[i];
    sum += readValue;
  }
  uint8_t average = map(sum / SOIL_READS, SOIL_WET_MAX, SOIL_DRY_MAX, 100, 0);
  average = average > 100 ? 100 : average;
  if (++soilCyclesWithoutSending > MAX_CYCLES_WITHOUT_SENDING || average != soilLastRead)
  {
    send(msgSoil.set(average, 0));
    soilLastRead = average;
    soilCyclesWithoutSending = 0;
  }
}

float roundMeasurement(float val)
{
  val = val * 10.0f;
  val = (val > (floor(val) + 0.5f)) ? ceil(val) : floor(val);
  val = val / 10.0f;
  return val;
}