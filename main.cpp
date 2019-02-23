#include <iostream>
#include <thread>
#include <string>
#include <vector>

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

void worker(const std::vector<Task>& tasks,
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
        }
    }
    std::cout << "Inserts succeeded: " << succ_inserts << std::endl;
    std::cout << "Removes succeeded: " << succ_removes << std::endl;
    std::cout << "Operations succeeded: " << succ_ops << std::endl;
    std::cout << "Operations failed: " << fail_ops << std::endl;
}

int main() {

    //insert 100,000 puts at start
    //in the end check that size(LL) = #succ insert - #succ remove

    std::vector<Task> tasks{
        {INSERT, 3, "three"},
        {INSERT, 4, "four"},
        {INSERT, 45, "rfour"},
        {REMOVE, 4, DUMMY_VAL},
        {REMOVE, 5, DUMMY_VAL},
        {CONTAINS, 2, DUMMY_VAL},
        {CONTAINS, 4, DUMMY_VAL}
    };

    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<int, std::string> LL(tx);
    worker(tasks, 0, 6, LL, tx, 2);

//    std::vector<std::thread> threads;
//    for (size_t i = 0; i < num_threads; ++i) {
//        threads.emplace_back([i, &l, &inputs_keys, &inputs_vals]() {
//           l.put(inputs_keys[i], inputs_vals[i]);
//        });
//    }
//    for (auto &t : threads) {
//        t.join();
//    }

    return 0;
}