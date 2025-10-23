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
- nejaký C++ kompilátor. Na Linux systémoch gcc, na windows by mal fungovať MinGW, prípade MS VS C++ toolchain.

Keď sa nachádzame v tomto priečinku
`mkdir build && cd build`
Následne vygenerujeme Makefile pomocou
`cmake ..`
A potom zbuildíme apku s
`make`
Výsledok bude v /output
