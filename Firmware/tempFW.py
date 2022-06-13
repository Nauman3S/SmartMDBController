# import time
# import smbus
# import time

# # Get I2C bus
# bus = smbus.SMBus(1)



# BAC= 10000#1BAC(g/dL) is 10000ppm
# PPM=1/BAC
# mgL=0.1#mg/dL

# def getAlcoholValue():
#     global bus,PPM,mgL
    
#     data = bus.read_i2c_block_data(0x50, 0x00, 2)

#     # Convert the data to 12-bits
#     raw_adc = (data[0] & 0x0F) * 256 + data[1]
#     concentration = (9.95 / 4096.0) * raw_adc + 0.05
    
#     print ("Alcohol Concentration : %.2f mg/l" %concentration)
#     concentration=concentration*mgL#conversion to mg/dL
#     concentration=concentration*0.001#conversion to g/dL
#     BACValue=concentration

#     print('ALCOHOL BAC=',BACValue)

#     return BACValue

# while 1:
#     time.sleep(1)
#     getAlcoholValue()


a=[1.1,2,3,4]
print(sum(a))