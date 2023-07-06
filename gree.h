#define DECODE_AC   true

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
//ВНИМАНИЕ !!!!
//Вручную поменять номер пина как $transmitter_pin перед компиляцией для нового устройства.
const uint16_t kIrLed = 14;
IRGreeAC ac(kIrLed); // Инициализируем передатчик


////////////////////////////////////////////////////////////////////////
//  irrecv Объект приёмника
////////////////////////////////////////////////////////////////////////
//ВНИМАНИЕ !!!!
//Вручную поменять номер пина как $receiver_pin перед компиляцией для нового устройства.
const uint16_t kRecvPin = 5;
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

    ac.begin();

    irrecv.setTolerance(kTolerance);
    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.enableIRIn();

    this->restore_state_(); // Восстановите состояние климатического устройства из НА

    // Инициализируем датчик температуры
    if(this->temp_sensor != nullptr){
      this->temp_sensor->add_on_raw_state_callback([this](float temp) { update_temp(temp); });
    }
  }

////////////////////////////////////////////////////////////////////////
//  update
////////////////////////////////////////////////////////////////////////
  void update() override
  {
    if (irrecv.decode(&results)) //если прочитали с приёмника
    {
      dumpACInfo(&results); //Обработка приёма
      irrecv.resume(); // Слушаем снова
    }
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

      getData(ac.toString().c_str()); // Разбираем строку на данные и заносим из в this->

      this->publish_state();// Публикуем данные в НА

      ESP_LOGD(TAG, "DumpACInfo after");
      ESP_LOGD(TAG, "Status AC: %s", ac.toString().c_str());// читаем данные из ас(IRGreeAC) после записи

    }else{
      ESP_LOGD(TAG, "DumpACInfo decode NO GREE"); // Не поняли пакет
    }

  }


////////////////////////////////////////////////////////////////////////
//  update_temp Обновляем датчик температуры
////////////////////////////////////////////////////////////////////////

  void update_temp(float temp) {
    if(isnan(temp)){
      return;
    }

    this->current_temperature = temp;
    this->publish_state();
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
    climateFanModes.insert(CLIMATE_FAN_FOCUS);

    traits.set_supported_fan_modes(climateFanModes);


    std::set<ClimateSwingMode> climateSwingModes;
    climateSwingModes.insert(CLIMATE_SWING_OFF);
    climateSwingModes.insert(CLIMATE_SWING_VERTICAL);
    climateSwingModes.insert(CLIMATE_SWING_BOTH);
    climateSwingModes.insert(CLIMATE_SWING_HORIZONTAL);

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
      //Нет
      if (preset == CLIMATE_PRESET_NONE) {

      }
      // Дома
      else if (preset == CLIMATE_PRESET_HOME) { // Что внутри выбирать ВАМ
        // Включено охлаждение
        ac.on();
        ac.setMode(kGreeCool);
        this->mode = CLIMATE_MODE_COOL;
        // Температура
        float temp = 27.0;
        ac.setTemp(temp);
        this->target_temperature = temp;
        // Минимальный вентилятор
        ac.setFan(kGreeFanMin);
        this->fan_mode = CLIMATE_FAN_LOW;
        ac.setTurbo(false);
        // Жалюзи вверх
        ac.setSwingVertical(false, kGreeSwingUp);
        this->swing_mode = CLIMATE_SWING_HORIZONTAL;
        // Индикатор включен
        ac.setLight(true);
      }
      //Не дома
      else if (preset == CLIMATE_PRESET_AWAY) {
        ac.off();
        this->mode = CLIMATE_MODE_OFF;
        ac.setLight(false);
      }
      //Турбо
      else if (preset == CLIMATE_PRESET_BOOST) {
        ac.on();
        ac.setMode(kGreeCool);
        this->mode = CLIMATE_MODE_COOL;
        ac.setFan(kGreeFanMax);
        ac.setTurbo(true);
        this->fan_mode = CLIMATE_FAN_FOCUS;
        ac.setSwingVertical(true, kGreeSwingAuto);
        this->swing_mode = CLIMATE_SWING_VERTICAL;
        ac.setLight(true);
      }
      //Комфорт
      else if (preset == CLIMATE_PRESET_COMFORT) {
        // Включено охлаждение
        ac.on();
        ac.setMode(kGreeHeat);
        this->mode = CLIMATE_MODE_HEAT;
        // Температура
        float temp = 19.0;
        ac.setTemp(temp);
        this->target_temperature = temp;
        // Минимальный вентилятор
        ac.setFan(kGreeFanMin);
        this->fan_mode = CLIMATE_FAN_LOW;
        ac.setTurbo(false);
        // Жалюзи вниз
        ac.setSwingVertical(false, kGreeSwingDown);
        this->swing_mode = CLIMATE_SWING_BOTH;
        // Индикатор включен
        ac.setLight(true);
      }
      //Эко
      else if (preset == CLIMATE_PRESET_ECO) {

      }
      //Сон
      else if (preset == CLIMATE_PRESET_SLEEP) {

      }
      //Активность
      else if (preset == CLIMATE_PRESET_ACTIVITY) {

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
    if (hvac == "off")
    {
      call.set_mode(CLIMATE_MODE_OFF);
    }
    else if (hvac == "heat_cool")
    {
      call.set_mode(CLIMATE_MODE_HEAT_COOL);
    }
    else if (hvac == "heat")
    {
      call.set_mode(CLIMATE_MODE_HEAT);
    }
    else if (hvac == "cool")
    {
      call.set_mode(CLIMATE_MODE_COOL);
    }
    else if (hvac == "dry")
    {
      call.set_mode(CLIMATE_MODE_DRY);
    }
    else if (hvac == "fan_only")
    {
      call.set_mode(CLIMATE_MODE_FAN_ONLY);
    }

////////////////////////////////////////////////////////////////////////
//  set_data target_temperature
////////////////////////////////////////////////////////////////////////

    call.set_target_temperature(temp);


////////////////////////////////////////////////////////////////////////
//  set_data fan
////////////////////////////////////////////////////////////////////////
    if (fan == "auto")
    {
      call.set_fan_mode(CLIMATE_FAN_AUTO);
    }
    else if (fan == "low")
    {
      call.set_fan_mode(CLIMATE_FAN_LOW);
    }
    else if (fan == "medium")
    {
      call.set_fan_mode(CLIMATE_FAN_MEDIUM);
    }
    else if (fan == "high")
    {
      call.set_fan_mode(CLIMATE_FAN_HIGH);
    }
    else if (fan == "focus")
    {
      call.set_fan_mode(CLIMATE_FAN_FOCUS);
    }


////////////////////////////////////////////////////////////////////////
//  set_data swing
////////////////////////////////////////////////////////////////////////
    if (swing == "off")
    {
      call.set_swing_mode(CLIMATE_SWING_OFF);
    }
    else if (swing == "botn")
    {
      call.set_swing_mode(CLIMATE_SWING_BOTH);
    }
    else if (swing == "horizontal")
    {
      call.set_swing_mode(CLIMATE_SWING_HORIZONTAL);
    }
    else if (swing == "vertical")
    {
      call.set_swing_mode(CLIMATE_SWING_VERTICAL);
    }


////////////////////////////////////////////////////////////////////////
//  set_data preset
////////////////////////////////////////////////////////////////////////
    if (preset == "activity")
    {
        call.set_preset(CLIMATE_PRESET_ACTIVITY);
    }
    else if (preset == "away")
    {
        call.set_preset(CLIMATE_PRESET_AWAY);
    }
    else if (preset == "boost")
    {
        call.set_preset(CLIMATE_PRESET_BOOST);
    }
    else if (preset == "comfort")
    {
        call.set_preset(CLIMATE_PRESET_COMFORT);
    }
    else if (preset == "eco")
    {
        call.set_preset(CLIMATE_PRESET_ECO);
    }
    else if (preset == "home")
    {
        call.set_preset(CLIMATE_PRESET_HOME);
    }
    else if (preset == "none")
    {
        call.set_preset(CLIMATE_PRESET_NONE);
    }
    else if (preset == "sleep")
    {
        call.set_preset(CLIMATE_PRESET_SLEEP);
    }


////////////////////////////////////////////////////////////////////////
//  set_data perform отправка на выполнение
////////////////////////////////////////////////////////////////////////
    ac.setLight(light);

    call.perform();
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


    while (true)
    {
        temp1 = base_str1.substr(0, base_str1.find(sep1));
        if (temp1.size() != 0)
        {
            base_str2 = temp1;
            int i = 0;
            while (true)
            {
                temp2 = base_str2.substr(0, base_str2.find(sep2));
                if (temp2.size() != 0)
                {
                    if (i % 2 == 0) {
                        if (temp2 == "Mode") flag = "Mode";
                        else if(temp2 == "Temp") flag = "Temp";
                        else if (temp2 == "Fan") flag = "Fan";
                        else if (temp2 == "Light") flag = "Light";
                        else if (temp2 == "Swing(V)") flag = "Swing(V)";
                    }
                    else
                    {
                        if (flag == "Mode") {
                            hvac = temp2;
                            flag = " ";
                        }
                        else if (flag == "Temp") {
                            temp = temp2;
                            flag = " ";
                        }
                        else if (flag == "Fan") {
                            fan = temp2;
                            flag = " ";
                        }
                        else if (flag == "Light") {
                            light = temp2;
                            flag = " ";
                        }
                        else if (flag == "Swing(V)") {
                            swing = temp2;
                            flag = " ";
                        }
                    }

                    i++;
                }
                if (temp2.size() == base_str2.size())
                {
                    break;
                }
                else
                {
                    base_str2 = base_str2.substr(temp2.size() + sep2_size);
                }
            }
        }
        if (temp1.size() == base_str1.size()) {
            break;
        }
        else
        {
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


    if (light == "On") // On, Off
        ac.setLight(true);
    else
        ac.setLight(false);


    if(hvac == "Auto"){
        this->mode = CLIMATE_MODE_HEAT_COOL;
        ESP_LOGD(TAG, "hvac = Auto (%s)", hvac.c_str());
    }
    else if(hvac == "Cool"){
        this->mode = CLIMATE_MODE_COOL;
        ESP_LOGD(TAG, "hvac = Cool (%s)", hvac.c_str());
    }
    else if(hvac == "Dry"){
        this->mode = CLIMATE_MODE_DRY;
        ESP_LOGD(TAG, "hvac = Dry (%s)", hvac.c_str());
    }
    else if(hvac == "Fan"){
        this->mode = CLIMATE_MODE_FAN_ONLY;
        ESP_LOGD(TAG, "hvac = Fan (%s)", hvac.c_str());
    }
    else if(hvac == "Heat"){
        this->mode = CLIMATE_MODE_HEAT;
        ESP_LOGD(TAG, "hvac = Heat (%s)", hvac.c_str());
    }
    else{
        this->mode = CLIMATE_MODE_OFF;
        ESP_LOGD(TAG, "hvac = ? (%s)", hvac.c_str());
    }



    if(fan == "Auto"){
        this->fan_mode = CLIMATE_FAN_AUTO;
        ESP_LOGD(TAG, "fan = Auto (%s)", fan.c_str());
    }
    else if(fan == "Low"){
        this->fan_mode = CLIMATE_FAN_LOW;
        ESP_LOGD(TAG, "fan = Low (%s)", fan.c_str());
    }
    else if(fan == "Medium"){
        this->fan_mode = CLIMATE_FAN_MEDIUM;
        ESP_LOGD(TAG, "fan = Medium (%s)", fan.c_str());
    }
    else if(fan == "High"){
        this->fan_mode = CLIMATE_FAN_HIGH;
        ESP_LOGD(TAG, "fan = High (%s)", fan.c_str());
    }
    else{
        this->fan_mode = CLIMATE_FAN_AUTO;
        ESP_LOGD(TAG, "fan = ? (%s)", fan.c_str());
    }



    if(swing == "Last"){
        this->swing_mode = CLIMATE_SWING_OFF;
        ESP_LOGD(TAG, "swing = Last (%s)", swing.c_str());
    }
    else if(swing == "Auto"){
        this->swing_mode = CLIMATE_SWING_VERTICAL;
        ESP_LOGD(TAG, "swing = Auto (%s)", swing.c_str());
    }
    else if(swing == "UNKNOWN"){
        if((char)swing_old[0] == '2'){
            this->swing_mode = CLIMATE_SWING_HORIZONTAL;
            ESP_LOGD(TAG, "swing = 2 (%s)", swing_old.c_str());
        }
        else if((char)swing_old[0] == '6'){
            this->swing_mode = CLIMATE_SWING_BOTH;
            ESP_LOGD(TAG, "swing = 6 (%s)", swing_old.c_str());
        }
        else
        {
            this->swing_mode = CLIMATE_SWING_OFF;
            ESP_LOGD(TAG, "swing = NOT 2 OR 6 (%s)", swing_old.c_str());
        }
    }
    else{
        this->swing_mode = CLIMATE_SWING_OFF;
        ESP_LOGD(TAG, "swing = ? (%s)", swing.c_str());
    }


    ESP_LOGD(TAG, "getData: hvac = %s, fan = %s, swing = %s, light = %s, temp = %s",
                                    hvac.c_str(), fan.c_str(), swing.c_str(), light.c_str(), temp.c_str());

//  пример строки с пульта
//  Model: 2 (YBOFB), Power: On, Mode: 1 (Cool), Temp: 27C, Fan: 1 (Low), Turbo: Off, IFeel: Off,
//  WiFi: On, XFan: Off, Light: On, Sleep: Off, Swing(V) Mode: Manual, Swing(V): 0 (Last), Timer: Off,
//  Display Temp: 2 (Inside)


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