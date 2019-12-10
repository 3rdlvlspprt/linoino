class Mp3Notify {
  public:
    static void OnError(uint16_t errorCode) {
      printf(FS("Com Error %d\n"), errorCode);
    }

    static void OnPlayFinished(uint16_t track) {
      printf(FS("End of track %d\n"), track);
      delay(100);
      nextTrack(track);
    }

    static void OnCardOnline(uint16_t code) {
      printf(FS("SD Karte online\n"));
    }

    static void OnCardInserted(uint16_t code) {
      printf(FS("SD Karte bereit\n"));
    }

    static void OnCardRemoved(uint16_t code) {
      printf(FS("SD Karte entfernt\n"));
    }

    static void OnUsbOnline(uint16_t code) {
      printf(FS("USB online\n"));
    }

    static void OnUsbInserted(uint16_t code) {
      printf(FS("USB bereit\n"));
    }

    static void OnUsbRemoved(uint16_t code) {
      printf(FS("USB entfernt\n"));
    }
};

DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);
