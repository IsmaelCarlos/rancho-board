#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <stdio.h>
#include <functional>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

void mensageminicial();
void mensagem_inicial_cartao();
void ler(void *params);
char* getUid(MFRC522&);
 
//Pinos Reset e SS módulo MFRC522
#define SS_PIN 21
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
 
const int rs = 13, en = 12, d4 = 2, d5 = 4, d6 = 16, d7 = 17;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

AsyncWebServer server(80);

MFRC522::MIFARE_Key key;

bool __created_task = false;
bool __read = false;
char* read_buffer;

TaskHandle_t longTaskHandle = NULL;
 
void setup()
{
	Serial.begin(115200);	 //Inicia a serial
	Serial.println("Configurando....");

	read_buffer = new char[512];
	for(int i = 0; i < 512; i++) read_buffer[i] = 0;
	
	SPI.begin(); //Inicia SPI bus
	mfrc522.PCD_Init();	 //Inicia MFRC522
 
	//Inicializa o LCD 16x2
	lcd.begin(16, 2);
 
	//Prepara chave - padrao de fabrica = FFFFFFFFFFFFh
	for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

	const char* ssid = "HFAS_2G";
	const char* password = "x-box 360";

	// Conectar-se à rede Wi-Fi
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Conectando ao WiFi...");
		lcd.clear();
		lcd.print("Aguarde...");
	}


	server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
		if(!__created_task){
			__created_task = true;
			xTaskCreate(
				ler,
				"leitura",
				2048,
				NULL,
				1,
				&longTaskHandle
			);
		}

		if(__read){
			AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", read_buffer);
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
			response->addHeader("Access-Control-Allow-Headers", "Content-Type");
			request->send(response);

			esp_restart();
		}
		else{
			AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "WAIT");
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
			response->addHeader("Access-Control-Allow-Headers", "Content-Type");
			request->send(response);
		}
	});

	server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
		response->addHeader("Access-Control-Allow-Origin", "*");
		response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
		response->addHeader("Access-Control-Allow-Headers", "Content-Type");
		request->send(response);
		esp_restart();
	});


	server.begin();


	Serial.println("Conectado a rede Wi-Fi");
	Serial.println("WiFi conectado");
	lcd.setCursor(2, 0);
	delay(3000);
	lcd.clear();
	Serial.println("Configuracao concluida");
	lcd.print("Configuracao");
	lcd.setCursor(0, 1);
	lcd.print("concluida...");
	delay(3000);
	mensageminicial();
}
 
void loop()
{
}

void mensageminicial()
{
	lcd.clear();
	lcd.print("Aguardando");
	lcd.setCursor(0, 1);
	lcd.print("operacoes...");
}
 
void mensagem_inicial_cartao()
{
	Serial.println("Aproxime o seu cartao do leitor...");
	lcd.clear();
	lcd.print(" Aproxime o seu");
	lcd.setCursor(0, 1);
	lcd.print("cartao do leitor");
}

void ler(void *params) {
	mensagem_inicial_cartao();
	//Aguarda cartao
	while ( ! mfrc522.PICC_IsNewCardPresent()) {
		delay(400);
		Serial.println("Aguardando");
		esp_task_wdt_reset();
	}
	if ( ! mfrc522.PICC_ReadCardSerial()) return;
 
	//Mostra UID na serial
	Serial.print(F("UID do Cartao: "));		//Dump UID
	for (byte i = 0; i < mfrc522.uid.size; i++)
	{
		Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
		Serial.print(mfrc522.uid.uidByte[i], HEX);
	}
	//Mostra o tipo do cartao
	Serial.print(F("nTipo do PICC: "));
	byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
	Serial.println(mfrc522.PICC_GetTypeName((MFRC522::PICC_Type)piccType));

	char* uid = getUid(mfrc522);
	__read = true;
	read_buffer = uid;
	__created_task = false;

	vTaskDelete(NULL);
	mensageminicial();
}

char* getUid(MFRC522 &rf){
	char *uidString = new char[rf.uid.size + 1]; // +1 para o caractere nulo de terminação

	// Limpar a string para garantir que ela esteja vazia antes de preenchê-la
	memset(uidString, 0, sizeof(uidString));

	// Converter cada byte do UID para sua representação hexadecimal e armazená-lo na string
	int index = 0;
	for (byte i = 0; i < rf.uid.size; i++) {
		// Converter o byte para sua representação hexadecimal
		char hex[3]; // 2 caracteres para o hexadecimal + 1 caractere nulo de terminação
		sprintf(hex, "%02X", rf.uid.uidByte[i]);
		
		// Concatenar o valor hexadecimal à string de UID
		strcat(uidString, hex);
		
		// Incrementar o índice pelo tamanho do valor hexadecimal (dois caracteres)
		index += 2;
	}

	return uidString;
}

