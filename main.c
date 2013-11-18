#include "byteStuffing.h"
#include "app.h"

int main(int argc, char** argv)
{
   int opcao;
   int port = 0;
   int tamanho = 1024;

   if(atoi(argv[1]) == 0)
   if(argc < 3)
   {
      printf("Too few arguments! Usage: msg << 1, 0 >> << file_path >> \nPress Enter to exit...");
      getchar();
      exit(1);
   }

   do{
      printf("0 - ");
      
      if(atoi(argv[1])== 0) printf("Enviar ");
      else if(atoi(argv[1]) == 1) printf("Receber ");

      printf("ficheiro (default: pinguim.gif)\n");
      
      printf("1 - Definir baudrate (default:B38400) \n");
      printf("2 - Definir timeout (default: 3)\n");
      printf("3 - Definir numero de tentativas de retransmissao (default: 3)\n");
      printf("4 - Definir porta (default: 0)\n");
      
      if(atoi(argv[1]) == 0)
         printf("5 - Definir tamanho de trama (default: 1024)\n");

      scanf("%d", &opcao);

      switch(opcao)
      {
         case 1:
         printf("\nBAUDRATE: ");
         scanf("%d", &baud);
         break;

         case 2:
         printf("\nTIMEOUT (s): ");
         scanf("%d", &timeout);
         break;

         case 3:
         printf("\nNUMERO DE TENTATIVAS: ");
         scanf("%d", &tries);
         break;

         case 4:
         printf("\nPORTA (0,1,2...): ");
         scanf("%d", &port);
         break;

         case 5:

         if(atoi(argv[1]) == 0)
         {
            printf("\nTAMANHO: ");
            scanf("%d", &tamanho);
            
         }

         break;
      }

   }while(opcao != 0);

   llopen(port,atoi(argv[1]));

   if ((atoi(argv[1])) == 0) appSender(argv[2], tamanho);
    
   if ((atoi(argv[1])) == 1) appReceiver();

   printf("\nTERMINATING COMMUNICATIONS\n");    

   if (llclose(atoi(argv[1])) == 0)
   {
      printf("Timed out %d times\n",nTimeouts);
      printf("Works!\n");
   }
   return 0;   
}