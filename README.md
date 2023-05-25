# LaserText v1.1
# This is the (improved) code for my portable laser projector - as seen here: https://youtu.be/u9TpJ-_hBR8
# Improved version - this fixes many issues from the version that was shown in the video (1.0)
# Changes:
# Switch statements instead of if/else because they are slightly faster
# Using clock cycle timer (NOP) instead of delayMicroseconds() - faster and more accurate
# Binary "pixel" arrays instead of positive and negative numbers for segments
# Increased max character count from 20 to 30 - due tighter letter spacing
# Moved mirror checking code into a function
#
# General usage notes:
# This was designed to run on an Arduino nano - I don't know if it will work on anything else or not
# You will have to set the left right alignment manually for each mirror - this is the first number as seen here: "85 + centerVal" this accounts for imperfections in the mirror array
# This will work with either serial data over USB or with an Arduino Bluetooth module. The BT module has to be disconnected to use USB though.
# Use the app "Serial Bluetooth Terminal" for BT communication
# Because of the switch statement structure, this now only supports lower case letters as input
#
# Enjoy!
