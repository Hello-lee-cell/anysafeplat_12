/*
 * SM4/SMS4 algorithm test programme
 * 2020-8-8
 */
#include <QApplication>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <bits/stdc++.h>
#include<QString>
#include <QtDebug>
#include<QRegExp>

#include"sm4/sm4.h"
#include"sm4_uilt.h"
#include"sm4/util.h"
#include"sm4/sm3.h"
#include"base64/base64.h"


bool hexString = false;
QString secretKey = "0123456789abcdef";
QString iv = "0000000000000000";

QString testcipher= "CPz+Kx7QrvUjBphEOJzFZg5Vh2vpqt1yeQyU/8pRYFKQo8GKeT5W0ky7R4NmFN64nF4PUsHJKOY8ebsAsd6I5kBd366/U+rrFLMHynNjvfW7bIKJkkLgG0xgP3oUgobZJLXMYxR+Iqz2JyTigk+lHbX9L8NfuJwODqq68nw9yWTckzJnGJTM6Afr5IvJevHxSfMIHyFH1sQmXBf4zcJir65R4Nm6fSYtIrBb7ff2J19y2+m3x3XLdXtbalI8FH09";
        //"BOuaxh/GmRrYtNnklwxF4nMKg0djjYDrNOhsPcqbNZUhnz/svrLICQjdAB/FzUO2YtnGNqaB+w1Luz39TlD4Dthpsd3iEvy/yRlh1/AR9gmZNPWO+ncDFYZzCPSuOGsLKaeM1Xw3G2ql0km5xtwRznwTKuQihUxkMXBPaHkbpE7vdhi8QPMd2zrcyKTqhh7g";
QString testplain = "<rows><row><ID>000005</ID><DATE>20200804100103</DATE><JYJID>0001</JYJID><JYQID>0001</JYQID><OPERATE>1</OPERATE><EVENT>1</EVENT></row></rows>";
QString testhmax = "2poCiYc2VRvUB4WlbMp6wggFbONuQqJ7zG9bsxG5gi8=";
QString testcipher1 = "BOuaxh/GmRrYtNnklwxF4u19brtYy4uIXQD5z5TZF2F/lSqLVXUFFymTDGQnbJrMvESuRuExnJjuYLbBGj2GtWL5k9/vr3IideogqKSznIVwXaeWlDFkT9WgO75VgCenjoSF86U8vOE/rfcJ/IR5Vopi7fKFrwLvTTViu3Lgl85mcQB1mfrxG5a/A1IrSHyZuwS4RqMMZAiiW16Ik9NJnRmubZ2DrKcYT/Xlg5qVFhbEkBlVMJUaT8cwRGOlnul0U3dhGVzSAxUGkaZgZtZ4brwBnelWQ31RMSCzwAPfKuGECpsOoa+xnAHztIURno5+NptM1SHAtTDVyVNiEKorQPPsnC7SR2RaOelZjUgHDgqSLP969nVO5XKxNE7mONEo";
QString testplain1 = "<rows><row><ID>000001</ID><DATE>20120317235959</DATE><AL>3:N;6:N;15:N;18:N;21:N;24:N;25:N;26:N;27:N;28:N;30:N;32:N;9:N;12:N;29:N;31:N;</AL><MB>0</MB><YZ>0</YZ><YGYL>0</YGYL><YGLY>0</YGLY><PVZT>0</PVZT><PVLJZT>0</PVLJZT><HCLZT>0</HCLZT><HCLND>0</HCLND><XYHQG>0</XYHQG></row></rows>";
QString testhmax1 = "ah5jSfHi8MIZn2NLPxmmsJ80VFDQ1iIwn1rzG3YmyIw=";

void test_sm4()
{	    
//    printf("%s ", (decryptData_CBC(testcipher1)).toLatin1().data());fflush(stdout);
//    printf("%s \n", (encryptData_CBC(testplain)).toLatin1().data());fflush(stdout);
//    printf("%d ", testplain1.length());fflush(stdout);
    printf("%s \n",encryptHMac(testcipher).toAscii().data());
}

QString encryptHMac(QString Text)
{
    sm4_context ctx;
    uchar *keyBytes;

    QByteArray  ByteArrsec = secretKey.toLatin1();
    keyBytes = (uchar*)ByteArrsec.data();

    uchar *inputini;
    uchar input[32];
    uchar output[42];
    QByteArray  ByteArrtx = Text.toLocal8Bit();
    inputini = (uchar*)ByteArrtx.data();
    int length = strlen((char*)inputini);

    SM3Calc(inputini, length, input);

    sm4_setkey_enc(&ctx,keyBytes);
    sm4_crypt_ecb(&ctx,input,output);

    for(int i=0;i<32;i++)
    {
       printf("%02x ",input[i]);
    }
    printf("\n");
    for(int i=0;i<32;i++)
    {
       printf("%02x ",output[i]);
    }
    printf("\n");

    QString cipherText = base64_encode(output,32);

    return cipherText;
}

QString encryptData_CBC(QString plainText)
{
    sm4_context ctx;
    ctx.isPadding = true;
    ctx.mode = SM4_ENCRYPT;

    uchar *keyBytes;
    uchar *ivBytes;
    if (hexString)
    {
       keyBytes =  hexStringToBytes(secretKey);
       ivBytes = hexStringToBytes(iv);
    }
    else
    {
        QByteArray  ByteArrsec = secretKey.toLatin1();
        QByteArray  ByteArriv = iv.toLatin1();
        keyBytes = (uchar*)ByteArrsec.data();
        ivBytes = (uchar*)ByteArriv.data();
    }
    uchar *inputini;
    uchar output[1000];
    QByteArray  ByteArrptx = plainText.toLocal8Bit();
    inputini = (uchar*)ByteArrptx.data();

    int p = 16 - (strlen((char*)inputini) %16);
    int length = strlen((char*)inputini)+p;
    unsigned char input[length];
    memcpy(input,inputini,strlen((char*)inputini));
    for (int i = 0; i < p; i++)
    {
       input[strlen((char*)inputini) + i]  = p;
    }

    sm4_setkey_enc(&ctx,keyBytes);
    sm4_crypt_cbc(&ctx,SM4_ENCRYPT,ivBytes,input,output);

    QString cipherText = base64_encode(output,length);

//    if(cipherText!= NULL && cipherText.trimmed().length()>0)
//    {
//       QRegExp reg("\\s*|\t|\r|\n");
//       reg.setMinimal(true);
//       cipherText.replace(reg,"");
//    }
    return cipherText;
}

QString decryptData_CBC(QString cipherText)
{
    sm4_context ctx;
    ctx.isPadding = true;
    ctx.mode = SM4_DECRYPT;
    
    uchar *keyBytes;
    uchar *ivBytes;
    if (hexString)
    {
       keyBytes =  hexStringToBytes(secretKey);
       ivBytes = hexStringToBytes(iv);
    }
    else
    {
        QByteArray  ByteArrsec = secretKey.toLatin1();
        QByteArray  ByteArriv = iv.toLatin1();
        keyBytes = (uchar*)ByteArrsec.data();
        ivBytes = (uchar*)ByteArriv.data();
    }

    uchar *input;
    uchar output[1000];
    QByteArray byteArray(cipherText.toStdString().c_str());
    QByteArray decdata=QByteArray::fromBase64(byteArray);
    input = (uchar*)decdata.data();

    sm4_setkey_dec(&ctx,keyBytes);
    sm4_crypt_cbc(&ctx,SM4_DECRYPT,ivBytes,input,output);

    printf("%d \n", cipherText.length());fflush(stdout);
    printf("%d \n", strlen((char*)input));fflush(stdout);

    return QString((char*)output).toLatin1();
}
