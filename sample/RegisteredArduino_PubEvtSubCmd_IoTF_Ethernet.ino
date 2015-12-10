#include <SPI.h>
#include <Ethernet.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

#define MS_PROXY                "u614vn.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT               1883
#define MQTT_MAX_PACKET_SIZE    100
#define MQTT_CLIENT_ID          "d:u614vn:iotsample-arduino:00ffbbccde02"
#define PUBLISH_TOPIC           "iot-2/evt/status/fmt/json"                             // iot-2/evt/<event_id>/fmt/<format_string>
#define SUBSCRIBE_TOPIC         "iot-2/cmd/+/fmt/json"                                  // iot-2/cmd/<command_id>/fmt/<format_string>
#define AUTHMETHOD              "use-token-auth"
#define AUTHTOKEN               "password"
byte mac[] = { 0x00, 0xFF, 0xBB, 0xCC, 0xDE, 0x02 };

int ledPin = 8;

// For Arduino Yun, instantiate a YunClient and use the instance to declare
// an IPStack ipstack(c) instead of EthernetStack with c being the YunClient
// e.g. YunClient c;
// IPStack ipstack(c);
EthernetClient c;               // replace by a YunClient if running on a Yun
IPStack ipstack(c);

MQTT::Client<IPStack, Countdown, 100, 1> client = MQTT::Client<IPStack, Countdown, 100, 1>(ipstack);

void messageArrived(MQTT::MessageData& md);

String deviceEvent;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);          //For Arduino Yun, use Bridge.begin()
  pinMode(ledPin, OUTPUT);
  delay(1000);
}

void loop() {
  int rc = -1;
  if (!client.isConnected()) {
    Serial.print("Connecting using Registered mode with client id : ");
    Serial.print(MQTT_CLIENT_ID);
    Serial.print("\tto MQTT Broker : ");
    Serial.print(MS_PROXY);
    Serial.print("\ton topic : ");
    Serial.println(PUBLISH_TOPIC);
    while (rc != 0) {
      rc = ipstack.connect(MS_PROXY, MQTT_PORT);
    }
    Serial.println("TCP connected");

    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    options.MQTTVersion = 3;
    options.clientID.cstring = MQTT_CLIENT_ID;
    options.username.cstring = AUTHMETHOD;
    options.password.cstring = AUTHTOKEN;
    options.keepAliveInterval = 10;
    rc = -1;
    while ((rc = client.connect(options)) != 0)
    {
      Serial.print("rc=");
      Serial.println(rc);
    }
    Serial.println("MQTT connected");

    //unsubscribe the topic, if it had subscribed it before.
    client.unsubscribe(SUBSCRIBE_TOPIC);

    //Try to subscribe for commands
    if ((rc = client.subscribe(SUBSCRIBE_TOPIC, MQTT::QOS0, messageArrived)) != 0) {
            Serial.print("Subscribe failed with return code : ");
            Serial.println(rc);
    } else {
          Serial.println("Subscribed\n");
    }

    Serial.println("Connected successfully\n");
    Serial.println("Temperature(in C)\tDevice Event (JSON)");
    Serial.println("____________________________________________________________________________");
  }

  MQTT::Message message;
  message.qos = MQTT::QOS0;
  message.retained = false;

  char json[56] = "{\"d\":{\"myName\":\"Arduino Uno\",\"temperature\":";
//  char buffer[10];
  float tempValue = getTemp();
  dtostrf(tempValue,1,2, &json[43]);
//  dtostrf(getTemp(),1,2, buffer);

  json[48] = '}';
  json[49] = '}';
  json[50] = '\0';
  Serial.print("\t");
  Serial.print(tempValue);
  Serial.print("\t\t");
  Serial.println(json);
  message.payload = json;
  message.payloadlen = strlen(json);

  rc = client.publish(PUBLISH_TOPIC, message);
  if (rc != 0) {
    Serial.print("Message publish failed with return code : ");
    Serial.println(rc);
  }
  client.yield(1000);
}

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;

    int topicLen = strlen(md.topicName.lenstring.data) + 1;
    char * topic = md.topicName.lenstring.data;
    topic[topicLen] = '\0';
    Serial.print("topic=");
    Serial.println(topic);

    int payloadLen = message.payloadlen + 1;
    char * payload = (char*)message.payload;
    payload[payloadLen] = '\0';
    Serial.print("payload=");
    Serial.println(payload);

    String topicStr = topic;
    String payloadStr = payload;

    //Command topic: iot-2/cmd/blink/fmt/json

    if(strstr(topic, "/cmd/blink") != NULL) {
      Serial.print("Command IS Supported : ");
      Serial.print(payload);
      Serial.println("\t.....\n");


      //Blink
      for(int i = 0 ; i < 2 ; i++ ) {
        digitalWrite(ledPin, HIGH);
        delay(1000);
        digitalWrite(ledPin, LOW);
        delay(1000);
      }

    } else {
      Serial.println("Command Not Supported:");
    }
}

/*
This function is reproduced as is from Arduino site => http://playground.arduino.cc/Main/InternalTemperatureSensor
*/
double getTemp(void) {
  unsigned int wADC;
  double t;

  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
