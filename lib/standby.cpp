void setstandbyTimer() {
  printf("=== setstandbyTimer()\n");
  sleepAtMillis = millis() + standbyTimer;
  printf("Sleeping at %d\n", sleepAtMillis);
}

void disablestandbyTimer() {
  printf("=== disablestandby()\n");
  sleepAtMillis = 0;
}

void pollStandby() {
  if (sleepAtMillis != 0 && millis() > sleepAtMillis) {
    printf("=== power off!i\n");
    // enter sleep state
    digitalWrite(shutdownPin, HIGH);
    delay(500);

    // http://discourse.voss.earth/t/intenso-s10000-powerbank-automatische-abschaltung-software-only/805
    // powerdown to 27mA (powerbank switches off after 30-60s)
    mfrc522.PCD_AntennaOff();
    mfrc522.PCD_SoftPowerDown();
    mp3.sleep();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();  // Disable interrupts
    sleep_mode();
  }
}
