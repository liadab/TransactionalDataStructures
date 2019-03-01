// TODO remove this file at all

#ifndef TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
#define TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H

#include <fstream>
#include <sstream>

#define main_write_to_log_file(str) //{write_to_log_file("MAIN: " + std::string(str) + "\n");}
//#define linked_list_write_to_log_file(str) //{write_to_log_file("\tLNKD_LST: " + std::string(str) + "\n");}
//#define index_write_to_log_file(str) //{write_to_log_file("\tINDEX:    " + std::string(str) + "\n");}

#define MAX_COUNT 100


template <class WriteObj>
static void write_to_log_file(const WriteObj& write_obj)
{
    std::thread::id thread_id = std::this_thread::get_id();
    std::stringstream file_name;
    file_name << "logs/log_file_" << thread_id << ".txt";

    std::ofstream log_file(file_name.str(), std::ios_base::out | std::ios_base::app );
    log_file << write_obj;
}

static void restart_log_file()
{
    try {
        system("rm -r ./logs/*");
    }
    catch (OptionalException){}
}

#endif //TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
