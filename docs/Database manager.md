Plugin na správu databáz

Argumenty pri init:
	- **"count"** : pocet databaz
	- **"db1"**	  : parametre pre prvu databazu. Tu bude
		- **"type"** : typ databazy, lokalna remote
		- **other...** : ostatne inicializacne Argumenty
	- **"dbX**" : parametre pre databazu 2,3,4.... podla countu
		
Argumenty pri run:
	- **"query"** : String, SQL query ktory potrebujem prehnat cez vsetky databazy
	