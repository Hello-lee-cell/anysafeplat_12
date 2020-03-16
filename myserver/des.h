#ifndef DES_H
#define DES_H
#include <QString>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef bool Bit;
typedef bool (*PSubKey)[16][48];

//加密解密
enum
{
	ENCRYPT	= 0,	//! 加密
	DECRYPT			//! 解密
};

//DES算法的模式
enum
{
	ECB		=	0,	//! ECB模式
	CBC				//! CBC模式
};
//Pad填充的模式
enum
{
	PAD_ISO_1 =	0,	//! ISO_1填充：数据长度不足8比特的倍数，以0x00补足，如果为8比特的倍数，补8个0x00
	PAD_ISO_2,		//! ISO_2填充：数据长度不足8比特的倍数，以0x80,0x00..补足，如果为8比特的倍数，补0x80,0x00..0x00
	PAD_PKCS_7		//! PKCS7填充：数据长度除8余数为n,以(8-n)补足为8的倍数
};
class DES
{
private:
	static const int IP_Table[64];
	static const int IPR_Table[64];
	static const int E_Table[48];
	static const int P_Table[32];
	static const int PC1_Table[56];
	static const int PC2_Table[48];
	static const int LOOP_Table[16];
	static const int S_Box[8][4][16];

	unsigned keylen;

private:
	static void ByteToBit(Bit *Out, const unsigned char *In, int bits);
	static void BitToByte(unsigned char *Out, const Bit *In, int bits);
	static void RotateL(Bit *In, int len, int loop);
	static void Xor(Bit *InA, const Bit *InB, int len);
	static void Transform(Bit *Out, Bit *In, const int *Table, int len);
	static void S_func(Bit Out[32], const Bit In[48]);
	static void F_func(Bit In[], const Bit Ki[]);
	bool RunPad(int nType, const unsigned char* In,
	            unsigned datalen, unsigned char* Out, unsigned& padlen);
	void SetSubKey(PSubKey pSubKey, const unsigned char Key[8]);
	void DES_Unit(unsigned char Out[8], const unsigned char In[8], const PSubKey pSubKey, bool Type);

	bool RunDes(bool bType,bool bMode,const unsigned char* In,
	                 unsigned char* Out,unsigned datalen,const unsigned char* Key,unsigned keylen);

public:
	DES(){};
	~DES(){};

	bool RunDesEncryption(const unsigned char *input,  unsigned char *output,unsigned int datalen,const QString Key ,int mode );
//    void RunDesEncryption(const unsigned char *input, unsigned char * output, const unsigned char * key );
	bool FileEncryption(const QString FileName, const QString key,int mode,bool rm);
	bool FileDecryption(const QString FileName, const QString key,int mode, bool rm);
//    void RunDesDecryption(const unsigned char *input, unsigned char * output, const unsigned char * key);
	bool RunDesDecryption(const unsigned char *input,  unsigned char *output,unsigned int datalen,const QString Key ,int mode);
	void Fill(unsigned char *plainBlock,char ch,int n);

};

#endif // DES_H
