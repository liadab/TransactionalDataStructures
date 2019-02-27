// TODO remove this file at all

#ifndef TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
#define TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H

#include <fstream>
static const char* LOG_FILE_PATH = "log_file.txt";
static std::mutex log_write;

template <class WriteObj>
static void write_to_log_file(const WriteObj& write_obj)
{
    std::lock_guard<std::mutex> l(log_write);
    std::ofstream log_file(LOG_FILE_PATH, std::ios_base::out | std::ios_base::app );
    log_file << write_obj;
}

static void restart_log_file()
{
    std::ofstream ofs;
    ofs.open(LOG_FILE_PATH, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

#endif //TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
