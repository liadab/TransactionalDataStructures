#include <iostream>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <stdlib.h>
#include <ctime>
#include <chrono>
#include <list>
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
    size_t key;
    size_t val;
};

static const int N_INIT_LIST = 1000;

class Worker
{
public:

    Worker(const std::vector<Task>& _tasks,
           const int _tasks_index_begin,
           const int _tasks_index_end,
           LinkedList<size_t, size_t>& _LL,
           std::shared_ptr<TX> _tx,
           const int _ops_per_transc,
           std::shared_ptr<RecordMgr<size_t, size_t>::record_manager_t> global_recordMgr,
           size_t tid
    ):
            tasks(_tasks),
            tasks_index_begin(_tasks_index_begin),
            tasks_index_end(_tasks_index_end),
            LL(_LL),
            tx(std::move(_tx)),
            ops_per_transc(_ops_per_transc),
            recordMgr(global_recordMgr, tid)
    {}

    void work()
    {
        int ops_in_tx = 0;
        int inserts_occurred_in_tx = 0;
        int removes_occurred_in_tx = 0;

        for (unsigned int index_task = tasks_index_begin; index_task < tasks_index_end; index_task++) {
            try {
                if (ops_in_tx == 0)
                {
                    tx->TXbegin();
                }
                ops_in_tx++;
                commit_task_and_update_counters(index_task, inserts_occurred_in_tx, removes_occurred_in_tx);

                if (ops_in_tx == ops_per_transc || index_task == tasks_index_end - 1)
                {
                    tx->TXend<size_t, size_t>(recordMgr);

                    inserts_occurred += inserts_occurred_in_tx;
                    removes_occurred += removes_occurred_in_tx;
                    succ_ops += ops_in_tx;

                    ops_in_tx = 0;
                    inserts_occurred_in_tx = 0;
                    removes_occurred_in_tx = 0;
                }
            }
            catch(TxAbortException& e)
            {
                tx->handle_abort<size_t, size_t>(recordMgr);
                fail_ops += ops_in_tx;

                ops_in_tx = 0;
                inserts_occurred_in_tx = 0;
                removes_occurred_in_tx = 0;
            }
        }
    }

    int getSucc_ops() const { return succ_ops; }

    int getFail_ops() const { return fail_ops; }

    int getInserts_occurred() const { return inserts_occurred; }

    int getRemoves_occurred() const { return removes_occurred; }

private:
    const std::vector<Task>& tasks;
    const int tasks_index_begin;
    const int tasks_index_end;
    LinkedList<size_t, size_t>& LL;
    std::shared_ptr<TX> tx;
    const int ops_per_transc;
    RecordMgr<size_t, size_t> recordMgr;

    int succ_ops = 0;
    int fail_ops = 0;
    int inserts_occurred = 0;
    int removes_occurred = 0;

    void commit_task_and_update_counters(int index_task,
                                         int &inserts_occurred_in_transc,
                                         int &removes_occurred_in_transc)
    {
        Task task = tasks.at(index_task);
        switch (task.task_type)
        {
            case TaskType::INSERT:
                if (LL.put(task.key, task.val, recordMgr) == NULLOPT)
                {
                    inserts_occurred_in_transc++;
                }
                break;
            case TaskType::REMOVE:
                if (!(LL.remove(task.key, recordMgr) == NULLOPT))
                {
                    removes_occurred_in_transc++;
                }
                break;
            case TaskType::CONTAINS:
                LL.containsKey(task.key, recordMgr);
                break;
        }
    }
};

Task get_random_task(TaskType task_op)
{
    size_t key = (rand() % (N_INIT_LIST * 10)) + 1;

    size_t val;
    if (task_op == TaskType::INSERT)
    {
        val = (rand() % (N_INIT_LIST * 10)) + 1;
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

void print_tasks_vector(const std::vector<Task> tasks) //for debug
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

int init_linked_list(LinkedList<size_t, size_t>& LL,
                     std::shared_ptr<TX> tx,
                     const RecordMgr<size_t, size_t>& recordMgr)
{
    tx->TXbegin(); //we use tx here to avoid singleton operation
    int init_LL_size = 0;
    for (int i = 0; i < N_INIT_LIST; i++)
    {
        Task task = get_random_task(INSERT);
        if (LL.put(task.key, task.val, recordMgr) == NULLOPT)
        {
            init_LL_size++;
        }
    }
    tx->TXend<size_t, size_t>(recordMgr);
    return init_LL_size;
}

void print_results(std::list<Worker>& workers, int linked_list_init_size, int n_threads,
                   std::chrono::duration<double>& running_time_sec)
{
    int total_linked_list_size = linked_list_init_size;
    int total_ops_succeed = 0;
    int total_ops_failed = 0;
    size_t count = 0;
    for (const auto &worker : workers) {
        int inserts_occurred = worker.getInserts_occurred();
        int removes_occurred = worker.getRemoves_occurred();
        int succ_ops = worker.getSucc_ops();
        int fail_ops = worker.getFail_ops();

        std::cout << "\nThread " << ++count << std::endl;
        std::cout << "inserts occurred:" << inserts_occurred << std::endl;
        std::cout << "removes occurred:" << removes_occurred << std::endl;
        std::cout << "succ ops:" << succ_ops << std::endl;
        std::cout << "fail ops:" << fail_ops << std::endl;

        total_linked_list_size += inserts_occurred;
        total_linked_list_size -= removes_occurred;
        total_ops_succeed += succ_ops;
        total_ops_failed += fail_ops;
    }

    std::cout << "\n////////\nToatl: " << std::endl;
    std::cout << "total ops succeed: " << total_ops_succeed << std::endl;
    std::cout << "total ops failed: " << total_ops_failed << std::endl;
    std::cout << "total LL size: " << total_linked_list_size << std::endl;

    std::cout << "total running time in secs: " << running_time_sec.count() << std::endl;
}

int main(int argc, char *argv[]) {
    //parameters:
    uint32_t n_threads = std::atoi(argv[1]);
    uint32_t n_tasks = std::atoi(argv[2]);
    uint32_t n_tasks_per_transaction = std::atoi(argv[3]);
    uint32_t x_of_100_inserts = std::atoi(argv[4]);
    uint32_t x_of_100_removes = std::atoi(argv[5]);

    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(n_threads + 1);

    //create random tasks:
    std::vector<Task> tasks;
    fill_tasks_vector(tasks, n_tasks, x_of_100_inserts, x_of_100_removes);
    //print_tasks_vector(tasks); //for debug

    //create linked list:
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    LinkedList<size_t, size_t> linked_list(tx, record_mgr);

    //init linked list:
    int init_LL_size = init_linked_list(linked_list, tx, record_mgr);
    std::cout << "initial linked list size:" << init_LL_size << std::endl;

    //create workers:
    std::list<Worker> workers;

    for (size_t i = 0; i < n_threads; i++)
    {
        int index_begin = i * n_tasks / n_threads;
        int index_end = (i + 1) * n_tasks / n_threads;

        workers.emplace_back(tasks, index_begin, index_end, linked_list, tx, n_tasks_per_transaction, global_record_mgr, i + 1);
    }

    //measure time start:
    auto start_time = std::chrono::high_resolution_clock::now();

    //run workers:
    std::vector<std::thread> threads;
    for (auto& worker: workers)
    {
        threads.push_back(std::thread( [&]() { worker.work(); } ));
    }
    for (auto &thread: threads)
    {
        thread.join();
    }

    //measure time end:
    auto end_time = std::chrono::high_resolution_clock::now();

    //done and print results:
    std::cout << "DONE" << std::endl;
    std::chrono::duration<double> running_time_sec = end_time - start_time;
    print_results(workers, init_LL_size, n_threads, running_time_sec);
    std::cout << "actual linked list size: " << linked_list.get_size() << std::endl;
    linked_list.deinit_list(record_mgr);
    return 0;
}