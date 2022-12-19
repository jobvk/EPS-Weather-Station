#include <math.h>
#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <MKRNB.h>

// initialize the library instance
Adafruit_SHT31 sht35 = Adafruit_SHT31();
NBClient client;
NB nbAccess;
GPRS gprs;
MqttClient mqttClient(client);

//pin's location of the sensors
const int RainSensorPin = 5; // The pin location of the rain sensor
const int WindSensorSpeedPin = 4; // The pin location of the wind speed sensor
const char WindSensorDirectionPin = A0; // The pin location of the wind direction sensor

//var things for MQTT
const char pinnumber[] = "";
const char broker[] = "";
const int port = 1883;
const char username[] = "";
const char password[] = "";
const char topic1[]  = "eps/weather-station/air/temperature";
const char topic2[]  = "eps/weather-station/air/humidity";
const char topic3[]  = "eps/weather-station/wind/direction";
const char topic4[]  = "eps/weather-station/wind/speed";
const char topic5[]  = "eps/weather-station/rain";
const int interval = 60000; // interval in milliseconds

var things for wind speed sensor
int Rotations; // cup rotation counter used in interrupt routine
long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine
float WindSpeedMPH; // speed miles per hour
float WindSpeedKPH; // speed kilometer per hour

//var things for wind direcation sensor
const int WindDirectionOffset = 280;
int AnalogDirectionValue;// raw analog value from wind vane
int DirectionDegreesWithoutOffset;// translated 0 - 360 direction
int WindDirectionDegrees;// converted value with offset applied

//var things for rain sensor
long ContactBounceBucketTime;
const double bucketAmount = 0.4; // mm to trip tipping-bucket
double Rain = 0.0; // rain accumulated

bool setRAT(String choice)
{
  String response;

  Serial.print("Disconnecting from network: ");
  MODEM.sendf("AT+COPS=2");
  MODEM.waitForResponse(2000);
  Serial.println("Disconnected from network.");

  Serial.print("Setting Radio Access Technology: ");
  MODEM.sendf("AT+URAT=%s", choice.c_str());
  MODEM.waitForResponse(2000, &response);
  Serial.println("Done setting radio access.");
  return true;
}

bool apply()
{
  Serial.print("Applying changes and saving configuration: ");
  MODEM.send("AT+CFUN=15");
  MODEM.waitForResponse(5000);
  delay(5000);

  do {
    delay(1000);
    MODEM.noop();
  } while (MODEM.waitForResponse(1000) != 1);
  
  Serial.println("Done with applying changes and saving.");
  return true;
}

void setup() {
  Serial.begin(9600); // Begin serial communication at a baud rate of 9600:
  while (!Serial) {
    Serial.println("Wait for serial port to connect");
  }
  Serial.println("Start setup procedure");
  
  if (! sht35.begin(0x44))
  {
    Serial.println("Couldn't find SHT35");
    while(1);
  }

  pinMode(RainSensorPin, INPUT_PULLUP);
  pinMode(WindSensorSpeedPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RainSensorPin), isr_bucket, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WindSensorSpeedPin), isr_rotation, CHANGE);

  if (! MODEM.begin()) {
    Serial.println("ERROR, no modem answer.");
    while(1);
  }
  while (!MODEM.noop());
  delay(1000);
  setRAT("8");
  delay(1000);
  apply();

  Serial.println("Attempting to connect to cellular network");
  bool connected = false;

  // Start NB module
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while (!connected) 
  {
    if ((nbAccess.begin(pinnumber) == NB_READY) && (gprs.attachGPRS() == GPRS_READY)) 
    {
      Serial.println("NB success");
      connected = true;
    } else {
      Serial.println("NB connection failed");
      delay(1000);
    }
  }

  Serial.print("Attempting to connect to the mqtt broker: mqtt://");
  Serial.print(broker);
  Serial.print(":");
  Serial.print(port);
  Serial.println();  

  mqttClient.setUsernamePassword(username, password);

  if (mqttClient.connect(broker, port)) {
    Serial.println("MQTT connected");
  } else {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());  
  }
  
  Serial.println("End setup procedure");
  Serial.println("===================");
}

void loop() 
{
  WindSpeedMPH = Rotations * 0.0375;
  WindSpeedKPH = WindSpeedMPH * 1.609344;

  Rotations = 0; // Set Rotations count to 0 ready for calculations

  AnalogDirectionValue = analogRead(WindSensorDirectionPin);
  DirectionDegreesWithoutOffset = map(AnalogDirectionValue, 0, 1023, 0, 360);
  WindDirectionDegrees = DirectionDegreesWithoutOffset + WindDirectionOffset;

  if (WindDirectionDegrees > 360)
  {
    WindDirectionDegrees = WindDirectionDegrees - 360;
  }

  if (WindDirectionDegrees < 0)
  {
    WindDirectionDegrees = WindDirectionDegrees + 360;
  }
  
  // Read the temperature and the humidity:
  float tempC = sht35.readTemperature();
  float humidity = sht35.readHumidity();

  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print("\xC2\xB0 "); // shows degree symbol
  Serial.print(" Humidity: ");
  Serial.print(humidity);
  Serial.print(" %RH ");
  Serial.print("WindDirection: ");
  Serial.print(WindDirectionDegrees);
  Serial.print(" Rotations: ");
  Serial.print(Rotations);
  Serial.print(" Wind Speed: ");
  Serial.print(WindSpeedKPH);
  Serial.print(" Rain: ");
  Serial.print(Rain);
  Serial.println();

  mqttClient.beginMessage(topic1);
  mqttClient.print(tempC, 2);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic2);
  mqttClient.print(humidity, 2);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic3);
  mqttClient.print(WindDirectionDegrees);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic4);
  mqttClient.print(WindSpeedKPH, 2);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic5);
  mqttClient.print(Rain, 1);
  mqttClient.endMessage();

  Rain = 0.0;
  
  Serial.println("Go to sleep");

  delay(interval);
  
  Serial.println("Wake up");
}

 This is the function that the interrupt calls to increment the rotation count
void isr_rotation ()
{
  if ((millis() - ContactBounceTime) > 15 )
  {
    Serial.println("add ratation");
    Rotations++;
    ContactBounceTime = millis();
  }
}

// This is the function that the interrupt calls to increment the bucket tips
void isr_bucket ()
{
  if ((millis() - ContactBounceBucketTime) > 1000 )
  {
    Serial.println("add bucketAmount");
    Rain += bucketAmount;
    ContactBounceBucketTime = millis();
  }
}
