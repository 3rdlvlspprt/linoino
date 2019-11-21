#define RST_PIN 9
#define SS_PIN 10
#define buttonPause A0
#define buttonUp A1
#define buttonDown A2
#define busyPin 4
#define shutdownPin 7
#define LONG_PRESS 1000

#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>

#include "lib/setup.h"
#include "lib/standby.h"
#include "lib/button_interface.h"
#include "lib/track_navigation.h"
#include "lib/card_management.h"
#include "lib/voice_menu.h"
#include "lib/mp3_init.h"

void setup() {
  mySetup();
}

void loop() {
  mp3.loop();
  handleButtonPresses();
  handleCardReader();
}

#include "lib/setup.cpp"
#include "lib/standby.cpp"
#include "lib/button_interface.cpp"
#include "lib/voice_menu.cpp"
#include "lib/card_management.cpp"
#include "lib/track_navigation.cpp"
