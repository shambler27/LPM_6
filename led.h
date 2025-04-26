class TempLEDs {
  public:
    TempLEDs(int warningPin, int alarmPin) {
      _warningPin = warningPin;
      _alarmPin = alarmPin;
      pinMode(_warningPin, OUTPUT);
      pinMode(_alarmPin, OUTPUT);
    }
    void changeMod(Status check) {
      switch (check) {
        case NORMAL:
          // проверить пины, отключить, если включены  !!!!!!!!!!!!!!!!!!!!!!!!!!!!
          digitalWrite(_warningPin, LOW);
          digitalWrite(_alarmPin, LOW);
          break;
        case WARNING:
          digitalWrite(_warningPin, HIGH);
          break;
        case ALARM:
          digitalWrite(_alarmPin, HIGH);
          break;
      }
    }
  private:
    int _warningPin;
    int _alarmPin;
};
