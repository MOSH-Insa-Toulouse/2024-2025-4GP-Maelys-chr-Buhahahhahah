//Config capteur graphite (CG) + potentiométre digitale
#include <SPI.h>
#define MCP_WRITE 0b00010001
#define PIN_CG A0
#define CS 10 
uint8_t step_de_pot_digit = 1;
float mesure=0;

void init_CG(uint16_t CSpin);
uint16_t mesure_CG(uint16_t analog_pin);
void write_SPI(uint16_t cmd, uint16_t data, uint8_t CSPin);

//Config LED
#define PIN_LED 9
void init_LED(int pin_led);
void led_on();
void led_off();

//Config module bluetooth hc-05
#include <SoftwareSerial.h>
#define rxPin 7                                             //Broche 7 en tant que RX, à raccorder sur TX du HC-05
#define txPin 8                                             //Broche 8 en tant que RX, à raccorder sur TX du HC-05
#define baudrate 9600
SoftwareSerial mySerial(rxPin ,txPin);                      //Définition du software serial
void init_HC05(uint8_t rx, uint8_t tx);



//Config encodeur rotatoire + menu
#define encoder0PinA  2                                     //CLK Output A Do not use other pin for clock as we are using uint16_terrupt
#define encoder0PinB  4                                     //DT Output B
#define Switch 5                                            //Switch connection if available
#define counter_max 4
uint16_t timer = 0;
uint16_t btn_timer=0;
uint16_t current_time = 0;
uint16_t interval = 100;
uint16_t btn_interval=150;
uint16_t delta=0;
uint16_t btn_delta=0;
volatile uint16_t encoder0Pos = 0;
uint8_t counter=0;
uint16_t btn_state=HIGH;
uint16_t selector=1;
void doEncoder();
void encoder_coutner();
void encoder_btn_checker();
void menu_activated(uint8_t count);
void encoder_init(uint16_t A,uint16_t B,uint16_t S);
char action_list[][11]={"Mes CG","Mes Flex","Conf. CG","Conf. Flex"};



//Config capteur Flex
#define flexPin  A1                                             // Entrée de capteur flex
#define VCC  5                                                  // Tension de sortie d'arduino
#define R_DIV  10000                                            // La résistance qui permet de faire un pont-diviseur de tension
uint16_t flatResistance = 22000;                                // La résistance de capteur flex sans flexion (0°)
uint16_t bendResistance = 37000;                                // La résistance de capteur flex à maximum de flexion (90°)
void init_capteur_flex(uint8_t pin_connectee);                  // Fonction qui permet d'initaliser le capteur, cad de configurer flatResistance et bendResistance
uint16_t mesure_capteur_flex(uint16_t pin_connectee);           // Fonction qui permet de mesurer l'angle de flexion
float Vflex = 0.0;
uint16_t Rflex = 0;


//Config de OLED
#include <Adafruit_SSD1306.h>
#define nombreDePixelsEnLargeur 128                             // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define nombreDePixelsEnHauteur 64                              // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define brocheResetOLED         -1                              // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define adresseI2CecranOLED     0x3C                            // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)
#define tailleDeCaractere       2                               // Definition de taille de caractere
#define position_X              0                               // Positionnement de curseur sur 20 dans les X
#define position_Y              25                              // Positionnement de curseur sur 25 dans les Y
void init_OLED();                                               // Permet de reinitializer OLED pour commencer de travailler avec
void print_OLED(char[]);                                        // Permet d'afficher un mot
Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED);



//Config general
uint8_t mes_counter = 0;
uint32_t mes_container = 0;


//***************************************************************************
//Fonctions générales
//***************************************************************************

void setup() {
  init_OLED();
  init_HC05(rxPin,txPin);
  encoder_init(encoder0PinA, encoder0PinB, Switch);
  init_LED(PIN_LED);
}

void loop() {
  current_time=millis(); 
  encoder_coutner();
  encoder_btn_checker();
  menu_selector(counter); 
}





//***************************************************************************
//Fonctions capteur graphite
//***************************************************************************

void init_CG(uint16_t CSpin){
  pinMode (CSpin, OUTPUT); //select pin output
  digitalWrite(CSpin, HIGH); //SPI chip disabled
  SPI.begin(); 
  write_SPI(MCP_WRITE, step_de_pot_digit, CSpin);
  mesure=mesure_CG(PIN_CG);
    mes_container=0;
    for(uint8_t i = 0; i<10; i++){
      mes_container=mes_container+mesure_CG(PIN_CG);
      delay(100);
    } 
    mes_container=mes_container/10; 
  while(mes_container>800){
    mes_container=0;
    for(uint8_t i = 0; i<10; i++){
      mes_container=mes_container+mesure_CG(PIN_CG);
      delay(100);
    } 
    mes_container=mes_container/10; 
    step_de_pot_digit++;
    write_SPI(MCP_WRITE, step_de_pot_digit, CSpin);
    delay(10);
  }
}
uint16_t mesure_CG(uint16_t analog_pin){
  return analogRead(analog_pin);
}

void write_SPI(uint16_t cmd, uint8_t data, uint16_t CSPin){
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0)); 
  digitalWrite(CSPin, LOW); // CS pin low to select chip
  SPI.transfer(cmd);        // Send command code
  SPI.transfer(data);       // Send associated value
  digitalWrite(CSPin, HIGH);// CS pin high to de-select chip
  SPI.endTransaction();
}



//***************************************************************************
//Fonctions module bluetooth
//***************************************************************************

void init_HC05(uint8_t rx, uint8_t tx){
  pinMode(rxPin,INPUT);
  pinMode(txPin,OUTPUT);
  mySerial.begin(baudrate);
}




//***************************************************************************
//Fonctions encodeur rotatoire + menu
//***************************************************************************

void encoder_init(uint16_t A,uint16_t B,uint16_t S){
  pinMode(A, INPUT); 
  digitalWrite(encoder0PinA, HIGH);       

  pinMode(B, INPUT); 
  digitalWrite(encoder0PinB, HIGH);

  pinMode(S, INPUT); 
  attachInterrupt(0, doEncoder, RISING); 
}

void doEncoder() {
  if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==HIGH) {
    encoder0Pos = 1;
  }
  else if (digitalRead(encoder0PinA)==HIGH && digitalRead(encoder0PinB)==LOW) {
    encoder0Pos = -1;
  }
}
void encoder_coutner(){
  if (encoder0Pos==1){
    delta=current_time-timer;
    if(delta > interval){
      counter++;
      timer=current_time;
      encoder0Pos=0;
    }
  }
  else if (encoder0Pos==-1){
    delta=current_time-timer;
    if(delta > interval){
      counter--;
      timer=current_time;
      encoder0Pos=0;
    }
  }
  else{
    timer=current_time;
  }
  if(counter>counter_max){
    counter=0;
  }
  else if(counter<0){
    counter=counter_max-1;
  }

}
void encoder_btn_checker(){
  btn_state = digitalRead(Switch);
  if (btn_state==LOW){
    btn_delta=current_time-btn_timer;
    if(btn_delta > btn_interval){
      selector=0;
      menu_activated(counter);

      btn_timer=current_time;
      btn_state=HIGH;
    }
  }
  else{
    btn_timer=current_time;
  }
}
void menu_activated(uint8_t count){
  switch (count) {
    case 0:
      if(mes_counter<5){
        led_on();
        mes_counter++;
      }
      else{
        mes_counter=0;
        led_off();
      } 
      print_OLED("CG mes");     
      delay(500); 
      print_OLED("3");     
      delay(500);    
      print_OLED("2");     
      delay(500);    
      print_OLED("1");     
      delay(500);
      mes_container=0;
      for(uint8_t i = 0; i<10; i++){
        mes_container=mes_container+mesure_CG(PIN_CG);
        delay(100);
      } 
      mes_container=mes_container/10;         
      mySerial.print(mes_container);       
      print_OLED("Pret");     
      delay(500); 
      counter--;
      selector=1;
    break;
    case 1:
      if(mes_counter<5){
        led_on();
        mes_counter++;
      }
      else{
        mes_counter=0;
        led_off();
      } 
      print_OLED("Flex mes");
      delay(500); 
      print_OLED("3");     
      delay(500);    
      print_OLED("2");     
      delay(500);    
      print_OLED("1");     
      delay(500);          
      mes_container=0;
      for(uint8_t i = 0; i<10; i++){
        mes_container=mes_container+mesure_capteur_flex(flexPin);
        delay(100);
      }
      mes_container=mes_container/10;         
      mySerial.print(mes_container);       
      print_OLED("Pret");     
      delay(500); 
      counter--;
      selector=1;
    break;
    case 2:
      mes_counter=0;
      print_OLED("Conf. CG...");
      init_CG(CS);
      mySerial.print("Rcal");
      delay(500);
      mySerial.print(step_de_pot_digit*10000.0/255.0);
      delay(500);
      mySerial.print("R0_C");
      delay(500);
      mySerial.print(mesure_CG(PIN_CG));
      delay(500); 
      mySerial.print("MesG");  
      delay(500);                         
      print_OLED("Pret");     
      delay(500); 
      counter--;
      selector=1;
    break;
    case 3:
      mes_counter=0;
      print_OLED("Conf. Flex...");
      delay(500);
      init_capteur_flex(flexPin);
      mySerial.print("R0_F");
      delay(500);
      mySerial.print(flatResistance);
      delay(500); 
      mySerial.print("MesF");  
      delay(500);       
      print_OLED("Pret");     
      delay(500); 
      counter--;
      selector=1;
    break;
  }
}
void menu_selector(uint16_t count){
  if(selector==1){
    switch (count) {
      case 0:
        print_OLED(action_list[count]);  
      break;
      case 1:
        print_OLED(action_list[count]);  
      break;
      case 2:
        print_OLED(action_list[count]);  
      break;
      case 3:
        print_OLED(action_list[count]);  
      break;
    }
  }
}




//***************************************************************************
//Fonctions écran OLED
//***************************************************************************

void init_OLED(){
    if(!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))while(1);                                // Arrêt du programme (boucle infinie) si échec d'initialisation
    ecranOLED.clearDisplay();                                                                               // Effaçage de l'uint16_tégralité du buffer
    ecranOLED.setTextSize(tailleDeCaractere);                                                               // Taille des caractères 
    ecranOLED.setCursor(position_X, position_Y);                                                            // Déplacement du curseur en position
    ecranOLED.setTextColor(SSD1306_WHITE);                                                                  // Couleur du texte, et couleur du fond
    ecranOLED.display();  
}
void print_OLED(char message[]){
    ecranOLED.clearDisplay(); 
    ecranOLED.setCursor(position_X, position_Y);                                  
    ecranOLED.write(message);
    ecranOLED.display();  
}




//***************************************************************************
//Fonctions capteur flex
//***************************************************************************

void init_capteur_flex(uint8_t pin_connectee){                                  // Fonction qui permet d'initaliser le capteur, cad de configurer flatResistance et bendResistance
  Rflex = R_DIV*(VCC/(analogRead(pin_connectee)*5.0/1023.0)-1.0);
  flatResistance=Rflex;
}
uint16_t mesure_capteur_flex(uint16_t pin_connectee){                           // Fonction qui permet de mesurer l'angle de flexion
  Rflex = R_DIV*(VCC/(analogRead(pin_connectee)*5.0/1023.0)-1.0);
  Rflex = abs(Rflex);
  return Rflex;
}

//***************************************************************************
//Fonctions de LED
//***************************************************************************

void init_LED(int pin_led){
  pinMode(pin_led, OUTPUT);
}
void led_on(){
  digitalWrite(PIN_LED, HIGH);
}
void led_off(){
  digitalWrite(PIN_LED, LOW);
}