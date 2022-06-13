import os
import paho.mqtt.client as mqtt #import the client
import time
import random, string
import threading
import serial
from datetime import datetime
import json

dataToWrite=""

def setSerialData(sd):
    global serialData
    serialData=sd
def getSerialData():
    global serialData
    return serialData

def getDataToWrite():
    global dataToWrite
    return dataToWrite
def setDataToWrite(data):
    global dataToWrite
    dataToWrite=data

class SerialThread(threading.Thread):
    def __init__(self, target, *args):
        
        super().__init__(target=target, args=args)
        self.ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
        self.ser.flush()
    def writeDataLoop(self):
        global getDataToWrite,setDataToWrite
        if(getDataToWrite()!=""):
            self.ser.write(getDataToWrite.encode('utf-8'))
            setDataToWrite("")

    def run(self, *args):
        global setSerialData
        print( self._args )
        while 1:
            #print( self._args )
            line = self.ser.readline().decode('utf-8').rstrip()
            print(line)
            setSerialData(line)
            time.sleep(0.1)
            self.writeDataLoop()
        self._target(*self._args)


class MQTTThread(threading.Thread):
    def __init__(self, target, *args):
        super().__init__(target=target, args=args)
        self.client = mqtt.Client(self.getClientID())
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect("broker.hivemq.com", 1883, 60)
        self.client.subscribe("smartvend/data")
        self.client.loop_start()
    def getClientID(self):
        l = ['A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']
        s=""
        for i in range(0,16):
            s=s+l[random.randint(0,25)]
        return s
    def getTimestamp(self):
        now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        return(str(now))
    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code "+str(rc))
    def on_message(self, client, userdata, msg):
        global setDataToWrite, getSerialData
        print(msg.topic+" "+str(msg.payload))
        payloadV=str((msg.payload).decode('utf-8'))
        topicV=str((msg.topic).decode('utf-8'))
        if(topicV=="smartvend/data"):
            if(payloadV!=""):
                setDataToWrite(payloadV)

    def run(self, *args):
        global getSerialData, getActivePlayer

        print( self._args )
        while 1:
            mdbResponse=getSerialData()
            if(len(mdbResponse)>2):   
                self.client.publish('smartvend/mdbResponse',mdbResponse)
            time.sleep(self._args[1])
        self._target(*self._args)


def funcThreading(say=''):
  print("ThreadName: %s" % say)

mqttThread = MQTTThread(funcThreading, 'MQTTThread',0.5)
serialThread = SerialThread(funcThreading, 'SerialThread',0.7)
mqttThread.start()
serialThread.start()