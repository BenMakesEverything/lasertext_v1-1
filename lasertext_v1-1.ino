/*LaserText v1.1 by Ben Makes Everything
 * https://youtu.be/u9TpJ-_hBR8
 * This fixes many issues from the first version and provides much better spacing on the text
 */
#include <PGMWrap.h>
#include <SoftwareSerial.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
SoftwareSerial mySerial(8, 9); // RX, TX

//Timing stuff - NOP = 1 clock cycle
#define NOP __asm__ __volatile__ ("nop\n\t")
#define LetterSpace __asm__ __volatile__ ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t")

int mirrorPin = 3; //Mirror tab counter sensor
int revPin = 2;    //Revolution tab counter sensor
int laserPin = 6;  // Laser output

/*for reference:
7-6-5-4-3-2-TX-RX (Port D)
PORTD = B01000000; // sets digital pin 6 HIGH
*/

volatile int mirrorFlag = 0; //Counts how many mirrors have passed lower sensor
int mirrorFlagOld = 0; //stores previous value of mirror flag

int centerVal = 1800; //Left/right adjustment
int avgWidth = 36;

const byte numChars = 32; //MAX characters value
char myData[numChars]; // an array to store the received data
boolean newData = false; // for checking serial

const int rows = 12; //Define rows (should be same as # of mirrors)
const int columns = 9; //Defines number of line segments

int mCount = 0; //message length counter

//These are the Characters that can be displayed represented as 2D array
const int space[ rows ][ columns ] PROGMEM = { //Space bar
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0}
};

const int a[ rows ][ columns ] PROGMEM = { //A
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int b[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0}
};

const int c[ rows ][ columns ] PROGMEM = {
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0}
};

const int d[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 0, 0}
};

const int e[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1}
};

const int f[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0}
};

const int g[ rows ][ columns ] PROGMEM = {
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0}
};

const int h[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int i[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1}
};

const int j[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 0, 0},
{1, 1, 0, 0, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 0, 0, 0}
};

const int k[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 0},
{1, 1, 0, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 0, 0, 0},
{1, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 0, 0, 0},
{1, 1, 0, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int l[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1}
};

const int m[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int n[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 0, 0, 1, 1},
{1, 1, 1, 1, 0, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1}
};

const int o[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0}
};

const int p[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0}
};

const int q[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 1, 1, 1, 0, 1, 1}
};

const int r[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 0, 0, 0},
{1, 1, 0, 1, 1, 1, 0, 0},
{1, 1, 0, 0, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int s[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 0, 0}
};

const int t[ rows ][ columns ] PROGMEM = {
 {1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int u[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0}
};

const int v[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 0, 0, 1, 1, 0},
{0, 1, 1, 0, 0, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int w[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0}
};

const int x[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1}
};

const int y[ rows ][ columns ] PROGMEM = {
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int z[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 0, 0},
{0, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1}
};

const int zero[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 1, 1, 1, 1},
{1, 1, 0, 0, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 1, 0, 1, 1},
{1, 1, 0, 1, 0, 0, 1, 1},
{1, 1, 1, 1, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0}
};

const int one[ rows ][ columns ] PROGMEM = {
{0, 0, 0, 0, 1, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1}
};

const int two[ rows ][ columns ] PROGMEM = {
{0, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 0, 0},
{0, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1}
};

const int three[ rows ][ columns ] PROGMEM = {
{0, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 0, 0}
};

const int four[ rows ][ columns ] PROGMEM = {
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 1},
{0, 0, 0, 1, 1, 1, 1, 1},
{0, 0, 1, 1, 1, 0, 1, 1},
{0, 1, 1, 1, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 0, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1}
};

const int five[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 1, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 0, 0}
};

const int six[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 1, 1, 1, 0, 0},
{1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0}
};

const int seven[ rows ][ columns ] PROGMEM = {
{1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 0, 0},
{0, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0, 0}
};

const int eight[ rows ][ columns ] PROGMEM = {
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{0, 1, 1, 0, 0, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 0, 0, 1, 1, 0},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0}
};

const int nine[ rows ][ columns ] PROGMEM = {
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{1, 1, 1, 0, 0, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 0, 0},
{0, 1, 1, 1, 0, 0, 0, 0},
{1, 1, 1, 0, 0, 0, 0, 0}
};

const int per[ rows ][ columns ] PROGMEM = {  //period
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int comma[ rows ][ columns ] PROGMEM = {  //period
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 0, 0, 0, 0},
{0, 1, 1, 0, 0, 0, 0, 0},
{0, 1, 0, 0, 0, 0, 0, 0}
};

const int sc[ rows ][ columns ] PROGMEM = { //semicolon
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0}
};

const int exc[ rows ][ columns ] PROGMEM = { //exclamation
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int ques[ rows ][ columns ] PROGMEM = { //question mark
{0, 0, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 0, 0, 1, 1, 1},
{1, 1, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 0, 1, 1, 1},
{0, 0, 0, 0, 1, 1, 1, 0},
{0, 0, 0, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0},
{0, 0, 0, 1, 1, 0, 0, 0}
};

const int checker[ rows ][ columns ] PROGMEM = { //question mark
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1},
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1},
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1},
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1},
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1},
{1, 0, 1, 0, 1, 0, 1, 0},
{0, 1, 0, 1, 0, 1, 0, 1}
};

const int block[ rows ][ columns ] PROGMEM = { //question mark
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}
};
int pulses[ rows ][ columns ];

void setup() {
  Serial.begin(9600);
  mySerial.begin(115200);
  pinMode(mirrorPin, INPUT_PULLUP); //Mirror counter
  pinMode(revPin, INPUT_PULLUP);    //Full rotation counter
  pinMode(laserPin, OUTPUT);        //Laser configured as output
  attachInterrupt(digitalPinToInterrupt(mirrorPin), ISR_mirror, RISING);
  attachInterrupt(digitalPinToInterrupt(revPin), ISR_rev, RISING);
}

void loop() {
  receiveData();
  showNewData();
  determineMirror();
}

void ISR_mirror() {
  mirrorFlag++;
}

void ISR_rev() {
  mirrorFlag = 0;
}

//The first value after nopTimer has to be set manually for each mirror - software left/right alignment
void determineMirror() {
  if (mirrorFlag != mirrorFlagOld) {
    mirrorFlagOld = mirrorFlag;
    switch (mirrorFlag) {
      case 1:
        nopTimer((centerVal - strlen(myData)*avgWidth));
        encodePulses(0);
        break;
      case 2:
        nopTimer(49 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(1);
        break;
      case 3:
        nopTimer(85 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(2);
        break;
      case 4:
        nopTimer(57 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(3);
        break;
      case 5:
        nopTimer(-24 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(4);
        break;
      case 6:
        nopTimer(9 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(5);
        break;
      case 7:
        nopTimer(15 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(6);
        break;
      case 8:
        nopTimer(122 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(7);
        break;
      case 9:
        nopTimer(58 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(8);
        break;
      case 10:
        nopTimer(98 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(9);
        break;
      case 11:
        nopTimer(38 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(10);
        break;
      case 12:
        nopTimer(10 + centerVal - (strlen(myData)*avgWidth));
        encodePulses(11);
        break;
    }
  }
}

//This function turns the laser on and off based on what serial data comes in
void encodePulses(int mirrorNum) {
  noInterrupts(); //disables interrupts for better timing
  mCount = strlen(myData) - 1; //sets length to serial data length, -1 just removes junk end character from string
  while (mCount > -1) {
    int segment = 7;
    while (segment > -1) { //reads right to left because the motor spins CCW :(
      switch (myData[mCount]) {
        case ' ':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&space[mirrorNum][segment]);
          break;
        case '/':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&checker[mirrorNum][segment]);
          break;
        case '|':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&block[mirrorNum][segment]);
          break;
        case 'a':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&a[mirrorNum][segment]);
          break;
        case 'b':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&b[mirrorNum][segment]);
          break;
        case 'c':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&c[mirrorNum][segment]);
          break;
        case 'd':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&d[mirrorNum][segment]);
          break;
        case 'e':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&e[mirrorNum][segment]);
          break;
        case 'f':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&f[mirrorNum][segment]);
          break;
        case 'g':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&g[mirrorNum][segment]);
          break;
        case 'h':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&h[mirrorNum][segment]);
          break;
        case 'i':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&i[mirrorNum][segment]);
          break;
        case 'j':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&j[mirrorNum][segment]);
          break;
        case 'k':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&k[mirrorNum][segment]);
          break;
        case 'l':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&l[mirrorNum][segment]);
          break;
        case 'm':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&m[mirrorNum][segment]);
          break;
        case 'n':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&n[mirrorNum][segment]);
          break;
        case 'o':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&o[mirrorNum][segment]);
          break;
        case 'p':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&p[mirrorNum][segment]);
          break;
        case 'q':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&q[mirrorNum][segment]);
          break;
        case 'r':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&r[mirrorNum][segment]);
          break;
        case 's':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&s[mirrorNum][segment]);
          break;
        case 't':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&t[mirrorNum][segment]);
          break;
        case 'u':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&u[mirrorNum][segment]);
          break;
        case 'v':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&v[mirrorNum][segment]);
          break;
        case 'w':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&w[mirrorNum][segment]);
          break;
        case 'x':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&x[mirrorNum][segment]);
          break;
        case 'y':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&y[mirrorNum][segment]);
          break;
        case 'z':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&z[mirrorNum][segment]);
          break;
        case '0':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&zero[mirrorNum][segment]);
          break;
        case '1':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&one[mirrorNum][segment]);
          break;
        case '2':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&two[mirrorNum][segment]);
          break;
        case '3':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&three[mirrorNum][segment]);
          break;
        case '4':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&four[mirrorNum][segment]);
          break;
        case '5':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&five[mirrorNum][segment]);
          break;
        case '6':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&six[mirrorNum][segment]);
          break;
        case '7':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&seven[mirrorNum][segment]);
          break;
        case '8':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&eight[mirrorNum][segment]);
          break;
        case '9':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&nine[mirrorNum][segment]);
          break;
        case ':':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&sc[mirrorNum][segment]);
          break;
        case '.':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&per[mirrorNum][segment]);
          break;
       case ',':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&comma[mirrorNum][segment]);
          break;
        case '!':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&exc[mirrorNum][segment]);
          break;
        case '?':
          pulses[mirrorNum][segment] = pgm_read_dword_near(&ques[mirrorNum][segment]);
        default:
          pulses[mirrorNum][segment] = pgm_read_dword_near(&space[mirrorNum][segment]);
          break;
      }
      //laser on/off for specified intervals
      if (pulses[mirrorNum][segment] > 0) {
        PORTD = B01000000; // sets digital pin 6 HIGH
        LetterSpace;
        LetterSpace;
      }
      else {
        PORTD = B00000000; // sets digital pin 6 LOW
        LetterSpace;
        LetterSpace;
      }
      segment--;
    }
    PORTD = B00000000; // sets digital pin 6 LOW
    LetterSpace;
    LetterSpace;
    mCount--;
  }
  interrupts(); //reenable interrupts
}

void receiveData() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      myData[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      myData[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    newData = false;
  }
}

//This is used instead of delayMicroseconds - more accurate
void nopTimer(int howLong) {
  for (int t = 0; t < howLong; t++) {
    NOP;
  }
}
