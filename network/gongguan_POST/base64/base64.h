#ifndef BASE64_H
#define BASE64_H

#include<QApplication>
#include <stdlib.h>
#include <string.h>

QString base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);

#endif // BASE64_H
