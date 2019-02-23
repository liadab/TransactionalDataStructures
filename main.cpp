#include <iostream>
#include <thread>
#include <string>
#include <vector>

#include "nodes/LNode.h"
#include "nodes/LNodeWrapper.h"
#include "datatypes/LinkedList.h"
//#include "nodes/utils.h"
//#include "nodes/Index.h"

enum OP{
    INSERT,
    REMOVE,
    CONTAINS,
};

struct Task{
    OP task_type;
    int key;
    std::string val;
};

void commit_task(const Task& task, LinkedList<int, std::string>& LL)
{
    switch (task.task_type)
    {
        case INSERT:
            LL.put(task.key, task.val);
        case REMOVE:
            LL.remove(task.key);
        case CONTAINS:
            LL.containsKey(task.key);
    }
}

void worker(const std::vector<Task>& tasks,
            const int tasks_index_begin,
            const int tasks_index_end,
            LinkedList<int, std::string>& LL,
            std::shared_ptr<TX> tx,
            const int ops_per_transc)
{
    int counter_ops_in_transc = 0;
    int succ_ops = 0;
    int fail_ops = 0;

    for (unsigned int i = tasks_index_begin; i < tasks_index_end; i++) {
        try {
            if (counter_ops_in_transc == 0)
            {
                tx->TXbegin();
            }

            commit_task(tasks.at(i), LL);
            counter_ops_in_transc++;

            if (counter_ops_in_transc == ops_per_transc)
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
    std::cout << "Operations succeeded: " << succ_ops << std::endl;
    std::cout << "Operations failed: " << fail_ops << std::endl;
}

int main() {

    //insert 100,000 puts at start
    //in the end check that size(LL) = #succ insert - #succ remove

    std::string DUMMY_VAL = "dummy";

    std::vector<Task> tasks{
            {INSERT, 3, "three"},
            {INSERT, 4, "four"},
            {REMOVE, 4, DUMMY_VAL},
            {CONTAINS, 2, DUMMY_VAL}
    };

    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<int, std::string> LL(tx);
    worker(tasks, 0, 4, LL, tx, 2);

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