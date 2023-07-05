#define DECODE_AC   true

#include "esphome.h"
#include "IRremoteESP8266.h"
#include "IRrecv.h"
#include "IRutils.h"
#include "Arduino.h"




//ВНИМАНИЕ !!!!
//Вручную поменять номер пина как $receiver_pin перед компиляцией для нового устройства.
const uint16_t kRecvPin = 5;
uint8_t kTimeout = 50;
const uint16_t kCaptureBufferSize = 1024;
//const uint8_t kTolerance = 25;
const uint16_t kMinUnknownSize = 12;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;


class MyReceiver : public PollingComponent, public CustomAPIDevice, public Sensor {

 private:
    std::string temp;
    std::string hvac; // off, heat_cool, 1:cool, heat, fan_only, dry
    std::string fan; // 1:low, medium, high, focus
    std::string swing; // off, both, vertical, horizontal, 2:UNKNOWN, 0:last 1:Auto
    std::string swing_mode; //Manual, Auto
    std::string light; // On
    std::string preset; // none, home, away, boost, comfort, eco, sleep, activity
    std::string power; // On
    std::string turbo; // Off
    std::string ifeel; // Off
    std::string xfan; // Off
    std::string sleep; // Off
    std::string timer; // Off
    std::string display_temp; // 0 (Off)
    std::string wifi; // Off, On

 public:

  MyReceiver() : PollingComponent(500) {}
  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void setup() override
  {
    irrecv.setTolerance(kTolerance);
    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.enableIRIn();
  }

  void update() override
  {
    if (irrecv.decode(&results))
    {
      dumpACInfo(&results);
      //sendData();
      irrecv.resume();
    }
  }

  void dumpACInfo(decode_results *results) {
    String description = "";

    if (results->decode_type == GREE) {
      ac.setRaw(results->state);
      description = ac.toString();
    }

    if (description != "") {
        ESP_LOGD(TAG, "MyReceiver dumpACInfo");
        ESP_LOGD(TAG, "Mesg Input: %s", description.c_str());
        parseDescription(description.c_str());
    }

  }


  void parseDescription(std::string description){

    std::string base_str1 = description;
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
                    break;
                else
                    base_str2 = base_str2.substr(temp2.size() + sep2_size);
            }
        }
        if (temp1.size() == base_str1.size())
            break;
        else
            base_str1 = base_str1.substr(temp1.size() + sep1_size);
    }

    this->hvac = getPropis(getMyStr(hvac));
    this->fan = getPropis(getMyStr(fan));
    this->swing = getPropis(getMyStr(swing));
    this->light = getPropis(light);
    temp.erase(std::remove(temp.begin(), temp.end(), 'C'), temp.end());
    this->temp = temp;

    ESP_LOGD(TAG, "MyReceiver parseDescription");
    ESP_LOGD(TAG, "Mesg Output: hvac = %s, temp = %s, fan = %s, swing = %s, light = %s, preset = none", this->hvac.c_str(), this->temp.c_str(), this->fan.c_str(), this->swing.c_str(), this->light.c_str());

  }

  std::string getMyStr(std::string text) {
      std::string r;
      const auto firstBracketIndex = text.find('(');
      const auto lastBracketIndex = text.rfind(')');
      for (size_t i = firstBracketIndex + 1; i < lastBracketIndex; i++)
          r = r + text[i];
      return r;
  }

  std::string getPropis(std::string s) {
      for (int i = 0; i < s.length(); i++)
          s[i] = (char)tolower(s[i]);
      return s;
  }

  void sendData(){
      std::string name;

      //name = (std::string)hostname_service->value() + "_set_data";
      name = "esp_12e_witty_12_set_data";

      call_homeassistant_service ( name, {
          { "hvac" , this->hvac },
          { "temp" , this->temp },
          { "fan" , this->fan },
          { "swing" , this->swing },
          { "light" , this->light },
          { "preset" , "none" },
      });




      ESP_LOGD(TAG, "MyReceiver sendData");
      ESP_LOGD(TAG, "Send: hvac = %s, temp = %s, fan = %s, swing = %s, light = %s, preset = none", this->hvac.c_str(), this->temp.c_str(), this->fan.c_str(), this->swing.c_str(), this->light.c_str());
  }

};