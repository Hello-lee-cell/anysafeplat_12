/*base64.c*/
#include "base64.h"

// 全局常量定义
const char  base64char[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

QString base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
     QString dest;
     int i = 0, j = 0;
     unsigned char char_array_3[3], char_array_4[4];

     while(in_len--)
     {
          char_array_3[i++] = *(bytes_to_encode++);
          if (3 == i)
          {
               char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
               char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
               char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
               char_array_4[3] = char_array_3[2] & 0x3f;

               for(i = 0; i < 4; i++)
                   dest += base64char[char_array_4[i]];
               i = 0;
            }
     }

     if (i)
     {
          for(j = i; j < 3; j++)
              char_array_3[j] = '\0';

          char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
          char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
          char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
          char_array_4[3] = char_array_3[2] & 0x3f;

          for (j = 0; j < (i + 1); j++)
               dest += base64char[char_array_4[j]];

          while((i++ < 3))
               dest += '=';
      }

     return dest;
}
