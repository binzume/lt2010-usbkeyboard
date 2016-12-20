@echo off
SETLOCAL
set PATH=C:\Users\kawahira\apps\WinAVR\bin;%PATH%
set TARGET=atmega88
set COMPILE=avr-gcc -Wall -Os -Iusbdrv -I. -mmcu=%TARGET%  -DF_CPU=12000000
set SRC=usbdrv/usbdrv.c usbdrv/oddebug.c main.c
set OBJECTS=usbdrv.o usbdrvasm.o oddebug.o main.o

IF %1.==clean. goto clean
IF %1.==write. goto write

%COMPILE% --version
rem %COMPILE% -S %SRC%
%COMPILE% -c %SRC%
%COMPILE% -x assembler-with-cpp -c usbdrv/usbdrvasm.S
%COMPILE% -o a.out %OBJECTS%

avr-objcopy -O ihex -R .eeprom a.out main.hex

exit /B

:write
avrspx main.hex
exit /B

:clean
del *.o a.out
exit /B


