bool isPlaying() { return !digitalRead(busyPin); }

uint16_t newRandomTrack(uint16_t numTracksInFolder) {
  uint16_t newTrack = random(1, numTracksInFolder + 1);
  if (newTrack == currentTrack)
    newTrack = newTrack == numTracksInFolder ? 1 : newTrack + 1;
  return newTrack;
}

void saveProgress()  { EEPROM.write(myCard.folder, currentTrack); }
void resetProgress() { EEPROM.write(myCard.folder, 1); }

void resetPlayback() {
  uint8_t currentVolume = getVolume();
  setVolume(0);
  mp3.playFolderTrack(myCard.folder, 1);
  pausePlayback();
  setVolume(currentVolume);
}

void pausePlayback() { mp3.pause(); }
void startPlayback() { mp3.start(); }

void nextTrack(uint16_t track) {
  if (track == lastTrackFinished)
    return;

  lastTrackFinished = track;
   
  // Return early if card is unknown
  if (knownCard == false)
    return;

  switch(myCard.mode) {
  case MODE_RANDOM:
    printf("Mode random -> stop\n");
    break;

  case MODE_SINGLE:
    printf("Mode single -> stop\n");
    break;

  case MODE_ALBUM:
    if (currentTrack < numTracksInFolder) {
      currentTrack = currentTrack + 1;
      printf("Mode album -> next track: %d\n", currentTrack);
      mp3.playFolderTrack(myCard.folder, currentTrack);
    } else {
      currentTrack = 1;
      printf("Mode album -> end\n");
      resetPlayback();
    }
    break;

  case MODE_PARTY:
    currentTrack = newRandomTrack(numTracksInFolder);
    printf("Mode party -> next random track: %d\n", currentTrack);
    mp3.playFolderTrack(myCard.folder, currentTrack);
    break;

  case MODE_BOOK:
    if (currentTrack < numTracksInFolder) {
      currentTrack = currentTrack + 1;
      printf("Mode book -> next track (saved): %d\n", currentTrack);
      mp3.playFolderTrack(myCard.folder, currentTrack);
      saveProgress();
    } else {
      currentTrack = 1;
      printf("Mode book -> end, resetting track to 1\n");
      saveProgress();
      resetPlayback();
    }
    break;
  }
}

void previousTrack() {
  switch(myCard.mode) {
  case MODE_RANDOM:
    printf("Mode random -> play track again\n");
    break;

  case MODE_ALBUM:
    currentTrack = max(currentTrack - 1, 1);
    printf("Mode album -> previous track\n");
    break;

  case MODE_PARTY:
    printf("Party mode -> Replay current track\n");
    break;

  case MODE_SINGLE:
    printf("Mode single -> Replay current track\n");
    break;

  case MODE_BOOK:
    currentTrack = max(currentTrack - 1, 1);
    printf("Mode book -> previous track (saved)\n");
    saveProgress();
    break;
  }

  mp3.playFolderTrack(myCard.folder, currentTrack);
}
