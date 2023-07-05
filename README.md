<h2>Дополнение к работе Hagakurje</h2>
https://github.com/Hagakurje/ESPHome_Gree_AC
Спасибо ему за работу. 
Кое-что добавил и изменил. Может кому-то пригодится.

Долго искал в нете информацию по этой библиотеке её не так много. 
Решил выложить свои труды может кому-ео поможет.

У меня несколько кондиционеров Gree GWH07AAA-K3NNA2A/I
Управление реализовано через ESP12E с самыми дешовыми
[передатчиками](https://www.aliexpress.com/item/1005003672242023.html?spm=a2g0o.order_list.order_list_main.46.66361802bqKA42).
Единственное разместить передатчик пришлось прямо на кондиционер и от него запитать
так как дальность действия передатчика 1,3м. Столкнулся с тем что большие помехи на приёмнике.
Добавил согласование уровней с 5В от приёмника на 3,3В к ESP. Не помогло.
Помогла замена блока питания на 5В на более качественный с фильтрами.
Приём и расшифровку команд с пульта пока реализовал в файле receiver.h, но если их записывать 
в поля HA они уходят в бесконечный цикл. Пока не работает. Буду копать...

Добавил предустановки 8шт и ещё две настройки жалюзи.
Предустановки и режимы жалюзи можно менять как хочется 
только названия в HA пока не знаю как поменять.

В файле transmitter.h на строке 190 и 203 можно добавить свои настройки для двух новых режимов жалюзи.
Для своих настроек надо поменять в этой строке второй параметр

`ac.setSwingVertical(false, kGreeSwingDown);
`
Взять их можно [тут](https://crankyoldgit.github.io/IRremoteESP8266/doxygen/html/ir__Gree_8h.html)

В строках с 216 по 295 можно добавить свои предустановки. 
Классы берём [тут](https://crankyoldgit.github.io/IRremoteESP8266/doxygen/html/classIRGreeAC.html#a1b571dea8a5bf553554e45074f3a01c0) 

Единственное с чем пока не разобрался это как передать номер пина в файл transmitter.h 
из глобальной переменной из файла ESP12e.yaml
Если есть у кого идеи пишите. Буду благодарен.
`
const uint16_t kIrLed = (uint16_t)transmitter_pin->value();
`
 работает только внутри класса в глобальной области не работает.