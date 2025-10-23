# Softverova Bezpecnost zadanie 1 monitor

# Znenie zadania
Zostavte programové riešenie, vo vami vybranom programovacom jazyku, pre monitorovanie integrity súborov - File Integrity Monitoring (FIM), ktoré bude disponovať nasledujúcou funkcionalitou

- Pri spustení načíta parametre svojho spustenia z konfiguračného súboru
- Zapisuje informácie do logovacieho súboru
- V prípade incidentu upozorní kompetentnú osobu (alert)
- Informácie o baseline verzii monitorovaných súborov budú uložené v databáze

Navrhnuté riešenie bude nasadené na webovom serveri, kde bude monitorovať integritu súborov webovej stránky.
Na index.html bude publikovaná informácia o tom, aký je dnešný dátum a kto má dnes meniny. Táto informácia sa bude vždy o pol noci meniť.

# Kompilacia

Treba mať tieto package/veci:
- git (duh)
- cmake
- make
- python
- pybind11
- nejaký C++ kompilátor. Na Linux systémoch gcc, na windows by mal fungovať MS VS C++ toolchain ale lepśie by bolo na wsl asi.

Keď sa nachádzame v tomto priečinku
`mkdir build && cd build`
Následne vygenerujeme Makefile pomocou
`cmake ..`
A potom zbuildíme apku s
`make`
Výsledok bude v /output

# Testovanie modulov.
Urobil som nahrubo v main funkcii iba kus kódu ktory loadne a spustí nejaký modul nech sa dá už testovať.
Po kompilácii stačí zapnúťv /output binárku takto:
`./monitor <moduleName> <initKey1> <initValue1> ... - <runKey1> <runValue2> ...`
tá - tam musí byť. Ak dáte key ale nedáte za tým value *malo* by ten key ignorovať. Vzorový output:
```
output git:main $ ./monitor mymodule name lukas - count 6
Module Name: mymodule
Init Args: {'name': 'lukas'}
Run Args: {'count': '6'}
---------------------- INIT ----------------------
[ModuleManager] Successfully loaded module 'mymodule'.
--------------------- RUNNING --------------------
{
  status: ok
  message: Hello lukas, processed count=66
}
```
Aby sa modul inicializoval tak MUSÍ vrátiť v dictionary {"status":"OK"}, OK musi byt velkym (viď mymodule.py).
Pri run je to prakticky jedno tam sa žiadny return value nekontroluje, to sa už dohodneme keď tak.
Ak python hodí nejaký exception mala by ho tá apka zachytiť takže kľudne hádžte keľo chcete, príklad kde som upravil ten script aby nemal žiadny config:
```
output git:main $ ./monitor mymodule                                          ✖ ✹
Module Name: mymodule
Init Args: {}
Run Args: {}
---------------------- INIT ----------------------
[ModuleManager] Successfully loaded module 'mymodule'.
--------------------- RUNNING --------------------
{
  status: error
  message: TypeError: unsupported operand type(s) for *: 'NoneType' and 'int'

At:
  /home/lukas/Repositories/softverova-bezpecnost-zadanie-1-monitor/output/mymodule.py(17): run

}
```

PS: ak budete pozerať C++ kód tak často tam nadávam v komentároch na kompilátor, nemažte mi to pls, je to pre mňa terapia :D
