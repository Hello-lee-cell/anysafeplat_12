#include"util.h"


uchar* hexStringToBytes(QString hexString)
{
   hexString = hexString.toAscii().toUpper().data();
   int length = hexString.length() /2;

   char hexChars[length+1] ;
   QByteArray  ByteArr = hexString.toLocal8Bit();
   memcpy(hexChars,ByteArr.data(),ByteArr.size()+1);

   uchar d[length];
   for (int i = 0; i < length; i++)
   {
       int pos = i * 2;
       d[i] = (uchar) (charToByte(hexChars[pos]) << 4 | charToByte(hexChars[pos + 1]));
   }
   return d;
}

uchar charToByte(char c)
{
    QString str = "0123456789ABCDEF";
    return (uchar)(str.indexOf(c));
}
