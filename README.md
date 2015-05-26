# xbee-alarm-monitor

## Introduction

Monitoring program for parsing XBee packets.

## Description

The programe "xbeemonit" is a Linux utility that can be used to parse packets
from XBee "End Points" and can then take various actions (e.g., sending emails, 
logging syslog messages) depending on the digital output pins values of the 
remote XBee.

The program can be compiled as:

>gcc -g -o xbeemonit xbee.c util.c xbeemonit.c

The program runs in foreground and can be invoked as:

>./xbeemonit email@xyz.com sms_email@xyz.com
