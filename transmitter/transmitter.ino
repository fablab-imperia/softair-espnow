#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
//Inclusione delle librerie
#include <Bounce2.h>


//Costanti e PIN

//Pin pulsante incorporato nel modulo joystick che abilita o disabilita il controllo dei motori
const unsigned int pinSwEnable = 32;

//Pin pulsante esterno che controllerà il servo del grilletto
const unsigned int pinSwTrigger = 33;

//Chip Select e Chip Enable della Radio
const unsigned int radioCE = 4;
const unsigned int radioCS = 5;

//Pin analogici per il joystick
const unsigned int jX = 34;
const unsigned int jY = 35;


/*
  Variabili utilizzate per definire min e max speed ed eseguire il mapping sui valori del joystick.
*/

const unsigned int maxSpeedForward = 180;
const unsigned int stopSpeed = 90;
const unsigned int maxSpeedBackward = 0;

/*
  La lettura dei potenziometri non è mai affidabile al 100%.
  Questo valore aiuta a determinare il punto da considerare come "Sta al centro" nei movimenti.
*/
const unsigned int treshold = 8;

//Millisecondi per il debonuce del bottone
const unsigned long debounceDelay = 20;

//Definisco struttura pacchetto da inviare
struct Packet {
  unsigned int speedX;
  unsigned int speedY;
  boolean enable;
  boolean trigger;
};

//Variabili di appoggio
long  valX, mapX, valY, mapY, tresholdUp, tresholdDown;

//Creo istanze dei bottoni
Bounce btnEnable = Bounce();  //istanzia un bottone dalla libreria Bounce
Bounce btnTrigger = Bounce();  //istanzia un bottone dalla libreria Bounce


//Creo ed inizializzo istanza pacchetto da inviare
Packet pkt = {
  stopSpeed,//speedX
  stopSpeed,//speedY
  false,//enable
  false //trigger
};

//MAC Address of receiver
uint8_t receiverAddress[] = {0x8C, 0xAA, 0xB5, 0x8B, 0x22, 0xBC};

/*
  Si occupa di leggere lo stato dei pulsanti ed aggiornare le variabili nel Packet
*/
void handlePulsanti() {

  btnEnable.update();
  if (btnEnable.fell()) {
    pkt.enable = !pkt.enable;
  }


  //Aggiorna stato di pressione del "grilletto"
  btnTrigger.update();
  pkt.trigger = !btnTrigger.read();

}

/*
  Si occupa di leggere i valori del joystick, mapparli ed aggiornare le variabili nel Packet
*/
void handleJoystick() {

  //esegui lettura analogica dei valori provenienti dai potenziometri del joystick
  valX = analogRead(jX);
  valY = analogRead(jY);

  //mappa i valori letti in funzione della velocità minima e massima
  mapX = map(valX, 0, 4095, 0, maxSpeedForward);
  mapY = map(valY, 0, 4095, 0, maxSpeedForward);

  Serial.print("Mapped X: ");
  Serial.println(mapX);
  Serial.print("Mapped Y: ");
  Serial.println(mapY);


  if (mapX <= tresholdDown) {
    //x va indietro
    pkt.speedX = map(mapX, maxSpeedBackward, tresholdDown, maxSpeedBackward, tresholdDown);
  } else if (mapX >= tresholdUp) {
    //x va avanti
    pkt.speedX = map(mapX, tresholdUp, maxSpeedForward, tresholdUp, maxSpeedForward);
  } else {
    //x sta fermo
    pkt.speedX = stopSpeed;
  }

  if (mapY <= tresholdDown) {
    //y va indietro
    pkt.speedY = map(mapY, maxSpeedBackward, tresholdDown, maxSpeedBackward, tresholdDown);
  } else if (mapY >= tresholdUp) {
    //y va avanti
    pkt.speedY = map(mapY, tresholdUp, maxSpeedForward, tresholdUp, maxSpeedForward);
  } else {
    //y sta fermo
    pkt.speedY = stopSpeed;
  }
}


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStato invio ultimi dati:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "KO");
}

void setup() {

  Serial.begin(115200);
  //Definizione delle modalità dei pin

  //Tasto enable
  pinMode(pinSwEnable, INPUT_PULLUP);

  //Tasto grilletto
  pinMode(pinSwTrigger, INPUT_PULLUP);

  //Inizializzo Wifi
   // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Stampo MAC
  Serial.println(WiFi.macAddress());

  //Inizializzo ESPNow
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallita aggiunta receiver!");
    return;
  }

  //Configuro istanze dei pulsanti
  btnEnable.attach(pinSwEnable);
  btnEnable.interval(debounceDelay);

  btnTrigger.attach(pinSwTrigger);
  btnTrigger.interval(debounceDelay);

  //Calcolo range valori entro i quali considerare la posizione del joystick come "Sta al centro"
  tresholdDown = (maxSpeedForward / 2) - treshold;
  tresholdUp = (maxSpeedForward / 2) + treshold;

}

void loop() {

  //gestisci stato dei pulsanti
  handlePulsanti();

  //gestisci valori dei potenziometri
  handleJoystick();

  //Invia dati tramite la radio
  Serial.print("enable: ");
  Serial.print(pkt.enable);
  Serial.print(" speedX: ");
  Serial.print(pkt.speedX);
  Serial.print(" speedY: ");
  Serial.print(pkt.speedY);
  Serial.print(" trigger: ");
  Serial.println(pkt.trigger);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &pkt, sizeof(pkt));
  
  if (result == ESP_OK) {
    Serial.println("Dati inviati correttamente");
  }
  else {
    Serial.println("Errore in invio dati");
  }

  delay(100);
}



