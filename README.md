# File Integrity Monitoring
App that monitors files and sends notifications if it changes.

Main features:
- Portable on windows and linux (not tested on ios)
- Periodic scans of files
- Ability to send notifications via email
- Ability to specify parts of files that can change
- I would say quite conifgurable

### Disclaimer
This was at first a university project, therefore its not meant for enterprise use.
I mean do what you will I wont stop you or anything but I warned you. <br> <br>

Also, this was a group project, and while I did about 80% of the code, my colleagues 
made some parts of the project, namely the mailing manager and database manager, as 
well as consultations and similiar stuff. Therefore a thank you for allowing me
to put the repository on my private github is in place.

# How to compile
## Linux
### Dependencies:
- git
- cmake
- make
- python
- libraries: pybind11, cppyaml, openssl
- g++

### Steps to compile
Navigate to the repository
```mkdir build && cd build```
Generate makefile with cmake
```cmake ..```
Then just compile the app
```make -j```
Resulting binary will be in /output

## Windows
I am primarily a Linux programmer, so if someone finds simpler/better way to compile 
on windows, by all means edit the readme
### Dependecies:
- Visual Studio 2022 (Community edition is fine)
    * During install, select **Desktop development with C++** workload.
    * In Individual components select **CMake tools for Windows**.
   
- Python 3.12
    * Has to be for some reason 3.12, [link](https://www.python.org/downloads/windows/)
    * Dont forget to check the box that says add python to PATH
    * During install its needed to go edit/modify -> next -> check download debugging symbols and download debug binaries
	
- Git for windows
- CMake for windows

### Step 1 - vcpkg and dependencies

We need to get vcpkg and packeges needed for development. In powershell
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

.\vcpkg install yaml-cpp:x64-windows
.\vcpkg install openssl:x64-windows
.\vcpkg install pybind11:x64-windows
```

### Step 2 - set up of CMake and vcpkg

1. Open **x64 Native Tools Command Prompt for VS 2022**.
2. Navigate to the repository.
3. start CMake like this:
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DPython3_ROOT_DIR="C:/Users/<username>/AppData/Local/Programs/Python/Python312" -DCMAKE_TOOLCHAIN_FILE="C:/<path>/<to>/vcpkg/scripts/buildsystems/vcpkg.cmake"
```
Be aware of the paths, replace \<username> with your username and \<path>\<to> with acutal paths

### Step 3 - Build
Still in VS cmd
```
cmake --build build --config Release
```
Result should be at /output/Release.

### Troubleshooting
- If python complains about some missing libraries when starting .\monitor.exe: ignore or `set PYTHONHOME=C:\Users\lukas\AppData\Local\Programs\Python\Python312` pred spustením.
- If you get linker error: `LINK : fatal error LNK1104: cannot open file 'python312.lib'`, you probably compiled a Debug or Optimize release during step 2. It works only on Release for some reason

# Usage
### Configuration
All you need to start using the application is a config file.
```
For windows:
C:\Users\<username>\AppData\Local\Monitor\config.yaml
For linux:
/etc/monitor/config.yaml
```
The config file must include at least the smallest config (look at config_minimal.yaml) which is needed to run the program properly. 
If the config file is not present on path mentioned above, a ./config.yaml fallback should exist
For configuration explanation, look at **config.yaml**. Every config option is used there with explanation on what it does and whether its mandatory or optional

### Starting of the program:
```
# When you have config ready, you can just run the program like this
./monitor

# Runs the program in security mode, meaning the program executes
# the utility specified in the arguments and then exits.
# In this case, it sets a new password. The password is required for encryption.
./monitor --security pwd

# Encrypts the config
./monitor --security encrypt config

# Decrypts the config
./monitor --security decrypt config

# Decrypts logs (for logs to be encrypted, monitor.logs.secure must be set to true in the config)
./monitor --security decrypt logs
```
Logs are stored here:
```
Windows:
C:\Users\<username>\AppData\Local\Monitor\logs
Linux:
/home/<username>/.local/state/monitor/logs
```
When the **--security decrypt logs** utility is used, the decrypted logs are placed in the same location, but in the **/decrypted** subdirectory.
