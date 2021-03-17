//
// Created by feng on 2021/3/15.
//

#include "Log.h"
#include <iostream>
#include <cstring>
#include <fstream>
using namespace std;

Log::Level Log::level(INFO);

#ifndef DEFAULT_LOG_PATH
#define DEFAULT_LOG_PATH "log/trojan.log"
#endif

static const array<string,6> name = {"ALL","INFO","WARN","ERROR","FATAL","OFF"};
ofstream Log::output(DEFAULT_LOG_PATH,ios_base::out|ios_base::app);
//ofstream Log::keyoutput;

void Log::log(const string &message, Level level) {

#ifdef ENABLE_LOG
    if (level >= Log::level) {
        cout<<message<<endl;
        if (output.is_open())
            output<<message<<endl;
        else
            cout<<"Open log file failed"<<endl;
    }
#endif
}


void Log::log_with_date_time(const string &message, Level level) {

#ifdef ENABLE_LOG
    time_t t = time(nullptr);
	    char ch[64] = {0};
	    strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));
        string level_string = "[" + name[level] + "] ";
        string tmp_data = string(ch);
        log(tmp_data + level_string + message, level);
#endif

}

void Log::stop(){
    if (output.is_open())
    {
        output.close();
    }
}

void Log::redirect(const string &filename) {

    ofstream tmp(filename,ios_base::out|ios_base::app);
    if (!tmp.is_open()) {
        throw runtime_error(filename + ": " + strerror(errno));
    }
    if (output.is_open()) {
        output.close();
    }
    output = move(tmp);
}
