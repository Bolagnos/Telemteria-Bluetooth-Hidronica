/************************************************************/
/*    Codigo para control de modem Quectel UC20 y EC20	    */
/************************************************************/
#define W_ENABLE_GPRS    BIT3 // P1.3  // Control de paso a bajo consumo del m�dulo de GPRS (W-ENABLE)
#define ON_OFF_Modem_VCC BIT6 // P1.6  // Control para encendido general del modem  GPRS (Alimentaci�n, habilita relay de estado s�lido)
#define ON_OFF_GPRS BIT7      // P1.7  // Se�al de encendido/apagado del m�dem GPRS (por su pin de control PERST#)
#define BitFlag_ONGPRS BIT6   // P6.6  // Puerto de entrada para detecci�n de que el m�dem se encuentra encendido
//#define LED_GPRS_EQ BIT7      // P5.7  // Salida para indicador LED de encendido del m�dem GPRS

#include "CDevice.h"
//#include "string.h"
#include "string"

const char*  APN_Providers[]={
              "internet.itelcel.com","webgprs","webgprs2002","TELCEL",       // TELCEL
              "internet.movistar.mx","movistar","movistar","MOVISTAR",       // MOVISTAR
              "web.iusacellgsm.mx","iusacellgsm","iusacellgsm","IUSACELL",   // IUSACELL
              };
const char* GPRSCommand[]={
                            "ATV1",                                        // 0 OK Vervose: Character
                            "ATE0",                                        // 1 OK Echo: None
                            "AT+CMEE=1",                                   // 2 OK Error report: Enabled
                            "AT&W",                                        // 3 OK Write config on modem EEPROM
                            "AT+CREG=1",                                   // 4 6 OK Registro al servicio GSM
                            "AT+CGREG=1",                                  // 5 OK Registro al servicio GSM
                            "AT+CSQ",                                      // 6 8 Signal Quality

                            "AT+QICSGP=1,1,\"", // 7 // "AT+QICSGP=1,1,\"internet.itelcel.com\",\"webgprs\",\"webgprs2002\",0",
                            "AT+QIACT=1",                                                           // 8
                            "AT+QIOPEN=1,0,\"TCP\",\"",                  // 9  // "AT+QIOPEN=1,0,\"TCP\",\"gprshost.tzo.com\",2007,0,2"

                            "",                                            // 10 Para env�o de reporte v�a GPRS (TCP/IP)
                            "+++",                                         // 11
                            "AT",                                          // 12
                            "AT+QICLOSE=1",                                // 13
                          };

/*const char* GPRSCommand[]={
                            "ATV1",                                       // 0 OK Vervose: Character
                            "ATE0",                                       // 1 OK Echo: None
                            "AT+WIND=4351",                               // 2
                            "AT+CMEE=1",                                   // 3 OK Error report: Enabled
                            "AT&W",                                        // 4 OK Write config on modem EEPROM

                            "AT+WSHS",                                     // 5 Estado de la SIM card Holder
                            "AT+CREG=1",                                   // 6 Registro al servicio GSM
                            "AT+WSTR=2",                                   // 7 Estado de la red
                            "AT+CSQ",                                      // 8 Signal Quality

                            "AT+WIPCFG=1",                                 // 9
                            "AT+WIPBR=1,6",                                // 10 Start Bearer
//                            "AT+WIPBR=2,6,11,\"internet.itelcel.com\"",    // 11
                            "AT+WIPBR=2,6,11,\"",                          // 11
//                            "AT+WIPBR=2,6,0,\"webgprs\"",                  // 12
                            "AT+WIPBR=2,6,0,\"",                  // 12
//                            "AT+WIPBR=2,6,1,\"webgprs2002\"",              // 13
                            "AT+WIPBR=2,6,1,\"",              // 13
                            "AT+WIPBR=4,6,0",                              // 14
//                            "AT+WIPCREATE=2,1,\"gprshost.tzo.com\",2007",  // 15
                            "AT+WIPCREATE=2,1,\"",  // 15
                            "AT+WIPDATA=2,1,1",                            // 16
                            "",                                            // 17 Para env�o de reporte v�a GPRS (TCP/IP)
                            "+++",                                         // 18
                            "AT+WIPCLOSE=2,1",                             // 19
                            "AT+WIPBR=5,6",                                // 20 Stop Bearer
                            "AT+WIPBR=0,6",                                // 21 Close Bearer

                            "AT+CPOF",                                     // 22 Turn off GSM module after set pin ON/OFF to GND
                            "AT+WOPEN=1"                                   // 23 Abre aplicaci�n inmersa
                            "AT+CFUN=1"                                    // 24 Reset completo del sistema

                            //"AT+CGDCONT=1,\"IP\",\"internet.movistar.mx\"\r",           // 6 telefonica
                            //"AT*ENAD=1,\"GPRS1\",\"movistar\",\"movistar\",1,0,0,1\r",  // 7 telefonica

                          };
*/

/****************************************************************************************************/
const char *ResponsesGPRS[]=
{
  "OK",           // 0
  "+WSHS: ",      // 1
  "+WSTR: ",      // 2
  "+CSQ: ",       // 3
  "CONNECT",      // 4
  "ADT: ",             // 5
  "+WIPPEERCLOSE:", // 6
  "END SESSION",  // 7
};

/*********************************************************************/
CGPRS::CGPRS()
{
  this->configGPRSCtrlPorts();
  this->UART=new CUart0();
  this->counterTimeout=0;
  this->timerReset=new CTimerB();
}
/*********************************************************************/
CGPRS::~CGPRS()
{
}
/*********************************************************************/
//Env�a 1 si no detect� alg�n mensaje inesperado o lleg� un mensaje inesperado que no se requiere
int CGPRS::evalWIND(char* l_Buffer, int len)
{
  char* p_ch=l_Buffer;
  int resp=1;

  while(p_ch<(l_Buffer+len) && p_ch)
  {
  p_ch=strstr(p_ch,"+WIND");

  if(p_ch)
  {
  switch(p_ch[7])
  {
  case '0':
            switch(p_ch[8])
            {
            case '\r':        // El SIM ha sido removido
                      this->state_GSM_Modem&=~BIT1;
                      resp=2;
                      break;
            default:
                      break;
            }
            break;
  case '1':
            switch(p_ch[8])
            {
            case '3':         // El rack del SIM se ha cerrado
                      this->state_GSM_Modem|=BIT0;
                      resp=7;
                      break;
            case '4':         // El rack del SIM se ha abierto
                      this->state_GSM_Modem&=~BIT0;
                      resp=8;
                      break;
            case '\r':        // El SIM ha sido insertado
                      this->state_GSM_Modem|=BIT1;
                      resp=3;
                      break;
            default:
                      break;
            }
            break;
  case '4':                 // El modem est� listo para recibir todos los comandos AT
            switch(p_ch[8])
            {
            case '\r':
                      this->state_GSM_Modem|=BIT2;
                      resp=4;
                      break;
            default:
                      break;
            }
            break;
  case '7':                 // El servicio de la red solo est� listo para recibir llamadas de emergencia
            switch(p_ch[8])
            {
            case '\r':
                      this->state_GSM_Modem|=BIT3;  // Enciende bit de detecci�n de llamada en espera
                      this->state_GSM_Modem&=~BIT4; // Apaga bit de estado de conexi�n a la red
                      resp=5;
                      break;
            default:
                      break;
            }
            break;
  case '8':                 // El servicio de la red solo est� listo para recibir llamadas de emergencia
            switch(p_ch[8])
            {
            case '\r':
                      this->state_GSM_Modem&=~(BIT0|BIT1|BIT2|BIT3);
                      this->state_GSM_Modem|=BIT4;
                      resp=6;
                      break;
            default:
                      break;
            }
            break;
  default:
            break;
  }
  p_ch+=8;
  }
  }

  return resp;
}


/****************************************************************************************************/
// Implementaci�n de Funciones.
/****************************************************************************************************/
void CGPRS::configGPRSCtrlPorts()
{
  int x;

  P6DIR&=~BitFlag_ONGPRS;   // Establece el puerto P6.0 como entrada
  P1DIR|=(ON_OFF_Modem_VCC|ON_OFF_GPRS);       // Establece el puerto P1.6 y P1.7 como salida.
  P1OUT&=~(ON_OFF_Modem_VCC|ON_OFF_GPRS);
//  P5DIR|=LED_GPRS_EQ;       // Establece P5.7 como salida
//  P5OUT|=LED_GPRS_EQ;       // Prende el LED en el PCB de estado del modem GSM

//  if(!this->isModemGPRSOn())
//  {
//    P5OUT&=~LED_GPRS_EQ;    // Apaga LED en el PCB de estado del modem GSM
//  }

//  for(x=0;x<sizeof(this->APN_GPRS_SERV);x++)
  for(x=0;x<103;x++)
  {this->APN_GPRS_SERV[x]=0x00;
  }
  this->setIDSP(0);  // Inicializa con Proveedor de Servicio GSM TELCEL
  this->setIP_Port("2007", 4);
  strcpy(this->DNS,"gprshost.tzo.com");

  this->state_GSM_Modem=0x00;
  this->stateGPRS=0x00;
  this->commCounter=0x00;
  this->enableSendingReport();

}
/****************************************************************************************************/
void CGPRS::setAPNConf(char* ID_APN)
{
  strcpy(this->APN_GPRS_SERV,APN_Providers[(*ID_APN)*4]);
  strcpy(this->APN_GPRS_USER,APN_Providers[((*ID_APN)*4)+1]);
  strcpy(this->APN_GPRS_PSW,APN_Providers[((*ID_APN)*4)+2]);
}
/****************************************************************************************************/
void CGPRS::sendFrameToGPRS(int Command)
{
     int mylen=0;

     sprintf((char *)&this->UART->Buffer_Tx[0],"%s",GPRSCommand[Command]);
     if(Command == 0 || Command == 5 || Command == 6 || Command == 12)
     {
        this->UART->setExtratimeOut(30);
     }
     else
     {
       if(Command == 9)
       {
         strcat((char *)this->UART->Buffer_Tx,this->DNS);
         mylen=strlen((char*)this->UART->Buffer_Tx);
         this->UART->Buffer_Tx[mylen++]='"';
         this->UART->Buffer_Tx[mylen++]=',';
         this->UART->Buffer_Tx[mylen]='\0';
         strcat((char *)this->UART->Buffer_Tx,this->IP_PORT);
         mylen=strlen((char*)this->UART->Buffer_Tx);
         this->UART->Buffer_Tx[mylen++]=',';
         this->UART->Buffer_Tx[mylen++]='0';
         this->UART->Buffer_Tx[mylen++]=',';
         this->UART->Buffer_Tx[mylen++]='2';
         this->UART->Buffer_Tx[mylen]='\0';
         this->UART->setExtratimeOut(60);
       }
       else
       {
        this->UART->setExtratimeOut(5);
       }
     }

     if (Command==7)
     {
       if(this->ID_SERV_PROV==3) // Selecciona un APN diferente ingresado por el usuario v�a Puerto Serie
       {
        strcat((char *)this->UART->Buffer_Tx,this->APN_GPRS_SERV);
        mylen=strlen((char*)this->UART->Buffer_Tx);
        this->UART->Buffer_Tx[mylen++]='"';
        this->UART->Buffer_Tx[mylen++]=',';
        this->UART->Buffer_Tx[mylen++]='"';
        this->UART->Buffer_Tx[mylen++]='\0';
        strcat((char *)this->UART->Buffer_Tx,this->APN_GPRS_USER);
        mylen=strlen((char*)this->UART->Buffer_Tx);
        this->UART->Buffer_Tx[mylen++]='"';
        this->UART->Buffer_Tx[mylen++]=',';
        this->UART->Buffer_Tx[mylen++]='"';
        this->UART->Buffer_Tx[mylen++]='\0';
        strcat((char *)this->UART->Buffer_Tx,this->APN_GPRS_PSW);
       }
       else
       {    // Establece el APN selccionado por usuario de los establecidos en la lista preestablecida
         strcat((char *)this->UART->Buffer_Tx,APN_Providers[(this->ID_SERV_PROV*4)]);
         mylen=strlen((char*)this->UART->Buffer_Tx);
         this->UART->Buffer_Tx[mylen++]='"';
         this->UART->Buffer_Tx[mylen++]=',';
         this->UART->Buffer_Tx[mylen++]='"';
         this->UART->Buffer_Tx[mylen++]='\0';
         strcat((char *)this->UART->Buffer_Tx,APN_Providers[(this->ID_SERV_PROV*4)+1]);
         mylen=strlen((char*)this->UART->Buffer_Tx);
         this->UART->Buffer_Tx[mylen++]='"';
         this->UART->Buffer_Tx[mylen++]=',';
         this->UART->Buffer_Tx[mylen++]='"';
         this->UART->Buffer_Tx[mylen++]='\0';
         strcat((char *)this->UART->Buffer_Tx,APN_Providers[(this->ID_SERV_PROV*4)+2]);
       }
       mylen=strlen((char*)this->UART->Buffer_Tx);
       this->UART->Buffer_Tx[mylen++]='"';
       this->UART->Buffer_Tx[mylen++]=',';
       this->UART->Buffer_Tx[mylen++]='0';
       this->UART->Buffer_Tx[mylen]='\0';
     }

     mylen=strlen((char *)&this->UART->Buffer_Tx[0]);
     if(Command!=11)
     {
      this->UART->Buffer_Tx[mylen++]='\r';
     }

     this->stateGPRS|=BIT0;    // Establece bandera de petici�n de comando AT
     this->UART->startTx((char *)&this->UART->Buffer_Tx[0],mylen); // Env�a la cadena de datos por el puerto serie.
}
/****************************************************************************************************/
void CGPRS::sendDataBasculaViaGPRS(CUart1* uartPort1)
{
     int mylen=0;


     mylen=strlen((char *)&uartPort1->Buffer_Rx[0]);

     this->UART->startTx((char *)&uartPort1->Buffer_Rx[0],mylen); // Env�a la cadena de datos por el puerto serie.

}
/****************************************************************************************************/
void CGPRS::sendStringGPRS(char* myStr)
{
     int mylen=0;
     this->UART->setExtratimeOut(30);
     mylen=strlen(myStr);
     this->UART->startTx(myStr,mylen); // Env�a la cadena de datos por el puerto serie.
}
/****************************************************************************************************/
void CGPRS::setupGPRS()
{
  this->stateGPRS&=~BIT1;
  this->commCounter=0;
  this->endCommandIndex=11;  //21;
  this->sendFrameToGPRS(this->commCounter);
}
/****************************************************************************************************/
void CGPRS::connectGPRS()
{
//  turnOnGPRS();
  this->stateGPRS&=~BIT1;
  this->commCounter=5;
  this->endCommandIndex=11; //21;   // el comando 12 es para el env�o de datos v�a GPRS el 14 es el comando de fin de comandos a transmitir.
  this->sendFrameToGPRS(this->commCounter);
}
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
//unsigned int CGPRS::evalGPRSResponse(int* commandSent, unsigned char* BufferRx, int* RxLength, CRTC_3029_I2C* Reloj, CEEPROM *memHandler, CLcd *myLCD)
unsigned int CGPRS::evalGPRSResponse(int* commandSent, unsigned char* BufferRx, int* RxLength, CRTC_3029_I2C* Reloj, CEEPROM *memHandler)
{
  unsigned int response=0x0000;
//  unsigned char res=0x00;
  char *myStr;

  char * mypointer;
  int x; //,y;
 if(this->stateGPRS&BIT0 || this->stateGPRS&BIT1) // Bandera de solicitud comando AT habilitado?
 {
  switch(*commandSent)
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 7:
  case 8:
//  case 9:
//  case 10:
//  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
  case 19:
  case 20:
  case 21:
  case 22:
          myStr=(char*)ResponsesGPRS[0];
          break;
  //case 5:
  //        myStr=(char*)ResponsesGPRS[1];
  //        break;
  /*case 7:
          myStr=(char*)ResponsesGPRS[2];
          break;*/
  case 6:
          myStr=(char*)ResponsesGPRS[3];
          break;
  //case 16:
  case 9:
          myStr=(char*)ResponsesGPRS[4];
          break;
  case 10:myStr=(char*)ResponsesGPRS[5];  // 17
          break;
  case 11:myStr=(char*)ResponsesGPRS[6];  // 18
          break;

  default: break;
  }

  char  Buffer[2];

  mypointer=(char *)strstr((const char *)&BufferRx[0],myStr);

  if(mypointer) // Respuesta adecuada?
  {
     switch(*commandSent)
     {
     /*case 5:mypointer+=7;
            sscanf(mypointer, "%1d", (int*)&Buffer[0]);
            if(Buffer[0])
            {
              if(strstr((const char *)mypointer,"OK"))
              {response=0x02;}  // response = 2 -> Sesi�n IP inactiva
            }
            break;
       */
     /*case 7:mypointer+=9;
            if(*mypointer==0x31)
            {
              if(strstr((const char *)mypointer,"OK"))
              {response=0x03;}                     // response = 3 -> se h� conectado al servicio de GPRS.
            }
            break;
       */
     case 6:mypointer+=6;  // Valida si existe se�al de servicio GSM adecuado.
            if(*(mypointer+1)>=0x30 && *(mypointer+1)<=0x39 )
            {sscanf(mypointer, "%2d", (int*)&Buffer[0]);}
            else
            {sscanf(mypointer, "%1d", (int*)&Buffer[0]);}
            if (Buffer[0]!= 0 && Buffer[0]!= 99)
            {response=0x04;}                    // response =4 -> Existe se�al adecuada para comunicar.
            break;
     case 14:mypointer+=6;
            response=0x05;
            if(strstr((const char *)mypointer,"+CGREG: 1"))
            {response=0x05;}                     // response = 3 -> se h� conectado al servicio de GPRS.
            break;
     case 15:mypointer+=6;
            if(strstr((const char *)mypointer,"+WIPREADY:"))
            {response=0x06;}                     // response = 3 -> se h� conectado al servicio de GPRS.
            break;
     //case 16:
     case 9:
              this->stateGPRS|=BIT1;            // Habilita bandera de inicio de sesi�n de comunicaci�n GPRS.
              this->stateGPRS|=BIT5;            // Habilita bandera de estado de comunicaciones en l�nea de datos GPRS
              response=0x07;                    // Response = 8 -> Se tiene la conexi�n realizada con el servidor en el puerto seleccionado.
              break;
     case 10:// Integrar c�digo para cambiar fecha y hora local
            if((char *)strstr((const char *)&BufferRx[0],(char*)ResponsesGPRS[7]))
            {
              mypointer+=5;
              sscanf(mypointer, "%2d%2d%4d%2d%2d%2d", (int*)&this->UART->Buffer_Tx[0],(int*)&this->UART->Buffer_Tx[2],(int*)&this->UART->Buffer_Tx[4],(int*)&this->UART->Buffer_Tx[6],(int*)&this->UART->Buffer_Tx[8],(int*)&this->UART->Buffer_Tx[10]);
              Reloj->hexToBcdRTC((int*)&this->UART->Buffer_Tx[0]);
              Reloj->setHour_Date(memHandler);
              Reloj->setRTCOnIntTime(memHandler,false);
              response=0x08;
              // C�digo en caso par requerir habilitar o deshabilitar el env�o del reportes (Solicitud remota)
              while(mypointer)
              {
                mypointer=this->evalAndExecuteReqRemote(mypointer);
              }
            }
            break;
     case 11:
            this->stateGPRS&=~BIT5;            // Habilita bandera de estado de comunicaciones en l�nea de datos GPRS
            response=0x09;
            break;
     case 12: //19
             this->stateGPRS&=~BIT1;
             response=0x0A;
             break;
     default:response=0x01;
             break;
     }


      for(x=0;x<*RxLength;x++)    // Borra el contenido del buffer de Rx
      {
        BufferRx[x]=0x00;
      }

  }
  else
  {
    switch(*commandSent)
    {
    case 10:
            if(strstr((const char *)&BufferRx[0],"+CME ERROR: 804"))
            {response=1;}
             for(x=0;x<*RxLength;x++)    // Borra el contenido del buffer de Rx
            {
              BufferRx[x]=0x00;
            }
            break;
    case 9: if(strstr((const char *)&BufferRx[0],"+CME ERROR: 844"))
            {
              response=1;
            }
            for(x=0;x<*RxLength;x++)    // Borra el contenido del buffer de Rx
            {
              BufferRx[x]=0x00;
            }
            break;
    default: break;
    }

  }

 }// Final if de evaluaci�n de bandera de comando AT solicitado

 // Eval�a resuestas insolicitadas cuando estas existan
/* res=this->evalWIND((char*)BufferRx, *RxLength);
  if(res)
  {
    response|=(res*0x0100);
    for(x=0;x<*RxLength;x++)    // Borra el contenido del buffer de Rx
      {
        BufferRx[x]=0x00;
      }
  }
*/



  return response;
}
/****************************************************************************************************/
/****************************************************************************************************/
//int CGPRS::timeoutTxRx(CRTC_3029_I2C *myClock, CLcd *myLCD, char* SerialNumber, CLogger* SatLogg, CMPG2* mMPG2, CEEPROM *memHandler)
int CGPRS::timeoutTxRx(CRTC_3029_I2C *myClock, char* SerialNumber, CLogger* SatLogg, CMPG2* mMPG2, CEEPROM *memHandler)
{
  int resp=0;
  int ID_Eval=0;

//  unsigned int dataSize;
//  char Buffer[4];
  if(this->UART->statePort&0x02)  // Comunicaciones Tx o Rx del m�dulo Satelital o puerto local?
  {

    this->UART->myWDT->stopWdt();
    this->UART->statePort&=~0x02;

    if (this->UART->UartError==0x01)  // Verifica si existe Error de Over flow del buffer de Rx.
    {
      this->UART->UartError&=~0x01;
      this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
      this->UART->clearBufferRx();
      this->stateGPRS&=~BIT0; // Deshabilita bandera de solicitud de comando AT
      this->UART->commTries=0;
      this->UART->strRxLen=0;
      this->UART->statePort&=~0x02;
//          resp=1;

      if(this->isIPSessionOpen() && this->stateGPRS&BIT5)
      {
         this->OFFfromGPRSSession();
      }
      else
      {
         this->OffModemGPRS();
      }
    }
    else
    {
      //if(this->UART->isSwUARTModem()) // Se tiene habilitado el puerto del modem satelital?
      //{
        if(this->UART->strRxLen)// Se recibi� una cadena de datos?
        {
          this->UART->Buffer_Rx[this->UART->strRxLen]=0x00; //Inserta 0 al final de la cadena recibida
          // C�digo para verificar comandos cuando son solicitados via comando AT
          ID_Eval=this->evalGPRSResponse(&this->commCounter, this->UART->Buffer_Rx, &this->UART->strRxLen, myClock, memHandler);
          if (ID_Eval & 0x00FF) // Se recibi� la respuesta de manera correcta del m�dulo Satelital?
          {
                 this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
                 this->UART->strRxLen=0;
//                 this->UART->clearBufferRx();
                 this->UART->extraTimeCounter=0;
                 this->UART->commTries=0;


/*            if((ID_Eval & 0xFF00)!=0x0100)// Se recibi� mensaje inesperado WIND?
            {// Revisar banderas y ejecutar la acci�n.
               if(this->stateGPRS&BIT0 || this->stateGPRS&BIT1)
               {
                  this->UART->myWDT->restartWdt();
               }
            }*/
            //if(ID_Eval& 0x00FF)//Se recibi� respuesta comando AT correcto correspondiente al solicitado?
            //{
              this->stateGPRS&=~BIT0; // Deshabilita bandera de solicitud de comando AT
              this->commCounter++;
              if(this->commCounter<=this->endCommandIndex)
              {
                if(this->commCounter==11) //18
                {
                   mMPG2->restartWRPointers();
                   SatLogg->clearNData();
                }
                if(this->commCounter==10) // 17
                {
                  this->SetFormatMessage((char*)this->UART->Buffer_Tx, SatLogg, mMPG2, memHandler, SerialNumber,*myClock->getTimeZone());
                  this->sendStringGPRS((char*)this->UART->Buffer_Tx);
                }
                else
                {
                  this->sendFrameToGPRS(this->commCounter);
                }
              }
              else
              {// Integrar fin de env�o de comandos
                 if(this->commCounter!=13)
                 {
                    //P5OUT|=BIT0;
                    this->OffModemGPRS();
                 }
                 else
                 {
                    this->turnOFF_VCC_GPRS();
                    this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5);
                    this->UART->UartError&=~0x01;
                    this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
                    this->UART->strRxLen=0;
                    this->UART->clearBufferRx();
                    this->UART->extraTimeCounter=0;
                    this->UART->commTries=0;
                    resp=1;
                 }
              }
            //}
          }
          else  //No se recibi� la respuesta correcta o falta informaci�n.
          {
            //if(this->stateGPRS&BIT0)// Se recibieron datos en puerto al enviar un comando AT?
            //{
            if((!(ID_Eval & 0x00FF)) && this->stateGPRS&BIT0)
            {
              if(this->UART->extraTimeCounter<this->UART->extraTimeout)// Espera de tiempo extra en espera para recibir datos en caso que no se haya recibido ning�n byte.
              {
                this->UART->statePort|=0x02;
                this->UART->extraTimeCounter++;
                this->UART->myWDT->restartWdt();
              }
              else
              {
                this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
                this->UART->clearBufferRx();
                this->UART->strRxLen=0;
                this->UART->extraTimeCounter=0;
                this->UART->commTries++;
                if(this->UART->commTries>this->UART->tries) // Cantidad de intentos de retransmisi�n en caso de que no haya recibido ning�n byte en respuesta.
                {
                // Finaliza retransmisiones a GPRS
                  if(this->stateGPRS&BIT1 && this->isModemGPRSOn()) // Sesion GPRS Abierta?
                  {
                    this->stateGPRS&=~BIT0;
                    if(this->stateGPRS&BIT5) // Se encuentra en l�nea de datos GPRS?
                    {
                      this->OFFfromGPRSSession();
                    }
                    else
                    {
                      this->OffModemGPRS();
                    }
                  }
                  else
                  {
                    if(this->stateGPRS&BIT0 && this->isModemGPRSOn()) // Se pidi� comandos AT ?
                    {
                      this->OffModemGPRS();
                    }
                    else
                    {
                      this->turnOFF_VCC_GPRS();
                      this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                      this->UART->commTries=0;
                      this->UART->strRxLen=0;
                      this->UART->UartError&=~0x01;
                      resp=1; //3// 1
                    }
                  }

                }
                else // Retransmite �ltimo comando solicitado.
                {
                  if(this->commCounter==10) // 17
                  {
                    //this->SetFormatMessage((char*)this->UART->Buffer_Tx, SatLogg, mMPG2, memHandler, SerialNumber);
                    //this->sendStringGPRS((char*)this->UART->Buffer_Tx);
                    //////////////////////////////////////////////////////////////
                    if(this->isModemGPRSOn())
                    {
                      if(this->isIPSessionOpen())
                      {
                        if(this->stateGPRS&BIT5)
                        {
                          this->OFFfromGPRSSession();
                        }
                        else
                        {
                          this->OffModemGPRS();
                        }
                      }
                      else
                      {
                        this->OffModemGPRS();
                      }
                    }
                    else
                    { // Finaliza comunicaciones
                        this->turnOFF_VCC_GPRS();
                        this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                        this->UART->clearBufferRx();
                        this->UART->commTries=0;
                        this->UART->strRxLen=0;
                        this->UART->UartError&=~0x01;
                        resp=1; //3// 1
                    }
                    //////////////////////////////////////////////////////////////
                  }
                  else
                  {
                    this->stateGPRS&=~BIT0; // Deshabilita bandera de solicitud de comando AT
                    this->sendFrameToGPRS(this->commCounter);
                  }
                  //}
//                  this->sendFrameToSat201C(*((unsigned int*)&this->UART->Buffer_Tx[2]));
                }
              }
            }
            else
            { // codigo para evaluar respuestas relacionadas a comandos externos
              if(this->isIPSessionOpen())
              {
                 if(this->stateGPRS&BIT5)
//                 if(strstr((char*)this->UART->Buffer_Rx,"+CME ERROR:") || strstr((char*)this->UART->Buffer_Rx,"NO CARRIER"))
                 {
                   this->OFFfromGPRSSession();
                 }
                 else
                 {
                   this->OffModemGPRS();
//                   this->OFFfromGPRSSession();
                 }
              }
              else
              {
                this->OffModemGPRS();
/*                this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
                this->UART->clearBufferRx();
//                this->stateGPRS&=~BIT0; // Deshabilita bandera de solicitud de comando AT
                this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                this->UART->commTries=0;
                this->UART->strRxLen=0;
                this->UART->UartError&=~0x01;
                resp=1;
                */
              }
            }
            //}
          }
        } // Fin if si se recibi� una cadena de datos
        else // si no recibi� una cadena de datos del modem satelital y se esperaba dependiendo el comando solicitado
        {
          if(this->UART->extraTimeCounter<this->UART->extraTimeout)// Espera de tiempo extra en espera para recibir datos en caso que no se haya recibido ning�n byte.
          {
            this->UART->statePort|=0x02;
            this->UART->extraTimeCounter++;
            this->UART->myWDT->restartWdt();
          }
          else
          {
            this->UART->clearRxBuffPointer(this->UART); // Inicializa puntero de Rx en el Buffer de Rx de Comunicaciones Serie
            this->UART->extraTimeCounter=0;
            this->UART->strRxLen=0;
            this->UART->commTries++;
            if(this->UART->commTries>this->UART->tries) // Cantidad de intentos de retransmisi�n en caso de que no haya recibido ning�n byte en respuesta.
            { //Finaliza retransimisiones a GPRS en caso de que no exista respuesta de Tx del modem
              if(this->isModemGPRSOn())
              {
                  if((this->stateGPRS&BIT1) && !(this->stateGPRS&BIT3)) // Sesion GPRS Abierta?
                  {
                    this->stateGPRS|=BIT3;
                    if(this->stateGPRS&BIT5) // Se encuentra en l�nea de datos GPRS?
                    {
                      this->OFFfromGPRSSession();
                    }
                    else
                    {
                      this->OffModemGPRS();
                    }
                  }
                  else
                  {
                    if((this->stateGPRS&BIT0) && !(this->stateGPRS&BIT3)) // Se pidi� comando AT ?
                    {/////////////////////////////////////////////////////////////////////////
                      this->stateGPRS|=BIT3;
                      if(this->commCounter==0||this->commCounter==5||this->commCounter==22)
                      {
                        //this->stateGPRS|=BIT6;
                        this->OFFfromGPRSSession();
                      }
                      else
                      {
                        this->OffModemGPRS();
                        //////////////////////////////////////////////////////////////////////
                      }
                    }
                    else
                    {
                      this->turnOFF_VCC_GPRS();
                      this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                      this->UART->clearBufferRx();
                      this->UART->commTries=0;
                      this->UART->strRxLen=0;
                      this->UART->UartError&=~0x01;
                      resp=1; //3// 1
                    }
                  }
              }
              else // Finaliza por completo las comunicaciones
              {
                  this->turnOFF_VCC_GPRS();
                  this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                  this->UART->clearBufferRx();
                  this->UART->commTries=0;
                  this->UART->strRxLen=0;
                  this->UART->UartError&=~0x01;
                  resp=1; //3// 1
              }
            }
            else // Retransmite �ltimo comando solicitado.
            {
              if(this->isModemGPRSOn())
              {
                if(this->isIPSessionOpen())
                {
                  if(this->stateGPRS&BIT5)
                  {
                    this->OFFfromGPRSSession();
                  }
                  else
                  {
                    this->OffModemGPRS();
//                    this->SetFormatMessage((char*)this->UART->Buffer_Tx, SatLogg, mItzyFlow, memHandler, SerialNumber);
//                    this->sendStringGPRS((char*)this->UART->Buffer_Tx);
                  }
                }
                else
                {
                  this->stateGPRS&=~BIT0; // Deshabilita bandera de solicitud de comando AT
                  this->sendFrameToGPRS(this->commCounter);
                }
              }
              else
              { // Finaliza comunicaciones
                  this->turnOFF_VCC_GPRS();
                  this->stateGPRS&=~(BIT0|BIT1|BIT3|BIT5); // Deshabilita bandera de solicitud de comando AT
                  this->UART->clearBufferRx();
                  this->UART->commTries=0;
                  this->UART->strRxLen=0;
                  this->UART->UartError&=~0x01;
                  resp=1; //3// 1

              }
            }
          }
        }
      //}
    }
  }
  return resp;
}
/****************************************************************************************************/
void CGPRS::disableSendingReport()
{
  this->stateGPRS&=~BIT2;
}
/****************************************************************************************************/
void CGPRS::enableSendingReport()
{
  this->stateGPRS|=BIT2;
}
/****************************************************************************************************/
bool CGPRS::isEnableSendingReport()
{
  if(this->stateGPRS&BIT2)
  {
    return true;
  }
  return false;
}
/****************************************************************************************************/
char* CGPRS::getAddrFlagsUARTstate()
{
  return &this->UART->statePort;
}
/****************************************************************************/
void CGPRS::OnModemGPRS()
{
  this->turnON_VCC_GPRS();
  P1OUT|=ON_OFF_GPRS; // Establece el puerto como entrada (Envia a 1)
}
/****************************************************************************/
void CGPRS::OffModemGPRS()
{
  P1OUT&=~ON_OFF_GPRS;  // Establece el puerto como salida (Envia a 0)

  this->turnOFF_VCC_GPRS();

  this->commCounter=12;
  this->endCommandIndex=12;
  this->sendFrameToGPRS(this->commCounter);
}
/****************************************************************************/
bool CGPRS::isModemGPRSOn()
{
  bool resp=false;
  if(P1IN & ON_OFF_Modem_VCC)
  {
    if(!(P6IN & BitFlag_ONGPRS))
    { resp=true;}
  }
  return resp;
}
/****************************************************************************/
void CGPRS::turnON_VCC_GPRS()
{
  P6DIR|=(ON_OFF_RS232|ENABLE_RS232);
  P6OUT|=ENABLE_RS232;
  P6OUT&=~ON_OFF_RS232;

  P1DIR|=W_ENABLE_GPRS;
  P1OUT|=(ON_OFF_Modem_VCC|W_ENABLE_GPRS);
}
/****************************************************************************/
void CGPRS::turnOFF_VCC_GPRS()
{
  P1OUT&=~(ON_OFF_Modem_VCC|W_ENABLE_GPRS);
}
/****************************************************************************/
unsigned long CGPRS::SC_OriginatedDefMsg(char* Buffer, CLogger* mLoggerSAT, CMPG2* mMPG2, CEEPROM *memHandler, char qtyMed)
{
  // Establecer en loggerSAt un l�mite de 0x300 datos
  int x=0, x_Acum=0;
  int y;
  char indexMed,reg_temp1;
  int p_startBlk,l_NBlocks;
  Buffer[x++]=0x0A;//commandHeader;
  Buffer[x++]=0x07; // # de comando
  x+=2;
  Buffer[x++]=0x05;//this->retry;
//  if(this->mha==0x00)
//  {this->mha=0x01;}
  Buffer[x++]=0xA0;//this->mha++;

  x_Acum+=x;

  for(indexMed=0;indexMed<*mMPG2->getNMedidores();indexMed++)
{
  x=0;

  switch(indexMed)
  {
  case 1: p_startBlk=3105; //0x061080;
          break;
  case 2: p_startBlk=3137; //0x062080;
          break;
  case 3: p_startBlk=3169; //0x063080;
          break;
  case 0: p_startBlk=3073; //0x060080;

  default: break;
  }

  l_NBlocks=(*(mMPG2->Flowmeter[indexMed]->getPointerWrite())-1)/128;
  if((l_NBlocks-(p_startBlk-2))>0)
  {
    for(;p_startBlk<=l_NBlocks+1;p_startBlk++)
    {
      mLoggerSAT->readLoggDataByBlk(&Buffer[x_Acum+x], memHandler, p_startBlk); // Solo para una memoria de 128 KB
      x+=128;
    }

    y=(*(mMPG2->Flowmeter[indexMed]->getPointerWrite()) & 0x0000007F);
    if(y)
    {
      x-=(128-y);
    }
    x_Acum+=x;

    if(indexMed==0) // Evalua si se obtuvieron datos de los primeros regitros del medidor 1 para obtener la cantidad de registros.
    {
      x-=11; // 11 corresponde a la cantidad de bytes de la cabecera que se inserta al inicio de los registros a enviar (1er medidor).
      reg_temp1=*mMPG2->Flowmeter[0]->getIDVariable();
      switch(reg_temp1) // Evalua el ID de tipo de variables leeidas del medidor 1
      {
      case 4:
      case 1:  reg_temp1=7; break;
      case 5:
      case 2:  reg_temp1=5; break;
      case 6:
      case 3:  reg_temp1=3; break;
      case 0:
      default: reg_temp1=9; break;
      }
      reg_temp1=(x/reg_temp1);
      Buffer[16]=reg_temp1; // Posici�n de byte identificador de cantidad de resgitros enviados.
    }
  }
  else
  {
    // C�digo para ensamblar la trama satelital para error de enlace de telemetr�a con Scadapack
    mLoggerSAT->readLoggDataByBlk(&Buffer[x_Acum], memHandler, 3073); // Solo para una memoria de 128 KB
    x_Acum+=8;
    break;
    //return 0;
  }

  }

  if(x_Acum>6)
  {
//    Establece la cantidad de datos a enviar
    Buffer[2]=(x_Acum+2)&0x00FF;         // LSB Length
    Buffer[3]=((x_Acum+2)/0x100)&0x00FF; // MSB Length


    // Calculo de CRC
    unsigned int l_CRC=this->calcFletcherCRC(Buffer,x_Acum+2);
    Buffer[x_Acum++]=l_CRC & 0x00FF;
    Buffer[x_Acum]=(l_CRC/0x0100)&0x00FF;

    mMPG2->restartWRPointers();
    mLoggerSAT->clearNData();

    return ++x_Acum; // Devuelve el tama�o del comando a enviar v�a serie
  }
  else
  {
    return 0;
  }

}
/**********************************************************************************/
/**********************************************************************************/
char CGPRS::calcCRC(char* xBuffer, int length)
{
  int CRC=0;
  for(int x=0; x<length; x++)
     CRC^=*xBuffer++;
  return CRC+1;
}
/**********************************************************************************/
char CGPRS::getCRCbuffer(char* xBuffer, int length)
{
  length-=4;
  return (asctobcd(xBuffer[length]) * 0x10) | asctobcd(xBuffer[length+1]);
}
/**********************************************************************************/
char CGPRS::asctobcd(char asc)
{
  if (asc>=0x30 && asc<=0x39)
    asc-=0x30;
  else
    asc-=0x37;

  return asc&0x0F;
}
/**********************************************************************************/
unsigned int CGPRS::calcFletcherCRC(char* Buffer, unsigned long length)
{
  char sum1=0x00;
  char sum2=0x00;
  int i;
  *(Buffer+length-1)=0;
  *(Buffer+length-2)=0;
  for(i=0;i<length;i++)
  {
    sum1+=*(Buffer+i);
    sum2+=sum1;
  }
  unsigned int result=(sum1-sum2)&0x00FF;
  result|=((sum2-(2*sum1))*0x0100)& 0xFF00;

  return result;
}
/****************************************************************************************************/
/****************************************************************************/
unsigned long CGPRS::SetFormatMessage(char* Buffer, CLogger* mLoggerSAT, CMPG2* mMPG2, CEEPROM *memHandler, char* Nserie, int TZ)
{
  int x=0, x_Acum=0;
  int y;
  char indexMed,reg_temp1;
  int p_startBlk,l_NBlocks;

  reg_temp1=*mLoggerSAT->getPNDataBCD();

  if(reg_temp1 && this->isEnableSendingReport())
  {
    sprintf(&Buffer[x],"%.3d",reg_temp1);
    x=10;
    for(indexMed=0;indexMed<*mMPG2->getNMedidores();indexMed++)
    {
      switch(indexMed)
      {
//        0x060000,0x061000,0x062000,0x063000
      case 1: p_startBlk=3105; //0x061080;
              break;
      case 2: p_startBlk=3137; //0x062080;
              break;
      case 3: p_startBlk=3169; //0x063080;
              break;
      case 0: p_startBlk=3073; //0x060080;
      default: break;
      }

      l_NBlocks=(*(mMPG2->Flowmeter[indexMed]->getPointerWrite())-1)/128;
      if((l_NBlocks-(p_startBlk-2))>0)
      {
        for(;p_startBlk<=l_NBlocks+1;p_startBlk++)
        {
          mLoggerSAT->readLoggDataByBlk(&Buffer[x_Acum+x], memHandler, p_startBlk); // Solo para una memoria de 128 KB
          x+=128;
        }
        y=(*(mMPG2->Flowmeter[indexMed]->getPointerWrite()) & 0x0000007F);
        if(y)
        {
          x-=(128-y);
        }
      }
      strcpy(&Buffer[x],"END BLOCK\r\n");
      x=23;       // Almacena la cantidad de registros en la cadena a enviar
      Buffer[x++]=Buffer[0];
      Buffer[x++]=Buffer[1];
      Buffer[x++]=Buffer[2];
      x=0;        // Establece el comando de env�o de reporte
      Buffer[x++]='H';
      Buffer[x++]='G';
      Buffer[x++]='P';
      Buffer[x++]='R';
      Buffer[x++]='S';
      Buffer[x++]='D';
      Buffer[x++]='A';
      Buffer[x++]='T';
      Buffer[x++]='A';
      Buffer[x++]=':';

      x=strlen(Buffer);
      Buffer[x++]=0x03;   //Caracter para fin de p�gina v�a HTTP
      Buffer[x++]=0x00;
    }

//    mItzyFlow->restartWRPointers();
//    mLoggerSAT->clearNData();
    x=strlen(Buffer); // Devuelve el tama�o del comando a enviar v�a serie
    return x;
  }
  else
  {
    if(mMPG2->get_ID_Rep()!=0x31)
    {
      if(!this->isEnableSendingReport()) // Se ejecuta como reporte de espera para volver a enviar datos cada hora en caso que est� deshabilitado para enviar el reporte
      {
        strcpy(&Buffer[x],"END BLOCK\r\n");
        x=strlen(Buffer);
//        mItzyFlow->restartWRPointers();
//        mLoggerSAT->clearNData();
        return x;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      Buffer[x++]='H';
      Buffer[x++]='G';
      Buffer[x++]='P';
      Buffer[x++]='R';
      Buffer[x++]='S';
      Buffer[x++]='D';
      Buffer[x++]='A';
      Buffer[x++]='T';
      Buffer[x++]='A';
      Buffer[x++]=':';
      Buffer[x++]=mMPG2->get_ID_Rep();
      Buffer[x++]=',';
      strncpy((char*)&Buffer[x],Nserie,10);
      x+=10;
      sprintf((char*)&Buffer[x],",%.2d",TZ);
      x+=3;
      if(TZ < 0)
      {
         x++;
      }
      strcpy(&Buffer[x],"\r\nEND BLOCK\r\n");

      x=strlen(Buffer);
      Buffer[x++]=0x03;   //Caracter para fin de p�gina v�a HTTP
      Buffer[x++]=0x00;
//      mItzyFlow->restartWRPointers();
//      mLoggerSAT->clearNData();
      return x;
    }
  }
}
/**********************************************************************************/
void CGPRS::OFFfromGPRSSession()
{
  P1OUT&=~ON_OFF_GPRS;  // Establece el puerto como salida (Envia a 0)
  this->commCounter=11;
  this->endCommandIndex=12;
  this->sendFrameToGPRS(this->commCounter);
}
/**********************************************************************************/
void CGPRS::setIP_Port(char* IP_Port_Buff, int len)
{
  int x;
  for(x=0;x<len;x++)
  {
    this->IP_PORT[x]=IP_Port_Buff[x];
  }
  this->IP_PORT[x]='\0';
}
/**********************************************************************************/
bool CGPRS::isIPSessionOpen()
{
  bool resp=false;

  if(this->stateGPRS&BIT1)
  {resp=true;}

  return resp;
}
/**********************************************************************************/
char* CGPRS::evalAndExecuteReqRemote(char* Buffer)
{
  char* p_Char=Buffer;
//  bool resp=false;

      p_Char=strstr(p_Char,"H");
      if(p_Char)
      {
        p_Char++;
        switch(*p_Char)
        {
        case 'W':
                 p_Char++;
                 switch(*p_Char)
                 {
                 case 'R':
                          p_Char++;
                          switch(*p_Char)
                          {
                          case 'D':
                                   p_Char+=2;
                                   if(*p_Char == 'E')
                                   {
                                     this->enableSendingReport();
                                   }
                                   else if(*p_Char == 'D')
                                   {
                                     this->disableSendingReport();
                                   }
                                   else
                                   {
                                     p_Char=0;
                                   }
                                   break;
                          default: p_Char=0; break;
                          }
                          break;
                 default: p_Char=0; break;
                 }
                 break;
        default: p_Char=0; break;

        }

      }

return p_Char;
}
/**********************************************************************************/
char* CGPRS::getAPN()
{
  return &this->APN_GPRS_SERV[0];
}
/**********************************************************************************/
char* CGPRS::getAPNUser()
{
  return &this->APN_GPRS_USER[0];
}
/**********************************************************************************/
char* CGPRS::getAPNPSW()
{
  return &this->APN_GPRS_PSW[0];
}
/**********************************************************************************/
char* CGPRS::getDNS()
{
  return &this->DNS[0];
}
/**********************************************************************************/
char* CGPRS::getIPPort()
{
  return &this->IP_PORT[0];
}
/**********************************************************************************/
char* CGPRS::getSPName()
{
  if(this->ID_SERV_PROV<3)
  {
    return (char*)APN_Providers[(this->ID_SERV_PROV*4)+3];
  }
  else
  {
    return "Personalizado";
  }
}
/**********************************************************************************/
char* CGPRS::getIDSP()
{
  return &this->ID_SERV_PROV;
}
/**********************************************************************************/
void CGPRS::setIDSP(int ID)
{
  this->ID_SERV_PROV=ID;
  this->setAPNConf(&this->ID_SERV_PROV);
}
/**********************************************************************************/
char* CGPRS::getAPNPorvByIndexList(int index)
{
  return (char*) APN_Providers[(index*4)+3];
}
/**********************************************************************************/

