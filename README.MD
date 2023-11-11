# AquaSenseLinux
*Note This is a version developed for Artila Matrix752 and/or Matrix518 which are Linux ARM based. For Matrix-310 (Arduino-based) version, please visit the [AquaSense](https://github.com/yurukute/AquaSense/) repository.*

An observing system using Vernier sensors to observe the quality of water in various locations, such as rivers, lakes, and reservoirs. The system will provide real-time data collection, analysis, and reporting capabilities to support environmental monitoring and decision-making processes.
## Feature
- Real-time data collection from water quality sensors, including pH, temperature, dissolved oxygen and decomposition products.
- Measure and manage the changes of water level.
- Integration with external systems or databases for data exchange.
## Getting started
### Setup serial port
Run `setuart` built-in command:
```sh
setuart -p 1 -t 485 -b 9600
```
*Note: The serial port’s mode and associated communication parameters will go back to factory default after system reboot.*
### Cross compile and run
1. Install the [toolchain](https://www.artila.com/download/i.MX537/toolchain/) under `/` directory to compile. 
   For Windows, download the toolchain from [CodeSourcery](http://www.codesourcery.com/sgpp/lite/arm/portal/package4547/public/arm-none-linux-gnueabi/arm-2009q1-203-arm-none-linux-gnueabi.exe)
2. Send the compiled binary file to Matrix518 via `ftp`.
3. Using `chmod +x` to make the file executable.
4. Add the program to `/etc/rc.local` to execute it at startup.