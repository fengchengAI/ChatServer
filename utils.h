//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_UTILS_H
#define UNIX_NETWORK_UTILS_H


#include <string>
#include <ctime>
#include <cstdlib> // Header file needed to use srand and rand
#include <climits>
using namespace std;

std::string getDateTime();
void setSockNonBlock(int sock);
std::string chartostring(char *s, size_t length);
bool readdata(int fd, char *buf, int length);
bool writedata(int fd, const char *buf, int length);
int getRandValue(int left = 1, int right = UINT_MAX);
int get4BitInt(char const *);

#endif //UNIX_NETWORK_UTILS_H
