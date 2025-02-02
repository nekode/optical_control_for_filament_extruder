#include <RotaryEncoder.h>
#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::FULL4WIRE, 6, 5, 7, 8); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
RotaryEncoder encoder(2, 3);
bool startflag = 1;
bool manual_mode = 0;
int current_speed = 250; // начальная скорость
const int delaycounterlimit = 350; // ограничивает скорость нарастания или спада оборотов протяжки. Больше значение - медленнее.
const int minspeed = 35; // минимальная скорость протяжки
const int maxspeed = 395; // максимальная скорость протяжки
int delaycounter = delaycounterlimit;
#define go_up 2
#define go_down 0
#define go_middle 1  
#define low_sensor 10
#define high_sensor 9
#define encoder_button 4
byte filament_direction = go_up;
boolean manual_pull = 0;
#define key_pressed 1
#define key_holded 2
#define keys_not_pressed 0
uint8_t key_data;


void setup()
{
  delay (3000);
  stepper.setMaxSpeed(500);
  stepper.setSpeed(250);  
  pinMode(encoder_button, INPUT);         //определяем тип порта (вход), кнопка
  pinMode(high_sensor, INPUT);  //определяем тип порта (вход), оптосенсор верхний
  pinMode(low_sensor, INPUT);   //определяем тип порта (вход), оптосенсор нижний
  pinMode(A1, OUTPUT);         //определяем тип порта (выход), светодиод
//  pinMode(A2, OUTPUT);         //определяем тип порта (выход), реле
//  digitalWrite (A2, HIGH);		// отключаем реле
  digitalWrite (A1, LOW);		// выключаем светодиод
  Serial.begin(9600);
  Serial.println("starting");
}

void loop()
{
  static int pos = 0;
  encoder.tick();
  int newPos = encoder.getPosition();
  key_data = get_key();  // вызываем функцию определения нажатия кнопок, присваивая возвращаемое ней значение переменной, которую далее будем использовать в коде
  if (key_data) // если значение, возвращённое функцией, отлично от нуля
	{
		if (key_data == key_holded)
			{
				manual_mode = !manual_mode;
			}
		else if (key_data == key_pressed)
			{
			if (manual_mode)
				{
					startflag = !(startflag);
				}
			else
				{
					current_speed = maxspeed;
					startflag = 1;
					filament_direction = go_up;
					goto mark3;					
				}
			}
	key_data = 0;  // обнуляем переменную функции кнопок для предотвращения ложных срабатываний далее по коду
	}
/*  
  if (!(digitalRead (encoder_button))) 
	{
		Serial.println("button");
		 if (pos != newPos) 
			{
				manual_mode = !manual_mode;
				delay (250);
			}
		else if (manual_mode)
			{
				startflag = !(startflag);
				delay (250);
			}
		else if (!manual_mode)
			{
				current_speed = maxspeed;
				startflag = 1;
				filament_direction = go_up;
				goto mark3;
			}
	}
*/	
  if (pos != newPos) 
  {
    Serial.print(newPos);
    Serial.println();
	if (manual_mode)
    {
		if (pos>newPos) {current_speed++;}
		else {current_speed--;}
		currentspeedlimit();
		stepper.setSpeed(current_speed);		
	}
    pos = newPos;
	Serial.print("speed=");
	Serial.print(current_speed);
	Serial.println();
  } 
  
  
  if (!manual_mode)
  {
  if (!(digitalRead(A1)))
   {
		digitalWrite (A1, HIGH);
   }
	if (digitalRead(low_sensor)) // сработал нижний датчик
		{
					filament_direction = go_down;
		}
	else if (digitalRead(high_sensor))	// сработал верхний датчик
		{
			filament_direction = go_middle;
		}
	else if ((!(digitalRead(low_sensor)) && (!(digitalRead(high_sensor)))))
		{
			if (filament_direction == go_middle)
				{
					filament_direction = go_up;
				}
		}
	if (filament_direction == go_down)
		{
			go_faster();
		}
	else if (filament_direction == go_up)
		{
			go_slower();
		}
	stepper.setSpeed(current_speed);	
  }
  else if (digitalRead(A1))
   {
		digitalWrite (A1, LOW);
   }
mark3:
switch_motor_state ();
if (startflag) 
	{
		stepper.runSpeed();
	}
}

void switch_motor_state()
{
if (startflag) 
	{
		stepper.enableOutputs();
	}
else 
	{ 
		stepper.disableOutputs();
	}
}

void currentspeedlimit()
{
if (current_speed>maxspeed) 
	{
		current_speed=maxspeed;
		filament_direction = go_up; // костыль. Иногда при несработке верхнего оптодатчика уходит в постоянную высокую скорость.
	}
else if (current_speed<minspeed) {current_speed=minspeed;}
}

void go_faster()
{
delaycounter--;
if (delaycounter <= 0)
	{
		delaycounter = delaycounterlimit;
		if (!startflag)
			{
				startflag = 1;
			}
		current_speed++;
		currentspeedlimit();
	}
}

void go_slower()
{
delaycounter--;
if (delaycounter <= 0)
	{
		delaycounter = delaycounterlimit;
		current_speed--;
		if (current_speed < minspeed)
			{
				startflag = 0;
			}
		currentspeedlimit();
	}	
}

byte get_key()
{
// версия 2 - значение возвращается только при отпускании кнопки (как для короткого нажатия, так и для удержания)
uint8_t trigger_push_hold_counter = 10; // задержка триггера кратковременного/длительного нажатия (проходов функции, умноженных на задержку)
static uint8_t val_button;
static uint32_t key_delay_millis;
if ((millis() - key_delay_millis) > 50) //обрабатываем нажатия инкрементом переменной только если после предыдущей обработки прошло не менее 50 миллисекунд
{
  if (!(PIND & (1 << PIND4))) {val_button++;} //нажатие 
  key_delay_millis = millis();
}
if ((val_button > 0) && (PIND & (1 << PIND4))) //если клавиша >>> отпущена, но перед этим была нажата 
{
  if (val_button <= trigger_push_hold_counter) {val_button = 0; return key_pressed;} //кратковременно - возвращаем 1
  else if (val_button > trigger_push_hold_counter) {val_button = 0; return key_holded;} //длительно - возвращаем 2
}
return 0; // если ни одна из кнопок не была нажата - возвращаем 0
}
