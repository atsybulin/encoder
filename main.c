#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "encoder.h"

#define F_CPU 1000000UL

#define SetBit(port, bit) port |= (1<<bit)
#define ClearBit(port, bit) port &= ~(1<<bit)

//это для наглядности кода
#define b00000011 3
#define b11010010 210
#define b11100001 225

unsigned char dig[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
volatile unsigned char n[4] = {0x00, 0x00, 0x00, 0x00};
unsigned int num = 99;
int pos = 1;

volatile unsigned int bufEnc = 0; //буфер энкодера

ISR(TIMER0_OVF_vect) {
//	TCNT1=RELOAD;
	PORTB = 0x00;
	PORTB = 1 << pos;
	PORTD = n[pos-1];
	pos++;
	if (pos>4) pos = 1;
}

void init_timer(void) {
	TCCR0=(0 << CS02) | (1 << CS01) | (0 << CS00); // PRESCALE 8
	TIMSK|=(1<<TOIE0); // разрешаем прерывание по переполнению
//	TCNT1=RELOAD;
}

void convert_data (unsigned int x) {
	unsigned int temp,res;
	temp=x;
	res=temp/1000;			//Calculate 1000-s
	n[0]=dig[res];
	temp=temp-res*1000;

	res=temp/100;			//Calculate 100-s
	n[1]=dig[res];
	temp=temp-res*100;

	res=temp/10;			//Calculaate 10-s
	n[2]=dig[res];
	temp=temp-res*10;

	n[3]=dig[temp];			//Calculate 1-s
}

// ================================================================
// ================================================================
// ================================================================

void main() {
	DDRD = 0xFF;
	DDRB = 0xFF;
	PORTB = 0xFF;

	ENC_InitEncoder();
	init_timer();
	sei();

	while (1) {
		ENC_PollEncoder();
		num += ENC_GetStateEncoder();
		if (num > 9999)
			num = 9999;
		if (num < 1)
			num = 1;
		convert_data(num);
	}
}

// ================================================================
// ================================================================
// ================================================================


//функция инициализации
//__________________________________________
void ENC_InitEncoder(void)
{
  ClearBit(DDR_Enc, Pin1_Enc); //вход
  ClearBit(DDR_Enc, Pin2_Enc);
  SetBit(PORT_Enc, Pin1_Enc);//вкл подтягивающий резистор
  SetBit(PORT_Enc, Pin2_Enc);
}

//функция опроса энкодера
//___________________________________________
void ENC_PollEncoder(void)
{
static unsigned char stateEnc; 	//хранит последовательность состояний энкодера
unsigned char tmp;
unsigned char currentState = 0;

//проверяем состояние выводов микроконтроллера
if ((PIN_Enc & (1<<Pin1_Enc))!= 0) {SetBit(currentState,0);}
if ((PIN_Enc & (1<<Pin2_Enc))!= 0) {SetBit(currentState,1);}

//если равно предыдущему, то выходим
tmp = stateEnc;
if (currentState == (tmp & b00000011)) return;

//если не равно, то сдвигаем и сохраняем в озу
tmp = (tmp<<2) | currentState;
stateEnc = tmp;

//сравниваем получившуюся последовательность
if (tmp == b11100001) bufEnc = LEFT_SPIN;
if (tmp == b11010010) bufEnc = RIGHT_SPIN;
return;
}

//функция возвращающая значение буфера энкодера
//_____________________________________________
unsigned int ENC_GetStateEncoder(void)
{
  unsigned int tmp = bufEnc;
  bufEnc = 0;
  return tmp;
}
