#ifndef APPLICATION_H
#define APPLICATION_H



void atende();
int waitForSignal(char* msg_send, char *rej, char sig, int res);
void appSender(char *file, int iSize);
void appReceiver();
int llopen(int port, int flag);
int llclose(int flag);
char *llread(int acknr,int *previous_n, int *size);
int llwrite(char * buffer, int size, int package_number);

extern int partitions;

extern int fd;
extern unsigned int baud;

extern int alarm_flag;
extern int conta;
extern int timeout;
extern int nTimeouts;
extern int tries;

#endif
