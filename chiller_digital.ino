/*
 * Programma scritto ed ideato da Grigolin Massimo 
 * grigomax@mcetechnik.it
 * 
 * Rilasciato su licenza GPL
 * 
 * Programma per inserire arduino all'interno di chiller fatto in casa.
 * Il programma prevede due funzionamenti, uno per una sala singola, che in questo
 * caso è la sala server, l'altro prevede anche la chiamata da un ambiente esterno
 * in questo caso gli uffici
 * 
 * un led per il funzionamento ed uno per gli errori..
 * 
 * Poi abbiamo 4 sonde di cui 3 digitali, per rilevare la temperatura in ingresso
 * ed in uscita dell'acqua dallo scambiatore, ed una che rileva che lo scambiatore non ghiacci
 * per il momento il sistema farà solo freddo, quella analogica è per tener sotto controllo la temperatura della
 * parte elettronica, arduino ed alimentatori
 * 
 * Abbiamo anche delle uscite, una per accendere la pompa dell'acqua, una per il funzionamento
 * della moto condensante, ed una che abilita le elettrovalvole per l'impianto esterno..
 * 
 * come ingressi digitali, abbiamo la chiamata del mobiletto sala server, la chiamata dell'impianto esterno
 * e un allarme pressione impianto.
 * 
 * 
 */

/*
 * 
 * Versione 2.2 del 30/04/2016
 * Abbassato il tempo di raffrescamento a 5 minuti..
 * Inizia a pesare il sistema anche calore..
 * 
 * 
 * Versione 2.1 del 26/04/2016
 * Messo su richiesta il terminale seriale
 * Disabilitato dal programma la seconda velocità.. intanto per permettere una 
 * 
 * Versione 2.0 del 24/04/2016
 * Dopo la prima prova su banco ho visto che non funzionava niente.
 * Ho dovuto aggiungere delle variabili per leggere di volta in volta gli ingressi e poterli gestire..
 * 
 * 
 * Versione iniziale del 15/04/2016
 *
 * In teoria un chiller dovrebbe far uscire l'acqua dopo il trattamento a meno di 20 gradi, quando fuori
 * ci sono 40 e arrivare a 4 gradi quando fuori ce ne sono 20..
 * 
 * 
 * Versione iniziale del 27/05/2016
 * Cambio invio data seriale..
 * 
 * Versione del 29/05/2016
 * Eliminazione attesa partenza pompa, 
 * Inizio di due tipologie di funzionamento una con il discorso solo server l'altra anche gli uffici.
 * Sistemazio e pulizia del codice..
 * 
 * Versione del 30/05/2016
 * Cambio Nomi Variabili e cambio metodologia per spegnimento e riaccensione macchina..
 * dati i primi collaudi, sistemazione e ottijmizzazione
 * 
 * 
 * Versione del 31/05/2016
 * Fatto pagina web con la dimostrazine delle temperature
 *  Inseriamo una scheda bluetooth
 *  
 *  
 *  Versione del 07/06/2016
 *  dopo i primi collaudi, dobbiamo sostituire le sonde..
 *  Passiamo alla lettura digitale sul pin 2..
 *  
 *  Versione del 20/06/2016
 *  Passati bene la fase di test, inibiamo la chiamata degli uffici in base all'ora di funzionamento
 *  diciamo che inseriamo le variabili di accensione e carichiamo le librerie time
 *  
 *  29/06/2016
 *  Ricablaggio finale del chiller e modifica pin per adattamenti..
 *  
 *  30/06/2016
 *  inserimento del giorno della settimana per disabilitare il sabato e domenica per gli uffici..
 *  //tm.Wday = 2
 *  
 *  01/07/2016
 *  Aggiornato e sostituito le variabili della pompa, aggiornato bug orario ed inserita la seconda velocità
 *  
 *  
 *  
 *  
 */

//------------------CARICAMENTO LIBRERIE OBBLIGATORIE------------------------
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Time.h>
#include <DS1307RTC.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define ONE_WIRE_BUS_PIN 2

/*-----( Declare objects )-----*/
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

//struttura time elements
TimeElements te;

//------------------------FINE librerie------------------------------


// definiamo le variabili
//tx e rx per la scheda bluetooth

//-------------------DEFINIZIONE DEGLI INGRESSI-----------------------------
#define S_chserver 6 //pin collegato al mobiletto del server
#define S_chuffici 5 //pin collegato alla chiamata delgli uffici
#define S_allarme 7 //pin collegato all'allarme pressione 
//#define S_ingresso A0 //pin collegato sensore temp. acqua ingresso
//#define S_uscita A1 //pin collegato sensore temp. acqua Uscita
//#define S_scambiatore A2 //pin collegato allo scambiatore
#define S_ambiente A0 //pin collegato sensore temp. arduino

//-------------------DEFINIZIONE DELLE USCITE--------------------------------
#define S_pompa 8 //pin uscita pompa circolatore
#define S_velocita 9 //pin uscita minima velocita spento massima / acceso minima..
#define S_compressore 10 //pin uscita accensione motocondensante
#define S_valvole 11 //pin uscita accensione circuito uffici
//#define S_LED 3 // status impianto 
#define S_BLOCK 3 // status impianto in caso di allarme

//----------------------DEFINIZIONI DELL'INDIRIZZO MAC DELLE SONDE------------------------------------
DeviceAddress S_ingresso = { 0x28, 0x54, 0x50, 0x01, 0x00, 0x00, 0x80, 0x7A };
DeviceAddress S_uscita = { 0x28, 0xA8, 0x48, 0x01, 0x00, 0x00, 0x80, 0x37 }; 
DeviceAddress S_scambiatore = { 0x28, 0x17, 0x50, 0x01, 0x00, 0x00, 0x80, 0x56 };


//-----------------------VARIABILI DI TEMPERATURA DA IMPOSTARE PER IL FUNZIONAMENTO---------------------

float S_TAMB = 0; //azzeriamo la temperatura ambianete
float S_TA_ON_SERVER = 18.0; //temp. di accensione automatica server
float S_TA_OFF_SERVER = 10.0; //temp. di spegnimento automatica
float S_TA_ON_UFFICI = 13.0; //temp. di accensione automatica uffici
float S_TA_OFF_UFFICI = 8.0; //temp. di spegnimento automatica
float S_Tscamb_MAX = -1.5; //temp. minima scambiatore ghiacciato
float S_Tuscita_MIN = 3.0; //temperatura oltre al quale la macchina non può andare..
int S_tempo_spegnimento = 3; // valore da impostare in minuti per il raffrescamento..
int S_tempo_isteresi = 3; // valore da impostare in minuti per non far spegnere e accendere la macchina per niente
int S_ACCENSIONE_H = 830; // valore hhmm senza punteggiature di accensione
//int S_ACCENSIONE_M = 30;
int S_SPEGNIMENTO_H = 1830; // valore hhmm senza punteggiature di spegnimento
//int S_SPEGNIMENTO_M = 30;



//------------------------------------------variabili da partire da zero..

int S_pausa = 0; //variale che mi conta quanto far girare la pompa prima di spegnere l'impianto..
int S_server = 0;
int S_uffici = 0;
int S_pressione = 0;
int S_prima_acc = 0;
float S_TA_ON = 0.0;
float S_TA_OFF = 0.0;
String POMPA;
String COMP;
String ALLARME;
int incomingByte;
int ciclo;
float S_TA_IN = 0; //temp. ingresso acqua
float S_TA_OUT = 0; //temp. uscita acqua
float S_Tscamb = 0; //temp. scambiatore acqua
int S_giorno;


//----------------------------INIZIO PROGRAMMA-----------------------------------------------------

//calcoliamo il periodo di pausa..
int S_tempo_raffrescamento = ((S_tempo_spegnimento * 60) / 2);
int S_tempo_OFF = ((S_tempo_isteresi * 60) / 2);

void setup()
{
  // put your setup code here, to run once:

  //accendiamo la porta seriale..
  Serial.begin(9600);

    // set the resolution to 10 bit (Can be 9 to 12 bits .. lower is faster)
  sensors.setResolution(S_ingresso, 10);
  sensors.setResolution(S_uscita, 10);
  sensors.setResolution(S_scambiatore, 10);

  //uscite
  pinMode(S_pompa, OUTPUT);
  pinMode(S_velocita, OUTPUT);
  pinMode(S_compressore, OUTPUT);
  pinMode(S_valvole, OUTPUT);
  //pinMode(S_LED, OUTPUT);
  pinMode(S_BLOCK, OUTPUT);
  
  //ingressi
  pinMode(S_chserver, INPUT);
  pinMode(S_chuffici, INPUT);
  pinMode(S_allarme, INPUT);

  digitalWrite(S_pompa, 1);
  digitalWrite(S_velocita, 1);
  POMPA = "OFF";
  //digitalWrite(S_velocita, 1);
  digitalWrite(S_compressore, 1);
  COMP = "OFF";
  digitalWrite(S_valvole, 1);

}


void loop() {
  // put your main code here, to run repeatedly:

  //leggiamo i termostati..

  // Command all devices on bus to read temperature  
  sensors.requestTemperatures();  

  S_TAMB = (analogRead(S_ambiente)*0.5);
  S_TA_IN = sensors.getTempC(S_ingresso);
  S_TA_OUT = sensors.getTempC(S_uscita);
  S_Tscamb = sensors.getTempC(S_scambiatore);

  //leggiamo gli ingressi..
  int S_server = digitalRead(S_chserver);
  int S_uffici = digitalRead(S_chuffici);
  int S_pressione = digitalRead(S_allarme);

  //leggiamo l'ora
  RTC.read(te);
  int S_ora = printf("%s%s",te.Hour,te.Minute);
  S_giorno = te.Wday;

  if (Serial.available() > 0)
  {
      //mandiamo alla porta seriale la lettura dei termostati..
      Serial.print("Tempo RTC = ");
      Serial.print(te.Hour);
      Serial.write(':');
      Serial.print(te.Minute);
      Serial.write(':');
      Serial.print(te.Second);
      Serial.print("---");
      Serial.print(S_ora;);
      Serial.println();
   
      Serial.print("Data RTC = ");
      Serial.print(te.Day);
      Serial.write('/');
      Serial.print(te.Month);
      Serial.write('/');
      Serial.print(te.Year + 1970);
      Serial.write("Giorno ");
      Serial.print(te.Wday);
      Serial.println();
      
      Serial.print("TA Ritorno ");
      Serial.print(S_TA_IN);
      Serial.println(" C");
      
      Serial.print("TA Mandata. ");
      Serial.print(S_TA_OUT);
      Serial.println(" C");
      
      Serial.print("TA Scamb. ");
      Serial.print(S_Tscamb);
      Serial.println(" C");
      
      Serial.print("TA scheda ");
      Serial.print(S_TAMB);
      Serial.println(" C");
      
      Serial.print("All. Imp. ");
      Serial.println(ALLARME);
      
      Serial.print("Server  ");
      Serial.println(digitalRead(S_chserver));
      
      Serial.print("Uffici  ");
      Serial.println(digitalRead(S_chuffici));
  
      Serial.print("Pompa ");
      Serial.print(POMPA);
      Serial.print(" cont. ");
      Serial.println(S_pausa);
  
      Serial.print("Compressore ");
      Serial.print(COMP);
      Serial.print(" cont. ");
      Serial.println(S_prima_acc);
  
      Serial.println("------------------------------------");
    
  }
  
  //prima di tutto vediamo che non ci siano allarmi..

  if(S_pressione == 0)
  {
    //facciamo lampeggiare il led blocco pressione..

    digitalWrite(S_BLOCK, 1);
    delay(1000);
    digitalWrite(S_BLOCK, 0);
    //spegniamo tutto

    POMPA = "OFF";
    digitalWrite(S_pompa, 1);
    digitalWrite(S_velocita, 1);
    COMP = "OFF";
    digitalWrite(S_compressore, 1);
    digitalWrite(S_valvole, 1);

    S_prima_acc = 0;
    ALLARME = "PRESSIONE";
    
  }
  else
  {
    ALLARME = "";
    //programma qui escludiamo il fatto che l'acqua in uscita ghiacci
      
    if(S_TA_OUT < S_Tuscita_MIN)
    {
      //facciamo lampeggiare il blocco.. e l'accensione
      digitalWrite(S_BLOCK, 1);
      //digitalWrite(S_LED, 1);
      delay(500);
      digitalWrite(S_BLOCK, 0);
      //digitalWrite(S_LED, 0);

      //vediamo come operare..
      //accendiamo la pompa..
      POMPA = "ON";
      digitalWrite(S_pompa, 0);
      digitalWrite(S_velocita, 1);
      COMP = "OFF";
      digitalWrite(S_compressore, 1);

      //Accendiamo le valvole..
      digitalWrite(S_valvole, 0);

      S_prima_acc = 0;

      ALLARME = "GHIACCIO";
      
    }
    else
    {
      ALLARME = "";
      
      //programma qui escludiamo il fatto che lo scambiatore sia ghiacciato..
      if(S_Tscamb < S_Tscamb_MAX)
      {
        //facciamo lampeggiare il blocco.. e l'accensione
        digitalWrite(S_BLOCK, 1);
        //digitalWrite(S_LED, 1);
        delay(500);
        digitalWrite(S_BLOCK, 0);
        //digitalWrite(S_LED, 0);
  
        //vediamo come operare..
        //accendiamo la pompa..
        POMPA = "ON";
        digitalWrite(S_pompa, 0);
        digitalWrite(S_velocita, 1);
        COMP = "OFF";
        digitalWrite(S_compressore, 1);
  
        //Accendiamo le valvole..
        digitalWrite(S_valvole, 0);
  
        S_prima_acc = 0;
  
        ALLARME = "SCAMBIATORE";
        
      }
      else
      {
        ALLARME = "";
        //proseguiamo con il programma
  
        //il programma non fa' niente se non c'è una chiamata..

        //qui in questo blocco settiamo la chiamata agli uffici uguale a zero nel caso in cui siamo fuori tempo
        //oppure che siamo al fine settimana.. da 0 a 6
        if(((S_ora < S_ACCENSIONE_H) || (S_ora > S_SPEGNIMENTO_H)) & (S_giorno > 5))
        {
          S_uffici = 0;
        }
  
        //Serial.println("merda2");
        if((S_server == 1) || (S_uffici == 1))
        {
  
          //Accendiamo la pompa.. 
          POMPA = "ON";
          digitalWrite(S_pompa, 0);
          //accendiamo la pompa al minimo..
          digitalWrite(S_pompa, 0);
  
          //ora se la chiamata arriva dagli uffici accendiamo le valvole..
          if(S_uffici == 1)
          {
            //accendiamo le valvole
            digitalWrite(S_valvole, 0);
            //accendiamo la pompa massimo..
            digitalWrite(S_pompa, 1);
            
          }
          else
          {
            //spegniamo le valvole
            digitalWrite(S_valvole, 1);
            //accendiamo la pompa al minimo..
            digitalWrite(S_pompa, 0);
          }
          
          //azzeriamo la variabile pausa..
          S_pausa = 0;
          //Serial.println("merda3");
          
          //ora che c'è stata una chiamata facciamo partire la macchina..
          //qui facciamo in modo di cambiare la temperatura di riferimento in base al tipo di chiamata..
  
          if(S_uffici == 1)
          {
            S_TA_ON = S_TA_ON_UFFICI;
            S_TA_OFF = S_TA_OFF_UFFICI;
          }
          else
          {
            S_TA_ON = S_TA_ON_SERVER;
            S_TA_OFF = S_TA_OFF_SERVER;
          }
  
          //verifichiamo la temperatura dell'acqua che non sia troppo fredda..
          if(S_TA_IN >= S_TA_OFF)
          {
  
            //verifichiamo la temperatura dell'acqua che non sia troppo Calda
            if((S_TA_IN >= S_TA_ON) || (S_prima_acc >= S_tempo_OFF))
            {
            
              //verifichiamo se è la prima accensione
              if(S_prima_acc >= S_tempo_OFF)
              {
                //accendiamo il compressore
                COMP = "ON";
                digitalWrite(S_compressore, 0);
                //settiamo prima accensione = 1
                S_prima_acc = S_tempo_OFF;
              }
              else
              {
                S_prima_acc++;
              }
              
            }
          
          }
          else
          {
            //spegnamo il compressore..
            COMP = "OFF";
            digitalWrite(S_compressore, 1);
            S_prima_acc = 0;
          }
  
        }
        else
        {
          //qui mettiamo il raffrescamento della macchina ovvero l'andata in pausa..
          //spegnamo il compressore
          COMP = "OFF";
          digitalWrite(S_compressore, 1);
          //quindi azzeriamo la prima accensione così in caso di ripartenza riconta
          S_prima_acc = 0;
          
          //facciamo girare ancora la pompa per un quarto d'ora..
          //Serial.println("merda");
          if(S_pausa != S_tempo_raffrescamento)
          {
            S_pausa ++;
          }
          else
          {
            //nessuna chiama spegnamo tutto
            digitalWrite(S_pompa, 1);
            digitalWrite(S_velocita, 1);
            POMPA = "OFF";
            digitalWrite(S_compressore, 1);
            COMP = "OFF";
            digitalWrite(S_valvole, 1);
            S_pausa = S_tempo_raffrescamento;
          }
  
        }//fine raffrescamento
        
      }//fine blocco scambiatore

    }//fine blocco sicurezza acqua in uscita giacciata

  }//fine blocco pressione impianto


  //qui mettiamo un delay per far si che si possa leggere la temperatura ogni due secondi

  delay(2000);
  
}
