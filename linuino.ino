#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>


// DFPlayer Mini
SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
uint16_t numTracksInFolder;
uint16_t currentTrack;

// this object stores nfc tag data
struct nfcTagObject {
  uint32_t cookie;
  uint8_t version;
  uint8_t folder;
  uint8_t mode;
  uint8_t special;
};

nfcTagObject myCard;

bool knownCard = false;

#include "lib/track_navigation.h"
#include "lib/card_management.h"
#include "lib/voice_menu.h"

// implement a notification class,
// its member methods will get called
//
class Mp3Notify {
public:
  static void OnError(uint16_t errorCode) {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(uint16_t track) {
    Serial.print("Track beendet");
    Serial.println(track);
    delay(100);
    nextTrack(track);
  }
  static void OnCardOnline(uint16_t code) {
    Serial.println(F("SD Karte online "));
  }
  static void OnCardInserted(uint16_t code) {
    Serial.println(F("SD Karte bereit "));
  }
  static void OnCardRemoved(uint16_t code) {
    Serial.println(F("SD Karte entfernt "));
  }
  static void OnUsbOnline(uint16_t code) {
      Serial.println(F("USB online "));
  }
  static void OnUsbInserted(uint16_t code) {
      Serial.println(F("USB bereit "));
  }
  static void OnUsbRemoved(uint16_t code) {
    Serial.println(F("USB entfernt "));
  }
};

static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);

// Leider kann das Modul keine Queue abspielen.
static uint16_t _lastTrackFinished;

// MFRC522
#define RST_PIN 9                 // Configurable, see typical pin layout above
#define SS_PIN 10                 // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522
MFRC522::MIFARE_Key key;
bool successRead;
byte sector = 1;
byte blockAddr = 4;
byte trailerBlock = 7;
MFRC522::StatusCode status;

#define buttonPause A0
#define buttonUp A1
#define buttonDown A2
#define busyPin 4

#define LONG_PRESS 1000

Button pauseButton(buttonPause);
Button upButton(buttonUp);
Button downButton(buttonDown);
bool ignorePauseButton = false;
bool ignoreUpButton = false;
bool ignoreDownButton = false;

uint8_t numberOfCards = 0;

bool isPlaying() { return !digitalRead(busyPin); }

void setup() {

  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle
                        // Schnittstelle
  randomSeed(analogRead(A0)); // Zufallsgenerator initialisieren

  Serial.println(F("TonUINO Version 2.0"));
  Serial.println(F("(c) Thorsten Voß"));

  // Knöpfe mit PullUp
  pinMode(buttonPause, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);

  // Busy Pin
  pinMode(busyPin, INPUT);

  // DFPlayer Mini initialisieren
  mp3.begin();
  mp3.setVolume(15);

  // NFC Leser initialisieren
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522
      .PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle bekannten
  // Karten werden gelöscht
  if (digitalRead(buttonPause) == LOW && digitalRead(buttonUp) == LOW &&
      digitalRead(buttonDown) == LOW) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
  }

}

void loop() {
  do {
    mp3.loop();
    // Buttons werden nun über JS_Button gehandelt, dadurch kann jede Taste
    // doppelt belegt werden
    pauseButton.read();
    upButton.read();
    downButton.read();

    if (pauseButton.wasReleased()) {
      if (ignorePauseButton == false) {
        if (isPlaying())
          mp3.pause();
        else
          mp3.start();
      }
      ignorePauseButton = false;
    } else if (pauseButton.pressedFor(LONG_PRESS) &&
               ignorePauseButton == false) {
      if (isPlaying())
        mp3.playAdvertisement(currentTrack);
      else {
        knownCard = false;
        mp3.playMp3FolderTrack(800);
        Serial.println(F("Karte resetten..."));
        resetCard();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
      }
      ignorePauseButton = true;
    }

    if (upButton.pressedFor(LONG_PRESS)) {
      Serial.println(F("Volume Up"));
      mp3.increaseVolume();
      ignoreUpButton = true;
    } else if (upButton.wasReleased()) {
      if (!ignoreUpButton)
        nextTrack(random(65536));
      else
        ignoreUpButton = false;
    }

    if (downButton.pressedFor(LONG_PRESS)) {
      Serial.println(F("Volume Down"));
      mp3.decreaseVolume();
      ignoreDownButton = true;
    } else if (downButton.wasReleased()) {
      if (!ignoreDownButton)
        previousTrack();
      else
        ignoreDownButton = false;
    }
    // Ende der Buttons
  } while (!mfrc522.PICC_IsNewCardPresent());

  // RFID Karte wurde aufgelegt

  if (!mfrc522.PICC_ReadCardSerial())
    return;

  if (readCard(&myCard) == true) {
    if (myCard.cookie == 322417479 && myCard.folder != 0 && myCard.mode != 0) {

      knownCard = true;
      _lastTrackFinished = 0;
      numTracksInFolder = mp3.getFolderTrackCount(myCard.folder);
      Serial.print(numTracksInFolder);
      Serial.print(F(" Dateien in Ordner "));
      Serial.println(myCard.folder);

      // Hörspielmodus: eine zufällige Datei aus dem Ordner
      if (myCard.mode == 1) {
        Serial.println(F("Hörspielmodus -> zufälligen Track wiedergeben"));
        currentTrack = random(1, numTracksInFolder + 1);
        Serial.println(currentTrack);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Album Modus: kompletten Ordner spielen
      if (myCard.mode == 2) {
        Serial.println(F("Album Modus -> kompletten Ordner wiedergeben"));
        currentTrack = 1;
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Party Modus: Ordner in zufälliger Reihenfolge
      if (myCard.mode == 3) {
        Serial.println(
            F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
        currentTrack = random(1, numTracksInFolder + 1);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Einzel Modus: eine Datei aus dem Ordner abspielen
      if (myCard.mode == 4) {
        Serial.println(
            F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
        currentTrack = myCard.special;
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
      if (myCard.mode == 5) {
        Serial.println(F("Hörbuch Modus -> kompletten Ordner spielen und "
                         "Fortschritt merken"));
        currentTrack = EEPROM.read(myCard.folder);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
    }

    // Neue Karte konfigurieren
    else {
      knownCard = false;
      setupCard();
    }
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

#include "lib/voice_menu.c"
#include "lib/card_management.c"
#include "lib/track_navigation.c"
