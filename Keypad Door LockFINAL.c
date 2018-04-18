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

//Program to unlock a motorized door lock if the user enters the correct keycode. If the door is left unopen after entering the correct code,
//door lock will reinitate. If opened and left open, door will check if something is blocking the path. Proximity sensors used to achieve this.
//Door will automatically relock if door is placed in the correct closed position. Buzzer beeps will begin sounding, with increasing volume
//the longer the door is left open. Project is upgradeable, as a large number of keys can be interfaced, with minor changes to code.
//All this is achieved exculsively on the ARM Cortex M4 Based Microcontroller

//Thana wrote a large majority of the code, while Pat designed and assembled a large portion of the supporting hardware.
//Thanarajasegaran Subramaniam & Patrick Stephane Surette-All rights reserved.

#include "MK64F12.h" // Embedded Microcontroller Library

#include <stdio.h>
#include <string.h>

int global_key = 51515; // Master lock code, program crosschecks with this code to see if correct Keycode is entered.
						// Increase this by a factor of 10 for every new button added
						
char *error_code="==========================Access Denied-Try Again=========================="; // Wrong 5 Digit code entered message
char *correct_code= "==================Access Granted-Opening Door-Stand Back==================="; // Correct Code entered, motor initiates
char *lockout= "==================Three Consecutive Errors - Lockout Initiated=================="; // After 3 consecutive failed codes, program locks temporarily
 char *retry="===========================Lockout Expired - Try Again=========================="; // Retry message after lockout timer expires
char *blocked="Door Is Blocked-Please Step Away                                                "; //Proximity sensor detects a block between the doorway
char *starkey="*";
char *dooropen="Door unlocked";
char *closedoor = "Please Close The Door Behind You";

// Initialise all supporting protocols and pins
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
// Add more port keys as required

	PORTC_PCR10 |=PORT_PCR_MUX(1); //green LED
	PORTC_PCR11 |=PORT_PCR_MUX(1); //red LED

	PORTB_PCR17 |= PORT_PCR_MUX(3); //UART0 Transmit
	PORTB_PCR16 |= PORT_PCR_MUX(3); //UART0 Receive

	PORTC_PCR3 |= PORT_PCR_MUX(1); //Motor

	//UART0 Stuff
	UART0_C2 |=0x0;// Initialization of transmit and receive
	UART0_C1 |=0x0;// UART0 for 8 bits

	UART0_BDH=0;
	UART0_BDL=0x88;

	UART0_C2 |= UART_C2_TE_MASK;
	UART0_C2 |= UART_C2_RE_MASK;
	
	// ADC things
	ADC_Init();
	
	// GPIO Things. Setting all GPIO Initial states
	GPIOC_PDDR |= 0x400; //LED Green
	GPIOC_PDDR |= 0x800;//LED Red
	GPIOC_PDDR |= 0x8;//Motor
	GPIOC_PDDR |= 0x00000000; //UART0
	GPIOB_PDDR |= 0x000000000; //keys


	//DAC Enable and DAC Reference
	DAC0_C0 = 0xF0;
	DAC0_C1 = 0x00;

	int key = 0;
	
	int count;
	int door =0;
	int blockcount =0;
	int convert;
	int response; // cross checks entered keys with the master lock
	

	

	int counter=0; // Based on the number of codes that have been entered,
				  //the counter will increment until the while loop fails and
				 //checks with master key

	for(;;) {
	count = 10000; //This makes sure that the first number pressed is the largest number of the keycode. 
	//If i press 5, it will be 50000, if I press 4 next, it will add 4000 to 50000, maing 54000 and so on so forth. 
//	INcrease by a factor of 10 for every new button added
	
	key = 0;


	
	while(counter<5){  //checks until the set numbers have keys have been pressed, Increase number(5) for every new key added
	while((GPIOB_PDIR & 0x100c0c)){ // All button GPIOs included 
						// This while will keep checking until a button is pressed
	}
	//Each GPIO key is assigned a number. The first key to be pressed is multipled by count, then every next key pressed will be
	//multiplied by count/10 and added to the first number, until the number set in the while loop is reached, in this case "5".
	
	// Example: If my code is 51515, the code will work as such: 5*10000 + 1*1000 + 5*100 + 1*10 + 5*1 = 51515.
	//Refer to the if statements below.
	
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
	count=count/10; // Reduce the multiplier for every consecutive key
	UARTx_putstring(starkey);
	Green_Led(); // ADD things that need to work WHEN keys are pressed here.
	DAC_Out(0x3FF); // Buzzer beeps everytime a key is pressed
	delay_shortest();// Play buzzer beep for the shortest time
	DAC_Out(0x000); // Turn off buzzer beep 
	
delay_shortest(); // Add very short delay after each individual key. Essentially debouncing the keys
}

	//count=1;
	response = check_key(key); // Check the key that has just been entered
	delay_short();
	
	if (response == 1){ // If key is correct, go through succesful entry cycle 
		Green_LedBlink();
		UARTx_putstring(correct_code);

		GPIOC_PSOR|=0x8; // Turn on motor to initiate door lock
		delay_door(); // keep motor running for certain time
		GPIOC_PCOR |=0x8; // Shut off motor when it is open
		
		//Custom delay for the door, after it is unlocked 
		int a_count = 0;
		while(a_count <= 10){
		delay_short(); //Give user time to open the door after it is unlocked
		a_count++;
	}
		convert=ADC_Convert(); // Proximity sensor checks distance
		delay_shortest();
		convert=ADC_Convert(); // Check again

		for(int g=0;g<=15;g++){
			delay_short();
		if(convert>=26000 && convert<=33000){// at this distance, door is left unopened eventhough code was correct
			UARTx_putstring(dooropen); // Let user know they can open door now
			convert=ADC_Convert(); // Check distance again
			delay_shortest();
			convert=ADC_Convert();
			door=1; //Flag to notify that door is unlocked but closed
			counter =0; // Reset the counter for number of keys pressed

		}
		else{
			door=0; //Flag that door is opened and unlocked
			break; // Leave this operation if the door is opened and still has not been closed
		}
		}

		if(door!=1){ //Door is left opened, something may be blocking it or may not, but it is definitely open
		a_count = 0;
			while(a_count <= 20){
			delay_short(); //Give user time to close the door behind them
			a_count++;
				}
			convert=ADC_Convert(); // Check distance again
			delay_shortest();
			convert=ADC_Convert(); //Re check distance

			while(convert>=34000 && convert<=53000){	//At this range, something is blocked the door from closing
			UARTx_putstring(blocked); // Door is blocked message
			delay_door();
			delay_door();
			DAC_Out(0xBFF); // beep to let user know that something is blocking the door
			delay_short();
			DAC_Out(0x000);
			convert=ADC_Convert(); // keep checking distances for changes
			delay_shortest();
			convert=ADC_Convert();
	}
	//After the user removes the blockage, give them time to close the door, if they don't close within a certin delay, start beeping again
	//See following code below
		delay_door();
		delay_door();
		delay_door();
		delay_door();

		convert=ADC_Convert();
		delay_shortest();
		convert=ADC_Convert();

		while(convert>=53500){		//Nothing blocking here, just door left open, notify user with beeps
			UARTx_putstring(closedoor); // Close the door message
			delay_door();
			delay_door();
			DAC_Out(0xBFF); // Start beeping until the usr closes this door
			delay_short();
			DAC_Out(0x000);
			convert=ADC_Convert(); //Keep checking distances to see if conditions are met
			delay_shortest();
			convert=ADC_Convert();
			}


		GPIOC_PSOR|=0x8; // Blockage removed, door is closed, allow motor to relock the door
		delay_door();
		GPIOC_PCOR |=0x8;

		int wrong3 = 0; // This counter will check for 3 consecutive errors, it will be reset to 0 eveytime a correct code is entered
		counter = 0; // reset counter to stop the red LEDs from blinking after check
	}
		else if(door==1){ // If the door was unlocked earlier but never opened, just relock after the delays created above. 
		//Note: not all the delays above are active to get to here, some of them condition based
			GPIOC_PSOR|=0x8; // Turn on motor to lock
			delay_door();
			GPIOC_PCOR |=0x8;
	}

	}
	
	else if(response!=1) { // Wrong code was entered, flash red LEDS and show wrong code message
		Red_LedBlink();
		wrong3++; // Increment wrong code counter
		UARTx_putstring(error_code); // Wrong code message
		counter = 0;
	}
	counter = 0; // Reset the key count
	if(wrong3==3||wrong3==6||wrong3==9){ // Any multiples of 3 up to 9 in this case will cause the program to lockout the user
		UARTx_putstring(lockout); // Display lockout message	
		delay(); // Delay program for 1 minute 3 seconds
		UARTx_putstring(retry); // Let user retry, counter reset below so they can
		counter =0;
	}
	}
}

void Green_Led(void){//short delay green LED-one blink
	GPIOC_PSOR |= 0x400; // 
	delay_shortest();
	GPIOC_PCOR |= 0x400;
}


void Green_LedBlink(void){ // three blinks from the Green LED, happens when the code is correct, buzzer is integrated into this
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
	DAC_Out(0xAFF); // Buzzer beeps
	delay_short();
	DAC_Out(0x000);
	delay_short();
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
	DAC_Out(0xAFF);
	delay_short();
	DAC_Out(0x000);
	delay_short();
	GPIOC_PSOR |= 0x400;
	delay_short();
	GPIOC_PCOR |= 0x400;
	DAC_Out(0xAFF);
	delay_door();
	delay_door();
	DAC_Out(0x000);
}

void Red_Led(void){//long delay red LED-one blink
	GPIOC_PSOR |= 0x800;
	delay_short();
	GPIOC_PCOR |= 0x800;
}

void Red_LedBlink(void){// three blinks from red LED, happens when the code fails, also buzzer is also integreted into this 
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;
	DAC_Out(0xFFF); // Loud buzzer beeps
	delay_short();
	DAC_Out(0x000);
	delay_shortest();
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;
	DAC_Out(0xFFF);
	delay_short();
	DAC_Out(0x000);
	delay_shortest();
	GPIOC_PSOR |= 0x800;
	delay_shortest();
	GPIOC_PCOR |= 0x800;
	DAC_Out(0xFFF);
	delay_door();
	delay_door();
	DAC_Out(0x000);
}

void delay(void) // Longest delay available : 1:30
{
	int c = 1, d = 1;
	for ( c = 1 ; c <= 32767 ; c++ )
		for ( d = 1 ; d <= 2000 ; d++ ){}
}

void delay_door(void){ //delay built specifically for the motor action
	int c = 1, d = 1;
		for ( c = 1 ; c <= 32750 ; c++ )
			for ( d = 1 ; d <= 30 ; d++ ){}
}

void delay_short(void)
{
	int c = 1, d = 1;
	for ( c = 1 ; c <= 32767 ; c++ ) 
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
	//DAC_Out(0x33F);
	//DAC_Out(0x000);
}

void ADC_Init()
{   // PTB2
	SIM_SCGC6|= SIM_SCGC6_ADC0_MASK;   // Enable clock for ADC0
	SIM_SCGC5|= SIM_SCGC5_PORTC_MASK;   //Enable clock for PORT B
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

