

char* stuffBytes(char *msg, int *msg_size)
{
   char* stuffed_msg;

   //malloc stuffed message
   stuffed_msg = (char *) malloc(sizeof(char) * (*msg_size));
   memcpy(stuffed_msg,msg,(*msg_size));

   int i = 0;
   int j = 0;

   for(i = 0; i < (*msg_size); i++)
   {
      if (stuffed_msg[i] == 0x7d)
      {
         (*msg_size)++;
         stuffed_msg = realloc(stuffed_msg, (*msg_size));
         memmove(&stuffed_msg[i+2], &stuffed_msg[i+1], sizeof(char) * ((*msg_size)-i-2));

         stuffed_msg[i] = 0x7d;
         stuffed_msg[i+1] = 0x5d;
      }
      else if (stuffed_msg[i] == 0x7e)
      {
         (*msg_size)++;
         stuffed_msg = realloc(stuffed_msg, (*msg_size));
         memmove(&stuffed_msg[i+2], &stuffed_msg[i+1], sizeof(char) * ((*msg_size)-i-2));

         stuffed_msg[i] = 0x7d;
         stuffed_msg[i+1] = 0x5e;
      }
   } 
   return stuffed_msg;
}


char* deStuffBytes(char *msg, int *msg_size)
{
   char *destuffed_msg;

   //malloc destuffed mesage
   destuffed_msg = (char *) malloc (sizeof(char) * (*msg_size));
   memcpy(destuffed_msg,msg,(*msg_size));
   int i;

   for (i = 0; i < (*msg_size); i++)
   {
      if (destuffed_msg[i] == 0x7d)
      {
         if (destuffed_msg[i+1] == 0x5d)
         {
            (*msg_size)--;
            destuffed_msg = realloc(destuffed_msg, (*msg_size-1));

            memmove(&destuffed_msg[i+1], &destuffed_msg[i+2], (*msg_size)-i-1);
         }
         else if(destuffed_msg[i+1] == 0x5e)
         {
            (*msg_size)--;

            destuffed_msg = realloc(destuffed_msg, (*msg_size-1));

            memmove(&destuffed_msg[i+1], &destuffed_msg[i+2], (*msg_size)-i-1);

            destuffed_msg[i] = 0x7e;
         }
      }
   }
   return destuffed_msg;
}
