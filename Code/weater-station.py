import random
from paho.mqtt import client as mqtt_client
import mariadb
import requests

broker = ''
port = 1883
topic = [("eps/weather-station/air/temperature",0),("eps/weather-station/air/humidity",0),("eps/weather-station/wind/direction",0),("eps/weather-station/wind/speed",0),("eps/weather-station/rain",0)]
client_id = f'python-mqtt-{random.randint(0, 100)}'
usernamemqtt = ''
passwordmqtt = ''

mydb =  mariadb.connect(
  host="",
  user="",
  password="",
  database=""
)

mycursor = mydb.cursor()

wundergroundID = ""
wundergroundPASSWORD = ""

AirTempterure = 0.0
AirHumidity = 0.0
WindDirection = 0
WindSpeed = 0.0
RainMilimeter = 0.0

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(usernamemqtt, passwordmqtt)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        if msg.topic == 'eps/weather-station/air/temperature':
            global AirTempterure 
            AirTempterure = float(msg.payload.decode())

        elif msg.topic == "eps/weather-station/air/humidity":
            global AirHumidity
            AirHumidity = float(msg.payload.decode())
            
        elif msg.topic == "eps/weather-station/wind/direction":
            global WindDirection
            WindDirection = int(msg.payload.decode())

        elif msg.topic == "eps/weather-station/wind/speed":
            global WindSpeed
            WindSpeed = float(msg.payload.decode())

        elif msg.topic == "eps/weather-station/rain":
            global RainMilimeter
            RainMilimeter = float(msg.payload.decode())

            try:
                mycursor.execute("INSERT INTO weather_data (`temp_celcius`, `humidity`, `wind_speed_meter_direction_degrees`, `wind_speed_meter_per_second`, `rain_millimeter`) VALUES ('{}','{}','{}','{}','{}');".format(AirTempterure, AirHumidity, WindDirection, WindSpeed, RainMilimeter))
                mydb.commit()
                print("1 record inserted, ID:", mycursor.lastrowid)
            except:
                print("An exception occurred while inserting data to database")

            try:
                indoor = False
                if indoor:
                    response = requests.get("https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?ID={}&PASSWORD={}&dateutc=now&indoortempf={}&indoorhumidity={}".format(wundergroundID, wundergroundPASSWORD, ((AirTempterure * 1.8) + 32), AirHumidity))
                else:
                    response = requests.get("https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?ID={}&PASSWORD={}&dateutc=now&tempf={}&humidity={}&winddir={}&windspeedmph={}&rainin={}".format(wundergroundID, wundergroundPASSWORD, ((AirTempterure * 1.8) + 32), AirHumidity, WindDirection, (WindSpeed *0.621371192), (RainMilimeter * 0.039)))
                
                if response.status_code == 200:
                    print("Data send to wunderground")
                else:
                    print("Data was not send to wunderground status code: {}".format(response.status_code))
            except:
                print("An exception occurred while inserting data to wunderground")

    client.subscribe(topic)
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()


if __name__ == '__main__':
    run()