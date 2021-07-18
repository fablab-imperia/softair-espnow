//Inclusione delle librerie
#include <ESP32Servo.h> 
#include <esp_now.h>
#include <WiFi.h>


//Costanti e PIN

//Pin servo X
const unsigned int pinServoX = 33;

//Pin servo Y
const unsigned int pinServoY = 27;

//Pin servo Trigger
const unsigned int pinServoTg = 32;



//definisco struttura pacchetto che riceverò
struct Packet {
  unsigned int speedX;
  unsigned int speedY;
  boolean enable;
  boolean trigger;
};


const unsigned int stopSpeed = 90;

//Creo istanza della "radio" passandogli il numero dei pin collegati a CE e CSN del modulo


//Creo istanze servo
Servo servoX;
Servo servoY;
Servo servoTg;

//Creo ed inizializzo istanza pacchetto che userò per i dati ricevuti
Packet pkt = {
  stopSpeed,
  stopSpeed,
  false,
  false
};


/*
  Interpreta i valori contenuti nella struttura Packet
  ed aziona i servo di conseguenza
*/
void pilotaServiMovimento(Packet pkt) {

  if (pkt.enable) {
    servoX.write(pkt.speedX);
    servoY.write(pkt.speedY);
  } else {
    servoX.write(stopSpeed);
    servoY.write(stopSpeed);
  }
}


// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&pkt, incomingData, sizeof(pkt));
  
  Serial.println("Dati ricevuti: ");
  Serial.print("enable: ");
  Serial.print(pkt.enable);
  Serial.print(" speedX: ");
  Serial.print(pkt.speedX);
  Serial.print(" speedY: ");
  Serial.print(pkt.speedY);
  Serial.print(" trigger: ");
  Serial.println(pkt.trigger);

  //Se nel pkt il valore del trigger è 1 ruoto di 90° il servo motor
  if (pkt.trigger) {
    servoTg.write(90);
  } else {
    //Altrimenti lo rimetto a 0
    servoTg.write(0);
  }
  //Interpreta i valori ricevuti ed aziona i motori di conseguenza
  pilotaServiMovimento(pkt);
}

void setup() {
  //Definizione delle modalità dei pin

  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);

    // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  servoX.setPeriodHertz(50);// Standard 50hz servo
  servoY.setPeriodHertz(50);// Standard 50hz servo
  servoTg.setPeriodHertz(50);// Standard 50hz servo

  //Associa pin ai servo
  servoX.attach(pinServoX);
  servoY.attach(pinServoY);
  servoTg.attach(pinServoTg);

  //Inizializzo la radio


  servoX.write(stopSpeed);
  servoY.write(stopSpeed);
  servoTg.write(0);

  Serial.print("RECEIVER INDIRIZO MAC: ");
  Serial.println(WiFi.macAddress());

  //Reegistro callback per ricezione dati 
  esp_now_register_recv_cb(OnDataRecv);
}





void loop() {


}

