#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <stdio.h>
#include <functional>
#include <ESPAsyncWebServer.h>


void mensageminicial();
void mensagem_inicial_cartao();
void menssagem_timeout();
char* ler();
char* getUid(MFRC522&);
 
//Pinos Reset e SS módulo MFRC522
#define SS_PIN 21
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
 
const int rs = 13, en = 12, d4 = 2, d5 = 4, d6 = 16, d7 = 17;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

AsyncWebServer server(80);

MFRC522::MIFARE_Key key;

#define TIMEOUT_TIME 10000 //10 segundos de timeout

void* tmp_buf;

bool timeout = false;
int timeout_counter = 0;

bool go_read_tag = false;
bool tag_already_read = false;
char* uid_tag = NULL;

bool show_timeout_message = false;

void setup() {
	Serial.begin(115200);	 //Inicia a serial
	Serial.println("Configurando....");

    tmp_buf = new char[512];
    memset(tmp_buf, 0, 512);
	
	SPI.begin(); //Inicia SPI bus
	mfrc522.PCD_Init();	 //Inicia MFRC522
 
	//Inicializa o LCD 16x2
	lcd.begin(16, 2);
 
	//Prepara chave - padrao de fabrica = FFFFFFFFFFFFh
	for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

	const char* ssid = "ismaelc";
	const char* password = "leamsi123";

	// Conectar-se à rede Wi-Fi
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Conectando ao WiFi...");
		lcd.clear();
		lcd.print("Aguarde...");
	}


	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(timeout){
            tag_already_read = false;
            timeout = false;
            timeout_counter = 0;

            AsyncWebServerResponse *response = request->beginResponse(408, "application/json", "{\"error\": \"TIMEOUT\",\"messsage\":\"Tempo limite atingido\"}");
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
			response->addHeader("Access-Control-Allow-Headers", "Content-Type");
			request->send(response);

            return;
        }

		if(tag_already_read){
            sprintf((char*)tmp_buf, "{\"message\":\"Lido com sucesso\",\"uid\":\"%s\"}", uid_tag);
			AsyncWebServerResponse *response = request->beginResponse(200, "application/json", (char*)tmp_buf);
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
			response->addHeader("Access-Control-Allow-Headers", "Content-Type");
			request->send(response);

            memset(tmp_buf, 0, 512);
            tag_already_read = false;
            delete[] uid_tag;
            uid_tag = NULL;
		}
		else{
            go_read_tag = true;
			AsyncWebServerResponse *response = request->beginResponse(202, "application/json", "{\"message\":\"Aguardando TAG...\",\"status\":\"WAITNG\"}");
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
			response->addHeader("Access-Control-Allow-Headers", "Content-Type");
            response->addHeader("Retry-After", "1");
            response->addHeader("Location", "/");
			request->send(response);
		}
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
    if(go_read_tag){
        uid_tag = ler();
        tag_already_read = true;
        go_read_tag = false;
        mensageminicial();
    }

    if(show_timeout_message){
        menssagem_timeout();
        show_timeout_message = false;
    }
}

void mensageminicial() {
	lcd.clear();
	lcd.print("Aguardando");
	lcd.setCursor(0, 1);
	lcd.print("operacoes...");
}
 
void mensagem_inicial_cartao() {
	Serial.println("Aproxime o seu cartao do leitor...");
	lcd.clear();
	lcd.print(" Aproxime o seu");
	lcd.setCursor(0, 1);
	lcd.print("cartao do leitor");
}

void menssagem_timeout() {
    Serial.println("Tempo limite atingido...\n");
}

char* ler() {
	mensagem_inicial_cartao();
	//Aguarda cartao
	while ( ! mfrc522.PICC_IsNewCardPresent()) {
        if(timeout_counter >= TIMEOUT_TIME){
            timeout = true;
            show_timeout_message = true;
            return NULL;
        }
		delay(400);
        timeout_counter += 400;
		Serial.println("Aguardando");
	}
	if ( ! mfrc522.PICC_ReadCardSerial()) return NULL;
 
	//Mostra UID na serial
	Serial.print(F("UID do Cartao: "));		//Dump UID
	for (byte i = 0; i < mfrc522.uid.size; i++)
	{
		Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
		Serial.print(mfrc522.uid.uidByte[i], HEX);
	}
	//Mostra o tipo do cartao
	Serial.print(F("\nTipo do PICC: "));
	byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
	Serial.println(mfrc522.PICC_GetTypeName((MFRC522::PICC_Type)piccType));

	return getUid(mfrc522);
}

char* getUid(MFRC522 &rf){
    int stringSize = rf.uid.size*2 + 1;
	char* uidString = new char[stringSize]; // +1 para o caractere nulo de terminação

	// Limpar a string para garantir que ela esteja vazia antes de preenchê-la
	memset(uidString, 0, stringSize);

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