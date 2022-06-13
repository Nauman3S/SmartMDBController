import sys
import time
from gas_detection import GasDetection

BAC = 10000  # 1BAC(g/dL) is 10000ppm
PPM = 1/BAC

print('Please wait for 10 seconds while the Sensor Calibrates itself.')
detection = GasDetection()

time.sleep(10)
loopCount = 0
valsArray = []


def getAlcoholValue():
    global PPM, BAC, detection

    print('Calibrating ...')

    try:

        ppm = detection.percentage()

        ppmValue = ppm[detection.ALCOHOL_GAS]
        BACValue = ppmValue*PPM
        values = [ppmValue, BACValue]

        return values

    except Exception as e:
        print('\nAborted!')
        return [0,0]


while True:

    v = getAlcoholValue()
    print('ALCOHOL: {} ppm'.format(v[0]))
    print('ALCOHOL: {} BAC'.format(v[1]))
    valsArray.append(v[1])
    loopCount = loopCount+1
    if(loopCount > 10):
        avg = sum(valsArray)/10.0
        print('ALCOHOL 10 Seconds Average: {} BAC'.format(avg))
        loopCount = 0

    time.sleep(1)
