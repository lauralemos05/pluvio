
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include "ConnectData.h"
#include <ESP8266WiFi.h>

#define DATA_INTERVAL 1000       // Intervalo para adquirir novos dados do sensor (milisegundos).
#define AVAILABLE_INTERVAL 5000  // Intervalo para enviar sinais de available (milisegundos)
#define LED_INTERVAL_MQTT 1000   // Intervalo para piscar o LED quando conectado no broker
#define JANELA_FILTRO 10         // Número de amostras do filtro para realizar a média

byte TRIG_PIN = 12;
byte ECHO_PIN = 14;

unsigned long dataIntevalPrevTime = 0;       // will store last time data was send
unsigned long availableIntevalPrevTime = 0;  // will store last time "available" was send

const int QTD_AGUA_POR_MOV = 3;                       // quantidade de agua que cai a cada movimento da bascula
int qtd_movimentos = 0;                               // Declara uma variável para contar os movimentos, inicialmente definida como 0
unsigned long tempoUsado_contabilizarUnicaVez = 500;  // Declarar uma variável que determine um tempo que contabilize o movimento uma única vez
unsigned long tempo_atual = 0;                        // Variável que determina o tempo atual em milissegundos
unsigned long tempo_ultimoMovimento = 0;              // Variável usada para registrar o tempo do último movimento
const int interruptPin = 13;                          // Define o pino de interrupção. Pino D5 no ESP8266 corresponde ao GPIO14


void ICACHE_RAM_ATTR somarMovimento() {  // Define a função de interrupção que será chamada quando ocorrer um evento de interrupção
  tempo_atual = millis();                // o tempo atual registra o tempo em milissegundos
  if (tempo_atual - tempo_ultimoMovimento > tempoUsado_contabilizarUnicaVez) {
    qtd_movimentos = qtd_movimentos + 1;  // Incrementa a variável qtd_movimentos em 1 a cada interrupção
    tempo_ultimoMovimento = tempo_atual;  // o tempo do ultimo movimento recebe o tempo atual
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);                                                     // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);                                                      // Sets the echoPin as an Input
  pinMode(interruptPin, INPUT);                                                  // Configura o pino de interrupção como entrada
  attachInterrupt(digitalPinToInterrupt(interruptPin), somarMovimento, RISING);  // Configura a interrupção no pino definido para acionar na borda de subida

  // Optional functionalities of EspMQTTClient
  //client.enableMQTTPersistence();
  client.enableDebuggingMessages();                                           // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridden with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                         // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  WiFi.mode(WIFI_STA);
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {
  availableSignal();
}

void availableSignal() {
  client.publish("cm/pluviografo/available", "online");
}

StaticJsonDocument<100> readSensor() {
  StaticJsonDocument<100> jsonDoc;
  jsonDoc["value"] = qtd_movimentos;  // colocar variavel que vai receber o valor da medição da chuva - check check
  return jsonDoc;
}

void metodoPublisher() {
  static unsigned int amostrasTotais = 0;   // variável para realizar o filtro de média
  static unsigned int amostrasValidas = 0;  // variável para realizar o filtro de média
  static float acumulador = 0;              // variável para acumular a média

  StaticJsonDocument<100> jsonSensor;
  jsonSensor = readSensor();

  // Realização de tratamento de erros
  if (jsonSensor["erro"] == false) {
    float temp = jsonSensor["value"];
    acumulador += temp;  // somente os valores sem erro serão utilizados na média
    amostrasValidas++;   // incremento das amostras onde não foram encontradas erros
  }
  amostrasTotais++;  // incremento de amostras total

  // realização de média
  if (amostrasTotais >= JANELA_FILTRO) {
    StaticJsonDocument<300> jsonDoc;
    jsonDoc["RSSI"] = WiFi.RSSI();
    jsonDoc["milimetro_chuva"] = qtd_movimentos * QTD_AGUA_POR_MOV;  //
    jsonDoc["erro"] = false;

    //colocar loop do codigo original



    String payload = "";
    serializeJson(jsonDoc, payload);

    client.publish(topic_name, payload);
    amostrasTotais = 0;
    amostrasValidas = 0;
    acumulador = 0;
  }
}

void blinkLed() {
  static unsigned long ledWifiPrevTime = 0;
  static unsigned long ledMqttPrevTime = 0;
  unsigned long time_ms = millis();
  bool ledStatus = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (client.isMqttConnected()) {
      if ((time_ms - ledMqttPrevTime) >= LED_INTERVAL_MQTT) {
        ledStatus = !digitalRead(LED_BUILTIN);
        digitalWrite(LED_BUILTIN, ledStatus);
        ledMqttPrevTime = time_ms;
      }
    } else {
      digitalWrite(LED_BUILTIN, LOW);  // liga led
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // desliga led
  }
}

void loop() {
  unsigned long time_ms = millis();
  client.loop();

  if (time_ms - dataIntevalPrevTime >= DATA_INTERVAL) {
    client.executeDelayed(1 * 100, metodoPublisher);
    dataIntevalPrevTime = time_ms;
  }

  if (time_ms - availableIntevalPrevTime >= AVAILABLE_INTERVAL) {
    client.executeDelayed(1 * 500, availableSignal);
    availableIntevalPrevTime = time_ms;
  }

  blinkLed();

  //Serial.println(qtd_movimentos);  // Imprime o valor atual de qtd_movimentos na porta serial
  delay(100);  // Aguarda 100 milissegundos antes de repetir o loop
}
