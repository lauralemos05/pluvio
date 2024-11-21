const String topic_name = "cm/pluviografo";


EspMQTTClient client(
  "wIFRN-IoT",
  "deviceiotifrn",
  "10.57.0.10",  // MQTT Broker server ip
  "tht",   // Can be omitted if not needed
  "senha123",   // Can be omitted if not needed
  "pluviografo",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);