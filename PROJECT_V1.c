/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// Authored By Patrick Stephane Surette and Thanarajasegaran Subramaniam
// KEYPAD CONTROL DOOR LOCK
//Write a description here
//
//Thanarajasegaran Subramaniam & Patrick Stephane Surette-All rights reserved.

#include "MK64F12.h"

#include <stdio.h>
#include <string.h>
int global_key = 11111; // Master lock- program cross check with this variable
char *error_code="==========================Access Denied-Try Again==========================";
char *correct_code= "==================Access Granted-Opening Door-Stand Back===================";
char *lockout= "==================Three Consecutive Errors - Lockout Initiated==================";
char *retry="===========================Lockout Expired - Try Again==========================";
char *blocked="Door Is Blocked-Please Step Away";
char *starkey="*";

int ADC_Convert(void);
void Green_Led(void);
void delay(void);
void delay_short(void);
int check_key(int key);
void UARTx_Putchar(char character);
void DAC_Out(short voltage);


int main(void){

	SIM_SCGC5 |=SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTB_MASK;
	SIM_SCGC4 |= SIM_SCGC4_UART0_MASK | SIM_SCGC4_UART1_MASK;// Clock for UART
	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK;
	SIM_SCGC6 |= SIM_SCGC6_DAC0_MASK;
	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
	SIM_SCGC2 |= SIM_SCGC2_DAC0_MASK;
	SIM_SCGC2 |= SIM_SCGC2_DAC1_MASK;


	//Buttons
	PORTB_PCR2 |= PORT_PCR_MUX(1); //key 1
	PORTB_PCR3 |= PORT_PCR_MUX(1); //key 2
	PORTB_PCR10 |= PORT_PCR_MUX(1); //key 3
	PORTB_PCR11 |= PORT_PCR_MUX(1); //key 4
	PORTB_PCR20 |= PORT_PCR_MUX(1); //key 5


	PORTC_PCR10 |=PORT_PCR_MUX(1); //green LED
	PORTC_PCR11 |=PORT_PCR_MUX(1); //red LED

	PORTB_PCR17 |= PORT_PCR_MUX(3); //UART0 Transmit
	PORTB_PCR16 |= PORT_PCR_MUX(3); //UART0 Receive

	//UART0 Stuff
	UART0_C2 |=0x0;// Initialization of transmit and receive
	UART0_C1 |=0x0;// UART0 for 8 bits

	UART0_BDH=0;
	UART0_BDL=0x88;

	UART0_C2 |= UART_C2_TE_MASK;
	UART0_C2 |= UART_C2_RE_MASK;
	//Just ADC things
	ADC_Init();
	// GPIO Things
	GPIOC_PDDR |= 0x400; //LED Green
	GPIOC_PDDR |= 0x800;//LED Red
	GPIOC_PDDR |= 0x00000000; //UART0
	GPIOB_PDDR |= 0x000000000; //keys

	//DAC Enable and DAC Reference
	DAC0_C0 = 0xF0;
	DAC0_C1 = 0x00;

	int key = 0;
	int count;
	int blockcount =0;
	int convert;
	int response; // cross checks entered keys with the master lock
	int wrong3 = 0; // Counter for consecutive errors
	/*while(1){
	convert=ADC_Convert();
	}*/
	int counter=0; // Based on the number of codes that have been entered,
				  //the counter will increment until the while loop fails and
				 //checks with master key

	for(;;) {
	count = 10000;
	key = 0;


	//wrong3 = 0;
	while(counter<5){  //checks until the set numbers have keys have been pressed
	while((GPIOB_PDIR & 0x4 && GPIOB_PDIR & 0x8 && GPIOB_PDIR & 0x400 && GPIOB_PDIR & 0x800 && GPIOB_PDIR & 0x100000)){
						// This while will keep checking until a button is pressed
	}
	if(!(GPIOB_PDIR & 0x4)) {//Push Button 1 Pressed
		key += 1 * count;
	}
	else if (!(GPIOB_PDIR & 0x8)){ //Pushbutton 2
		key += 2 * count;
	}
	else if(!(GPIOB_PDIR & 0x400)){ //Button 3 pressed
		key += 3 *count;
	}
	else if(!(GPIOB_PDIR & 0x800)){ //Button 4 pressed
		key += 4 *count;
	}
	else if(!(GPIOB_PDIR & 0x100000 /**/)){ //Button 5 pressed
		key += 5 *count;
	}

	counter++;
	count=count/10;
	UARTx_putstring(starkey);
	Green_Led(); // ADD things that need to work WHEN keys are pressed here.
	delay_shortest(); // Add very short delay after each individual key. Have not tried without delay.
}

	//count=1;
	response = check_key(key);
	delay_short();
	if (response == 1){
		Green_LedBlink();

		UARTx_putstring(correct_code);

		DAC_Out(0xFFF);
		delay_door();
		DAC_Out(0x000);



		convert=ADC_Convert();
		delay_shortest();
		convert=ADC_Convert();

		while(convert>=30000 && convert<=60000){		//change range here
			UARTx_putstring(blocked); //Also dont forget to add a buzzer beeps here
			delay_door();
			convert=ADC_Convert();
		}

		int a_count = 0;
		while(a_count <= 10){
		delay_short(); //Give user time to open the door after it is unlocked
		a_count++;
	}

		DAC_Out(0xFFF);
		delay_door();
		DAC_Out(0x000);

		wrong3 = 0;
		counter = 0; // reset counter to stop the red LEDs from blinking after check
	}
	else {
		Red_LedBlink();
		wrong3++;
		UARTx_putstring(error_code);
		counter = 0;
	}
	if(wrong3==3||wrong3==6||wrong3==9){
		UARTx_putstring(lockout);
		delay();
		UARTx_putstring(retry);
	}
	}
}

void Green_Led(void){//long delay green LED-one blink
	GPIOC_PSOR |= 0x400;
	delay_shortest();
	GPIOC_PCOR |= 0x400;
}

void Green_LedBlink(void){ // three blinks from the Green LED
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
	delay_short();
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
	delay_short();
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
}

void Red_Led(void){//long delay red LED-one blink
	GPIOC_PSOR |= 0x800;
	delay_short();
	GPIOC_PCOR |= 0x800;
}

void Red_LedBlink(void){// three blinks from red LED
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;
	delay_shortest();
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;
	delay_shortest();
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;

}

void delay(void)
{
	int c = 1, d = 1;
	for ( c = 1 ; c <= 32767 ; c++ )
		for ( d = 1 ; d <= 2000 ; d++ ){}
}

void delay_door(void){ //delay built specifically for the motor action
	int c = 1, d = 1;
		for ( c = 1 ; c <= 32767 ; c++ )
			for ( d = 1 ; d <= 28 ; d++ ){}
}

void delay_short(void)
{
	int c = 1, d = 1;
	for ( c = 1 ; c <= 32767 ; c++ ) //Change later
 		for ( d = 1 ; d <= 20 ; d++ ){}
}


void delay_shortest(void)
{
	int c = 1, d = 1;
	for ( c = 1 ; c <= 32767 ; c++ )
 		for ( d = 1 ; d <= 10 ; d++ ){}
}

int check_key(int key){
	if(global_key!= key)
		return 0;
	else
		return 1;
}

void UARTx_Putchar(char character){//Transmit
	while (!(UART0_S1 & UART_S1_TDRE_MASK)){}
		UART0_D = character;
}

void UARTx_putstring (char *str)
{
	while(*str){
	UARTx_Putchar(*str);
	str++;
	}
}

void DAC_Out(short voltage){
	//Turn on lock(open)
	DAC0_DAT0L = voltage & 0x00FF;
	DAC0_DAT0H = ((voltage & 0x0F00) >> 8);
}

void ADC_Init()
{   // PTB2
	SIM_SCGC6|= SIM_SCGC6_ADC0_MASK;   // Enable clock for ADC0
	SIM_SCGC5|= SIM_SCGC5_PORTC_MASK;   //Enable clock for PORT 
	// Configuration
	ADC0_CFG1 = 0b101100; // ADIV=01, ADLSMP=0, Mode=11, ADICLK=0
	ADC0_CFG2 = 0b000000; // MUXSEL=0, ADHSC=0
	ADC0_SC1A = 0b01110; // AIEN=0, DIFF=0, ADCH=01100 (AD ch 12)
	ADC0_SC2 = 0b0000000; // ADACT=1, ADTRG=0, ACFE=0, DMAEN=0, REFSEL=00
	ADC0_SC3 = 0b000000; // CAL=0, ADCO=0
}

int ADC_Convert(void)
{	// Conversion
	int convert;
	ADC0_SC1A = 0b01110; // AIEN=0, DIFF=0, ADCH=01100 (AD ch 12)
	while (!(ADC0_SC1A & 0x8e)); // exits the loop when ADC0_SC1A=0x8e
	convert = ADC0_RA; // COCO
	return convert;
}

