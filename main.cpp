#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <stdlib.h>
#include "algorithm"

#include "nodes/LNode.h"
#include "nodes/LNodeWrapper.h"
#include "datatypes/LinkedList.h"
//#include "nodes/utils.h"
//#include "nodes/Index.h"

enum OP
{
    INSERT,
    REMOVE,
    CONTAINS,
};

struct Task
{
    OP task_type;
    int key;
    std::string val;
};

static const std::string DUMMY_VAL = "dummy";
static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const int N_INIT_LIST = 1000;

void commit_task(const Task& task, LinkedList<int, std::string>& LL, int& succ_inserts, int& succ_removes)
{
    switch (task.task_type)
    {
        case INSERT:
            if (LL.put(task.key, task.val) == NULLOPT)
            {
                succ_inserts++;
            }
            break;
        case REMOVE:
            if (!(LL.remove(task.key) == NULLOPT))
            {
                succ_removes++;
            }
            break;
        case CONTAINS:
            LL.containsKey(task.key);
            break;
    }
}

void work(const std::vector<Task>& tasks,
           const int tasks_index_begin,
           const int tasks_index_end,
           LinkedList<int, std::string>& LL,
           std::shared_ptr<TX> tx,
           const int ops_per_transc)
{
    int succ_inserts = 0;
    int succ_removes = 0;

    int succ_ops = 0;
    int fail_ops = 0;

    int counter_ops_in_transc = 0;

    for (unsigned int i = tasks_index_begin; i < tasks_index_end; i++) {
        try {
            if (counter_ops_in_transc == 0)
            {
                tx->TXbegin();
            }

            commit_task(tasks.at(i), LL, succ_inserts, succ_removes);
            counter_ops_in_transc++;

            if (counter_ops_in_transc == ops_per_transc || i == tasks_index_end - 1)
            {
                tx->TXend<int, std::string>();
                succ_ops += counter_ops_in_transc;
                counter_ops_in_transc = 0;
            }
        }
        catch(TxAbortException& e)
        {
            fail_ops += counter_ops_in_transc;
            counter_ops_in_transc = 0;
            std::cout << "catched: " << i << std::endl;
        }
    }
    std::cout << "Inserts succeeded: " << succ_inserts << std::endl;
    std::cout << "Removes succeeded: " << succ_removes << std::endl;
    std::cout << "Operations succeeded: " << succ_ops << std::endl;
    std::cout << "Operations failed: " << fail_ops << std::endl;

}

Task get_random_task(OP task_op)
{
    int key = (rand() % (N_INIT_LIST*10)) + 1;

    std::string val;
    val += alphanum[rand() % (sizeof(alphanum) - 1)];
    val += alphanum[rand() % (sizeof(alphanum) - 1)];
    val += alphanum[rand() % (sizeof(alphanum) - 1)];

    return {task_op, key, val};
}

void fill_tasks_vector(std::vector<Task>& tasks,
                       uint32_t n_tasks,
                       uint32_t x_of_100_inserts,
                       uint32_t x_of_100_removes)
{
    uint32_t n_inserts = n_tasks * x_of_100_inserts / 100;
    uint32_t n_removes = n_tasks * x_of_100_removes / 100;
    uint32_t n_contains = n_tasks - (n_inserts + n_removes);

    for (int i = 0; i < n_inserts; i++)
    {
        tasks.push_back(get_random_task(INSERT));
    }
    for (int i = 0; i < n_removes; i++)
    {
        tasks.push_back(get_random_task(REMOVE));
    }
    for (int i = 0; i < n_contains; i++)
    {
        tasks.push_back(get_random_task(CONTAINS));
    }

    std::random_shuffle(tasks.begin(), tasks.end());
}

//for debuging:
void print_tasks_vector(const std::vector<Task> tasks)
{
    for (unsigned int i = 0; i < tasks.size(); i++)
    {
        const Task & task = tasks.at(i);
        switch (task.task_type)
        {
            case INSERT:
                std::cout << "INSERT " << task.key << " " << task.val << std::endl;
                break;
            case REMOVE:
                std::cout << "REMOVE " << task.key << " " << task.val << std::endl;
                break;
            case CONTAINS:
                std::cout << "CONTAI " << task.key << " " << task.val << std::endl;
                break;
        }
    }
}

int init_linked_list(LinkedList<int, std::string>& LL, std::shared_ptr<TX> tx)
{
    tx->TXbegin(); //we use tx here to avoid singleton operation
    int init_LL_size = 0;
    for (int i = 0; i < N_INIT_LIST; i++)
    {
        Task task = get_random_task(INSERT);
        if (LL.put(task.key, task.val) == NULLOPT)
        {
            init_LL_size++;
        }
    }
    tx->TXend<int, std::string>();
    return init_LL_size;
}

int main(int argc, char *argv[]) {

//    uint32_t n_tasks = argv[0];
//    uint32_t n_threads = argv[1];
//	  uint32_t n_tasks_per_transaction = argv[2];
//	  uint32_t x_of_100_inserts = std::stoi(argv[3]);
//    uint32_t x_of_100_removes = std::stoi(argv[4]);
//
//    std::vector<Task> tasks2{
//            {INSERT, 3, "three"},
//            {INSERT, 4, "four"},
//            {INSERT, 45, "rfour"},
//            {REMOVE, 4, DUMMY_VAL},
//            {REMOVE, 5, DUMMY_VAL},
//            {CONTAINS, 2, DUMMY_VAL},
//            {CONTAINS, 4, DUMMY_VAL}
//    };

    uint32_t n_threads = 2;
    uint32_t n_tasks = 10;
    uint32_t n_tasks_per_transaction = 2;
    uint32_t x_of_100_inserts = 50;
    uint32_t x_of_100_removes = 25;

    std::vector<Task> tasks;
    fill_tasks_vector(tasks, n_tasks, x_of_100_inserts, x_of_100_removes);

    std::shared_ptr<TX> tx = std::make_shared<TX>();

    LinkedList<int, std::string> linked_list(tx);
    int init_LL_size = init_linked_list(linked_list, tx);
    std::cout << "linked list size:" << init_LL_size << std::endl;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < n_threads; ++i) {
        int index_begin = i * n_tasks / n_threads;
        int index_end = (i + 1) * n_tasks / n_threads;

        threads.emplace_back([&tasks, index_begin, index_end, &linked_list, tx, n_tasks, n_tasks_per_transaction]() {
            work(tasks, index_begin, index_end, linked_list, tx, n_tasks_per_transaction);
        });
    }
    for (auto &t : threads) {
        t.join();
    }

    std::cout << "DONE" << std::endl;
    return 0;
}