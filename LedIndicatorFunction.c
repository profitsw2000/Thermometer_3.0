/*
 * LedIndicatorFunction.c
 *
 * Created: 10.08.2020 11:39:40
 *  Author: Анализатор спектра
 */ 

#include "LedIndicatorFunction.h"
#include "usart.h"

#define SEG_A						0
#define SEG_B						2
#define SEG_C						4
#define SEG_D						6
#define SEG_E						7
#define SEG_F						1
#define SEG_G						3
#define SEG_DP						5
#define DIGIT0_ANODE				2
#define DIGIT1_ANODE				3
#define DIGIT2_ANODE				4
#define DIGIT3_ANODE				5
#define DIGIT4_ANODE				6


/*таблица перекодировки*/
const unsigned char led_indicator_code_table[] =
{
	(1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(0<<SEG_G)|(0<<SEG_DP), //0
	(0<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(0<<SEG_G)|(0<<SEG_DP), //1
	(1<<SEG_A)|(1<<SEG_B)|(0<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(0<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //2
	(1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //3
	(0<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //4
	(1<<SEG_A)|(0<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //5
	(1<<SEG_A)|(0<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //6
	(1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(0<<SEG_G)|(0<<SEG_DP), //7
	(1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //8
	(1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //9
	(0<<SEG_A)|(0<<SEG_B)|(0<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(1<<SEG_G)|(0<<SEG_DP), //-
	(0<<SEG_A)|(0<<SEG_B)|(0<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(0<<SEG_G)|(0<<SEG_DP), //пустой
};

/*таблица для подачи сигнала на анод*/
const unsigned char led_anode_table[] =
{
	(1<<DIGIT0_ANODE), //цифра 0
	(1<<DIGIT1_ANODE), //цифра 1
	(1<<DIGIT2_ANODE), //цифра 2
	(1<<DIGIT3_ANODE), //цифра 3
	(1<<DIGIT4_ANODE), //цифра 4
};

unsigned char mbi_clk_counter = 0	;

/*
Коды символов для вывода на 16-сегментный индикатор.
Буква	Letter1  Letter0
А      - 0x78 0xC6
Б      - 0x38 0xDE
В      - 0x7A 0x1D
Г      - 0x00 0xC6
Д      - 0x4A  0x1F
Е      - 0x10  0xDE
Ж      - 0x87  0x21
З      - 0x68  0x1E
И      - 0xC8  0xE0
К      - 0x94  0xC0
Л      - 0xC8  0x20
М      - 0xC9  0xC0
Н      - 0x78  0xC0
О      - 0x48  0xDE
П      - 0x48  0xC6
Р      - 0x72  0x07
С      - 0x00  0xDE
Т      - 0x02  0x07
У      - 0x78  0x98
Ф      - 0x72  0x87
Х      - 0x85  0x20
Ц      - 0x02  0xD9
Ч      - 0x32  0x81
Ш      - 0x4A  0xD9
Э      - 0x32  0x0B
Ю      - 0x5A  0xD5
Я      - 0x32  0xC3 

*/

/*Преобразует значение температуры temperature в код для вывода на дисплей
\param code  - массив байтов для вывода на дисплей
\param temperature - считанная температура
*/


void GetDisplayCode(Digit_code * code, unsigned int * temperature, unsigned char sensors_num)
{
	
	unsigned char temp_digits[4]	;
	unsigned char temp	;
	
	//разложить на цифры значение температуры
	for(unsigned char i = 0; i < sensors_num; i++)
	{		
		if(temperature[i] & 0x8000)				//если температура отрицательная
		{
			//десятые доли градуса
			temp = (temperature[i] & 0x0F) | 0xF0	;
			temp = ~temp	;
			
			if (temp == 0)
			{
				temp_digits[0] = ((625*temp)/1000)	;
				temp = ((temperature[i] & 0xFF0) >> 4)	;
				temp = ~temp	;
			} 
			else
			{				
				temp_digits[0] = ((625*temp)/1000) + 1	;
				temp = (temperature[i] & 0xFF0) >> 4	;
				temp = ~temp	;
				if (temp_digits[0] == 10) 
				{
					temp_digits[0] = 0	;
					temp = temp + 1	;
				}
			}
			//единицы градусов			
			temp_digits[1] = temp%10	;
			temp_digits[2] = temp/10	;
			temp_digits[3] = 0xA	;
			if (temp_digits[2] == 0)
			{
				temp_digits[2] = 0x0A	;
				temp_digits[3] = 0x0B	;
			}
		}
		else
		{
			//десятые доли градуса
			temp = temperature[i] & 0x0F	;
			temp_digits[0] = (625*temp)/1000	;
			//единицы градусов
			temp = (temperature[i] >> 4) & 0x7F	;
			temp_digits[1] = temp%10	;
			temp_digits[2] = (temp%100)/10	;
			temp_digits[3] = (temp/100)	;
			if (temp_digits[2] == 0)
			{
				temp_digits[3] = 0x0B	;
				temp_digits[2] = 0x0B	;
			}
			else if (temp_digits[3] == 0)
			{
				temp_digits[3] = 0x0B	;
			}
		}
		
		//letters[0] = 0x7898	;
		
		//записать коды цифр в память
		code[i].dig_code[0] = led_indicator_code_table[temp_digits[0]]	;
		code[i].dig_code[1] = led_indicator_code_table[temp_digits[1]] | (1<<SEG_DP)	;
		code[i].dig_code[2] = led_indicator_code_table[temp_digits[2]]	;
		code[i].dig_code[3] = led_indicator_code_table[temp_digits[3]]	;
		code[i].dig_code[4] = letters[i]	;
	}
	
}

void Init_timers()
{
	unsigned char temp	;
	TCCR0 = (1<<WGM01) | (1<<CS02) | (1<<CS00)	;
	OCR0 = OCR0_register	;
	temp = TIMSK	;
	temp |= (1<<OCIE0)	;
	TIMSK = temp	;
}

void Init_mbi_ports()
{
	unsigned char ports_temp	;
	
	ports_temp = LED_DRIVER_DDR	;
	ports_temp |= (1<<MBI_CLK) | (1<<MBI_LE) | (1<<MBI_OE) | (1<<MBI_SDI_H) | (1<<MBI_SDI_L)	;
	LED_DRIVER_DDR = ports_temp	;
	
	ports_temp = ANODE_DDR	;
	ports_temp |= (1<<DIGIT0_ANODE) | (1<<DIGIT1_ANODE) | (1<<DIGIT2_ANODE) | (1<<DIGIT3_ANODE) | (1<<DIGIT4_ANODE)	;
	ANODE_DDR = ports_temp	;
}

void Timer2_ON()
{
	unsigned char int_state	;
	
	TCCR2 = (1<<CS20) | (1<<WGM21)	;
	OCR2 = OCR2_register	;
	int_state = TIMSK	;
	int_state = int_state | (1<<OCIE2)	;
	TIMSK = int_state	;
}


void Timer2_OFF()
{	
	TCCR2 = 0	;
}



void Change_digit(unsigned char sensors_num)
{
	unsigned char port_stack	;
	
	dig_num++	;
	if (dig_num > 4)
	{
		dig_num = 0	;
	}
	
	if ((dev_num + 2) <= sensors_num)
	{
		output_high = led_codes[dev_num].dig_code[dig_num]	;
		output_low = led_codes[dev_num + 1].dig_code[dig_num]	;
	} 
	else
	{
		output_high = led_codes[dev_num].dig_code[dig_num]	;
		output_low = 0	;		
	}
	
	//счётчик 10 секунд
	ten_seconds_counter++	;
	if (ten_seconds_counter > TEN_SECONDS_TOP)
	{
		ten_sec_flag = 0xFF	;
		ten_seconds_counter = 0	;
		dev_num = dev_num + 2	;
		if (dev_num >= sensors_num)
		{
			dev_num = 0	;
		}
	}
	
	//счётчик 1-ой секунды
	one_second_counter++	;
	if (one_second_counter > ONE_SECOND_TOP)
	{
		one_sec_flag = 0xFF	;
		one_second_counter = 0	;
	}
	
	//моргание светодиодом при критическом заполнении памяти и индикация включения кнопки.
	led_counter++	;
	if (led_counter >= led_pulse_period) {
		led_counter = 0	;
	} else {
		if (led_counter < led_pulse_duration) {
			out_flag = 0xFF	;
		} else {
			out_flag = 0	;
		}
	}
	
	//включение/выключение светодиода				
	led_flag = out_flag | in_flag	;
	if (led_flag)
	{
		unsigned char temp	;
		temp = PORTC	;
		temp |= (1<<LED)	;
		PORTC = temp	;
	}
	else
	{
		unsigned char temp	;
		temp = PORTC	;
		temp &= (~(1<<LED))	;
		PORTC = temp	;
	}
	
	mbi_clk_counter = 0	;
		
	//вывод сигнала на м/схему mbi5026
	port_stack = LED_DRIVER_PORT	;
	port_stack |= (1<<MBI_OE)	;				
	LED_DRIVER_PORT = port_stack	;
	
	//избавиться от эффекта тени
	port_stack = ANODE_PORT	;
	//обнулить все биты состояния анода, а остальные установить; 
	port_stack &= (~((1<<DIGIT0_ANODE) | (1<<DIGIT1_ANODE) | (1<<DIGIT2_ANODE) | (1<<DIGIT3_ANODE) | (1<<DIGIT4_ANODE)))	;
	ANODE_PORT = port_stack	;	
	Timer2_ON()	;									//запуск вывода сигнала на драйвер дисплея
}

//генерация тактового сигнала для драйвера дисплея и выдача побитно данных из регистра output_high и output_low 
void MBISignalGenerator(void)
{
	unsigned char port_stack	;
	
	if (mbi_clk_counter < 32)
	{
		if ((mbi_clk_counter & 0x01) == 0)
		{
			//если счетчик тактового сигнала четный происходит обнуление линии clk и сдвиг на выход старшего бита из регистров output_high и output_low
			port_stack = LED_DRIVER_PORT	;
			port_stack &= ~(1<<MBI_CLK)	;
			LED_DRIVER_PORT = port_stack	;
			
			if ((output_high & 0x8000) == 0)					
			{
				port_stack = LED_DRIVER_PORT	;
				port_stack &= ~(1<<MBI_SDI_H)	;
				LED_DRIVER_PORT = port_stack	;				
			} 
			else
			{				
				port_stack = LED_DRIVER_PORT	;
				port_stack |= (1<<MBI_SDI_H)	;
				LED_DRIVER_PORT = port_stack	;
			}
			
			if ((output_low & 0x8000) == 0)
			{
				port_stack = LED_DRIVER_PORT	;
				port_stack &= ~(1<<MBI_SDI_L)	;
				LED_DRIVER_PORT = port_stack	;				
			} 
			else
			{				
				port_stack = LED_DRIVER_PORT	;
				port_stack |= (1<<MBI_SDI_L)	;
				LED_DRIVER_PORT = port_stack	;
			}
			output_high = output_high << 1	;
			output_low = output_low << 1	;
		} 
		else
		{
			//если счетчик тактового сигнала нечетный происходит установка линии clk и сдвиг на выход старшего бита из регистров output_high и output_low
			port_stack = LED_DRIVER_PORT	;
			port_stack |= (1<<MBI_CLK)	;
			LED_DRIVER_PORT = port_stack	;
		}
	} 
	else if(mbi_clk_counter == 32)
	{
		port_stack = LED_DRIVER_PORT	;
		port_stack &= ~((1<<MBI_SDI_L) | (1<<MBI_SDI_H) | (1<<MBI_CLK))	;
		LED_DRIVER_PORT = port_stack	;
	}
	else if(mbi_clk_counter == 33)
	{
		port_stack = LED_DRIVER_PORT	;
		port_stack |= (1<<MBI_LE)	;
		LED_DRIVER_PORT = port_stack	;		
	}
	else if(mbi_clk_counter == 34)
	{
		port_stack = LED_DRIVER_PORT	;
		port_stack &= ~(1<<MBI_LE)	;
		LED_DRIVER_PORT = port_stack	;		
	}
	else if(mbi_clk_counter == 35)
	{
		port_stack = LED_DRIVER_PORT	;
		port_stack &= ~(1<<MBI_OE)	;
		LED_DRIVER_PORT = port_stack	;
	}
	else
	{
		mbi_clk_counter = 0	;
		//вывод сигнала на анод
		port_stack = ANODE_PORT	;
		//сначала обнулить все биты состояния анода, а остальные установить; затем - установить нужный бит и подать сигнал на вывод
		port_stack &= (~((1<<DIGIT0_ANODE) | (1<<DIGIT1_ANODE) | (1<<DIGIT2_ANODE) | (1<<DIGIT3_ANODE) | (1<<DIGIT4_ANODE)))	;
		port_stack = port_stack | led_anode_table[dig_num];
		ANODE_PORT = port_stack	;
		Timer2_OFF()	;
		return	;
	}
	mbi_clk_counter++	;
}
