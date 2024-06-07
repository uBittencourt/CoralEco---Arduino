// BIBLIOTECAS PARA LEITURA DA TEMPERATURA
#include <OneWire.h>
#include <DallasTemperature.h>

// BIBLIOTECA PARA DISPLAY I2C
#include <LiquidCrystal_I2C.h>

// BIBLIOTECA PARA O RELÓGIO EM TEMPO REAL
#include <RTClib.h>

// BIBLIOTECA PARA MEMÓRIA EEPROM
#include <EEPROM.h>

// DEFININDO PINO DO SENSOR
#define ONE_WIRE_BUS 2 

// DEFININDO PINO DOS LEDS
#define ledGreen 3
#define ledRed 4

// AJUSTANDOO FUSO HORÁRIO 
#define UTC_OFFSET -3

// OPÇÃO PARA ATIVAR A LEITURA DO LOG 
#define LOG_OPTION 1    

// INICIALIZANDO OBJETOS PARA COMUNICAÇÃO E UTILIZAÇÃO DO SENSOR
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// INICIALIZANDO OBJETO PARA COMUNICAÇÃO COM DISPLAY
LiquidCrystal_I2C lcd(0x27, 16, 2);

// INICIALIZANDO OBJETO PARA UTULIZAÇÃO DO RELÓGIO
RTC_DS1307 RTC;

// CONFIGURAÇÕES DA EEPROM
const int maxRecords = 100;
const int recordSize = 6; // TAMANHO DO REGISTRO EM BYTES
int startAddress = 0;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;

int lastLoggedMinute = -1;

// TRIGGER DE TEMPERATURA
float trigger_min = 18.0; 
float trigger_max = 30.0; 

void setup() {
  // INICIALIZANDO COMUNICAÇÃO SERIAL
  Serial.begin(9600); 

  // INICIALIZANDO O SENSOR
  sensors.begin();

  // INICIALIZANDO O RELÓGIO E AJUSTANDO SUA DATA E HORÁRIO
  RTC.begin();
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // INICIALIZANDO DISPLAY E LIGANDO O BACKLIGHT
  lcd.init();
  lcd.backlight();

  // INICIALIZANDO EEPROM
  EEPROM.begin();
  
  // DEFININDO PINOS LED COMO SAÍDA
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
}

void loop() {
  DateTime now = RTC.now();

  // CALCULANDO O DESLOCAMENTO DO FUSO HORÁRIO
  int offsetSeconds = UTC_OFFSET * 3600; 
  now = now.unixtime() + offsetSeconds; 

  // CONVERTENDO NOVO TEMPO PARA DATATIME
  DateTime adjustedTime = DateTime(now);

  // SOLICITANDO LEITURA DA TEMPERATURA 
  sensors.requestTemperatures();  
  
  // CHAMANDO FUNÇÃO PARA APRESENTAR EEPROM
  if (LOG_OPTION) get_log();

  if (adjustedTime.minute() != lastLoggedMinute) {
    lastLoggedMinute = adjustedTime.minute();

    // OBTENDO TEMPERATURA EM CELSIUS
    float tempC = sensors.getTempCByIndex(0);

    lcd.clear();
    lcd.print("Ultima temp:");
    lcd.setCursor(0,1);
    lcd.print(" -> ");
    lcd.print(tempC);
    lcd.print(" C");

    // VERIFICANDO SE TEMPERATURA É MAIOR QUE 30°C
    if (tempC < trigger_min || tempC > trigger_max){
      int tempInt = (int)(tempC * 100);

      // ESCREVENDO DADOS NA EEPROM
      EEPROM.put(currentAddress, now.unixtime());
      EEPROM.put(currentAddress + 4, tempInt);

      // PULANDO PARA PRÓXIMO ENDEREÇO NA EEPROM
      getNextAddress();

      // CASO SEJA MAIOR O LED VERMELHO SE ACENDERÁ
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledRed, HIGH);
    } else {

      // CASO NÃO SEJA MAIOR O VERDE FICARÁ ACESO
      digitalWrite(ledRed, LOW);
      digitalWrite(ledGreen, HIGH);
    }
  }
  
  // lcd.setCursor(0,0);
  // lcd.print("DATA: ");
  // lcd.print(adjustedTime.day() < 10 ? "0" : ""); // Adiciona zero à esquerda se dia for menor que 10
  // lcd.print(adjustedTime.day()); 
  // lcd.print("/"); 
  // lcd.print(adjustedTime.month() < 10 ? "0" : ""); // Adiciona zero à esquerda se mês for menor que 10
  // lcd.print(adjustedTime.month()); 
  // lcd.print("/"); 
  // lcd.print(adjustedTime.year());
  // lcd.setCursor(0,1); 
  // lcd.print("HORA: ");
  // lcd.print(adjustedTime.hour() < 10 ? "0" : ""); // Adiciona zero à esquerda se hora for menor que 10
  // lcd.print(adjustedTime.hour()); 
  // lcd.print(":"); 
  // lcd.print(adjustedTime.minute() < 10 ? "0" : ""); // Adiciona zero à esquerda se minuto for menor que 10
  // lcd.print(adjustedTime.minute()); 
  // lcd.print(":"); 
  // lcd.print(adjustedTime.second() < 10 ? "0" : ""); // Adiciona zero à esquerda se segundo for menor que 10
  // lcd.print(adjustedTime.second());

  delay(1000);  
}

// FUNÇÃO PARA PULAR PARA PRÓXIMO ENDEREÇO NA EEPROM
void getNextAddress() {
    currentAddress += recordSize;
    if (currentAddress >= endAddress) {
      // VOLTA PARA O INÍCIO SE ATINGIR O LIMITE
      currentAddress = 0; 
    }
}

void get_log() {
    Serial.println("\nData stored in EEPROM:");
    Serial.println("Timestamp\t\tTemperature");

    for (int address = startAddress; address < endAddress; address += recordSize) {
        long timeStamp;
        int tempInt;

        // LER DADOS DA EEPROM
        EEPROM.get(address, timeStamp);
        EEPROM.get(address + 4, tempInt);

        // CONVERTENDO TEMPERATURA
        float tempC = tempInt / 100.0;

        // VERIFICAR SE OS DADOS SÃO VÁLIDOS PARA IMPRIMIR
        if (timeStamp != 0xFFFFFFFF) { // 0xFFFFFFFF é o valor padrão de uma EEPROM não inicializada
            //Serial.print(timeStamp);
            DateTime dt = DateTime(timeStamp);
            Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
            Serial.print("\t");
            Serial.print(tempC);
            Serial.println(" °C");
        }
    }
}
