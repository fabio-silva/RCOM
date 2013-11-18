#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h> 
#include <stdint.h>
#include "byteStuffing.h"
#include "constants.h"

conta = 1;
alarm_flag=0;
timeout = 3;
nTimeouts = 0;
tries = 10;
partitions = 1;
baud = 0x0000000f;// B38400
fd = 0;
struct termios oldtio,newtio;



void atende()              
{
   printf("alarme # %d\n", conta);
   alarm_flag=1;
   conta++;
   nTimeouts++;
}


int waitForSignal(char* msg_send, char *rej, char sig, int res)
{
   conta = 1;
   int i = 0;
   int success = 0;
   char msg[255];

   while(conta < tries)
   {  
      alarm_flag = 0; 
      alarm(timeout);          
      while (i != 5)
      {
         res = read(fd,&msg[i],1);
         if (res < 0 && alarm_flag == 1)//TIMEOUT
         {
            printf("Didn't receive %x\n", sig);
            
            if(rej == NULL)
            {
               res = write(fd,msg_send,5);
               printf("msg sent: 0x%x%x%x%x%x\n", msg_send[0],msg_send[1],msg_send[2],msg_send[3],msg_send[4]);
            }

            else
            {
               write(fd, rej, 5);

            }

            break;

         }

         if (i == 0 && msg[i] != FLAG)
         {
            i--;
         }

         switch(i)
         {
            case 1:
            if (msg[i] != A)
            {
               i--;
            }
            break;
            case 2:
            if (msg[i] != sig)
            {
               i--;
            }
            break;
            case 3:
            if (msg[i] != (msg[1]^msg[2]))
            {
               i--;
            }
            break;
            case 4: 
            if (msg[i] != FLAG)
            {
               i--;
            }
            else
            {
               success = 1;
            }
            break;
         }
         i++;
      }

      if (success)
      {
         printf("Successfully received the message!\n");
         conta = tries;
         return 0;
      }
      alarm(0);
   }
   return -1;
}

char *llread(int acknr,int *previous_n, int *size)
{
   unsigned char response[5];
   unsigned char rej_response[5];
   unsigned char buffer[11000];
   int package_ok = 0;
   int res;
   int i = 0, j = 0, same_packet = 0;
   int countFlags = 0;
   char r[1];
   *size = 0;

   response[0] = FLAG;
   response[1] = A;
   if ((acknr % 2) == 0)
   {
      printf("\nRecalling with RR0...\n");
      response[2] = RR0;
   }
   else
   {
      printf("\nRecalling with RR1...\n");
      response[2] = RR1;
   }

   response[3] = response[1]^response[2];
   response[4] = FLAG;

   rej_response[0] = FLAG;
   rej_response[1] = A;
   if ((acknr % 2) == 0)
   {
      rej_response[2] = REJ0;
   }
   else
   {
      rej_response[2] = REJ1;
   }

   rej_response[3] = rej_response[1]^rej_response[2];
   rej_response[4] = FLAG; 

   conta = 1;
      
   while(conta < tries)
   {  
      countFlags = 0;
      alarm_flag = 0; 
      alarm(timeout);          
      while (1)
      {
         res = read(fd,r,1);
         buffer[i] = r[0];

         if (res < 0 && alarm_flag == 1)//TIMEOUT
         {
            printf("Didn't receive any information packet!TIMEOUT\n");
            break;
         }
         if(buffer[i] == FLAG)
         {
            countFlags++;
            if(countFlags == 2)
            {
               if (buffer[5] == *previous_n)
               {
                  printf("Repeated packet, N = %d\n", buffer[5]);
                  same_packet = 1;
               }

               conta = tries;
               i++;
               printf("Read %d bytes\n", i);
               break;
            }
         }
         i++;
      }
      alarm(0);
   }

   if (acknr >= 1) 
   {
      *previous_n = buffer[5];
      printf("Previous_N = %d\n", *previous_n);
   }
      
   if (!same_packet)
   {
      unsigned char data_package[11000];
      int data_package_size = i-9;
      memmove(data_package,&buffer[8],data_package_size);
      unsigned char *destuffed_message = deStuffBytes(data_package, &data_package_size);
      int real_size = 256 * buffer[6] + buffer[7];
      int j;
      unsigned char BCC2;
         
      for(j = 0; j < (data_package_size-1); j++)
      {
         if (j == 1)
         {
            BCC2 = destuffed_message[j-1]^destuffed_message[j]; 
         }  
         else if (j > 1 && j < data_package_size)
         {
            BCC2 = BCC2^destuffed_message[j];
         }
      }

      //Check if BCC's are ok
      if ((buffer[3] == (buffer[1]^buffer[2])) && BCC2 == destuffed_message[data_package_size-1] && real_size == ( data_package_size - 1))
      {
         package_ok = 1;
         *size += (data_package_size - 1);
      }
      else
      {
         package_ok = 0;
      }

      if(package_ok)
      { 
         printf("BCCs are fine, response sent\n");
         write(fd, response, 5); 
         return destuffed_message;
      }
      else 
      {
         printf("BCCS wrong!\n");
         write(fd, rej_response, 5);
         return NULL;
      } 
   }
   else
   {  
      write(fd, response, 5);
      char a[]={"A"};
      return a;
   }
}

int llwrite(char * buffer, int size, int package_number)
{
   write(fd, buffer, size);
   int i = 0, j = 0;
   int res;
   conta = 1;
   int sucess = 0;
   char response[5];
   char r[1];
      
   while(conta < tries)
   {  
      alarm_flag = 0; 
      alarm(timeout);          
      while (j != 5)
      {
         res = read(fd,r,1);
         printf("R = 0x%x\n", r[0]);
         response[j] = r[0];
            
         if (res < 0 && alarm_flag == 1)//TIMEOUT
         {
            printf("Didn't receive any response on package %d!TIMEOUT\n", package_number);
            //Send information packet again
            write(fd,buffer,size);
            break;
         }

         if(j == 0 && response[j] != FLAG)
         { 
            printf("First flag - bad reception\n");
            j--;
         }
         switch(j)
         {
            case 1:
            if(response[j] != A) 
            {
               if (response[j] == FLAG) j = 0;
               else j = -1;
               printf("A - bad reception\n");
            }
            break;
            case 2:
            if((package_number % 2) == 0)
            {
               if (response[j] != RR1 && response[j] != REJ1)
               {
                  if(response[j] == FLAG) j = 0;
                  else if (response[j] == A) j = 1;
                  printf("RR1 - bad reception\n");
               }
            }
            else 
            {
               if (response[j] != RR0 && response[j] != REJ0)
               {
                  if(response[j] == FLAG) j = 0;
                  else if (response[j] == A) j = 1;
                  printf("RR0 - bad reception\n");
               }
            }
            break;
            case 3:
            if(response[j] != (response[1]^response[2]) ) 
            {
               if(response[j] == FLAG) j = 0;
               else if (response[j] == A) j = 1;
               else if ( ((package_number % 2 == 1) && (response[j] == RR0 || response[j] == REJ0)) || ((package_number % 2 == 0) && (response[j] == RR1 || response[j] == REJ1))) j = 2;  
               printf("BCC - bad reception\n");
            }
            break;
            case 4:
            if(response[j] != FLAG) 
            {
               if (response[j] == A) j = 1;
               else if ( ((package_number % 2 == 1) && (response[j] == RR0 || response[j] == REJ0)) || ((package_number % 2 == 0) && (response[j] == RR1 || response[j] == REJ1))) j = 2;  
               else if (response[j] == response[1]^response[2]) j = 3;
               printf("Final flag - bad reception\n");
            }
            else sucess = 1;
            break;
         }
         j++;
      }

      if (sucess == 1)
      {
         printf("Sent %d bytes\n", size + 9);
         printf("\n\n\n--------------------------------\n\n\n");
         if (response[2] == RR0 || response[2] == RR1)
         {
            return size;
         }
               
         if (response[2] == REJ0 || response[2] == REJ1)
         {
            return -1;
         }

         break;
      }
      alarm(0);
   }
   if(!sucess) exit(1);  
}

void appSender(char *file, int iSize)
{
   unsigned char msg_control[4096];
   unsigned char msg_data[4096];
   unsigned int size, package_number = 0;
   unsigned char *information_packet, *previous_information_packet;
   int i, j, firstMessage = 1, nRetransmitted = 0, cnt = 0;
   struct stat st;
   unsigned char **dataPackages;
   fflush(stdin);
   alarm(0);
   //read file
   int file_desc = open(file, O_RDONLY,0);

   fstat(file_desc, &st);
   
   size = st.st_size;

   partitions =  (size + (iSize-1)) / iSize;
   printf("Partitions: %d\n", partitions);

   dataPackages = (char**) malloc(sizeof(char*) * partitions);

   //START
   msg_control[0] = 1;

   //SIZE
   msg_control[1] = 0; //Campo T
   msg_control[2] = 4; //Campo L

   for(i = 0; i < 4;i++)
   {
      msg_control[i+3] = (size & (0xFF << (24 - 8 * i) )) >> (24 - 8 * i);
   }

   //NOME
   msg_control[7] = 1; //Campo T
   msg_control[8] = strlen(file); //Campo L

   for(i = 0; i < strlen(file); i++)
   {
      msg_control[i+9] = file[i];
   }
   
   int count = 0;
   char BCC2 = NULL; 

   //Send Control Packet
   write(fd,msg_control,9 + strlen(file));

   for(i = 0; i < partitions; i++)
   {
      printf("\n\n\n---------- PARTICAO %d ------------\n\n\n", i +1);

      dataPackages[i] = (unsigned char*)malloc(sizeof(unsigned char) * (iSize + 4));

      char * data = (char *) malloc (sizeof(char) * (iSize + 5)); //Data + BCC2

      count = 0;
      for( j = 4; j < iSize + 4; j++)
      {
         char buf[1];
         if(read(file_desc, buf, 1) == 0) break;
         dataPackages[i][j] = buf[0];
         
         if (j == 5)
         {
            BCC2 = dataPackages[i][j-1] ^ dataPackages[i][j]; 
         }
         else if (j > 5)
         {
            BCC2 = BCC2 ^ dataPackages[i][j];
         }
         count++;
      }

      cnt+=count;
      printf("Partition Bytes = %d , total so far: %d\n", count,cnt);

      dataPackages[i][0] = 0;
      if ((i % 255) != 126)
      {
         dataPackages[i][1] = i % 255; //N
      }
      else
      {
         dataPackages[i][1] = -1;
      }
      dataPackages[i][2] = count / 256; //L2
      dataPackages[i][3] = count % 256; //L1

      printf("256 * %d + %d = %d\n",  dataPackages[i][2], dataPackages[i][3], 256 * dataPackages[i][2] + dataPackages[i][3]);

      for(j = 0; j < count + 4; j++) data[j] = dataPackages[i][j];
      data[count+4] = BCC2;
      
      size = count+1;

      char *stuffedMsg = stuffBytes(&data[4], &size);
      //printf("Data + BCC = %d\n", size);

      information_packet = realloc(information_packet, (sizeof(char) * size + 9));
         
      //Set up the first 4 bytes of the information packet
      information_packet[0] = FLAG;
      information_packet[1] = A;
      if ((package_number % 2) == 0)
      {
        // printf("Calling with NS = 0...\n");
         information_packet[2] = C_NS0;
      }
      else
      {
        // printf("Calling with NS = 1...\n");
         information_packet[2] = C_NS1;
      }
      information_packet[3] = information_packet[1]^information_packet[2];
      information_packet[4] = dataPackages[i][0];
      information_packet[5] = dataPackages[i][1];
      information_packet[6] = dataPackages[i][2];
      information_packet[7] = dataPackages[i][3];
      //Concatenate the data package and the information one
      memmove(&information_packet[8], stuffedMsg, size);

      //Add the FLAG byte to the end of the char*
      information_packet[size + 8] = FLAG;

      if(llwrite(information_packet, size + 9, package_number) == -1)
      {
         i--;
         package_number--;
         nRetransmitted++;
      }

      package_number++;
   }

   //Send termination control packet

   //END
   msg_control[0] = 2;

   write(fd,msg_control,9 + strlen(file));

   printf("%d Packages retransmitted\n", nRetransmitted);
   close(file_desc);
}

void appReceiver()
{
   unsigned char response[5];
   unsigned char rej_response[5];
   int acknr = 1;
   unsigned char control_buffer[11000];
   int package_ok = 0;
   unsigned int file_size = 0;
   unsigned char file_name[4096] = {};
   int res;
   int n_bytes = 9;
   int i = 0, j = 0;

   //Wait for the control packet
   while(conta < tries)
   {  
      alarm_flag = 0; 
      alarm(timeout);          
      while (i != n_bytes)
      {
         res = read(fd,&control_buffer[i],1);
         if (res < 0 && alarm_flag == 1)//TIMEOUT
         {
            printf("Didn't receive any control packet!TIMEOUT\n");
            break;
         }

         i++;

         if(i == 9) n_bytes += control_buffer[8];
      }
      if (i == n_bytes)
      {
         file_size = (control_buffer[3] << 24) | (control_buffer[4] << 16) | (control_buffer[5] << 8) | control_buffer[6];

         for(j = 9; j < n_bytes; j++)
         {
            char c[2] = {control_buffer[j], '\0'};
            strcat(file_name, c);
         }

         // printf("Finished reading control packet!\n");
         printf("File: %s, size: %d\n", file_name, file_size);
         conta = tries;
         break;
      }
      alarm(0);
   }

   //New Penguim file
   int new_file = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   
   int bytesRead = 0, repeatedPacks = 0, nRejs = 0;
   int bytesReturned = 0;
   int previous_n = -1;
   char *fileSt;
   int k = 0;

   while(bytesRead < file_size)
   {
      printf("\n\n\n---------- PARTICAO %d ------------\n\n\n", k +1);

      char a[]={"A"};
      fileSt = llread(acknr, &previous_n, &bytesReturned);
      char r[1];
      if(fileSt == NULL) 
      {
         acknr--;
         nRejs++;
      }
      else if (strcmp(fileSt,a) == 0) repeatedPacks++;
      else if (fileSt != NULL && strcmp(fileSt,a) != 0)
      {
         int i = 0;
	 k++;
         bytesRead += bytesReturned;
         for(i = 0; i < bytesReturned; i++)
         {
            write(new_file, &fileSt[i], 1);
         }    
      }

      acknr++;
   }

   //Wait for the termination control packet
   n_bytes = 9;
   conta = 1;
   i = 0;
   char file_name2[4096];
   while(conta < tries)
   {  
      alarm_flag = 0; 
      alarm(timeout);          
      while (i != n_bytes)
      {
         res = read(fd,&control_buffer[i],1);

         if (res < 0 && alarm_flag == 1)//TIMEOUT
         {
            printf("Didn't receive any control packet!TIMEOUT\n");
            break;
         }

         i++;

         if(i == 9) n_bytes += control_buffer[8];
      }

      if (i == n_bytes)
      {
         file_size |= control_buffer[3]; file_size <<= 24;
         file_size |= control_buffer[4]; file_size <<= 16;
         file_size |= control_buffer[5]; file_size <<= 8;
         file_size |= control_buffer[6];

         for(j = 9; j < n_bytes; j++)
         {
            char c[2] = {control_buffer[j], '\0'};
            strcat(file_name2, c);
         }
         printf("Received the terminating control packet!\n");
         printf("File: %s, size: %d\n", file_name2, file_size);
         getchar();
         conta = tries;
         break;
      }
      alarm(0);
   }

   printf("Received %d duplicated packets,%d rejected\n", repeatedPacks, nRejs);
   close(new_file);
}


int llopen(int port, int flag)
{
	int c, res;
	char porta[255] = "/dev/ttyS";
	struct sigaction sa;
	int success = 0;
   sa.sa_handler = atende;
   sa.sa_flags = 0;
   conta = 1;
	sigaction(SIGALRM, &sa, NULL);
	sprintf(porta+strlen(porta),"%d",port);
	printf("Port: %s\n", porta);

	/*
   	Open serial port device for reading and writing and not as controlling tty
   	because we don't want to get killed if linenoise sends CTRL-C.
  	*/

   fd = open(porta, O_RDWR | O_NOCTTY );

   if (fd <0) 
   {
   	printf("Erro ao abrir a porta %s", porta);
   	return -1; 
   }

   /* save current port settings */
   if ( tcgetattr(fd,&oldtio) == -1) 
   { 
   	perror("tcgetattr");
   	exit(-1);
   }

   bzero(&newtio, sizeof(newtio));
   newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR;
   newtio.c_oflag = 0;

   /* set input mode (non-canonical, no echo,...) */
   newtio.c_lflag = 0;

   newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
   newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

   tcflush(fd, TCIOFLUSH);

   if ( tcsetattr(fd,TCSANOW,&newtio) == -1) 
   {
   	perror("tcsetattr");
   	return -1;
   }

   if (flag == 0)
   {
    	//Emissor mode, send SET message code
   	unsigned char SET_MSG[5];

   	SET_MSG[0] = FLAG;
   	SET_MSG[1] = A;
   	SET_MSG[2] = C_SET;
   	SET_MSG[3] = SET_MSG[1]^SET_MSG[2];
   	SET_MSG[4] = FLAG;

   	res = write(fd,SET_MSG,5);

		//Receive acknolegment message code
   	char msg_received[255];

   	int i = 0;

   	fflush(stdin);
      waitForSignal(SET_MSG, NULL, UA, res);
   }
   else if (flag == 1)
   {
    	//Receiver mode
      char resposta[] = { FLAG, A, UA, A^UA, FLAG };
      char msg[255];   		

      int j = 0;
      while(j != 5 )
      {
         res=read(fd,&msg[j], 1);
         if(res<0)
         {
            printf("Erro  de leitura");
            exit(1);
         }
         if( j == 0 && msg[j] != FLAG)
         { 
            j--;
         }
         switch(j)
         {
            case 1:
            if(msg[j] != A) j--;
            break;
            case 2:
            if(msg[j] != 0x03) j--;
            break;
            case 3:
            if(msg[j] != (msg[1]^msg[2]) ) j--;
            break;
            case 4:
            if(msg[j] != FLAG) j--;
            else success = 1;
            break;
         }
         
         j++;
      }

      if( success)
      {  
         write(fd,resposta, 5);
         return 0;
      }
   }
   else
   {
    return -1;
   }
 sleep(1);
}

//Closes the comunication
int llclose(int flag)
{
   int res = 0;
   char msg[5];
   conta = 1;
   int i = 0;
   char msg_received[5];

   if (flag == 0)
   {
      //Send DISC

      sleep(1);

      conta = 1;
      msg[0] = FLAG;
      msg[1] = A;
      msg[2] = DISC;
      msg[3] = A^DISC;
      msg[4] = FLAG;
      int success = 0;

      write(fd,msg,5);

      //Wait for DISC

      waitForSignal(msg, NULL, DISC, res);

      //Send UA

      msg[0] = FLAG;
      msg[1] = A;
      msg[2] = UA;
      msg[3] = A^UA;
      msg[4] = FLAG;

      res = write(fd,msg,5);
   }
   else if (flag == 1)
   {
      conta = 1;
      //Send DISC
      msg[0] = FLAG;
      msg[1] = A;
      msg[2] = DISC;
      msg[3] = A^DISC;
      msg[4] = FLAG;
      int success = 0;
      int j = 0;

      while(j != 5 )
      {
         res=read(fd,&msg_received[j], 1);
         if(res < 0)
         {
            printf("Erro  de leitura\n");
            //exit(1);
         }

         if( j == 0 && msg[j] != FLAG)
         { 
            j--;
         }
         switch(j)
         {
            case 1:
            if(msg_received[j] != A) j--;
            break;
            case 2:
            if(msg_received[j] != DISC) j--;
            break;
            case 3:
            if(msg_received[j] != (msg_received[1]^msg_received[2]) ) j--;
            break;
            case 4:
            if(msg_received[j] != FLAG) j--;
            else success = 1;
            break;
         }
         j++;
         if( success) res = write(fd,msg,5);
      }
      
      success = 0;
      conta = 1;

      //Wait for UA
      waitForSignal(msg, NULL, UA, res);
   }
   else
   {
   	return -1;
   }

   sleep(1);

   if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) 
   {
   	perror("tcsetattr");
   	exit(-1);
   }

   close(fd);
   return 0;
}