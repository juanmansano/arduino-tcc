#include <DM02A.h>
#include <BH1750.h>
#include <Wire.h>

BH1750 sensorLuminosidadeInterno, sensorLuminosidadeExterno;
DM02A dimmer1(D7, D8);
int nivel_1, nivel_2;
int feedback_1, feedback_2;
const int pinoSensor = D6;
int leitura;
int LIMITE_LUMINOSIDADE = 100;
int desliga_luz = 0;
int lampadas_desligadas = 0;
float luminosidadeInterna, luminosidadeExterna;
int cortina_aberta = 0;

void setup()
{
  Serial.begin(9600);
  Wire.begin(D3, D4);
  sensorLuminosidadeInterno.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  sensorLuminosidadeExterno.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire);
  nivel_1 = 35;
  nivel_2 = 35;
  pinMode(pinoSensor, INPUT);
}

void loop()
{

  if (desliga_luz == 600)
  {
    desliga_lampadas();
  }
  luminosidadeInterna = sensorLuminosidadeInterno.readLightLevel();
  if (luminosidadeInterna <= LIMITE_LUMINOSIDADE - (LIMITE_LUMINOSIDADE * 0.10) && lampadas_desligadas == 0)
  {
    aumenta_dimmer();
  }
  else if (luminosidadeInterna >= LIMITE_LUMINOSIDADE + (LIMITE_LUMINOSIDADE * 0.10) && lampadas_desligadas == 0)
  {
    diminui_dimmer();
  }
  luminosidadeExterna = sensorLuminosidadeExterno.readLightLevel();
  if (luminosidadeExterna >= luminosidadeInterna && cortina_aberta == 0)
  {
    notifica_abertura();
  }
  else if (luminosidadeInterna >= luminosidadeExterna && cortina_aberta == 1)
  {
    Serial.println("Pode fechar a sua cortina");
    cortina_aberta = 0;
  }

  if (digitalRead(pinoSensor) == HIGH)
  {
    presenca();
  }
  Serial.println("[{'luminosidade interna' : '" + (String)luminosidadeInterna + "'}]");
  Serial.println("[{'luminosidade externa' : '" + (String)luminosidadeExterna + "'}]");
  atualiza_presenca();
}

void presenca()
{
  desliga_luz = 0;
  lampadas_desligadas = 0;
  Serial.println("Movimento Detectado!");
}

void aumenta_dimmer()
{
  if (nivel_1 < 70)
  {
    nivel_1 = nivel_1 + 2;
  }
  if (nivel_2 < 70)
  {
    nivel_2 = nivel_2 + 2;
  }
  envia_nivel_dimer();
}

void diminui_dimmer()
{
  nivel_1 = nivel_1 - 2;
  nivel_2 = nivel_2 - 2;
  envia_nivel_dimer();
}

void desliga_lampadas()
{
  nivel_1 = 0;
  nivel_2 = 0;
  envia_nivel_dimer();
  lampadas_desligadas = 1;
}

void envia_nivel_dimer()
{
  dimmer1.EnviaNivel(nivel_1, 0);
  dimmer1.EnviaNivel(nivel_2, 1);
}

void atualiza_presenca()
{
  desliga_luz = desliga_luz + 1;
  delay(500);
}

void notifica_abertura()
{
  Serial.println("Abra a sua cortina");
  cortina_aberta = 1;
}
