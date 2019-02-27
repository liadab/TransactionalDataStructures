
#ifndef TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
#define TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H

#include <fstream>
static const char* LOG_FILE_PATH = "log_file.txt";

template <class WriteObj>
static void write_to_log_file(const WriteObj& write_obj)
{
    std::ofstream log_file(LOG_FILE_PATH, std::ios_base::out | std::ios_base::app );
    log_file << write_obj << std::endl;
}


#endif //TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
