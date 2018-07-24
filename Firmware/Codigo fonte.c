//Linhas(Tamanho da memoria 8 linhas = 64bytes)
#define iniFlash          (signed int)0x7EC0
#define finFlash          (signed int)0x7FC0
//Inicio e fim do Buffer de 64 bytes
#define iniArrayEvento    0
#define finArrayEvento    63
//Linhas dados da tabela Time[] RTC
#define time_seg         0
#define time_min         1
#define time_hora        2
#define time_dow         3
#define time_dia         4
#define time_mes         5
#define time_ano         6
//Sensibilidade do loop em ms
#define sensLoop         200

typedef short int BYTE;
// ************PARAMENTROS DO BLOCO FIXO******************
BYTE blocoFixo[64];
BYTE blocoFixoID[8] = {0xEE,0xDD,0xCC,0xBB,0xAA,0xBB,0xCC,0xDD}; //ID do bloco
signed int endIniLog, endFinLog, endBlocoFixo, endEvento;
signed int dataLoggerID;
//****************PARAMETROS PARA RTC********************
BYTE time[7];
// *******************************************************

BYTE evento[64];     //Array para Tabela Evento 64bytes
BYTE eventoIndex;     //Index para Tabela Evento 64bytes
int contadorEvento;   //Contagem de eventos(15bits = 32768 Eventos)
BYTE tipoEvento;         //Tipo do evento:   1 = ENTRADA    0 = SAIDA
int db; //Variavel para Debug
BYTE limpaPORTB; // Limpa mismatch no PORTB-LATB
BYTE trataInt = 0; //Hibilita tratamento de evento
unsigned char dadoBT[4]; // Codigo dado recebido pela serial

// ****************FUNCOES PARA MEMORIA********************

unsigned short int procBlocoFixoID(){
  unsigned int i,j;
  unsigned short int ok = 0;
  //Procurar blocoFixoID na flash
  for(i=iniFlash;i<=finFlash;i+=64){  // varredura na flash de 64 em 64 bytes
    FLASH_Read_N_Bytes(i,blocoFixo,sizeof(blocoFixo));
    ok = 0xFF;                      // Setar Ok e ...
    for(j=0;j<8;j++){
      if(blocoFixo[j] != blocoFixoID[j]){  // Se houver diferença
            ok = 0; break; //       Limpa Ok e vai para o proximo bloco de 64bytes
        }
      }
      // Se Ok verdadeiro, salva o endereco do Bloco fixo e finaliza varredura na flash
      if(ok) {endBlocoFixo = i; break;}
    }
    Delay_us(10);
    return ok;
}

void trata_memoria(){
  int i;
  //Procurar blocoFixoID na flash

  if(procBlocoFixoID())    //----------------------------Se encontrar...
  {
    //Le da flash e atualiza array do bloco fixo
    FLASH_Read_N_Bytes(endBlocoFixo,&blocoFixo,sizeof(blocoFixo));
    Delay_us(10);
    //Converter dados do endBlocoFixo e copiar para as variaveis correspondentes
    endIniLog = (signed int)((((blocoFixo[10]&0xFF)<<8)&0xFF00) | (blocoFixo[11]&0xFF));
    endFinLog = (signed int)((((blocoFixo[12]&0xFF)<<8)&0xFF00) | (blocoFixo[13]&0xFF));
    dataLoggerID =(signed int)((((blocoFixo[14]&0xFF)<<8)&0xFF00) | (blocoFixo[15]&0xFF));
    contadorEvento =(int)((((blocoFixo[16]&0xFF)<<8)&0xFF00) | (blocoFixo[17]&0xFF));
    eventoIndex = blocoFixo[18]&0xFF;

    //Le da flash e atualiza array de dados Evento
    endEvento = endBlocoFixo-64;
    FLASH_Read_N_Bytes(endEvento,evento,sizeof(evento));

  }else{                //----------------------------Se nao encontrar...
    //Inicializar Variaveis do blocoFixo
    endBlocoFixo =(signed int)iniFlash+64;
    endEvento = (signed int)iniFlash;
    endIniLog = iniFlash;
    endFinLog = iniFlash;
    dataLoggerID = 0;
    contadorEvento = 0;
    eventoIndex = 0;

    //Converte dados das variaveis e copia para o array bloco fixo
    for(i=0;i<8;i++){
      blocoFixo[i] =  blocoFixoID[i];
    }
    blocoFixo[8] = (BYTE)((endBlocoFixo >> 8) & 0xFF);
    blocoFixo[9] = (BYTE)(endBlocoFixo & 0xFF);
    blocoFixo[10] = (BYTE)((endIniLog >> 8) & 0xFF);
    blocoFixo[11] = (BYTE)(endIniLog & 0xFF);
    blocoFixo[12] = (BYTE)((endFinLog >> 8) & 0xFF);
    blocoFixo[13] = (BYTE)(endFinLog & 0xFF);
    blocoFixo[14] = (BYTE)((dataLoggerID >> 8) & 0xFF);
    blocoFixo[15] = (BYTE)(dataLoggerID & 0xFF);
    blocoFixo[16] = (BYTE)((contadorEvento >> 8) & 0xFF);
    blocoFixo[17] = (BYTE)(contadorEvento & 0xFF);
    blocoFixo[18] = eventoIndex;

    //Escreve array do bloco fixo na flash
    FLASH_Erase_Write_64(endBlocoFixo, blocoFixo);
    Delay_us(10);
    
    for(i=0;i<64;i++){
      evento[i]=1;
    }
    evento[7]=0;
    for(i=iniFlash;i<finFlash;i+=64){
      FLASH_Erase_Write_64(i, evento);
      Delay_us(10);
    }
  }

}

//****************FUNCOES PARA BLUETOOTH********************
void leBT(unsigned char *leString, int numBytesLe){
  int a;
  char limpaBuffer;
  for(a=0;a<numBytesLe;a++)
  {
    if(PIR1.RCIF)
    {
      *leString = UART1_Read();
      PIR1.RCIF=0;
      Delay_ms(20);
      leString++;
    }
  }
  while(PIR1.RCIF)
  {
    limpaBuffer = UART1_Read();
    PIR1.RCIF=0;
    Delay_ms(10);
  }
}

void escreveBT(unsigned char *escString, int numBytesEsc){
  int i;
  for(i=0;i<numBytesEsc;i++)
  {
    UART1_Write(*escString);
    Delay_ms(30);
    escString++;
  }
   //UART1_Write('\n');
}


BYTE bin2bcd(BYTE binary_value){
BYTE temp;
BYTE retval;
temp = binary_value;
retval = 0;
while(1)
{
// Get the tens digit by doing multiple subtraction
// of 10 from the binary value.
if(temp >= 10)
{
temp -= 10;
retval += 0x10;
}
else // Get the ones digit by adding the remainder.
{
retval += temp;
break;
}
}
return(retval);
}

// Input range - 00 to 99.
BYTE bcd2bin(BYTE bcd_value){
BYTE temp;
temp = bcd_value;
// Shifting upper digit right by 1 is same as multiplying by 8.
temp >>= 1;
// Isolate the bits for the upper digit.
temp &= 0x78;
// Now return: (Tens * 8) + (Tens * 2) + Ones
return(temp + (temp >> 2) + (bcd_value & 0x0f));
}

void inicializaRTC(){
 int i;
 //----------------------- Leitura de inicializacao -----------------------//
 I2C1_Init(100000);   //Inicializa I2C em 100Khz
 I2C1_Start();        // Inicia comunicação I2C
 I2C1_Wr(0xD0);       //Escrita
 I2C1_Wr(0x00);       //End. Inicial     = 0x00
 I2C1_Repeated_Start();        // Reinicia comunicação I2C
 I2C1_Wr(0xD1);       //Leitura
 time[0] = bcd2bin(I2C1_Rd(1)&0x7F);
 time[1] = bcd2bin(I2C1_Rd(1)&0x7F);
 time[2] = bcd2bin(I2C1_Rd(1)&0x3F);
 time[3] = bcd2bin(I2C1_Rd(1)&0x07);
 time[4] = bcd2bin(I2C1_Rd(1)&0x3F);
 time[5] = bcd2bin(I2C1_Rd(1)&0x1F);
 time[6] = bcd2bin(I2C1_Rd(0)); // Le valor do endereço 6 e AK = 0
 I2C1_Stop();         //Finaliza comunicação
 Delay_us(10);
 //----------------------- Escrita de inicializacao -----------------------//
 for(i=0;i<=6;i++){
  time[i] = bin2bcd(time[i]);    //Converte valores em time[] para BCD
 }
 time[0] &= 0x7F;     //Liga RTC (Habilita clock do ds1307)
 time[2] &= 0x3F;     //Seleciona modo 24hs

 I2C1_Start();         //Inicia comunicação I2C
 I2C1_Wr(0xD0);        //Escrita
 I2C1_Wr(0x00);        //End.  inicial      = 0x00
 for(i=0;i<=6;i++){
  I2C1_Wr(time[i]);    //Escreve valores de time[] nos endereços de 0 a 6
 }
 I2C1_Repeated_Start();         //Reinicia comunicação I2C
 I2C1_Wr(0xD0);        //Escrita
 I2C1_Wr(0x07);        //End.  inicial      = 0x07 (Control Register)
 I2C1_Wr(0x00);        //Disable squarewave output and clear pin
 I2C1_Stop();          //Finaliza Comunicacao I2C
 Delay_us(10);
}

void escreveRTC(){
 int i;
 for(i=0;i<=6;i++){
 time[i] = bin2bcd(time[i]);    //Converte valores em time[] para BCD
 }
 time[0] &= 0x7F;     //(mantem) RTC Ligado (Habilita clock do ds1307)
 time[2] &= 0x3F;     //(mantem) modo 24hs

 I2C1_Start();         //Inicia comunicação I2C
 I2C1_Wr(0xD0);        //Escrita
 I2C1_Wr(0x00);        //End.  inicial      = 0x00
 for(i=0;i<=6;i++){
  I2C1_Wr(time[i]);    //Escreve valores nos endereços de 0 a 6
 }
 I2C1_Stop();          //Finaliza Comunicacao I2C
 Delay_us(10);
}

void leRTC(){
 I2C1_Start();        // Inicia comunicação I2C
 I2C1_Wr(0xD0);       //Escrita
 I2C1_Wr(0x00);       //End. Inicial     = 0x00
 I2C1_Repeated_Start();        // Reinicia comunicação I2C
 I2C1_Wr(0xD1);       //Leitura
 time[0] = bcd2bin(I2C1_Rd(1)&0x7F);
 time[1] = bcd2bin(I2C1_Rd(1)&0x7F);
 time[2] = bcd2bin(I2C1_Rd(1)&0x3F);
 time[3] = bcd2bin(I2C1_Rd(1)&0x07);
 time[4] = bcd2bin(I2C1_Rd(1)&0x3F);
 time[5] = bcd2bin(I2C1_Rd(1)&0x1F);
 time[6] = bcd2bin(I2C1_Rd(0)); // Le valor do RTC endereço 6 e AK = 0
 I2C1_Stop();         //Finaliza comunicação
 Delay_us(10);
 }

 void trata_evento(){

    BYTE limpaArray[64],i;

    if(PORTB.F4==1)    //----------------------------RECEBE EVENTO I
    {
      LATA.F0 = 1;Delay_ms(500);LATA.F0 = 0;Delay_ms(500);
      tipoEvento=0;  // Define evento como saida
    }else if(PORTB.F5==1)  //----------------------  RECEBE EVENTO II
    {
      LATA.F0 = 1;Delay_ms(500);LATA.F0 = 0;Delay_ms(500);
      tipoEvento=1;  // Define evento como entrada
    }
    //********************************************************************************


    //Atualiza horario e data na tabela time[]
    leRTC();
    //Atualiza tabela Evento
    evento[eventoIndex] = time[time_ano];
    eventoIndex++;
    evento[eventoIndex] = time[time_mes];
    eventoIndex++;
    evento[eventoIndex] = time[time_dia];
    eventoIndex++;
    evento[eventoIndex] = time[time_hora];
    eventoIndex++;
    evento[eventoIndex] = time[time_min];
    eventoIndex++;
    evento[eventoIndex] = time[time_seg];
    eventoIndex++;
    evento[eventoIndex] = (((contadorEvento<<1)+ tipoEvento)&0xFF00)>>8; //Low
    eventoIndex++;
    evento[eventoIndex] = (((contadorEvento<<1)+ tipoEvento)&0x00FF);   //High

    //Atualiza dados do evento
    contadorEvento++;
    if(contadorEvento>32767){
      contadorEvento=0;
    }
    //********************************************************************************

    //Posiciona eventoIndex para proximo dado
    eventoIndex++;
    //Atualiza endFinLog
    endFinLog = endEvento + eventoIndex;

    // ********************* Buffer evento[] cheio ****************************
    if(eventoIndex > finArrayEvento){
      //Atualiza endereco para escrita do bloco fixo na flash)
      LATA.F0 = 1;Delay_ms(3000);LATA.F0 = 0;
      endBlocoFixo += 64;

    //Atualiza end. de inicio do Log
    if(endBlocoFixo==endIniLog){
      endIniLog = endBlocoFixo+64;
    }
    if(endIniLog >= finFlash){
      endIniLog = iniFlash;
    }

      if(endBlocoFixo>finFlash){     //Se bloco fixo chegou ao final da memoria...
        for(i=0;i<64;i++){           //Limpa ultimo endereco da flash...
          limpaArray[i]=0xFF;
          //Escreve array do bloco fixo na flash
          FLASH_Erase_Write_64((endBlocoFixo-64), limpaArray);
          Delay_us(10);
        }
        endBlocoFixo=iniFlash+64;      //Seta endereco do BF para inicio da flash
        if(endIniLog<endBlocoFixo){
          endIniLog = endBlocoFixo + 64; //Desloca endereco inicial do Log
        }
      }

      //Atualiza endereco para escrita do array Evento na flash
      endEvento = endBlocoFixo-64;

      //Retorna ao inicio do buffer
      eventoIndex=iniArrayEvento;
    }

    //Converte dados das variaveis para o array bloco fixo
    blocoFixo[8] = (BYTE)((endBlocoFixo >> 8) & 0xFF);
    blocoFixo[9] = (BYTE)(endBlocoFixo & 0xFF);
    blocoFixo[10] = (BYTE)((endIniLog >> 8) & 0xFF);
    blocoFixo[11] = (BYTE)(endIniLog & 0xFF);
    blocoFixo[12] = (BYTE)((endFinLog >> 8) & 0xFF);
    blocoFixo[13] = (BYTE)(endFinLog & 0xFF);
    blocoFixo[16] = (BYTE)((contadorEvento >> 8) & 0xFF);
    blocoFixo[17] = (BYTE)(contadorEvento & 0xFF);
    blocoFixo[18] = (BYTE)(eventoIndex & 0xFF);

    //Escreve array do bloco fixo na flash
    FLASH_Erase_Write_64(endBlocoFixo, blocoFixo);
    Delay_us(10);

    //Escreve dados do Evento na Flash
    FLASH_Erase_Write_64(endEvento, evento);
    Delay_us(10);

    //Finaliza tartamento do evento
    trataInt = 0;
}


BYTE ajustaTime(char *txt,int lenght, int dadoIndex, int min, int max){
  BYTE dadoLido, status;
  escreveBT(txt,lenght);
  UART1_Write('\n');
  trataInt = 0;
  while(trataInt==0);
  dadoLido =(BYTE)(atoi(&dadoBT));
  if((dadoLido>=min)&&(dadoLido<=max)){
    time[dadoIndex] = dadoLido;
    escreveBT("OK!",strlen("OK"));
    status=1;
  }else{
    escreveBT("Invalido",strlen("Invalido"));
    status=0;
  }
  UART1_Write('\n');
  return status;
}

 void interface(){
  signed int i,j,bufferInt;
  BYTE low,high;
  char buffer[7];
  //Strings para controle da interface
  unsigned char cmdLog[4] = {'l','o','g','\0'};
  unsigned char cmdHor[4] = {'h','o','r','\0'};
  unsigned char cmdAju[4] = {'a','j','u','\0'};
  unsigned char cmdFim[4] = {'f','i','m','\0'};
  unsigned char cabecalho[25]={"DATALOGGER ID:\tDTL"};
  unsigned char cabecalho1[]={"VERSAO:\t1.0"};
  unsigned char cabecalho3[]={"ANO\tMES\tDIA\tHORA\tMIN\tSEG\tID\tEVENTO"};

  LATA.F0 = 1;
  Delay_ms(500);
  LATA.F0=0;
  Delay_ms(500);

  //INTCON.RBIE = 0;    //Desabilita modo operacao

  if((memcmp(&dadoBT,&cmdLog,(strlen(&cmdLog)-1))==0)){
    PIR1.RCIE=0;
    IntToStrWithZeros(dataLoggerID, &buffer);
    strcat(&cabecalho, &buffer);
    escreveBT(&cabecalho, strlen(&cabecalho));
    UART1_Write('\n');
    Delay_us(100);
    escreveBT(&cabecalho1, strlen(&cabecalho1));
    UART1_Write('\n');
    Delay_us(100);
    escreveBT(&cabecalho3, strlen(&cabecalho3));
    UART1_Write('\n');
    Delay_us(100);
    for(i=endIniLog;i!=endFinLog;i+=8){
      if(i>=finFlash){
        i=iniFlash;
      }
      for(j=0;j<8;j++){
        IntToStrWithZeros((signed int)(FLASH_Read((signed int)(i+j))), buffer);
        if(j==6){ //ID
          high = (BYTE)(FLASH_Read((signed int)(i+j)));
          low  = (BYTE)(FLASH_Read((signed int)(i+j+1)));
          bufferInt = (signed int)(((high<<8)&0xFF00)|(low&0xFF));
          bufferInt &= 0xFE;
          IntToStrWithZeros(bufferInt, buffer);
          escreveBT(&buffer[1], 6);
        }else if(j==7){ //tipoEvento
          if (((FLASH_Read((signed int)(i+j)))&1)){
            escreveBT("ENTRADA",strlen("ENTRADA"));
          }else{
            escreveBT("SAIDA",strlen("SAIDA"));
          }
        }else{//Ano,mes,dia,hora,min,seg
          escreveBT(&buffer[4],3);
        }
        UART1_Write('\t');
      }
      Delay_us(100);
      UART1_Write('\n');
    }
    PIR1.RCIE=1;
  }else if((memcmp(&dadoBT,&cmdHor,(strlen(&cmdHor)-1))==0)){
    PIR1.RCIE=0;
    leRTC();
    //Converte e envia dia/mes/ano
    IntToStrWithZeros((signed int)time[time_dia],&buffer);
    escreveBT(&buffer[4],3);
    escreveBT("/",strlen("/"));
    IntToStrWithZeros((signed int)time[time_mes],&buffer);
    escreveBT(&buffer[4],3);
    escreveBT("/",strlen("/"));
    IntToStrWithZeros((signed int)time[time_ano],&buffer);
    escreveBT("20",strlen("20"));
    escreveBT(&buffer[4],3);
    escreveBT(" - ",strlen(" - "));
    //Converte e envia hora:min:seg
    IntToStrWithZeros((signed int)time[time_hora],&buffer);
    escreveBT(&buffer[4],3);
    escreveBT(":",strlen(":"));
    IntToStrWithZeros((signed int)time[time_min],&buffer);
    escreveBT(&buffer[4],3);
    escreveBT(":",strlen(":"));
    IntToStrWithZeros((signed int)time[time_seg],&buffer);
    escreveBT(&buffer[4],3);
    UART1_Write('\n');
    PIR1.RCIE=1;
  }else if((memcmp(&dadoBT,cmdAju,(strlen(&cmdAju)-1))==0)){
    do{
      escreveBT("Informe os dados solicitados:",strlen("Informe os dados solicitados:"));
      UART1_Write('\n');
      if(!ajustaTime("Dia(DD): ",strlen("Dia(DD): "), time_dia, 1, 31))break;
      if(!ajustaTime("Mes(MM): ",strlen("Mes(MM): "), time_mes, 1, 12))break;
      if(!ajustaTime("Ano(AA): ",strlen("Ano(AA): "), time_ano, 17, 99))break;
      if(!ajustaTime("Hora(hh) 00 a 23: ",strlen("Hora(hh) 00 a 23: "), time_hora, 0, 23))break;
      if(!ajustaTime("Minuto(mm): ",strlen("Minuto(mm): "), time_min, 0, 59))break;
      if(!ajustaTime("Segundo(ss): ",strlen("Segundo(ss): "), time_seg, 0, 59))break;
      escreveRTC();
      break;
    }while(1);
  }else if((memcmp(&dadoBT,cmdFim,(strlen(&cmdFim)-1))==0)){
    PIR1.RCIE=0;
    escreveBT("Fecha menu",strlen("Fecha menu"));
    UART1_Write('\n');
    PIR1.RCIE=1;
  }else{
    PIR1.RCIE=0;
    escreveBT(&cmdLog,strlen(&cmdLog));
    UART1_Write('\n');
    escreveBT(&cmdHor,strlen(&cmdHor));
    UART1_Write('\n');
    escreveBT(&cmdAju,strlen(&cmdAju));
    UART1_Write('\n');
    escreveBT(&cmdFim,strlen(&cmdFim));
    UART1_Write('\n');
    PIR1.RCIE=1;
  }
  trataInt = 0;
  //INTCON.RBIE = 1;   //Habilita modo operacao
}


//********************INTERRUPCOES****************************
void interrupt(){
  //***************TRATA SERIAL-BLUETOOTH**********************
  if(PIR1.RCIF==1){
    leBT(&dadoBT,3);
    dadoBT[3]='\0';
    trataInt = 1;
  }
  //***************TRATA EVENTOS I E II**********************
  if(INTCON.RBIF==1){
    limpaPORTB = PORTB; INTCON.RBIE=0;INTCON.RBIF=0;
    Delay_ms(sensLoop);
    if((PORTB.F4==1)||(PORTB.F5==1)){
         trataInt = 2;
       }else{
         limpaPORTB = PORTB;
         INTCON.RBIF=0; INTCON.RBIE=1; INTCON.RBIF=0;
       }
  }

}

//********************MAIN VOID****************************
void main(){
   unsigned int timer;
   ADCON1 = 0x0F; // SETA PORTA COMO DIGITAL
   CMCON = 0x07; // DESABILITA COMPARADORES
   PORTA.F0 = 0; TRISA.F0 = 0; // SETA RA0 COMO SAÍDA

   TRISC.F6 = 0; TRISC.F7 = 1;
   PIR1.RCIF=0;
   PIE1.RCIE = 1; PIE1.TXIE = 0;

   // HABILITA INTERRUPCOES GLOBAIS E PERIFERICAS E HABILITA INTERRUPCAO PORTB
   TRISB.F4 = 1; TRISB.F5 = 1;
   TRISB.F6 = 0; TRISB.F7 = 0;
   PORTB.F6 = 0; PORTB.F7 = 0;
   INTCON2.RBPU = 1; //DESLI. PULL-UPS
   INTCON = 0xC0; //Habilita Int. global e periferica
   limpaPORTB = PORTB;
   INTCON.RBIF = 0;
   INTCON.RBIE = 1;

   LATA.F0 = 1;
   inicializaRTC();
   Delay_ms(5000);

   UART1_Init(9600);               // Initialize UART module at 9600 bps
   Delay_ms(10);                  // Wait for UART module to stabilize

   trata_memoria();
   LATA.F0 = 0;

   while(1)
   {
      if(trataInt==1){
         interface();
      }
      if(trataInt==2){
         trata_evento();Delay_ms(2);
         while((PORTB.F4==1)||(PORTB.F5==1)){
           Delay_ms(10);
         }
         limpaPORTB = PORTB;
         INTCON.RBIF=0; INTCON.RBIE=1; INTCON.RBIF=0;
      }
      while(trataInt==0){
         Delay_ms(1);timer++;
         if(timer<5){LATA.F0 = 1;}
         if((timer>=50)&&(timer<=100)){LATA.F0 = 0;}
         if(timer>2000){timer=0;}
         //asm sleep;    //Entra em modo de baixo consumo
      }
   }

}
//************************************************************