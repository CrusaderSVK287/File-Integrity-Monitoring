Plugin na posielanie email alertov

Argumenty pri init:
	- **"mail_list"** : Array stringov, emailové adresy na ktoré sa budú posielať alerty
	- **"max_notifications"** : unsgined Integer, počet emailov ktoré sa pošlú na daný problém (id súboru), kým sa prestanú emaily posielať až do vyriešenia problému.
	- **"reset_timeout"**: unsgined Integer, počet sekúnd, ktoré ak ubehnú od posledného incidentu, nastane reset counteru notifikácii na 0. (súvisí s max_notifications)
	
Argumenty pri run:
	- **"message"** : String, chybová správa o incidente (nie celý pripravený mail, trocha formatovania asi by bolo fajn, idk to je na vas)
	- **"id"** : pravdepodobne unsigned 32b integer, číslo ktoré bude zodpovedať id incidentu. Reálne to bude asi hash code súboru takže každý súbor bude mať jak keby iné id, aby sa dali posielať tie maily nezávisle.
	- **"type"** : pravdepodobne unsigned int, číslo ktoré bude reprezentovať typ emailu/incidentu. Zatiaľ by som povedal že
		- 0 : Security alert (nesedí nejaký checksum configu abo zlé heslo sa dalo 3x abo také dačo)
		- 1 : Incident happened (file sa zmenil, posielať len ak nebolo pre id dosiahnute max_notifications)
		- 2 : Incident resolved (file je znovu ok tak ako bol, resetovať counter emailov pre toto id na 0)
		