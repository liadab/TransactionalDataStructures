import subprocess
import re
from argparse import ArgumentParser
from datetime import datetime
import matplotlib.pyplot as plt
plt.switch_backend('agg')

JAVA_EXE_PATH = r"./run"

TDS_EXE_PATH = r"./../build/tds"
TDS_DEBRA_EXE_PATH = r"./../build/tds_debra"
TDS_UNSAFE_EXE_PATH = r"./../build/tds_unsafe"

OUTPUT_FOLDER = r"./out/"

def avg(lst):
    if len(lst) == 0:  # means there was a problem
        return 100
    return sum(lst) / float(len(lst))


def get_float_from_line(line):
    return float(re.findall('[0-9\.]+', line)[0])


def parse_program_res(res, dict_res):
    for line in res.split('\n'):
        if 'total ops succeed' in line:
            dict_res['NUM_SUCC_OPS'].append(get_float_from_line(line))
        elif 'total running time in secs' in line:
            dict_res['RUNTIME_IN_SEC'].append(get_float_from_line(line))


def run_impl(exe_path, n_threads, tasks, amount, inserts, removes, amount_runs, runtime_lst, succ_ops_lst):
    dict_res = {'NUM_SUCC_OPS': [], 'NUM_FAIL_OPS': [], 'RUNTIME_IN_SEC': []}

    for run in range(amount_runs):
        try:
            res = subprocess.check_output([exe_path, str(n_threads), str(tasks), str(amount), str(inserts), str(removes)], stderr=subprocess.STDOUT, timeout=300)
            parse_program_res(res.decode('utf-8'), dict_res)

        except Exception:
            continue

    dict_avg_res = {'NUM_SUCC_OPS': avg(dict_res['NUM_SUCC_OPS']),
                    'RUNTIME_IN_SEC': avg(dict_res['RUNTIME_IN_SEC'])}

    runtime_lst.append(dict_avg_res['RUNTIME_IN_SEC'])
    succ_ops_lst.append(dict_avg_res['NUM_SUCC_OPS'])

    return dict_avg_res


def get_date():
    curr = datetime.now()
    return curr.strftime("%Y%m%d_%H%M%S")


def create_plot(tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes, max_threads, amount_runs):
    x_axis = []

    java_runtime = []
    tds_runtime = []
    tds_debra_runtime = []
    tds_unsafe_runtime = []

    java_succ_ops = []
    tds_succ_ops = []
    tds_debra_succ_ops = []
    tds_unsafe_succ_ops = []

    for n_threads in range(1, max_threads + 1):
        x_axis.append(n_threads)

        print(n_threads, "threads, running java...")
        run_impl(JAVA_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes,
                 amount_runs, java_runtime, java_succ_ops)

        print(n_threads, "threads, running tds...")
        run_impl(TDS_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes,
                 amount_runs, tds_runtime, tds_succ_ops)

        print(n_threads, "threads, running tds debra...")
        run_impl(TDS_DEBRA_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes,
                 amount_runs, tds_debra_runtime, tds_debra_succ_ops)

        print(n_threads, "threads, running tds unsafe...")
        run_impl(TDS_UNSAFE_EXE_PATH, n_threads, tasks, tasks_per_tx, x_of_100_inserts, x_of_100_removes,
                 amount_runs, tds_unsafe_runtime, tds_unsafe_succ_ops)

    # save runtime plot
    plt.plot(x_axis, java_runtime, label="JAVA")
    plt.plot(x_axis, tds_runtime, label="TDS")
    plt.plot(x_axis, tds_debra_runtime, label="TDS_DEBRA")
    plt.plot(x_axis, tds_unsafe_runtime, label="TDS_UNSAFE")

    plt.xlabel("number threads")
    plt.ylabel("time in sec")
    plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.)

    plt.savefig(OUTPUT_FOLDER
                + "out" + "_"
                + "runtime" + "_"
                + str(tasks) + "_"
                + str(tasks_per_tx) + "_"
                + str(x_of_100_inserts) + "_"
                + str(x_of_100_removes) + "_"
                + str(amount_runs) + "_"
                + str(get_date())
                + ".png")

    plt.close()

    # save succ ops plot
    plt.plot(x_axis, java_succ_ops, label="JAVA")
    plt.plot(x_axis, tds_succ_ops, label="TDS")
    plt.plot(x_axis, tds_debra_succ_ops, label="TDS_DEBRA")
    plt.plot(x_axis, tds_unsafe_succ_ops, label="TDS_UNSAFE")

    plt.xlabel("number threads")
    plt.ylabel("operations succeed")
    plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.)

    plt.savefig(OUTPUT_FOLDER
                + "out" + "_"
                + "succeedOps" + "_"
                + str(tasks) + "_"
                + str(tasks_per_tx) + "_"
                + str(x_of_100_inserts) + "_"
                + str(x_of_100_removes) + "_"
                + str(amount_runs) + "_"
                + str(get_date())
                + ".png")


def main():
    parser = ArgumentParser()
    parser.add_argument("-t", "--tasks", type=int, default=10000, help="how much tasks to run")
    parser.add_argument("-a", "--tasks-per-tx", type=int, default=10, help="amount of tasks per transaction (default: %(default))")
    parser.add_argument("-i", "--x-of-100-inserts", type=int, default=45, help="inserts precentage out of all the tasks")
    parser.add_argument("-r", "--x-of-100-removes", type=int, default=45, help="removes precentage out of all the tasks")
    parser.add_argument("-m", "--max-threads", type=int, default=30, help="max threads")
    parser.add_argument("-n", "--amount-runs", type=int, default=10, help="amount times to run each process, we take the avarage time upon runnings")

    args = parser.parse_args()
    create_plot(args.tasks, args.tasks_per_tx, args.x_of_100_inserts, args.x_of_100_removes, args.max_threads, args.amount_runs)


if __name__ == '__main__':
    main()

