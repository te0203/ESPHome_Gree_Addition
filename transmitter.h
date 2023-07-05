#include "esphome.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "ir_Gree.h"

static char const *TAG = "! ! ! ! ! ! !";

//const uint16_t kIrLed = (uint16_t)transmitter_pin->value();
//ВНИМАНИЕ !!!!
//Вручную поменять номер пина как $transmitter_pin перед компиляцией для нового устройства.
const uint16_t kIrLed = 14;
IRGreeAC ac(kIrLed);




class GreeAC : public Component, public Climate, public CustomAPIDevice
{
private:
  sensor::Sensor* temp_sensor{nullptr};


public:
  void set_temp_sensor(sensor::Sensor *sensor)
  {
    this->temp_sensor = sensor;
  }



  void setup() override
  {
    register_service(&GreeAC::set_data, "set_data", {"hvac", "temp", "fan", "swing", "light", "preset"});

    ac.begin();
    ac.on();
    ac.setMode(kGreeAuto);
    ac.setTemp(22);
    ac.setFan(kGreeFanAuto);
    ac.setSwingVertical(true, kGreeSwingAuto);

    if(this->temp_sensor != nullptr){
      this->temp_sensor->add_on_raw_state_callback([this](float temp) { update_temp(temp); });
    }
  }


  void update_temp(float temp) {
    if(isnan(temp)){
      return;
    }

    this->current_temperature = temp;
    this->publish_state();
  }

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

  void control(const ClimateCall &call) override
  {
    if (call.get_mode().has_value())
    {
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

    if (call.get_target_temperature().has_value())
    {
      float temp = *call.get_target_temperature();
      ac.setTemp(temp);

      this->target_temperature = temp;
      this->publish_state();
    }

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

    if (call.get_swing_mode().has_value())
    {
      ClimateSwingMode swingMode = *call.get_swing_mode();
      switch (swingMode)
      {
      case CLIMATE_SWING_OFF:
        ac.setSwingVertical(false, kGreeSwingLastPos);
        break;
      case CLIMATE_SWING_BOTH:
        ac.setSwingVertical(false, kGreeSwingDown);
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


    // Preset
    if (call.get_preset().has_value()) {
      ClimatePreset preset = *call.get_preset();
      //Нет
      if (preset == CLIMATE_PRESET_NONE) {

      }
      // Дома
      else if (preset == CLIMATE_PRESET_HOME) {
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


    ac.send();

    ESP_LOGD(TAG, "GreeAC control");
    ESP_LOGD(TAG, "  %s", ac.toString().c_str());
  }

  void set_data(std::string hvac, float temp, std::string fan, std::string swing, bool light, std::string preset)
  {
    auto call = this->make_call();

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

    call.set_target_temperature(temp);

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

    ac.setLight(light);

    call.perform();
  }


};

class GreeLightSwitch : public PollingComponent, public Switch {
 public:
  GreeLightSwitch() : PollingComponent(5000) {}

  void write_state(bool state) override {
    ac.setLight(state);
    ac.send();

    ESP_LOGD(TAG, "transmitter_pin = %d", (uint16_t)transmitter_pin->value());
    ESP_LOGD(TAG, "receiver_pin = %d", (uint16_t)receiver_pin->value());

    publish_state(ac.getLight());
  }

  void update() override
  {
    publish_state(ac.getLight());
  }
};