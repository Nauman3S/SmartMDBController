#!/usr/bin/env python

# SPI_mon.py
# 2016-09-21
# Public Domain

import time
import argparse
import pigpio

desc = """
This script monitors SPI activity and shows the data flowing on the
MISO and MOSI lines.

By default the main SPI device is monitored.  I.e. GPIO MISO 9,
MOSI 10, SCLK 11, CE 8 (channel 0).

The -a, --aux option may be used to change the default to the 
auxiliary SPI device.  I.e. GPIO MISO 19, MOSI 20, SCLK 21,
CE 18 (channel 0).

The defaults may be overridden by using the -k (SCLK), -i (MISO),
-o (MOSI), -s (CE) options.

The SPI channel may alternatively be set with the -c (CE) option.

The SPI mode (default 0) may be set with the -m option.

The output may be set to binary (default), decimal, or hex.

You will need to limit the SPI speed for this script to work.  With
the default pigpio sampling rate of 5 micros a SPI rate of 70 kbps
or less should be okay.
"""

p = argparse.ArgumentParser(
   formatter_class=argparse.RawDescriptionHelpFormatter, epilog=desc)

p.add_argument("-a", "--aux", help="select auxiliary SPI", action="store_true")

p.add_argument("-k", "--clk", help="set clock GPIO", type=int, default=None)
p.add_argument("-i", "--miso",  help="set MISO GPIO", type=int, default=None)
p.add_argument("-o", "--mosi",  help="set MOSI GPIO", type=int, default=None)

p.add_argument("-m", "--mode",  help="set SPI mode", type=int, default=0)

g1 = p.add_mutually_exclusive_group(required=False)
g1.add_argument("-s", "--ss", help="set slave select GPIO", type=int, default=None)
g1.add_argument("-c", "--ce", help="set SPI channel", type=int, default=None)

g2 = p.add_mutually_exclusive_group(required=False)
g2.add_argument("-b", "--binary",  help="show as binary",  action="store_true")
g2.add_argument("-d", "--decimal", help="show as decimal", action="store_true")
g2.add_argument("-x", "--hex",     help="show as hex"   ,  action="store_true")

args = p.parse_args()

CE=8
MISO=9
MOSI=10
SCLK=11

if args.aux:
   CE=18
   MISO=19
   MOSI=20
   SCLK=21

if args.clk is not None:
   SCLK = args.clk

if args.miso is not None:
   MISO = args.miso

if args.mosi is not None:
   MOSI = args.mosi

if args.ss is not None:
   CE = args.ss

if args.ce is not None:
   CE -= args.ce

print(CE, MISO, MOSI, SCLK)

if args.mode & 1:
   CPHA=1
else:
   CPHA=0

if args.mode & 2:
   CPOL=1
else:
   CPOL=0

def display_bits(g, bits):

   if args.decimal or args.hex:

      if args.decimal:
         fmt = "%4d"
      else:
         fmt = "%3x"

      s = g + ":"
      for i in range(0, len(bits), 8):
         s += fmt % int(bits[i:i+8],2)
      print(s)

   else:
      print("{}: {}".format(g, bits))

def cbf(gpio, level, tick):
   global miso_level, miso_bits, mosi_level, mosi_bits

   if gpio == CE:
      if level == 0: # Start of SPI transaction.
         miso_bits = ""
         mosi_bits = ""
      elif level == 1: # End of SPI transaction.
         display_bits("MOSI", mosi_bits)
         display_bits("MISO", miso_bits)

   elif gpio == MISO:
      miso_level = level

   elif gpio == MOSI:
      mosi_level = level

   elif gpio == SCLK:

      if CPHA:

         if level == CPOL:
            if miso_level == 0:
               miso_bits += "0"
            else:
               miso_bits += "1"

            if mosi_level == 0:
               mosi_bits += "0"
            else:
               mosi_bits += "1"
      else:

         if level != CPOL:
            if mosi_level == 0:
               mosi_bits += "0"
            else:
               mosi_bits += "1"

            if miso_level == 0:
               miso_bits += "0"
            else:
               miso_bits += "1"

pi = pigpio.pi()

if not pi.connected:
   exit()

miso_level = pi.read(MISO)
miso_bits = ""

mosi_level = pi.read(MOSI)
mosi_bits = ""

cb0 = pi.callback(MOSI, pigpio.EITHER_EDGE, cbf)
cb1 = pi.callback(MISO, pigpio.EITHER_EDGE, cbf)
cb2 = pi.callback(SCLK, pigpio.EITHER_EDGE, cbf)
cb3 = pi.callback(CE,   pigpio.EITHER_EDGE, cbf)

while True:
   try:
      time.sleep(60)
   except KeyboardInterrupt:
      break

print("\nTidying up")
   
cb0.cancel()
cb1.cancel()
cb2.cancel()
cb3.cancel()

pi.stop()

