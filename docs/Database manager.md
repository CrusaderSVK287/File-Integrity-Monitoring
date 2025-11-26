Plugin na správu databáz

Argumenty pri init:
	- **"count"** : pocet databaz
	- **"db1"**	  : parametre pre prvu databazu. Tu bude
		- **"type"** : typ databazy, lokalna remote
		- **other...** : ostatne inicializacne Argumenty
	- **"dbX**" : parametre pre databazu 2,3,4.... podla countu
		
Argumenty pri run:
	- **"query"** : String, SQL query ktory potrebujem prehnat cez vsetky databazy
	
## Plugin
* Pre zobrazovanie databazi napr. vo VSC je potrebné mať nainštalovaný plugin, použítý bol tento :
Name: SQLite Viewer
Id: qwtel.sqlite-viewer
Description: SQLite Viewer for VS Code
Version: 0.10.6
Publisher: Florian Klampfer
VS Marketplace Link: https://marketplace.visualstudio.com/items?itemName=qwtel.sqlite-viewer

## Knižnica
> sqlite3 --version

> apt install sqlite3


## test_dp.py
Testovanie fungovania databazy 