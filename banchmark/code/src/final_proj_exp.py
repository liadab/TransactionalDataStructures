import subprocess
import re
from argparse import ArgumentParser
from datetime import datetime
import matplotlib.pyplot as plt
plt.switch_backend('agg')

JAVA_EXE_PATH = r"./run"

TDS_EXE_PATH = r"./../CPP_proj_multi_proc/TransactionalDataStructures/build/tds"
TDS_DEBRA_EXE_PATH = r"./../CPP_proj_multi_proc/TransactionalDataStructures/build/tds_debra"
TDS_UNSAFE_EXE_PATH = r"./../CPP_proj_multi_proc/TransactionalDataStructures/build/tds_unsafe"

OUTPUT_FOLDER = r"./out/"


def avg(lst):
    return sum(lst) / float(len(lst))


def get_float_from_line(line):
    return float(re.findall('[0-9\.]+', line)[0])


def parse_program_res(res, dict_res):
    for line in res.split('\n'):
        if 'total ops succeed' in line:
            dict_res['NUM_SUCC_OPS'].append(get_float_from_line(line))
        elif 'total ops failed' in line:
            dict_res['NUM_FAIL_OPS'].append(get_float_from_line(line))
        elif 'total running time in secs' in line:
            dict_res['RUNTIME_IN_SEC'].append(get_float_from_line(line))


def run_impl(exe_path, n_threads, tasks, amount, inserts, removes, amount_runs):
    dict_res = {'NUM_SUCC_OPS': [], 'NUM_FAIL_OPS': [], 'RUNTIME_IN_SEC': []}

    for run in range(amount_runs):
        try:
            res = subprocess.check_output(
                [exe_path,
                 str(n_threads),
                 str(tasks),
                 str(amount),
                 str(inserts),
                 str(removes)]
            )

            parse_program_res(res.decode('utf-8'), dict_res)

        except Exception:
            print("failed")

    dict_avg_res = {'NUM_SUCC_OPS': avg(dict_res['NUM_SUCC_OPS']),
                    'NUM_FAIL_OPS': avg(dict_res['NUM_FAIL_OPS']),
                    'RUNTIME_IN_SEC': avg(dict_res['RUNTIME_IN_SEC'])}

    return dict_avg_res


def get_date():
    curr = datetime.now()
    return curr.strftime("%Y%m%d_%H%M%S")


def create_plot(tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, max_threads, amount_runs):
    x_axis = []
    java_y_axis = []
    tds_y_axis = []
    # tds_debra_y_axis = []
    tds_unsafe_y_axis = []

    for n_threads in range(1, max_threads):
        x_axis.append(n_threads)

        print(n_threads, "threads, running java...")
        java_dict_avg_res = run_impl(JAVA_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, amount_runs)
        print(n_threads, "threads, java results:      ", java_dict_avg_res)
        java_y_axis.append(java_dict_avg_res['RUNTIME_IN_SEC'])

        print(n_threads, "threads, running tds...")
        tds_dict_avg_res = run_impl(TDS_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, amount_runs)
        print(n_threads, "threads, tds results:       ", tds_dict_avg_res)
        tds_y_axis.append(tds_dict_avg_res['RUNTIME_IN_SEC'])

        # print(n_threads, "threads, running tds debra...")
        # tds_debra_dict_avg_res = run_impl(TDS_DEBRA_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, amount_runs)
        # print(n_threads, "threads, tds debra results: ", tds_debra_dict_avg_res)
        # tds_debra_y_axis.append(tds_debra_dict_avg_res['RUNTIME_IN_SEC'])

        print(n_threads, "threads, running tds unsafe...")
        tds_unsafe_dict_avg_res = run_impl(TDS_UNSAFE_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, amount_runs)
        print(n_threads, "threads, tds unsafe results:", tds_unsafe_dict_avg_res)
        tds_unsafe_y_axis.append(tds_unsafe_dict_avg_res['RUNTIME_IN_SEC'])

    plt.plot(x_axis, java_y_axis, label="JAVA")
    plt.plot(x_axis, tds_y_axis, label="TDS")
    # plt.plot(x_axis, tds_debra_y_axis, label="TDS_DEBRA")
    plt.plot(x_axis, tds_unsafe_y_axis, label="TDS_UNSAFE")

    plt.xlabel("number threads")
    plt.ylabel("time in sec")
    plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3,
               ncol=2, mode="expand", borderaxespad=0.)

    plt.savefig(OUTPUT_FOLDER
                + "out" + "_"
                + str(tasks) + "_"
                + str(tasks_per_tx) + "_"
                + str(x_of_100_inserts) + "_"
                + str(x_of_100_removes) + "_"
                + str(amount_runs) + "_"
                + str(get_date())
                + ".png")


def main():
    parser = ArgumentParser()
    parser.add_argument("-t", "--tasks", type=int, required=True, help="how much tasks to run")
    parser.add_argument("-a", "--tasks-per-tx", type=int, default=10, help="amount of tasks per transaction (default: %(default))")
    parser.add_argument("-i", "--x-of-100-inserts", type=int, default=45, help="inserts precentage out of all the tasks")
    parser.add_argument("-r", "--x-of-100-removes", type=int, default=45, help="removes precentage out of all the tasks")
    parser.add_argument("-m", "--max-threads", type=int, default=30, help="max threads")
    parser.add_argument("-n", "--amount-runs", type=int, default=10, help="amount times to run each process, we take the avarage time upon runnings")

    args = parser.parse_args()
    create_plot(args.tasks, args.tasks_per_tx, args.x_of_100_inserts, args.x_of_100_removes, args.max_threads, args.amount_runs)


if __name__ == '__main__':
    main()

