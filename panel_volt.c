/*
 * File:   main.c
 * Author: lbogdanov
 *
 * Created on April 1, 2026, 12:13 AM
 */
#include <xc.h>

#define ADC_OFFSET 2 //Short to ground, watch the display, enter the number here.
#define OFFSET_DV  2 // 0.2V offset
#define K_CALIB    1106 //K_CALIB = Vin_reference / Vmeasured = 90V / 81V = 1.111 => 1111

// CONFIG1
#pragma config FEXTOSC = HS
#pragma config RSTOSC = HFINT1 
#pragma config CLKOUTEN = OFF
#pragma config CSWEN = ON
#pragma config FCMEN = OFF

// CONFIG2
#pragma config MCLRE = ON
#pragma config PWRTE = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = ON
#pragma config BORV = LOW
#pragma config PPS1WAY = ON
#pragma config STVREN = ON
#pragma config DEBUG = OFF

// CONFIG3
#pragma config WDTE = OFF

#define _XTAL_FREQ 4000000  // 4 MHz

// I2C address (write)
#define VK16K33_ADDR 0xE0

//ChatGPT generated
//0, 1, 2, 3, 4, 5, 6, 7, 8, 9, space
uint8_t digit_to_led_map[11] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00
};

//ChatGPT generated
void i2c_init(void){
    // 1. PPS MAPPING (RC1 = SDA, RC2 = SCL)
    RC1PPS = 0x19;          // SDA1 Output -> RC1
    RC2PPS = 0x18;          // SCL1 Output -> RC2
    SSP1DATPPS = 0x11;      // RC1 -> SDA1 Input
    SSP1CLKPPS = 0x12;      // RC2 -> SCL1 Input

    // 2. PIN SETUP
    TRISCbits.TRISC1 = 1;   // Must be input for I2C
    TRISCbits.TRISC2 = 1;
    ANSELCbits.ANSC1 = 0;   // Digital
    ANSELCbits.ANSC2 = 0;
    ODCONCbits.ODCC1 = 1;   // Open-Drain
    ODCONCbits.ODCC2 = 1;

    // 3. MSSP1 MASTER SETUP
    SSP1STAT = 0x80;        // Slew rate disabled for 100kHz
    SSP1CON1 = 0x28;        // Master mode, SSP Enable
    
    // NEW BAUD RATE for 4MHz
    SSP1ADD = 9;            // 100kHz @ 4MHz Fosc
}

//ChatGPT generated
void i2c_start(void){    
    SSP1CON2bits.SEN = 1;
    while(SSP1CON2bits.SEN);
}

//ChatGPT generated
void i2c_write(unsigned char data){        
    SSP1BUF = data;  // Address + Read (1)
    while(SSP1STATbits.BF);     // Wait for address to send
    while(SSP1CON2bits.ACKSTAT);// Wait for Slave ACK
}

//ChatGPT generated
void i2c_stop(void){   
    SSP1CON2bits.PEN = 1;       
    while(SSP1CON2bits.PEN);
}

//ChatGPT generated
void vk16k33_init(void){
    // Oscillator ON
    i2c_start();
    i2c_write(VK16K33_ADDR);
    i2c_write(0x21);
    i2c_stop();

    // Display ON, no blink
    i2c_start();
    i2c_write(VK16K33_ADDR);
    i2c_write(0x81);
    i2c_stop();

    // Brightness max
    i2c_start();
    i2c_write(VK16K33_ADDR);
    i2c_write(0xEF);
    i2c_stop();
}

void vk16k33_display_8888(void){
    i2c_start();
    i2c_write(VK16K33_ADDR);

    i2c_write(0x00); // RAM address start

    i2c_write(0xff); // '8.'
    i2c_write(0x00);

    i2c_write(0xff); // '8.'
    i2c_write(0x00);
    
    i2c_write(0xff); // ':'
    i2c_write(0x00);

    i2c_write(0xff); // '8.'
    i2c_write(0x00);

    i2c_write(0xff); // '8.'
    i2c_write(0x00);

    i2c_stop();
}

//Some parts of this code are ChatGPT generated
void vk16k33_display_digit(uint8_t position, uint8_t value, uint8_t dot){
    uint8_t seg;
        
    seg = digit_to_led_map[value];
     
    // Turn on decimal point if requested
    if(dot){
        seg |= 0x80;   // DP is bit 7
    }
    
    i2c_start();
    i2c_write(VK16K33_ADDR);

    //Select address
    switch(position){
        case 0:
            i2c_write(0x00);
            break;
        case 1:
            i2c_write(0x02);
            break;
        case 2:
            i2c_write(0x04);
            break;
        case 3:
            i2c_write(0x06);
            break;
        case 4:
            i2c_write(0x08);
            break;
    }
    
    //Display digit
    i2c_write(seg); 
    i2c_write(0x00);

    i2c_stop();
}

//ChatGPT generated
void adc_init(void){
    // --- Configure FVR ---
    FVRCONbits.FVREN = 1;      // Enable FVR
    FVRCONbits.ADFVR = 0b10;   // 2.048V for ADC

    // Wait for FVR to stabilize
    while(!FVRCONbits.FVRRDY);
    
    // Set RC4 as input
    TRISCbits.TRISC4 = 1;
    
    WPUCbits.WPUC4 = 0;
    INLVLCbits.INLVLC4 = 0;
    ODCONCbits.ODCC4 = 0;

    // Set RC4 as analog
    ANSELCbits.ANSC4 = 1;

    // Select channel AN20 (RC4=ANC4)
    ADCON0bits.CHS = 0b010100; // AN20

    // Vref = VDD
    //ADCON1bits.ADPREF = 0b00;  // Vref+ = VDD
    ADCON1bits.ADPREF = 0x03;  // 11 = FVR
    ADCON1bits.ADNREF = 0x00;  // Vref- = VSS
    
    // Right justified result
    ADCON1bits.ADFM = 1;

    // ADC clock = Fosc/4
    ADCON1bits.ADCS = 0b100;

    // Enable ADC
    ADCON0bits.ADON = 1;
}

//ChatGPT generated
uint16_t adc_read(void){
    __delay_us(10);               // Acquisition time

    ADCON0bits.GO_nDONE = 1;      // Start conversion
    while (ADCON0bits.GO_nDONE);  // Wait

    return ((ADRESH << 8) | ADRESL);
}

//ChatGPT generated
// Displays up to 3 digits: e.g. 110.0 ? [1][1][0][0]
void split_voltage_digits(uint16_t adc, uint8_t *digits){
    uint32_t dv;
    
//    if (adc > ADC_OFFSET){
//        adc -= ADC_OFFSET;
//    }
//    else{
//        adc = 0;
//    }

    // Direct conversion: ADC ? real voltage in decivolts
    dv = ((uint32_t)adc * 1100 + 511) / 1023;

    // ? Calibration (adjust this!)
    dv = (dv * K_CALIB) / 1000;
    
//    if (dv > OFFSET_DV)
//        dv -= OFFSET_DV;
//    else
//        dv = 0;

    // Split digits (XXX.X)
    digits[0] = (dv / 1000) % 10;
    digits[1] = (dv / 100) % 10;
    digits[2] = (dv / 10) % 10;
    digits[3] = dv % 10;
}

void main(void){
    volatile uint32_t adc_value = 0; 
    uint8_t digits[4];
    uint8_t i;
            
    OSCCON1 = 0x70; //External OSC=4MHz, div=1
    
    i2c_init();

    __delay_ms(100);
    
    vk16k33_init();

    __delay_ms(10);

    vk16k33_display_8888();
    
    __delay_ms(1000);
    
    adc_init();    
    
    while(1){
        for(i = 0; i < 100; i++){
            adc_value += adc_read();
            __delay_ms(1);
        }
        
        adc_value /= 100;       
        
        split_voltage_digits(adc_value, digits);
        
        vk16k33_display_digit(0, digits[0], 0);
        vk16k33_display_digit(1, digits[1], 0);
        vk16k33_display_digit(2, digits[10], 0);
        vk16k33_display_digit(3, digits[2], 1);
        vk16k33_display_digit(4, digits[3], 0);                       
    }
}
