<h2>Дополнение к работе Hagakurje</h2>
https://github.com/Hagakurje/ESPHome_Gree_AC
Спасибо ему за работу. 
Кое-что добавил и изменил. Может кому-то пригодится.

Добавил предустановки 8шт и ещё две настройки жалюзи.
Предустановки и режимы жалюзи можно менять как хочется 
только названия в HA пока не знаю как поменять.

В файле gree.h на строке 209 и 212 можно добавить свои настройки для двух новых режимов жалюзи.
Для своих настроек надо поменять в этой строку второй параметр

`ac.setSwingVertical(false, kGreeSwingDown);
`
Взять их можно <a href=”https://crankyoldgit.github.io/IRremoteESP8266/doxygen/html/ir__Gree_8h.html”>тут</a>

В строках с 226 по 307 можно добавить свои предустановки. 
Классы берём <a href=”https://crankyoldgit.github.io/IRremoteESP8266/doxygen/html/classIRGreeAC.html#a1b571dea8a5bf553554e45074f3a01c0”>тут</a> 
