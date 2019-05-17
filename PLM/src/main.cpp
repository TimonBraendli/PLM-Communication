#include <Arduino.h>
#include <CheapStepper.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"


#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

const int Sensor1 = 12;// Sensor zur Packeterkennung auf Pin12
//int Sensor1State =0;
CheapStepper stepper (6,9,10,11);
enum States{waitingPosition, loadPosition, unloadPosition, loaded};
States currState = waitingPosition;
bool Sensor1State;
String message;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


bool ReadSensor( int PinSensor){
    int counter =0;
    for(int i=0; i<=3;i++){
        if(digitalRead(PinSensor)==1){
            counter++;
        }
        delay(100);
    }
    if (counter){
        return 1;
    }
    else{
        return 0;
    }
}

void loadPackage(){

    stepper.setRpm(10);
    stepper.moveDegrees (true, 3*360); // true wird wird deffiniert als Postion der Aufladerampe (links/rechts)

}

void unloadPackage(bool right){

    stepper.setRpm(10);
    stepper.moveDegrees (right, 3*360);
}

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void initBLE() {
  Serial.println(F("Adafruit Bluefruit Command Mode Example"));
  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  ble.println("AT+GAPDEVNAME=Zahnfee"); //Name des Bluetooth Modul

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();
}
String listenBLE() {
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return("0");
  }
  Serial.println(ble.buffer);
  return(ble.buffer);
  //handleApiCommands(ble.buffer);
}

// Send message over Bluetooth
void sendBLE(String msg) {
  ble.print("AT+BLEUARTTX=");
  ble.println(msg);

  // check response stastus
  if (! ble.waitForOK() ) {
    Serial.println(F("Failed to send?"));
  }
}



// the setup function runs once when you press reset or power the board
void setup() {
   Serial.begin(9600);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Sensor1,INPUT);


  initBLE();
  ble.verbose(false);  // debug info is a little annoying after this point!
}


void loop() {
  message =  listenBLE();
    switch (currState)  //Statemachine
    {
        case loadPosition:{ // Roboter befindet sich an der Ladestation und soll aufladen
         Serial.println("loading..."); // Serielle Kommunikation zu Debug zwecken
          loadPackage();  // lässt den Stepper Motor 3 Umdrehungen machen, so dass das Paket in der Mitte der Führung zu liegen kommt
         if (1==ReadSensor(Sensor1)){ // Auslesen des Kontaktsensors( true wenn sich eine Ladung an Bord befindet)
           sendBLE("package loaded"); //Informationsaustausch mit Leitsystem über BLE: Ladung befindet sich an Bord
           currState = unloadPosition;  // Wechsel des State
         }
         else{
           sendBLE("you failed!!!!"); //Informationsaustausch mit Leitsystem über BLE: Ladung befindet sich nicht an Bord
           currState = waitingPosition;// Wechsel des State
         }
          break;
        }
        case unloadPosition:{ // Roboter hat Bauteil geladen und ist bereit zum Entladen
          Serial.println("ready for unloading..."); // Serielle Kommunikation zu Debug zwecken
          if (message=="unload right"){ //Informationsaustausch mit Leitsystem über BLE: Abladebefhl des Leitsystems,
          //Fallunterscheidung ob Packet links oder rechts abgeladen werden soll
          unloadPackage(true); // unloadPackage(true) lädt das Paket mit 3 vollen Umdrehungen nach rechts ab
            if (0==ReadSensor(Sensor1)){ // Sensor überprüft ob das Packet immer noch an Bord ist
            sendBLE("package unloaded"); //Informationsaustausch mit Leitsystem über BLE: Packet wurde erfolgreich abgeladen
            currState = waitingPosition; // Wechsel in den Wartezustand
            }
            else{
              sendBLE("unloading failed"); //Informationsaustausch mit Leitsystem über BLE: Packet befindet sich noch immer auf der Führung
            }
          }
          if (message=="unload left"){
            unloadPackage(false);
            if (0==ReadSensor(Sensor1)){ //ReadSensor( int PinSensor)==0
            sendBLE("package unloaded"); //Informationsaustausch mit Leitsystem
            currState = waitingPosition; //Wechsel in den Wartezustand
            }
            else{
              sendBLE("unloading failed");
            }
          }
          break;
        }
        case waitingPosition:{//Default State um den Load befehl ab zu warten
          Serial.println("waiting...");
          if (message=="load"){//Informationsaustausch mit Leitsystem über BLE: Packet soll geladen werden
            currState = loadPosition; // Wechsel des State
          }
            break;
        }
    }
}
