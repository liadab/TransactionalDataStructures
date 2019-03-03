
#define TRANSACTIONALDATASTRUCTURES_LOG_FILE_HELPER_H
// TODO remove this file at all

#ifndef TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H
#define TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H

#include <map>
#include <fstream>
#include <sstream>
#include <thread>

static std::string get_file_name()
{
    std::thread::id thread_id = std::this_thread::get_id();
    std::stringstream file_name;
    file_name << "logs/log_file_" << thread_id << ".txt";

    return file_name.str();
}

static thread_local std::ofstream log_file(get_file_name(), std::ios_base::out | std::ios_base::app );

static void write_to_log_file(const std::string& str)
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    log_file << now << " : " << str << std::endl;
}

static void restart_log_file()
{
    system("rm -r ./logs/*");
}

#endif //TRANSACTIONALDATASTRUCTURES_LOGFILEHELPER_H

