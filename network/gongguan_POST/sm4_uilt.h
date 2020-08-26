#ifndef SM4_TEST_H
#define SM4_TEST_H

#include <QApplication>


void test_sm4();

QString encryptHMac(QString Text);
QString encryptData_CBC(QString plainText);
QString decryptData_CBC(QString cipherText);

#endif // SM4_TEST_H
