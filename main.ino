/*
   main.ino
   Concepted and written by Nicolas Huber
   Version: 20221123 / exhibition model
   Updated by 2024-03-31 as of the Schweizer Jugend forscht 2024 initiative
*/

////////// PARAMETERS //////////

// TONE (to disable tone set value to 0, other 1)

int index_tone = 0;

// WARNINGS (to disable warnings set value to 0, other 1)

int left = 1;
int right = 0;

// SENSITIVITY (the smaller the following parameter, the more sensitive the system reacts to changes in data)

int sensitivity = 1.25;

////////// BASICS //////////

// ------ LIBRARIES ------

#include <UTFTGLUE.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <HX711-multi.h>

// ------ COLORS ------

#define BLACK   0x0000
#define GREY    64, 64, 64
#define BLUE    64, 52, 235
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ------ DISPLAY ------

UTFTGLUE myGLCD(0, A2, A1, A3, A4, A0);

// ------ LOAD CELLS ------

#define CLK A8
#define DOUT1 A9
#define DOUT2 A10

byte DOUTS[2] = {DOUT1, DOUT2};

#define CHANNEL_COUNT sizeof(DOUTS)/sizeof(byte)

long int results[CHANNEL_COUNT];

HX711MULTI scales(CHANNEL_COUNT, DOUTS, CLK);

// ------ BUZZER ------

#define buzzer 37

////////// SETUP FUNCTION //////////

void setup() {

    tone(52, 220, 500);

    Serial.begin(9600);

    pinMode(buzzer, OUTPUT);

    myGLCD.InitLCD();
    myGLCD.setFont(SmallFont);

    setupInterface();

}

////////// MAIN FUNCTION //////////

void loop() {

// ------ VARIABLES ------

    int milliseconds;
    int runtime_sec;
    int data_left;
    int data_right;
    int average_left;
    int average_right;
    int danger_index_left;
    int danger_index_right;

// ------ RUNTIME ------

    milliseconds = millis();
    runtime_sec = calculateTime(milliseconds);
    displayTime(runtime_sec);

// ------  INPUT DATA ------

    data_left = getDataLeft();
    data_right = getDataRight();
    displayData(data_left, data_right);

// ------ FLOATING AVERAGE ------

    average_left = calculateAverageLeft(data_left);
    average_right = calculateAverageRight(data_right);
    displayAverage(average_left, average_right);

// ------ GRAPHING ------

    graphingLeft(runtime_sec, data_left, average_left);
    graphingRight(runtime_sec, data_right, average_right);

// ------ WARNINGS ------

    danger_index_left = getDangerIndexLeft(average_left, data_left, left);
    danger_index_right = getDangerIndexRight(average_right, data_right, right);
    visualWarning(danger_index_left, danger_index_right);
    audioWarning(danger_index_left, danger_index_right);

// ------ RESET DISPLAY ------

    resetGraphs(runtime_sec);

// ------ SERIAL PRINT ------

    serialPrint(milliseconds, data_left, data_right, average_left, average_right, danger_index_left, danger_index_right);

}

////////// FUNCTIONAL METHODS //////////

void setupInterface(){

    // ------ INTERFACING ------

    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(0, 0, 480, 320);

    // Kopfzeile

    myGLCD.setColor(64, 52, 235);
    myGLCD.fillRect(0, 0, 479, 26);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(64, 52, 235);
    myGLCD.print("FLIEGEN AM LIMIT - Klapperwarnsystem", CENTER, 9);

    // Fusszeile

    myGLCD.setColor(64, 64, 64);
    myGLCD.fillRect(0, 306, 479, 319);
    myGLCD.setBackColor(64, 64, 64);
    myGLCD.setColor(255, 255, 0);
    myGLCD.print("www.nicolas-paragliding.com", CENTER, 307);

    // Datenbox linke Schirmseite

    myGLCD.setColor(196, 192, 192);
    myGLCD.fillRect(47, 270, 120, 300);
    myGLCD.setColor(64, 74, 4);
    myGLCD.setBackColor(196, 192, 192);
    myGLCD.print("rec. ", 50, 273);
    myGLCD.print("n", 104, 273);
    myGLCD.print("av. ", 50, 286);
    myGLCD.print("n", 104, 286);


    // Datenbox rechte Schirmseite

    myGLCD.setColor(196, 192, 192);
    myGLCD.fillRect(360, 270, 432, 300);
    myGLCD.setColor(64, 74, 4);
    myGLCD.setBackColor(196, 192, 192);
    myGLCD.print("rec. ", 363, 273);
    myGLCD.print("kg", 416, 273);
    myGLCD.print("av. ", 363, 286);
    myGLCD.print("kg", 416, 286);

    // Datenbox Laufzeit

    myGLCD.setColor(196, 192, 192);
    myGLCD.drawRect(127, 270, 353, 300);

    // Graphing-Feld linke Schirmseite

    myGLCD.setColor(102, 102, 102);
    myGLCD.fillRect(50, 35, 428, 143);
    myGLCD.setColor(WHITE);
    myGLCD.setBackColor(102, 102, 102);
    myGLCD.print("Beschleunigerleine", 177, 40);

    myGLCD.setColor(WHITE);
    myGLCD.drawLine(70, 57, 70, 128);
    myGLCD.drawLine(70, 128, 408, 128);
    myGLCD.drawLine(408, 57, 408, 128);

    myGLCD.drawLine(70, 57, 75, 57);
    myGLCD.drawLine(403, 57, 408, 57);
    myGLCD.drawLine(70, 76, 75, 76);
    myGLCD.drawLine(403, 76, 408, 76);
    myGLCD.drawLine(70, 95, 75, 95);
    myGLCD.drawLine(403, 95, 408, 95);
    myGLCD.drawLine(70, 114, 75, 114);
    myGLCD.drawLine(403, 114, 408, 114);

    myGLCD.print("[n]", 68, 40);
    myGLCD.print("[t]", 235, 132);

    myGLCD.print(" +", 53, 53);
    myGLCD.print("", 53, 89);
    myGLCD.print("0", 61, 124);

    // Graphing-Feld rechte Schirmseite

    myGLCD.setColor(102, 102, 102);
    myGLCD.fillRect(50, 153, 428, 261);
    myGLCD.setColor(WHITE);
    myGLCD.setBackColor(102, 102, 102);
    myGLCD.print("Vord. Leinenebene", 177, 158);

    myGLCD.setColor(WHITE);
    myGLCD.drawLine(70, 175, 70, 246);
    myGLCD.drawLine(70, 246, 408, 246);
    myGLCD.drawLine(408, 175, 408, 246);

    myGLCD.drawLine(70, 175, 75, 175);
    myGLCD.drawLine(403, 175, 408, 175);
    myGLCD.drawLine(70, 196, 75, 196);
    myGLCD.drawLine(403, 196, 408, 196);
    myGLCD.drawLine(70, 215, 75, 215);
    myGLCD.drawLine(403, 215, 408, 215);
    myGLCD.drawLine(70, 234, 75, 234);
    myGLCD.drawLine(403, 234, 408, 234);

    myGLCD.print("[kg]", 68, 158);
    myGLCD.print("[t]", 235, 250);

    myGLCD.print(" +", 53, 171);
    myGLCD.print("", 53, 209);
    myGLCD.print("0", 61, 243);
}

int calculateTime(int milliseconds){

    int runtime_sec = (milliseconds / 1000) % 60;
    return runtime_sec;

}

void displayTime(int runtime_sec){

    myGLCD.setBackColor(BLACK);
    myGLCD.setColor(255, 255, 255);
    myGLCD.printNumI(runtime_sec, 240, 281);
    myGLCD.print("s", 260, 281);

}

int getDataLeft(){

    scales.read(results);

    int input_left = results[0];
    int data_left = abs(input_left * 0.001);

    if(data_left > 60){
        int data_left = 60;
        return data_left;    }

    return data_left;

}

int getDataRight(){

    scales.read(results);

    int input_right = results[1];
    int data_right = abs(input_right * 0.001);

    if(data_right > 60){
        int data_right = 60;
        return data_right;
    }

    return data_right;

}

void displayData(int data_left, int data_right){

    myGLCD.setBackColor(196, 192, 192);
    myGLCD.setColor(197, 252, 201);
    myGLCD.printNumI(data_left, 85, 273);
    myGLCD.printNumI(data_right, 398, 273);

}

int calculateAverageLeft(int data_left){

    /*
     *The calculation of the floating average is based on: https://forum.arduino.cc/?_gl=1*y3keuz*_ga*MjI2NTc4MzE5LjE2NTg3MzQyODY.*_ga_NEXN8H46L5*MTY1ODczNDI4NS4xLjEuMTY1ODczNDUyNi42MA..
     */

    float value_left = data_left;
    const byte nvalues_left = 250;
    static byte recent_left = 0;
    static byte cvalues_left = 0;
    static float sum_left = 0;
    static float values_left[nvalues_left];

    sum_left += value_left;

    if (cvalues_left == nvalues_left)
        sum_left -= values_left[recent_left];

    values_left[recent_left] = value_left;
    if (++recent_left >= nvalues_left)
        recent_left = 0;

    if (cvalues_left < nvalues_left)
        cvalues_left += 1;

    int average_left = sum_left / cvalues_left;

    return average_left;

}

int calculateAverageRight(int data_right){

    /*
    *The calculation of the floating average is based on: https://forum.arduino.cc/?_gl=1*y3keuz*_ga*MjI2NTc4MzE5LjE2NTg3MzQyODY.*_ga_NEXN8H46L5*MTY1ODczNDI4NS4xLjEuMTY1ODczNDUyNi42MA..
    */

    float value_right = data_right;
    const byte nvalues_right = 250;
    static byte recent_right = 0;
    static byte cvalues_right = 0;
    static float sum_right = 0;
    static float values_right[nvalues_right];

    sum_right += value_right;

    if (cvalues_right == nvalues_right)
        sum_right -= values_right[recent_right];

    values_right[recent_right] = value_right;
    if (++recent_right >= nvalues_right)
        recent_right = 0;

    if (cvalues_right < nvalues_right)
        cvalues_right += 1;

    int average_right = sum_right / cvalues_right;

    return average_right;

}

void displayAverage(int average_left, int average_right){

    myGLCD.setBackColor(196, 192, 192);

    myGLCD.setColor(139, 0, 0);
    myGLCD.printNumI(average_left, 85, 286);

    myGLCD.setColor(139, 0, 0);
    myGLCD.printNumI(average_right, 398, 286);

}

void graphingLeft(int runtime_sec, int data_left, int average_left){

    int x_raw_left = runtime_sec;
    int y_raw_left = data_left;
    int y_average_raw_left = average_left;

    int x_left = x_raw_left * 5.3833 + 76;
    int y_left = y_raw_left * -1.1667 + 127;
    int y_average_left = y_average_raw_left * -1.1667 + 127;

    if (y_left > 0){

        myGLCD.setColor(223, 242, 166);
        myGLCD.fillRect(x_left, y_average_left, x_left + 4, 127);
        myGLCD.setColor(189, 255, 0);
        myGLCD.drawLine(x_left, y_left, x_left, 127);

        if (average_left - data_left >= 15) {
            myGLCD.setColor(RED);
            myGLCD.drawLine(x_left, y_left, x_left, 127);
        }

    }

}

void graphingRight(int runtime_sec, int data_right, int average_right){

    int x_raw_right = runtime_sec;
    int y_raw_right = data_right;
    int y_average_raw_right = average_right;

    int x_right = x_raw_right * 5.3833 + 76;
    int y_right = y_raw_right * -1.1667 + 245;
    int y_average_right = y_average_raw_right * -1.1667 + 245;

    myGLCD.setColor(214, 255, 253);
    myGLCD.fillRect(x_right, y_average_right, x_right + 4, 245);
    myGLCD.setColor(185, 249, 245);
    myGLCD.setColor(0, 255, 240);
    myGLCD.drawLine(x_right, y_right, x_right, 245);

    if (average_right - data_right >= 15) {
        myGLCD.setColor(RED);
        myGLCD.drawLine(x_right, y_right, x_right, 245);
    }

}

int getDangerIndexLeft(int average_left, int data_left, int left){

    int grenzwert_left = abs((-0.4142*data_left) - 0.1975);

    if(abs(data_left - average_left)*sensitivity <= grenzwert_left){
        int danger_index_left;
        return danger_index_left = 2*left;
    }

    else if(average_left > data_left){
        int danger_index_left;
        return danger_index_left = 1*left;
    }

    else {
        int danger_index_left;
        return danger_index_left = 0*left;
    }

}

int getDangerIndexRight(int average_right, int data_right, int right){

    int grenzwert_right = abs((-0.4142*data_right) - 0.1975);

    if(abs(data_right - average_right)*sensitivity<= grenzwert_right){
        int danger_index_right;
        return danger_index_right = 2*right;
    }

    else if(average_right > data_right){
        int danger_index_right;
        return danger_index_right = 1*right;
    }

    else {
        int danger_index_right;
        return danger_index_right = 0*right;
    }

}

void visualWarning(int danger_index_left, int danger_index_right){

    // visual warning left

    if (danger_index_left == 2) {
        myGLCD.setColor(RED);
        myGLCD.fillRect(0, 28, 40, 304);
        myGLCD.setBackColor(RED);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("L", 18, 278);
    }

    else if (danger_index_left == 1) {
        myGLCD.setColor(YELLOW);
        myGLCD.fillRect(0, 28, 40, 304);
        myGLCD.setBackColor(YELLOW);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("L", 18, 278);
    }

    else if (danger_index_left == 0) {
        myGLCD.setColor(0, 153, 76);
        myGLCD.fillRect(0, 28, 40, 304);
        myGLCD.setBackColor(0, 153, 76);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("L", 18, 278);
    }

    // visual warning right

    if (danger_index_right == 2) {
        myGLCD.setColor(RED);
        myGLCD.fillRect(439, 28, 479, 304);
        myGLCD.setBackColor(RED);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("R", 457, 278);
    }

    else if (danger_index_right == 1) {
        myGLCD.setColor(YELLOW);
        myGLCD.fillRect(439, 28, 479, 304);
        myGLCD.setBackColor(YELLOW);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("R", 457, 278);
    }

    else if (danger_index_right == 0){
        myGLCD.setColor(0, 153, 76);
        myGLCD.fillRect(439, 28, 479, 304);
        myGLCD.setBackColor(0, 153, 76);
        myGLCD.setColor(255, 255, 255);
        myGLCD.print("R", 457, 278);
    }

}

void audioWarning(int danger_index_left, int danger_index_right){

    if(index_tone == 1){

        if(danger_index_left == 2 || danger_index_right == 2){

            tone(37, 400, 1000);
            delay(100);
            tone(37, 200, 1000);
            delay(1000);
        }

    }

    else {}

}

void resetGraphs(int milliseconds){

    if ((milliseconds/1000) % 60 == 0 ) {

        // upper graphing box (lefthand side)

        myGLCD.setColor(102, 102, 102);
        myGLCD.fillRect(76, 56, 399, 127);
        myGLCD.setColor(WHITE);

        // lower graphing box (righthand side)

        myGLCD.setColor(102, 102, 102);
        myGLCD.fillRect(76, 174, 399, 245);
        myGLCD.setColor(WHITE);
        myGLCD.setBackColor(102, 102, 102);
        myGLCD.print("rechte Schirmseite", 177, 158);

        // time box

        myGLCD.setColor(BLACK);
        myGLCD.fillRect(128, 271, 352, 299);
    }

}

void serialPrint(int milliseconds, int data_left, int data_right, int average_left, int average_right, int danger_index_left, int danger_index_right){

    Serial.print("Index: data, average, danger index | ");
    Serial.print(data_left);
    Serial.print(" [kg],");
    Serial.print(average_left);
    Serial.print(" [kg],");
    Serial.print(danger_index_left);
    Serial.print(" [i] |");

    if(danger_index_left*left == 2 || danger_index_right*right == 2){

        Serial.print("");
        Serial.print(" ----> !!! SERIOUS DANGER !!! <----");

    }

    else if(danger_index_left*left == 1 || danger_index_right*right == 1){

        Serial.print("");
        Serial.print(" ----> ??? POTENTIAL DANGER ??? <----");

    }

    else{

        Serial.print("");
        Serial.print(" ----> ... SAFE FLIGHT ... <----");

    }

    Serial.println();

}