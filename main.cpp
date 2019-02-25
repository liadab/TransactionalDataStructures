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

enum TaskType
{
    INSERT,
    REMOVE,
    CONTAINS,
};

struct Task
{
    TaskType task_type;
    int key;
    std::string val;
};

static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const int N_INIT_LIST = 1000;

class Worker
{
public:

    Worker(const std::vector<Task>& _tasks,
           const int _tasks_index_begin,
           const int _tasks_index_end,
           LinkedList<int, std::string>& _LL,
           std::shared_ptr<TX> _tx,
           const int _ops_per_transc,
           int& _succ_ops,
           int& _fail_ops,
           int& _inserts_occurred,
           int& _removes_occurred):
                   tasks(_tasks),
                   tasks_index_begin(_tasks_index_begin),
                   tasks_index_end(_tasks_index_end),
                   LL(_LL),
                   tx(_tx),
                   ops_per_transc(_ops_per_transc),
                   succ_ops(_succ_ops),
                   fail_ops(_fail_ops),
                   inserts_occurred(_inserts_occurred),
                   removes_occurred(_removes_occurred)
    {}

    void work()
    {
        int cntr_ops_in_transc = 0;
        int cntr_inserts_occurred_in_tx = 0;
        int cntr_removes_occurred_in_tx = 0;

        for (unsigned int index_task = tasks_index_begin; index_task < tasks_index_end; index_task++) {
            try {
                if (cntr_ops_in_transc == 0)
                {
                    tx->TXbegin();
                }
                cntr_ops_in_transc++;
                commit_task_and_update_counters(index_task, cntr_inserts_occurred_in_tx, cntr_removes_occurred_in_tx);

                if (cntr_ops_in_transc == ops_per_transc || index_task == tasks_index_end - 1)
                {
                    tx->TXend<int, std::string>();

                    inserts_occurred += cntr_inserts_occurred_in_tx;
                    removes_occurred += cntr_removes_occurred_in_tx;
                    succ_ops += cntr_ops_in_transc;

                    cntr_ops_in_transc = 0;
                    cntr_inserts_occurred_in_tx = 0;
                    cntr_removes_occurred_in_tx = 0;
                }
            }
            catch(TxAbortException& e)
            {
                fail_ops += cntr_ops_in_transc;

                cntr_ops_in_transc = 0;
                cntr_inserts_occurred_in_tx = 0;
                cntr_removes_occurred_in_tx = 0;
            }
        }
    }

private:
    const std::vector<Task>& tasks;
    const int tasks_index_begin;
    const int tasks_index_end;
    LinkedList<int, std::string>& LL;
    std::shared_ptr<TX> tx;
    const int ops_per_transc;

    int& succ_ops;
    int& fail_ops;
    int& inserts_occurred;
    int& removes_occurred;

    void commit_task_and_update_counters(int index_task,
                                         int &cntr_inserts_occurred_in_transc,
                                         int &cntr_removes_occurred_in_transc)
    {
        Task task = tasks.at(index_task);
        switch (task.task_type)
        {
            case TaskType::INSERT:
                if (LL.put(task.key, task.val) == NULLOPT)
                {
                    cntr_inserts_occurred_in_transc++;
                }
                break;
            case TaskType::REMOVE:
                if (!(LL.remove(task.key) == NULLOPT))
                {
                    cntr_removes_occurred_in_transc++;
                }
                break;
            case TaskType::CONTAINS:
                LL.containsKey(task.key);
                break;
        }
    }
};

Task get_random_task(TaskType task_op)
{
    int key = (rand() % (N_INIT_LIST * 5)) + 1;

    std::string val;
    if (task_op == TaskType::INSERT)
    {
        val += alphanum[rand() % (sizeof(alphanum) - 1)];
        val += alphanum[rand() % (sizeof(alphanum) - 1)];
        val += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

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
        tasks.push_back(get_random_task(TaskType::INSERT));
    }
    for (int i = 0; i < n_removes; i++)
    {
        tasks.push_back(get_random_task(TaskType::REMOVE));
    }
    for (int i = 0; i < n_contains; i++)
    {
        tasks.push_back(get_random_task(TaskType::CONTAINS));
    }

    std::random_shuffle(tasks.begin(), tasks.end());
}

void print_tasks_vector(const std::vector<Task> tasks)
{
    for (unsigned int i = 0; i < tasks.size(); i++)
    {
        const Task & task = tasks.at(i);
        switch (task.task_type)
        {
            case TaskType::INSERT:
                std::cout << "INSERT " << task.key << " " << task.val << std::endl;
                break;
            case TaskType::REMOVE:
                std::cout << "REMOVE " << task.key << " " << task.val << std::endl;
                break;
            case TaskType::CONTAINS:
                std::cout << "CONTAINS " << task.key << " " << task.val << std::endl;
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

void print_results(std::vector<int> succ_ops, std::vector<int> fail_ops,
                   std::vector<int> inserts_occurred, std::vector<int> removes_occurred,
                   int linked_list_init_size, int n_threads)
{
    int total_linked_list_size = linked_list_init_size;
    int total_ops_succeed = 0;
    int total_ops_failed = 0;

    for (int i = 0; i < n_threads; i++)
    {
        std::cout << "\nThread " << i << std::endl;
        std::cout << "inserts occurred:" << inserts_occurred.at(i) << std::endl;
        std::cout << "removes occurred:" << removes_occurred.at(i) << std::endl;
        std::cout << "succ ops:" << succ_ops.at(i) << std::endl;
        std::cout << "fail ops:" << fail_ops.at(i) << std::endl;

        total_linked_list_size += inserts_occurred.at(i);
        total_linked_list_size -= removes_occurred.at(i);
        total_ops_succeed += succ_ops.at(i);
        total_ops_failed += fail_ops.at(i);
    }

    std::cout << "\n////////\nToatl: " << std::endl;
    std::cout << "total ops succeed: " << total_ops_succeed << std::endl;
    std::cout << "total ops failed: " << total_ops_failed << std::endl;
    std::cout << "total LL size: " << total_linked_list_size << std::endl;
}

int main(int argc, char *argv[]) {
    //parameters:
    uint32_t n_threads = std::atoi(argv[1]);
    uint32_t n_tasks = std::atoi(argv[2]);
    uint32_t n_tasks_per_transaction = std::atoi(argv[3]);
    uint32_t x_of_100_inserts = std::atoi(argv[4]);
    uint32_t x_of_100_removes = std::atoi(argv[5]);

    //create random tasks:
    std::vector<Task> tasks;
    fill_tasks_vector(tasks, n_tasks, x_of_100_inserts, x_of_100_removes);
    //print_tasks_vector(tasks); //for debug

    //create linked list:
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<int, std::string> linked_list(tx);

    //init linked list:
    int init_LL_size = init_linked_list(linked_list, tx);
    std::cout << "linked list size:" << init_LL_size << std::endl;

    //create workers:
    std::vector<Worker> workers;
    std::vector<int> succ_ops;
    std::vector<int> fail_ops;
    std::vector<int> inserts_occurred;
    std::vector<int> removes_occurred;
    for (size_t i = 0; i < n_threads; i++)
    {
        int index_begin = i * n_tasks / n_threads;
        int index_end = (i + 1) * n_tasks / n_threads;

        succ_ops.emplace_back(0);
        fail_ops.emplace_back(0);
        inserts_occurred.emplace_back(0);
        removes_occurred.emplace_back(0);

        workers.push_back(Worker(tasks,
                                 index_begin,
                                 index_end,
                                 linked_list,
                                 tx,
                                 n_tasks_per_transaction,
                                 succ_ops.at(i),
                                 fail_ops.at(i),
                                 inserts_occurred.at(i),
                                 removes_occurred.at(i)));
    }

    //run workers:
    std::vector<std::thread> threads;
    for (auto &worker: workers)
    {
        threads.push_back(std::thread( [&]{ worker.work(); } ));
    }
    for (auto &thread: threads)
    {
         thread.join();
    }

    //done:
    std::cout << "DONE" << std::endl;
    print_results(succ_ops, fail_ops, inserts_occurred, removes_occurred, init_LL_size, n_threads);
    return 0;
}

//TODO delete comments:
//
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