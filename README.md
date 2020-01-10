# RPI-ModbusClient
A Modbus Client/Master for PLC controller, base on QT5

<BR><BR>
TOYO controller in action
![FABRIK Test1](gif/Toyo0105.gif width="400") <br/>
IAI controller in action
![FABRIK Test2](gif/IAI0109.gif) <br/><br/>


# MBPoll command line read RTU:
---
  -#!/bin/bash
  -#check TC100 model name
  -#
  -mbpoll -b19200 -Pnone -t4:hex -r 0x10E0 -0 -c3 /dev/ttyS0 -1
  -mbpoll -b19200 -Pnone -t4:hex -r 0x10F0 -0 -c1 /dev/ttyS0 -1


### References
  - [MBPoll](https://github.com/epsilonrt/mbpoll)

