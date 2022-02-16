/*
 * RECEPTOR E TRANSMISSOR POR LORA COM ESP32 (PAGER LORA)
 * 
 * Autores: Lucas Viegas e Yuri Minar 
 * data: 03/10/2021
 */

#include <cctype>
#include <map>
#include <sstream>
#include "heltec.h"
#define BAND 868E6
#define MAX_MESSAGE_SIZE 46

std::map<std::string, char> mapa = {
  {".-",  'A'},
  {"-...", 'B'},
  {"-..", 'C'},
  {"-..", 'D'},
  {".", 'E'},
  {" ..-.", 'F'},
  {"--.", 'G'},
  {"....", 'H'},
  {"..", 'I'},
  {".---", 'J'},
  {"-.-", 'K'},
  {".-..", 'L'},
  {"--", 'M'},
  {"-.", 'N'},
  {"---", 'O'},
  {".--.", 'P'},
  {"--.-", 'Q'},
  {".-.", 'R'},
  {"...", 'S'},
  {"-", 'T'},
  {"..-", 'U'},
  {"...-", 'V'},
  {".--", 'W'},
  {"-..-", 'X'},
  {"-.--", 'Y'},
  {"--..", 'Z'},
  {"-----", '0'},
  {"·----", '1'},
  {"··---", '2'},
  {"···--", '3'},
  {"····-", '4'},
  {"·····", '5'},
  {"-····", '6'},
  {"--···", '7'},
  {"---··", '8'},
  {"----·", '9'},
  {"·--·-·", '@'},
  {"···-··-", '$'}
};

// Mapeia um caractere para seu valor correspondente em morse
std::string convert(const std::map<std::string, char> &tabela, char c) {
  for (auto item : tabela) {
    if (item.second == c)
      return item.first;
  }

  return "";
}

// Recebe uma mensagem completa demarcada por @ e $ (incluídos na string)
// Retorna o texto ao receber uma mensagem completa, senão retorna nulo
char *ReceberMorse()
{
  static char mensagem[MAX_MESSAGE_SIZE] = {0};
  static int contagem = 0;
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    while (LoRa.available() && contagem < MAX_MESSAGE_SIZE) {
      unsigned char simb = LoRa.read();
      mensagem[contagem] = simb;
      contagem++;
      if (simb == '@') {
        mensagem[0] = '@';
        contagem = 1;
      } else if (simb == '$') {
        return mensagem;
      }
    }
  }
  return 0;
}

// Exibe a mensagem previamente capturada por ReceberMorse e exibe na tela OLED
// Há uma bug em que o último caractere aparece dobrado no receptor. Por enquanto só descarta.
void ExibirMensagem(std::string msg) {
  msg = msg.substr(1, msg.size() - 3); // Descarta @ e $. Também descarta o espaço antes de $.
  std::istringstream saida{msg};

  std::string simb;
  std::string decodificado;
  while (!saida.eof()) {
      saida >> simb;
      auto encontrado = mapa.find(simb);
      if (encontrado != mapa.end()) {
          decodificado += encontrado->second;
      }
  }

  Heltec.display->clear();
  Heltec.display->drawString(63, 31, decodificado.c_str());
  Heltec.display->display();
}

// Codifica a mensagem em morse e envia
void EnviarMorse (const char *texto){
  LoRa.beginPacket();
  LoRa.print('@');
  while(*texto != 0){
    char caractere = *texto;
    if (caractere >= 'A' && caractere <= 'Z'){
      std::string simbolo = convert(mapa, caractere);
      LoRa.print(simbolo.c_str());
    } else if (caractere >= '0' && caractere <= '9'){
      std::string simbolo = convert(mapa, caractere);
      LoRa.print(simbolo.c_str());
    }

    LoRa.print(" ");
    caractere++;
    texto++;
  }
  LoRa.print('$');
  LoRa.endPacket();
}
 
void setup () {
  Heltec.begin(true, true, true, true, BAND);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  Heltec.display->clear();
  Heltec.display->drawString(63, 31, "Transmissor");
  Heltec.display->display();

  Serial.begin(115200);
}

void loop () {
  static char texto[64] = {0};
  static int posicao = 0;
  static bool enviar = false;
  // Recebe texto da serial para enviar por rádio
  while (Serial.available() && posicao < 64 - 1) {
    char c = std::toupper(Serial.read());
    if ((c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '\n') {
      Serial.println("Caractere invalido");
    } else if(c == '\n') {
      texto[posicao] = 0;
      enviar = true;
    } else {
      texto[posicao++] = c;
    }
  }

  // Envia o texto no buffer e reinicia o buffer
  if (enviar) {
    Heltec.display->clear();
    Heltec.display->drawString(63, 31, String("Enviando: ") + texto);
    Heltec.display->display();
    EnviarMorse(texto);
    enviar = false;
    posicao = 0;
  }

  // Recebe uma mensagem por rádio e exibe na tela OLED
  const char *msg = ReceberMorse();
  if (msg) {
    Serial.println("Recebida mensagem");
    ExibirMensagem(msg);
  }
}
