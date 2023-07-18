#define DECODE_AC   true
#define IR_LISTEN
#define IR_DECODE
#define IR_RETURN

// Для дальней комнаты
#define IN_SEL_MYSWING        "input_select.dalnac_myswing"
#define IN_SEL_XFAN           "input_boolean.dalnac_xfan"
// Для зала zal
//#define IN_SEL_MYSWING        "input_select.zalac_myswing"
//#define IN_SEL_XFAN           "input_boolean.zalac_xfan"
// Для спальня spalnya
//#define IN_SEL_MYSWING        "input_select.spalnac_myswing"
//#define IN_SEL_XFAN           "input_boolean.spalnac_xfan"
// Для кухни kuxnya
//#define IN_SEL_MYSWING        "input_select.kuxnac_myswing"
//#define IN_SEL_XFAN           "input_boolean.kuxnac_xfan"
// Для тёмной комнаты  temnaya
//#define IN_SEL_MYSWING        "input_select.temnac_myswing"
//#define IN_SEL_XFAN           "input_boolean.temnac_xfan"


#include "esphome.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "ir_Gree.h"
#include "IRrecv.h"
#include "IRutils.h"
#include "Arduino.h"

static char const *TAG = "! ! ! ! ! ! ! ! !";// для вывода в лог

////////////////////////////////////////////////////////////////////////
//  ас Объект передатчика
////////////////////////////////////////////////////////////////////////
//Вручную поменять номер пина как $transmitter_pin перед компиляцией для нового устройства.
// Спальня
//const uint16_t kIrLed = 14;
// Зал
//const uint16_t kIrLed = 14;
// Дальняя
const uint16_t kIrLed = 14;
// Тёмная
//const uint16_t kIrLed = 14;
// Кухня
//const uint16_t kIrLed = 14;

IRGreeAC ac(kIrLed); // Инициализируем передатчик


////////////////////////////////////////////////////////////////////////
//  irrecv Объект приёмника
////////////////////////////////////////////////////////////////////////
//Вручную поменять номер пина как $receiver_pin перед компиляцией для нового устройства.
// Спальня
//const uint16_t kRecvPin = 12;
// Зал
//const uint16_t kRecvPin = 12;
// Дальняя
const uint16_t kRecvPin = 5;
// Тёмная
//const uint16_t kRecvPin = 12;
// Кухня
//const uint16_t kRecvPin = 12;

uint8_t kTimeout = 50;
const uint16_t kCaptureBufferSize = 1000;
const uint16_t kMinUnknownSize = 12;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true); // Инициализируем приёмник
decode_results results;


////////////////////////////////////////////////////////////////////////
//  GreeAC Объект климата в НА
////////////////////////////////////////////////////////////////////////
// Component поменял на PollingComponent
class GreeAC : public PollingComponent, public Climate, public CustomAPIDevice
{
  private:
  sensor::Sensor* temp_sensor{nullptr};

  std::string myswing;
  bool xfan;

  public:
  GreeAC() : PollingComponent(500) {} // 500мс частота проверки приёмника
  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void set_temp_sensor(sensor::Sensor *sensor)
  {
    this->temp_sensor = sensor;
  }

////////////////////////////////////////////////////////////////////////
//  setup
////////////////////////////////////////////////////////////////////////
  void setup() override
  {
    //сервис в НА
    register_service(&GreeAC::set_data, "set_data", {"hvac", "temp", "fan", "swing", "light", "preset"});
    register_service(&GreeAC::set_mydata, "set_mydata", {"myswing", "xfan"});

    ac.begin();

    irrecv.setTolerance(kTolerance);
    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.enableIRIn();

    this->restore_state_(); // Восстановите состояние климатического устройства из НА

    // Инициализируем датчик температуры
    if (this->temp_sensor != nullptr){
      this->temp_sensor->add_on_raw_state_callback([this](float temp) { update_temp(temp); });
    }
  }

////////////////////////////////////////////////////////////////////////
//  update
////////////////////////////////////////////////////////////////////////
  void update() override
  {
    #ifdef IR_LISTEN
    if (irrecv.decode(&results)) //если прочитали с приёмника
    {
      dumpACInfo(&results); //Обработка приёма
      irrecv.resume(); // Слушаем снова
    }
    #endif
  }


////////////////////////////////////////////////////////////////////////
//  dumpACInfo обработка приёма
////////////////////////////////////////////////////////////////////////
  void dumpACInfo(decode_results *results) {

    std::string description = "";

    if (results->overflow) { // Если слишком большой пакет
      ESP_LOGD(TAG, "IR code is too big for buffer. This result shouldn't be trusted until this is resolved. Edit & increase kCaptureBufferSize.");
    }

    if (results->decode_type == GREE) {// Если опознали код с приёмника

      ESP_LOGD(TAG, "DumpACInfo befor");
      ESP_LOGD(TAG, "Status AC: %s", ac.toString().c_str());// читаем данные из ас(IRGreeAC) до записи
      ac.setRaw(results->state); // Записываем в ас(IRGreeAC) данные из команды
      #ifdef IR_DECODE
      getData(ac.toString().c_str()); // Разбираем строку на данные и заносим из в this->
      #endif
      this->publish_state();// Публикуем данные в НА
      ESP_LOGD(TAG, "DumpACInfo after");
      ESP_LOGD(TAG, "Status AC: %s", ac.toString().c_str());// читаем данные из ас(IRGreeAC) после записи

    } else {
      ESP_LOGD(TAG, "DumpACInfo decode NO GREE"); // Не поняли пакет
    }

  }


////////////////////////////////////////////////////////////////////////
//  update_temp Обновляем датчик температуры
////////////////////////////////////////////////////////////////////////

  void update_temp(float temp) {
    if (isnan(temp)){
      ESP_LOGD(TAG, "Temp sensor isnan");
      return;
    }
    this->current_temperature = temp;
    this->publish_state();
    ESP_LOGD(TAG, "Update Temp sensor");
  }


////////////////////////////////////////////////////////////////////////
//  Traits Что может АС Настройки
////////////////////////////////////////////////////////////////////////

  climate::ClimateTraits traits()
  {
    auto traits = climate::ClimateTraits();

    traits.set_supports_current_temperature(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_visual_min_temperature(10);
    traits.set_visual_max_temperature(35);
    traits.set_visual_target_temperature_step(1.0);
    traits.set_visual_current_temperature_step(0.1);


    std::set<ClimateMode> climateModes;
    climateModes.insert(CLIMATE_MODE_OFF);
    climateModes.insert(CLIMATE_MODE_HEAT_COOL);
    climateModes.insert(CLIMATE_MODE_COOL);
    climateModes.insert(CLIMATE_MODE_HEAT);
    climateModes.insert(CLIMATE_MODE_DRY);
    climateModes.insert(CLIMATE_MODE_FAN_ONLY);

    traits.set_supported_modes(climateModes);


    std::set<ClimateFanMode> climateFanModes;
    climateFanModes.insert(CLIMATE_FAN_AUTO);
    climateFanModes.insert(CLIMATE_FAN_LOW);
    climateFanModes.insert(CLIMATE_FAN_MEDIUM);
    climateFanModes.insert(CLIMATE_FAN_HIGH);
    climateFanModes.insert(CLIMATE_FAN_FOCUS); // Turbo

    traits.set_supported_fan_modes(climateFanModes);


    std::set<ClimateSwingMode> climateSwingModes;
    climateSwingModes.insert(CLIMATE_SWING_OFF); // Не двигаются
    climateSwingModes.insert(CLIMATE_SWING_VERTICAL); // Авто
    climateSwingModes.insert(CLIMATE_SWING_BOTH); // Жалюзи вниз
    climateSwingModes.insert(CLIMATE_SWING_HORIZONTAL); // Жалюзи вверх

    traits.set_supported_swing_modes(climateSwingModes);


    std::set<ClimatePreset> climatePreset;
    climatePreset.insert(CLIMATE_PRESET_ACTIVITY);
    climatePreset.insert(CLIMATE_PRESET_AWAY);
    climatePreset.insert(CLIMATE_PRESET_BOOST);
    climatePreset.insert(CLIMATE_PRESET_COMFORT);
    climatePreset.insert(CLIMATE_PRESET_ECO);
    climatePreset.insert(CLIMATE_PRESET_HOME);
    climatePreset.insert(CLIMATE_PRESET_NONE);
    climatePreset.insert(CLIMATE_PRESET_SLEEP);

    traits.set_supported_presets(climatePreset);

    return traits;
  }


////////////////////////////////////////////////////////////////////////
//  control Формирование команд для АС
////////////////////////////////////////////////////////////////////////
  void control(const ClimateCall &call) override
  {
    if (call.get_mode().has_value())
    {
////////////////////////////////////////////////////////////////////////
//  control climateMode
////////////////////////////////////////////////////////////////////////
      ClimateMode climateMode = *call.get_mode();
      switch (climateMode)
      {
        case CLIMATE_MODE_HEAT:
          ac.setMode(kGreeHeat);
          ac.on();
          break;
        case CLIMATE_MODE_COOL:
          ac.setMode(kGreeCool);
          ac.on();
          break;
        case CLIMATE_MODE_HEAT_COOL:
          ac.setMode(kGreeAuto);
          ac.on();
          break;
        case CLIMATE_MODE_DRY:
          ac.setMode(kGreeDry);
          ac.on();
          break;
        case CLIMATE_MODE_FAN_ONLY:
          ac.setMode(kGreeFan);
          ac.on();
          break;
        case CLIMATE_MODE_OFF:
          ac.off();
          ac.setLight(false);
          break;
      }

      this->mode = climateMode;
      this->publish_state();
    }

////////////////////////////////////////////////////////////////////////
//  control target_temperature Желаемая температура
////////////////////////////////////////////////////////////////////////
    if (call.get_target_temperature().has_value())
    {
      float temp = *call.get_target_temperature();
      ac.setTemp(temp);
      this->target_temperature = temp;
      this->publish_state();
    }

////////////////////////////////////////////////////////////////////////
//  control fanMode
////////////////////////////////////////////////////////////////////////
    if (call.get_fan_mode().has_value())
    {
      ClimateFanMode fanMode = *call.get_fan_mode();
      switch (fanMode)
      {
        case CLIMATE_FAN_AUTO:
          ac.setFan(kGreeFanAuto);
          ac.setTurbo(false);
          break;
        case CLIMATE_FAN_LOW:
          ac.setFan(kGreeFanMin);
          ac.setTurbo(false);
          break;
        case CLIMATE_FAN_MEDIUM:
          ac.setFan(kGreeFanMed);
          ac.setTurbo(false);
          break;
        case CLIMATE_FAN_HIGH:
          ac.setFan(kGreeFanMax);
          ac.setTurbo(false);
          break;
        case CLIMATE_FAN_FOCUS:
          ac.setFan(kGreeFanMax);
          ac.setTurbo(true);
          break;
      }

      this->fan_mode = fanMode;
      this->publish_state();
    }

////////////////////////////////////////////////////////////////////////
//  control swingMode
////////////////////////////////////////////////////////////////////////
// Поддерживается только 4 режима в НА но что внутри них решать ВАМ
    if (call.get_swing_mode().has_value())
    {
      ClimateSwingMode swingMode = *call.get_swing_mode();
      switch (swingMode)
      {
        case CLIMATE_SWING_OFF:
          ac.setSwingVertical(false, kGreeSwingLastPos);
          break;
        case CLIMATE_SWING_BOTH:
          ac.setSwingVertical(false, kGreeSwingDown); // kGreeSwingDown можно менять
          break;
        case CLIMATE_SWING_HORIZONTAL:
          ac.setSwingVertical(false, kGreeSwingUp);
          break;
        case CLIMATE_SWING_VERTICAL:
          ac.setSwingVertical(true, kGreeSwingAuto);
          break;
      }

      this->swing_mode = swingMode;
      this->publish_state();

    }

////////////////////////////////////////////////////////////////////////
//  control Preset
////////////////////////////////////////////////////////////////////////
    // Preset
    if (call.get_preset().has_value()) {
      ClimatePreset preset = *call.get_preset();
      if (preset == CLIMATE_PRESET_NONE) { // Нет

      } else if (preset == CLIMATE_PRESET_HOME) { // Дома // Охлажнение
        ac.on();
        ac.setMode(kGreeCool); // Включено охлаждение
        this->mode = CLIMATE_MODE_COOL;
        float temp = 27.0; // Температура
        ac.setTemp(temp);
        this->target_temperature = temp;
        ac.setFan(kGreeFanMin); // Минимальный вентилятор
        this->fan_mode = CLIMATE_FAN_LOW;
        ac.setTurbo(false);
        ac.setSwingVertical(false, kGreeSwingUp); // Жалюзи вверх
        this->swing_mode = CLIMATE_SWING_HORIZONTAL;
        ac.setLight(true); // Индикатор включен
      } else if (preset == CLIMATE_PRESET_AWAY) { // Не дома
        ac.off();
        this->mode = CLIMATE_MODE_OFF;
        ac.setLight(false);
      } else if (preset == CLIMATE_PRESET_BOOST) { // Турбо
        ac.on();
        ac.setMode(kGreeCool);
        this->mode = CLIMATE_MODE_COOL;
        ac.setFan(kGreeFanMax);
        ac.setTurbo(true);
        this->fan_mode = CLIMATE_FAN_FOCUS;
        ac.setSwingVertical(true, kGreeSwingAuto);
        this->swing_mode = CLIMATE_SWING_VERTICAL;
        ac.setLight(true);
      } else if (preset == CLIMATE_PRESET_COMFORT) { // Комфорт // Обогрев
        ac.on();
        ac.setMode(kGreeHeat); // Включен нагрев
        this->mode = CLIMATE_MODE_HEAT;
        float temp = 19.0; // Температура
        ac.setTemp(temp);
        this->target_temperature = temp;
        ac.setFan(kGreeFanMin);
        this->fan_mode = CLIMATE_FAN_LOW; // Минимальный вентилятор
        ac.setTurbo(false);
        ac.setSwingVertical(false, kGreeSwingDown); // Жалюзи вниз
        this->swing_mode = CLIMATE_SWING_BOTH;
        ac.setLight(true);// Индикатор включен
      } else if (preset == CLIMATE_PRESET_ECO) { // Эко

      } else if (preset == CLIMATE_PRESET_SLEEP) { // Сон

      } else if (preset == CLIMATE_PRESET_ACTIVITY) { // Активность

      }

      this->preset = preset;
      this->publish_state();
    }

////////////////////////////////////////////////////////////////////////
//  control send Оправка сигнала на АС
////////////////////////////////////////////////////////////////////////
    ac.send(); // Отправить текущее внутреннее состояние в виде ИК-сообщения.

    ESP_LOGD(TAG, "GreeAC control");
    ESP_LOGD(TAG, "  %s", ac.toString().c_str());
  }


////////////////////////////////////////////////////////////////////////
//  set_data приём данных из сервиса НА
////////////////////////////////////////////////////////////////////////
  void set_data(std::string hvac, float temp, std::string fan, std::string swing, bool light, std::string preset)
  {
    auto call = this->make_call();
////////////////////////////////////////////////////////////////////////
//  set_data hvac
////////////////////////////////////////////////////////////////////////
    if ( hvac == "off" ){
      call.set_mode(CLIMATE_MODE_OFF);
    } else if ( hvac == "heat_cool" ){
      call.set_mode(CLIMATE_MODE_HEAT_COOL);
    } else if ( hvac == "heat" ) {
      call.set_mode(CLIMATE_MODE_HEAT);
    } else if ( hvac == "cool" ){
      call.set_mode(CLIMATE_MODE_COOL);
    } else if ( hvac == "dry" ){
      call.set_mode(CLIMATE_MODE_DRY);
    } else if ( hvac == "fan_only" ) {
      call.set_mode(CLIMATE_MODE_FAN_ONLY);
    }

////////////////////////////////////////////////////////////////////////
//  set_data target_temperature
////////////////////////////////////////////////////////////////////////
    call.set_target_temperature(temp);

////////////////////////////////////////////////////////////////////////
//  set_data fan
////////////////////////////////////////////////////////////////////////
    if ( fan == "auto" ){
      call.set_fan_mode(CLIMATE_FAN_AUTO);
    } else if ( fan == "low" ){
      call.set_fan_mode(CLIMATE_FAN_LOW);
    } else if ( fan == "medium" ) {
      call.set_fan_mode(CLIMATE_FAN_MEDIUM);
    } else if ( fan == "high" ){
      call.set_fan_mode(CLIMATE_FAN_HIGH);
    } else if ( fan == "focus" ){
      call.set_fan_mode(CLIMATE_FAN_FOCUS);
    }

////////////////////////////////////////////////////////////////////////
//  set_data swing
////////////////////////////////////////////////////////////////////////
    if ( swing == "off" ){
      call.set_swing_mode(CLIMATE_SWING_OFF);
    } else if ( swing == "botn" ){
      call.set_swing_mode(CLIMATE_SWING_BOTH);
    } else if ( swing == "horizontal" ){
      call.set_swing_mode(CLIMATE_SWING_HORIZONTAL);
    } else if ( swing == "vertical" ){
      call.set_swing_mode(CLIMATE_SWING_VERTICAL);
    }

////////////////////////////////////////////////////////////////////////
//  set_data preset
////////////////////////////////////////////////////////////////////////
    if ( preset == "activity" ){
      call.set_preset(CLIMATE_PRESET_ACTIVITY);
    } else if ( preset == "away" ){
      call.set_preset(CLIMATE_PRESET_AWAY);
    } else if ( preset == "boost" ){
      call.set_preset(CLIMATE_PRESET_BOOST);
    } else if ( preset == "comfort" ){
      call.set_preset(CLIMATE_PRESET_COMFORT);
    } else if ( preset == "eco" ){
      call.set_preset(CLIMATE_PRESET_ECO);
    } else if ( preset == "home" ){
      call.set_preset(CLIMATE_PRESET_HOME);
    } else if ( preset == "none" ) {
      call.set_preset(CLIMATE_PRESET_NONE);
    } else if ( preset == "sleep" ){
      call.set_preset(CLIMATE_PRESET_SLEEP);
    }

////////////////////////////////////////////////////////////////////////
//  set_data perform отправка на выполнение
////////////////////////////////////////////////////////////////////////
    ac.setLight(light);
    call.perform();
  }

////////////////////////////////////////////////////////////////////////
//  set_mydata Приём моих новых данных из сервиса НА
////////////////////////////////////////////////////////////////////////
  void set_mydata(std::string myswing, bool xfan)
  {
    bool flag = false;

    // myswing
    if ( this->myswing != myswing ){ // если текущее состояние не совпадает с принятым
      if ( myswing == "middleup" ){
        ac.setSwingVertical(false, kGreeSwingMiddleUp);
      } else if( myswing == "middle" ){
        ac.setSwingVertical(false, kGreeSwingMiddle);
      } else if( myswing == "middledown" ){
        ac.setSwingVertical(false, kGreeSwingMiddleDown);
      }
      flag = true;
      this->swing_mode = CLIMATE_SWING_OFF;
      this->myswing = myswing; // запоминаем новое значение
    }

    // xfan
    if ( this->xfan != xfan ){ // если текущее состояние не совпадает с принятым
      if ( xfan ){
        ac.setXFan(true);
        flag = true;
        this->xfan = xfan; // запоминаем новое значение
      } else {
        ac.setXFan(false);
        flag = true;
        this->xfan = xfan; // запоминаем новое значение
      }
    }

    if ( flag ){
      ac.send(); // Отправить текущее внутреннее состояние в виде ИК-сообщения.
      ESP_LOGD(TAG, "set_mydata:  %s", ac.toString().c_str());
    }
  }


////////////////////////////////////////////////////////////////////////
//  getData Разбираем строку с пульта
////////////////////////////////////////////////////////////////////////
  void getData(std::string base_str1){

    std::string base_str2;
    std::string sep1 = ", ";
    std::string sep2 = ": ";
    size_t sep1_size = sep1.size();
    size_t sep2_size = sep2.size();
    std::string temp1;
    std::string temp2;
    std::string flag;
    std::string hvac;
    std::string temp;
    std::string fan;
    std::string swing;
    std::string light;
    std::string xfan;

    while (true)
    {
      temp1 = base_str1.substr(0, base_str1.find(sep1));
      if ( temp1.size() != 0 ){
        base_str2 = temp1;
        int i = 0;
        while (true){
          temp2 = base_str2.substr(0, base_str2.find(sep2));
          if ( temp2.size() != 0 ){
            if ( i % 2 == 0 ) {
              if ( temp2 == "Mode" ) flag = "Mode";
              else if ( temp2 == "Temp" ) flag = "Temp";
              else if ( temp2 == "Fan" ) flag = "Fan";
              else if ( temp2 == "Light" ) flag = "Light";
              else if ( temp2 == "Swing(V)" ) flag = "Swing(V)";
              else if ( temp2 == "XFan" ) flag = "XFan";
            } else {
              if ( flag == "Mode" ) {
                hvac = temp2;
                flag = " ";
              } else if ( flag == "Temp" ) {
                temp = temp2;
                flag = " ";
              } else if ( flag == "Fan" ) {
                fan = temp2;
                flag = " ";
              } else if ( flag == "Light" ) {
                light = temp2;
                flag = " ";
              } else if ( flag == "Swing(V)" ) {
                swing = temp2;
                flag = " ";
              } else if ( flag == "XFan" ) {
                xfan = temp2;
                flag = " ";
              }
            }
            i++;
          } if ( temp2.size() == base_str2.size() ){
            break;
          } else {
            base_str2 = base_str2.substr(temp2.size() + sep2_size);
          }
        }
      }
      if ( temp1.size() == base_str1.size() ) {
        break;
      } else {
        base_str1 = base_str1.substr(temp1.size() + sep1_size);
      }
    }

    hvac = getMyStr(hvac);// 0:Auto, 1:Cool, 2:Dry , 3:Fan , 4:Heat
    fan = getMyStr(fan);// 0:Auto, 1:Low, 2:Medium, 3:High
    std::string swing_old = swing;
    swing = getMyStr(swing);// 0:Last, 1:Auto, 2:Вверх, 3, 4, 5, 6.Вниз, 7, 9, 11
    temp.erase(std::remove(temp.begin(), temp.end(), 'C'), temp.end()); // 27
    this->target_temperature = std::stof(temp);
    this->preset = CLIMATE_PRESET_NONE;

    if (light == "On") ac.setLight(true);
    else ac.setLight(false);

    if (xfan == "On"){ // On, Off
      this->xfan = true;
      #ifdef IR_RETURN
      call_homeassistant_service ( "input_boolean.turn_on" , { // отправляем в НА
        { "entity_id" , IN_SEL_XFAN },
      });
      #endif
      ESP_LOGD(TAG, "xfan = On");
    } else {
      this->xfan = false;
      #ifdef IR_RETURN
      call_homeassistant_service ( "input_boolean.turn_off" , { // отправляем в НА
        { "entity_id" , IN_SEL_XFAN },
      });
      #endif
      ESP_LOGD(TAG, "xfan = Off");
    }

    if ( hvac == "Auto" ){
      this->mode = CLIMATE_MODE_HEAT_COOL;
      ESP_LOGD(TAG, "hvac = %s", hvac.c_str());
    } else if ( hvac == "Cool" ){
      this->mode = CLIMATE_MODE_COOL;
      ESP_LOGD(TAG, "hvac = %s", hvac.c_str());
    } else if ( hvac == "Dry" ){
      this->mode = CLIMATE_MODE_DRY;
      ESP_LOGD(TAG, "hvac = %s", hvac.c_str());
    } else if ( hvac == "Fan" ){
      this->mode = CLIMATE_MODE_FAN_ONLY;
      ESP_LOGD(TAG, "hvac = %s", hvac.c_str());
    } else if ( hvac == "Heat" ){
      this->mode = CLIMATE_MODE_HEAT;
      ESP_LOGD(TAG, "hvac = %s", hvac.c_str());
    }

    if ( fan == "Auto" ){
      this->fan_mode = CLIMATE_FAN_AUTO;
      ESP_LOGD(TAG, "fan = %s", fan.c_str());
    } else if ( fan == "Low" ){
      this->fan_mode = CLIMATE_FAN_LOW;
      ESP_LOGD(TAG, "fan = %s", fan.c_str());
    } else if ( fan == "Medium" ){
      this->fan_mode = CLIMATE_FAN_MEDIUM;
      ESP_LOGD(TAG, "fan = %s", fan.c_str());
    } else if ( fan == "High" ){
      this->fan_mode = CLIMATE_FAN_HIGH;
      ESP_LOGD(TAG, "fan = %s", fan.c_str());
    }

    if ( swing == "Last" ){
      this->swing_mode = CLIMATE_SWING_OFF;
      ESP_LOGD(TAG, "swing = %s", swing.c_str());
    } else if ( swing == "Auto" ){
      this->swing_mode = CLIMATE_SWING_VERTICAL;
      ESP_LOGD(TAG, "swing = %s", swing.c_str());
    } else if ( swing == "UNKNOWN" ){
      if ( (char)swing_old[0] == '2' ){
        this->swing_mode = CLIMATE_SWING_HORIZONTAL;
        ESP_LOGD(TAG, "swing = %s", swing_old.c_str());
      } else if ( (char)swing_old[0] == '3' ){
        this->swing_mode = CLIMATE_SWING_OFF;
        this->myswing = "middleup";
        #ifdef IR_RETURN
        call_homeassistant_service ( "input_select.select_option" , { // отправляем в НА
          { "entity_id" , IN_SEL_MYSWING },
          { "option" , "middleup" },
        });
        #endif
        ESP_LOGD(TAG, "swing = %s", swing_old.c_str());
      } else if ( (char)swing_old[0] == '4' ){
        this->swing_mode = CLIMATE_SWING_OFF;
        this->myswing = "middle";
        #ifdef IR_RETURN
        call_homeassistant_service ( "input_select.select_option" , { // отправляем в НА
          { "entity_id" , IN_SEL_MYSWING },
          { "option" , "middle" },
        });
        #endif
        ESP_LOGD(TAG, "swing = %s", swing_old.c_str());
      } else if ( (char)swing_old[0] == '5' ){
        this->swing_mode = CLIMATE_SWING_OFF;
        this->myswing = "middledown";
        #ifdef IR_RETURN
        call_homeassistant_service ( "input_select.select_option" , { // отправляем в НА
          { "entity_id" , IN_SEL_MYSWING },
          { "option" , "middledown" },
        });
        #endif
        ESP_LOGD(TAG, "swing = %s", swing_old.c_str());
      } else if ( (char)swing_old[0] == '6' ){
        this->swing_mode = CLIMATE_SWING_BOTH;
        ESP_LOGD(TAG, "swing = %s", swing_old.c_str());
      }
    }


    ESP_LOGD(TAG, "getData: hvac = %s, fan = %s, swing = %s, light = %s, temp = %s, xfan = %s, myswing = %s",
                   hvac.c_str(), fan.c_str(), swing.c_str(), light.c_str(), temp.c_str(), xfan.c_str(), myswing.c_str());

//  пример строки с пульта
//  Model: 2 (YBOFB), Power: On, Mode: 1 (Cool), Temp: 27C, Fan: 1 (Low), Turbo: Off, IFeel: Off,
//  WiFi: On, XFan: Off, Light: On, Sleep: Off, Swing(V) Mode: Manual, Swing(V): 0 (Last), Timer: Off,
//  Display Temp: 2 (Inside)
//
//      Model -> 2:YBOFB,
//      Power -> On, Off
// ---  Mode -> 0:Auto, 1:Cool, 2:Dry , 3:Fan , 4:Heat
// ---  Temp -> float
// ---  Fan -> 0:Auto, 1:Low, 2:Medium, 3:High
//      Turbo -> On, Off
// ---  IFeel -> On, Off
// ---  WiFi -> On, Off
//      XFan-> On, Off
// ---  Light -> On, Off
//      Sleep-> On, Off
//      Swing(V) Mode -> Manual
// ---  Swing(V) ->
//   ---    0: Last (kGreeSwingLastPos),
//   ---    1: Auto(kGreeSwingAuto), CLIMATE_SWING_VERTICAL
//   ---    2: Вверх(kGreeSwingUp), CLIMATE_SWING_HORIZONTAL
//          3: Верх-середина (kGreeSwingMiddleUp), middleup
//          4: Середина (kGreeSwingMiddle), middle
//          5: Середина-низ (kGreeSwingMiddleDown ), middledown
//   ---    6: Вниз (kGreeSwingDown), CLIMATE_SWING_BOTH
//          7: Авто верх (kGreeSwingUpAuto),
//          9: Авто середина (kGreeSwingMiddleAuto),
//          11: Авто низ (kGreeSwingDownAuto)
//      Timer -> Off, 00:30 кратный 30мин
//              kGreeTimerMax,
//      Display Temp ->
//          0: Off (TempOff)
//          1: Set (TempSet)
//          2: Inside (TempInside)
//          3: Outside (TempOutside)
//
// kGreeEcono ?????



  }

  std::string getMyStr(std::string text) {
      std::string r;
      const auto firstBracketIndex = text.find('(');
      const auto lastBracketIndex = text.rfind(')');
      for (size_t i = firstBracketIndex + 1; i < lastBracketIndex; i++)
          r = r + text[i];
      return r;
  }


};



////////////////////////////////////////////////////////////////////////
//  GreeLightSwitch  Выключатель дисплея
////////////////////////////////////////////////////////////////////////
class GreeLightSwitch : public PollingComponent, public Switch {
 public:
  GreeLightSwitch() : PollingComponent(5000) {}

  void write_state(bool state) override {
    ac.setLight(state);
    ac.send();

    publish_state(ac.getLight());
  }

  void update() override
  {
    publish_state(ac.getLight());
  }

};