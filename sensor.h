#include <OneWire.h>
#include <DallasTemperature.h>

// Пороги температуры
const float WARNING_THRESHOLD_WATER = 90.0;  // Предупреждение для воды
const float ALARM_THRESHOLD_WATER = 98.0;   // Тревога для воды
const float WARNING_THRESHOLD_OIL = 79.0;    // Предупреждение для масла
const float ALARM_THRESHOLD_OIL = 87.0;     // Тревога для масла

enum Status { NORMAL, WARNING, ALARM };

class TempSensors {
  public:
    int brokenSensor;
    float averTemp;

    TempSensors(const int (&pins)[3], const float warningTemp, const float alertTemp) {
      _warningTemp = warningTemp;
      _alarmTemp = alertTemp;

      for (int i = 0; i < 3; i++) {
        _wires[i] = OneWire( pins[i] );
        _sensors[i] = DallasTemperature( &_wires[i] );
      }
    }

    // Запуск сенсоров
    void startSensors() {
      for (int i = 0; i < 3; i++) {
        _sensors[i].begin();
      }
    }

    // Запись новых или обновление показателей температуры
    void updateTempratures() {
      for (int i = 0; i < 3; i++) {
        _sensors[i].requestTemperatures();
        _temp[i] = _sensors[i].getTempCByIndex(0);
      }
    }

    // Поиск сломанного сенсора, нужны показатели температуры
    void updateBroken() {
      const double maxAllowedDiff = 10.0; // Максимально допустимая разница между исправными датчиками (2 * 5°C)

      // Проверяем, все ли датчики "согласны" друг с другом в пределах погрешности
      bool t1_close_to_t2 = (abs(_temp[0] - _temp[1]) <= maxAllowedDiff);
      bool t1_close_to_t3 = (abs(_temp[0] - _temp[2]) <= maxAllowedDiff);
      bool t2_close_to_t3 = (abs(_temp[1] - _temp[2]) <= maxAllowedDiff);

      if (t1_close_to_t2 && t1_close_to_t3 && t2_close_to_t3) {
        // Все три датчика показывают близкие значения → все исправны
        brokenSensor = 0;
      }
      else if (t1_close_to_t2 && !t1_close_to_t3) {
        // 1 и 2 близки, 3 отличается → 3 сломан
        brokenSensor = 3;
      }
      else if (t1_close_to_t3 && !t1_close_to_t2) {
        // 1 и 3 близки, 2 отличается → 2 сломан
        brokenSensor = 2;
      }
      else if (t2_close_to_t3 && !t1_close_to_t2) {
        // 2 и 3 близки, 1 отличается → 1 сломан
        brokenSensor = 1;
      }
    }

    // Средняя температура (желательно проверить наличие сломанного сенсора)
    float getAverage() {
      if (brokenSensor) {
        averTemp = (_temp[0] + _temp[1] + _temp[2] - _temp[brokenSensor - 1]) / 2.0;
        return averTemp;
      } else {
        averTemp = (_temp[0] + _temp[1] + _temp[2]) / 3.0;
        return averTemp;
      }
    }

    // Проверка состояния, необходимо ли предупреждение или тревога
    Status checkWarning() {
      return (averTemp < _warningTemp) ? NORMAL :
             (averTemp < _alarmTemp) ? WARNING : ALARM;
    }

  private:
    OneWire _wires[3];
    DallasTemperature _sensors[3];
    float _temp[3];
    float _warningTemp;
    float _alarmTemp;
};
