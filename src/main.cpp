#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <stdio.h>

void mensageminicial();
void mensagem_inicial_cartao();
void modo_leitura();
void modo_gravacao();
char* getUid(MFRC522&);
char* getBufferStr(byte*, int);
 
//Pinos Reset e SS módulo MFRC522
#define SS_PIN 21
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);
 
const int rs = 13, en = 12, d4 = 2, d5 = 4, d6 = 16, d7 = 17;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

#//define LED_BUILTIN 2
#define pino_botao_le 36
#define pino_botao_gr 39
 
MFRC522::MIFARE_Key key;
 
void setup()
{
	Serial.begin(115200);	 //Inicia a serial
	Serial.println("Configurando....");
	pinMode(pino_botao_le, INPUT);
	pinMode(pino_botao_gr, INPUT);
	
	SPI.begin();			//Inicia	SPI bus
	mfrc522.PCD_Init();	 //Inicia MFRC522
 
	//Inicializa o LCD 16x2
	lcd.begin(16, 2);
 
	//Prepara chave - padrao de fabrica = FFFFFFFFFFFFh
	for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

	const char* ssid = "ismael";
	const char* password = "leamsi123";

	// Conectar-se à rede Wi-Fi
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Conectando ao WiFi...");
	}
	Serial.println("Conectado à rede Wi-Fi");

	Serial.println("Configuração concluida");

	mensageminicial();
}
 
void loop()
{
	//Verifica se o botao modo leitura foi pressionado
	int modo_le = digitalRead(pino_botao_le);
	if (modo_le != 0)
	{
		lcd.clear();
		Serial.println("Modo leitura selecionado");
		lcd.setCursor(2, 0);
		lcd.print("Modo leitura");
		lcd.setCursor(3, 1);
		lcd.print("selecionado");
		while (digitalRead(pino_botao_le) == 1) {}
		delay(3000);
		modo_leitura();
	}
	//Verifica se o botao modo gravacao foi pressionado
	int modo_gr = digitalRead(pino_botao_gr);
	if (modo_gr != 0)
	{
		lcd.clear();
		Serial.println("Modo gravacao selecionado");
		lcd.setCursor(2, 0);
		lcd.print("Modo gravacao");
		lcd.setCursor(3, 1);
		lcd.print("selecionado");
		while (digitalRead(pino_botao_gr) == 1) {}
		delay(3000);
		modo_gravacao();
	}
}

void requisicao_post(String url, String params){
	Serial.println("Executando requisição....");
	// Configurar a solicitação POST
	HTTPClient http;
	http.begin(url);

	// Configurar os dados a serem enviados
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// Enviar a solicitação POST
	int httpResponseCode = http.POST(params);

	// Verificar a resposta do servidor
	if (httpResponseCode > 0) {
		Serial.print("Resposta do servidor: ");
		Serial.println(http.getString());
	} else {
		Serial.print("Erro na solicitação HTTP: ");
		Serial.println(httpResponseCode);
	}

	Serial.println("Finalizando...");
	// Libere os recursos
	http.end();
	Serial.println("Finalizado.");
}

void mensageminicial()
{
	Serial.println("Selecione o modo leitura ou gravacao...");
	Serial.println();
	lcd.clear();
	lcd.print("Selecione o modo");
	lcd.setCursor(0, 1);
	lcd.print("leitura/gravacao");
}
 
void mensagem_inicial_cartao()
{
	Serial.println("Aproxime o seu cartao do leitor...");
	lcd.clear();
	lcd.print(" Aproxime o seu");
	lcd.setCursor(0, 1);
	lcd.print("cartao do leitor");
}
 
void modo_leitura()
{
	mensagem_inicial_cartao();
	//Aguarda cartao
	while ( ! mfrc522.PICC_IsNewCardPresent())
	{
		delay(100);
	}
	if ( ! mfrc522.PICC_ReadCardSerial())
	{
		return;
	}
	//Mostra UID na serial
	Serial.print("UID da tag : ");
	String conteudo = "";
	byte letra;
	for (byte i = 0; i < mfrc522.uid.size; i++)
	{
		Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
		Serial.print(mfrc522.uid.uidByte[i], HEX);
		conteudo.concat(String(mfrc522.uid.uidByte[i]<0x10 ? " 0" : " "));
		conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
	}
	Serial.println();
 
	//Obtem os dados do setor 1, bloco 4 = Nome
	byte sector				 = 1;
	byte blockAddr			= 4;
	byte trailerBlock	 = 7;
	byte status;
	byte buffer[18];
	byte size = sizeof(buffer);
 
	//Autenticacao usando chave A
	status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
																	trailerBlock, &key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}
	status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Read() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
	}
	//Mostra os dados do nome no Serial Monitor e LCD
	lcd.clear();
	lcd.setCursor(0, 0);
	for (byte i = 1; i < 16; i++)
	{
		Serial.print(char(buffer[i]));
		lcd.write(char(buffer[i]));
	}
	Serial.println();
 
	//Obtem os dados do setor 0, bloco 1 = Sobrenome
	sector				 = 0;
	blockAddr			= 1;
	trailerBlock	 = 3;
 
	//Autenticacao usando chave A
	status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
																	trailerBlock, &key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK)
	{
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}
	status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
	if (status != MFRC522::STATUS_OK)
	{
		Serial.print(F("MIFARE_Read() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
	}
	//Mostra os dados do sobrenome no Serial Monitor e LCD
	 //Mostra os dados do sobrenome no Serial Monitor e LCD
	lcd.setCursor(0, 1);
	for (byte i = 0; i < 16; i++)
	{
		Serial.print(char(buffer[i]));
		lcd.write(char(buffer[i]));
	}
	Serial.println();
 
	// Halt PICC
	mfrc522.PICC_HaltA();
	// Stop encryption on PCD
	mfrc522.PCD_StopCrypto1();
	delay(3000);
	mensageminicial();
}
 
void modo_gravacao()
{
	mensagem_inicial_cartao();
	//Aguarda cartao
	while ( ! mfrc522.PICC_IsNewCardPresent()) {
		delay(100);
	}
	if ( ! mfrc522.PICC_ReadCardSerial())		return;
 
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
 
	byte buffer[34];
	byte block;
	byte status, len;
 
	Serial.setTimeout(20000L) ;
	Serial.println(F("Digite o UID do animal,em seguida o caractere #"));
	lcd.clear();
	lcd.print("Digite o UID");
	lcd.setCursor(0, 1);
	lcd.print(" + #");
	len = Serial.readBytesUntil('#', (char *) buffer, 30) ;
	for (byte i = len; i < 30; i++) buffer[i] = ' ';
 
	block = 1;
	//Serial.println(F("Autenticacao usando chave A..."));
	status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
																		block, &key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}
 
	//Grava no bloco 1
	status = mfrc522.MIFARE_Write(block, buffer, 16);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}
 
	block = 2;
	//Serial.println(F("Autenticacao usando chave A..."));
	status=mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
																		block, &key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("PCD_Authenticate() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}
 
	//Grava no bloco 2
	status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
	if (status != MFRC522::STATUS_OK) {
		Serial.print(F("MIFARE_Write() failed: "));
		Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
		return;
	}

	
	// char *params = "chave1=valor1&chave2=valor2";
	char *params = new char[180];
	for(int i = 0; i < 180; i++) params[i] = '\0';
	sprintf(
		params,
		"uid_brinco=%s&display_brinco=%s",
		getUid(mfrc522),
		getBufferStr(buffer, len)
	);
	requisicao_post("http://192.168.1.5:6754/teste", params);
 
	
	{
		Serial.println(F("Dados gravados com sucesso!"));
		Serial.println((const char *)buffer);
		lcd.clear();
		lcd.print("Gravacao OK!");
	}
 
	mfrc522.PICC_HaltA(); // Halt PICC
	mfrc522.PCD_StopCrypto1();	// Stop encryption on PCD
	delay(5000);
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

char* getBufferStr(byte* buffer, int len){
	char *str = new char[100];
	for(int i = 0; i < 100; i++) str[i] = '\0';
	strncpy(str, (char *)buffer, len);
	return str;
}