package main;

import main.*;
import sun.security.util.Length;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Random;

public class Main {

	static Integer N_INIT_LIST;

	static enum OP { INSERT, REMOVE, CONTAINS }

	static private class Task
	{
		public Task(OP task_type, Integer key, long val)
		{
			this.task_type = task_type;
			this.key = key;
			this.val = val;
		}

		public OP task_type;
		public Integer key;
		public long val;

	}

	static class Worker extends Thread
	{
		ArrayList<Task> tasks;
		Integer tasks_index_begin;
		Integer tasks_index_end;
		LinkedList LL;
		Integer ops_per_transc;

		Integer succ_ops = 0;
	    Integer fail_ops = 0;
	    Integer inserts_occurred = 0;
	    Integer removes_occurred = 0;

		public Worker(ArrayList<Task> tasks,
					  int tasks_index_begin,
				      int tasks_index_end,
				      LinkedList LL,
				      int ops_per_transc)
		{
			super();
			this.tasks = tasks;
			this.tasks_index_begin = tasks_index_begin;
			this.tasks_index_end = tasks_index_end;
			this.LL = LL;
			this.ops_per_transc = ops_per_transc;
		}

		private void commit_task_and_update_counters(int task_index,
													 Integer[] inserts_occurred_in_tx,
													 Integer[] removes_occurred_in_tx)
		{
			Task task = tasks.get(task_index);
			switch (task.task_type)
			{
				case INSERT:
					if (LL.put(task.key, task.val) == null)
					{
						inserts_occurred_in_tx[0]++;
					}
					break;
				case REMOVE:
					if (!(LL.remove(task.key) == null))
					{
						removes_occurred_in_tx[0]++;
					}
					break;
				case CONTAINS:
					LL.containsKey(task.key);
					break;
			}
		}

		@Override
		public void run()
		{
		    Integer[] inserts_occurred_in_tx = new Integer[] {0};
			Integer[] removes_occurred_in_tx = new Integer[] {0};
			int ops_in_tx = 0;

			for (int task_index = tasks_index_begin; task_index < tasks_index_end; task_index++) {
				try {
					if (ops_in_tx == 0)
					{
						TX.TXbegin();
					}
					ops_in_tx++;
					commit_task_and_update_counters(task_index, inserts_occurred_in_tx, removes_occurred_in_tx);

					if (ops_in_tx == ops_per_transc || task_index == tasks_index_end - 1)
					{
						TX.TXend();

						inserts_occurred += inserts_occurred_in_tx[0];
		                removes_occurred += removes_occurred_in_tx[0];
		                succ_ops += ops_in_tx;

		                ops_in_tx = 0;
		                inserts_occurred_in_tx[0] = 0;
		                removes_occurred_in_tx[0] = 0;
					}
				}
				catch(TXLibExceptions.AbortException exp)
				{
					fail_ops += ops_in_tx;

					ops_in_tx = 0;
		            inserts_occurred_in_tx[0] = 0;
		            removes_occurred_in_tx[0] = 0;
				}
			}
		}

		public Integer getSucc_ops() {
			return succ_ops;
		}

		public Integer getFail_ops() {
			return fail_ops;
		}

		public Integer getInserts_occurred() {
			return inserts_occurred;
		}

		public Integer getRemoves_occurred() {
			return removes_occurred;
		}
	}

	static Task get_random_task(OP task_op)
	{
		Random rand = new Random();
	    int key = (rand.nextInt(N_INIT_LIST * 10) + 1);

	    long val = 0;
	    if (task_op == OP.INSERT)
	    {
		    val = rand.nextLong();
	    }

	    return (new Task(task_op, key, val));
	}

	static void fill_tasks_vector(ArrayList<Task> tasks,
								  Integer n_tasks,
								  Integer x_of_100_inserts,
								  Integer x_of_100_removes)
	{
	    Integer n_inserts = n_tasks * x_of_100_inserts / 100;
	    Integer n_removes = n_tasks * x_of_100_removes / 100;
	    Integer n_contains = n_tasks - (n_inserts + n_removes);

	    for (int i = 0; i < n_inserts; i++)
	    {
	        tasks.add(get_random_task(OP.INSERT));
	    }
	    for (int i = 0; i < n_removes; i++)
	    {
	        tasks.add(get_random_task(OP.REMOVE));
	    }
	    for (int i = 0; i < n_contains; i++)
	    {
	        tasks.add(get_random_task(OP.CONTAINS));
	    }

	    Collections.shuffle(tasks);
	}

	static void print_tasks_vector(ArrayList<Task> tasks)
	{
	    for (int i = 0; i < tasks.size(); i++)
	    {
	        Task task = tasks.get(i);
	        switch (task.task_type)
	        {
	            case INSERT:
	                System.out.println("INSERT " + task.key + " " + task.val);
	                break;
	            case REMOVE:
	            	System.out.println("REMOVE " + task.key + " " + task.val);
	                break;
	            case CONTAINS:
	            	System.out.println("CONTAINS " + task.key + " " + task.val);
	                break;
	        }
	    }
	}

	static int init_linked_list(LinkedList LL)
	{
	    TX.TXbegin();
	    int init_LL_size = 0;
	    for (int i = 0; i < N_INIT_LIST; i++)
	    {
	        Task task = get_random_task(OP.INSERT);
	        if (LL.put(task.key, task.val) == null)
	        {
	            init_LL_size++;
	        }
	    }
	    TX.TXend();
	    return init_LL_size;
	}

	static void print_results(Worker[] workers, int linked_list_init_size, int n_threads,
							  double running_time_sec, long actual_linked_list_size)
	{
		int expected_total_linked_list_size = linked_list_init_size;
		int total_ops_succeed = 0;
		int total_ops_failed = 0;

		for (int i = 0; i < n_threads; i++)
		{
			int inserts_occurred = workers[i].getInserts_occurred();
			int removes_occurred = workers[i].getRemoves_occurred();
			int succ_ops = workers[i].getSucc_ops();
			int fail_ops = workers[i].getFail_ops();

			 System.out.println("\nThread " + i);
			 System.out.println("inserts occurred:" + inserts_occurred);
			 System.out.println("removes occurred:" + removes_occurred);
			 System.out.println("succ ops:" + succ_ops);
			 System.out.println("fail ops:" + fail_ops);

			 expected_total_linked_list_size += inserts_occurred;
			 expected_total_linked_list_size -= removes_occurred;
			 total_ops_succeed += succ_ops;
			 total_ops_failed += fail_ops;
		}

		System.out.println("\nTotal: ");
		System.out.println("total ops succeed: " + total_ops_succeed);
		System.out.println("total ops failed: " + total_ops_failed);
		System.out.println("expected linked list size: " + expected_total_linked_list_size);
		System.out.println("actual linked list size: " + actual_linked_list_size);

		System.out.println("total running time in secs: " + running_time_sec);
	}

	public static void main(String[] args)
	{
		//parameters:
		Integer n_threads = Integer.parseInt(args[0]);
		Integer n_tasks = Integer.parseInt(args[1]);
		Integer n_tasks_per_transaction = Integer.parseInt(args[2]);
		Integer x_of_100_inserts = Integer.parseInt(args[3]);
		Integer x_of_100_removes = Integer.parseInt(args[4]);

	    //create random tasks:
		N_INIT_LIST = n_tasks / 10;
	    ArrayList<Task> tasks = new ArrayList<Task>();
	    fill_tasks_vector(tasks, n_tasks, x_of_100_inserts, x_of_100_removes);
	    //print_tasks_vector(tasks); //for debug

	    //create linked list:
	    LinkedList linked_list = new LinkedList();

	    //initialize linked list:
	    int init_LL_size = init_linked_list(linked_list);
	    System.out.println("initial linked list size: " + init_LL_size);

	    //measure start time:
	    long startTime = System.nanoTime();

	    //create and run workers:
	    Worker[] workers = new Worker[n_threads];
	    for (int i = 0; i < workers.length; i++)
	    {
	    	int index_begin = i * n_tasks / n_threads;
	        int index_end = (i + 1) * n_tasks / n_threads;

	        workers[i] = new Worker(tasks, index_begin, index_end, linked_list, n_tasks_per_transaction);
	    	workers[i].start();
	    }
	    for (int i = 0; i <  workers.length; i++)
	    {
	    	try{
	    		workers[i].join();
	    	}
	    	catch (InterruptedException e) { e.printStackTrace(); System.exit(-1); }
	    }

	    //measure end time:
	    long endTime = System.nanoTime();

	    //done:
	    double elapsed_time = (double)(endTime - startTime) / 1_000_000_000.0;
	    print_results(workers, init_LL_size, n_threads, elapsed_time, linked_list.getLinkedListSize());
	}		
}