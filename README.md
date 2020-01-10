# RPI-ModbusClient
A Modbus Client/Master for PLC controller, base on QT5
<img src="gif/QtModbusClientMaster0110.png" width="800"/>
<br/>

TOYO controller![Toyo TC100 ]  &nbsp;&nbsp;&nbsp;  IAI controller![IAI MC0162]<br/>
<img src="gif/Toyo0105.gif" width="300"/> &nbsp; <img src="gif/IAI0109.gif" width="300"/> <br/><br/>


# MBPoll command line read RTU:
---
    #!/bin/bash
    #check TC100 model name
    #
    mbpoll -b19200 -Pnone -t4:hex -r 0x10E0 -0 -c3 /dev/ttyS0 -1
    mbpoll -b19200 -Pnone -t4:hex -r 0x10F0 -0 -c1 /dev/ttyS0 -1


### References
  - [RPI SerialPort Enable](https://www.raspberrypi.org/documentation/configuration/uart.md)
  - [MBPoll commandline utility](https://github.com/epsilonrt/mbpoll)

