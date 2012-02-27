#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifdef __cplusplus
extern "C"{
#endif

void beginSerial(long);
void serialWrite(unsigned char);
int serialAvailable(void);
int serialRead(void);
void serialFlush(void);

void printByte(unsigned char c);
void printNewline(void);
void printString(const char *s);
void printInteger(long n);
void printHex(unsigned long n);
void printOctal(unsigned long n);
void printBinary(unsigned long n);
void printIntegerInBase(unsigned long n, unsigned long base);

#ifdef __cplusplus
} // extern "C"
#endif


#endif /* _SERIAL_H_ */
