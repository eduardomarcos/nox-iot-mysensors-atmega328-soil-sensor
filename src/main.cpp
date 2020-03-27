#define MY_DEBUG
#define MY_RADIO_RF24

#define MY_RF24_PA_LEVEL RF24_PA_MIN
#define MY_RF24_CHANNEL (72)

#define MY_NODE_ID 2

#include <MySensors.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Update interval to send data (milliseconds)
static const uint64_t UPDATE_INTERVAL = 7000;
static const uint16_t MAX_CYCLES_WITHOUT_SENDING = 10;

#define DALLAS_PIN 2

#define CHILD_ID_TEMP 1

OneWire oneWire(DALLAS_PIN);
DallasTemperature sensors(&oneWire);

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

float lastTemperatureRead = 0;
int cyclesWithoutSending = 0;

void readTemperatureAndSoilMoisture();
float roundMeasurement(float val);
void processTemperature(float temperature);

void presentation()
{
  sendSketchInfo("SoilSensor", "1.0");
  present(CHILD_ID_TEMP, S_TEMP);
}

void before()
{
  sensors.begin();
}

void setup()
{
  // analogReference(INTERNAL);
  sensors.setWaitForConversion(false);
}

void loop()
{
  readTemperatureAndSoilMoisture();
  sleep(UPDATE_INTERVAL);
}

void readTemperatureAndSoilMoisture()
{
  sensors.requestTemperatures();
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  sleep(conversionTime);
  float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(0) * 10.)) / 10.;
  processTemperature(temperature);
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
    if (++cyclesWithoutSending > MAX_CYCLES_WITHOUT_SENDING || temperature != lastTemperatureRead)
    {
      send(msgTemp.set(temperature, 1));
      lastTemperatureRead = temperature;
      cyclesWithoutSending = 0;
    }
  }
}

float roundMeasurement(float val)
{
  val = val * 10.0f;
  val = (val > (floor(val) + 0.5f)) ? ceil(val) : floor(val);
  val = val / 10.0f;
  return val;
}