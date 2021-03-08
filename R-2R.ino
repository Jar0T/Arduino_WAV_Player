// includes
#include <SPI.h>
#include <SD.h>

// defines
#define SD_CS 4
#define BUF_SIZE 2048
#define ROOT "/"

// function prototypes
void readHeaders();
int countFiles();
int openFile(int fileNumber);
void setupPins();
void setupTimer();
void pinInterruptSetup();

// typedefs
typedef struct RIFFChunk {
  uint8_t chunkID[4];
  uint32_t chunkSize;
  uint8_t format[4];
} RIFFChunk;

typedef struct FMTSubChunk {
  uint8_t subchunkID[4];
  uint32_t subchunkSize;
  uint16_t audioFormat;
  uint16_t noChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
} FMTSubChunk;

typedef struct DataSubChunk {
  uint8_t subchunkID[4];
  uint32_t subchunkSize;
} DataSubChunk;

// variables
File root;
File myFile;

// WAV data
RIFFChunk riffChunk;
FMTSubChunk fmtSubChunk;
DataSubChunk dataSubChunk;
uint8_t trashBuf[100];  // to read all unrecognized headers

uint16_t buf[BUF_SIZE] = {0};
bool empty = true;
uint16_t count = 0;

int numberOfFiles = 0;
int fileNumber = 0;

void setup() {
  cli();
  setupPins();
  setupTimer();
  pinInterruptSetup();
  sei();
  
  Serial.begin(115200);

  // setup sd card
  if (!SD.begin(SD_CS)) {
    Serial.println("Error opening sd card");
    while(1);
  }

  root = SD.open(ROOT);

  numberOfFiles = countFiles();
}

void loop() {
  // every time all samples from buffer are played, refill buffer with new samples
  if (empty) {
    if (myFile.available()) {
      myFile.read(buf, sizeof(buf));
      empty = false;
    }
    else {
      myFile.close();
      if (++fileNumber > numberOfFiles) {
        fileNumber = 1;
      }
      openFile(fileNumber);
      readHeaders();
    }
  }
}

void readHeaders() {
  // read wav file headers
  myFile.read(&riffChunk, sizeof(riffChunk));

  myFile.read(&fmtSubChunk, sizeof(fmtSubChunk));

  // skip all unnecessary headers until data header
  while(1) {
    myFile.read(&dataSubChunk, sizeof(dataSubChunk));
    if (dataSubChunk.subchunkID[0] == 'd' &&
        dataSubChunk.subchunkID[1] == 'a' &&
        dataSubChunk.subchunkID[2] == 't' &&
        dataSubChunk.subchunkID[3] == 'a')
      break;
    myFile.read(trashBuf, dataSubChunk.subchunkSize);
  }
  Serial.println("Read headers");
}

int countFiles() {
  root.rewindDirectory();
  int nof = 0;
  while(true) {
    myFile = root.openNextFile();
    if (!myFile) {
      return nof;
    }
    nof++;
  }
}

int openFile(int fNumber) {
  root.rewindDirectory();
  for (int i = 0; i < fNumber - 1; i++) {
    root.openNextFile();
  }
  myFile = root.openNextFile();
  String fName = myFile.name();
  int nameLen = strlen(fName.c_str());
  if (strcmp(".WAV", &fName[nameLen-strlen(".WAV")]) == 0) {
    return 1;
  }
  return -1;
}

void setupPins() {
  DDRA = 0xFF;  // set port A as output
  DDRC = 0xFF;  // set port C as output
  DDRD = 0x00;  // set port D as input
  
  PORTA = 0x00; // set all pins on port A to 0
  PORTC = 0x00; // set all pins on port A to 0
  PORTD = 0xFF; // set pull-up resistors on port D
}

void setupTimer() {
  // timer1 setup for 48 kHz interrupt
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT1 = 0;        // set timer value to 0
  OCR1A = 330;      // set compare register A to 330
  TCCR1A |= 0;      //
  TCCR1B |= 1 << 3; //set CTC mode
  TCCR1B |= 1 << 0; // set no clock prescaler
  TCCR1C |= 0;      //
  TIMSK1 |= 1 << 1; // set OCIE1A interrupt
}

void pinInterruptSetup() {
  EICRA = 0;
  EICRB = 0;
  EICRA |= 2 << 0;  // set asynchronus interrupt int0 to be triggered on falling edge
  EICRA |= 2 << 4;  // set asynchronus interrupt int2 to be triggered on rising edge
  EIMSK |= 1 << 0;  // enable int0
  EIMSK |= 1 << 2;  // enable int2
}

uint16_t sample = 0;

// Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {
  sample = buf[count] + 0x7FFF;
  PORTA = (uint8_t)sample;
  PORTC = (uint8_t)(sample >> 8);
  count++;

  if (count == BUF_SIZE) {
    count = 0;
    empty = true;
  }
}

unsigned long check = millis();
unsigned long lastCheck = check;

ISR(INT0_vect) {
  check = millis();
  if (check - lastCheck >= 30) {
    if ((PIND >> 1) & 1) {
      
    }
    else {
      if (++fileNumber > numberOfFiles) {
        fileNumber = 1;
      }
    }
    Serial.println(fileNumber);
    lastCheck = check;
  }
}

ISR(INT2_vect) {
  check = millis();
  if (check - lastCheck >= 100) {
    if (--fileNumber < 1) {
      fileNumber = numberOfFiles;
    }
    myFile.close();
  }
}
