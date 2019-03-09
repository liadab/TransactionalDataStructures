# Transactional Data Structures Library
This library implement a transactional data structure library, which enables the use of transactions.
These transactions ensures atomicity of a series of operations on different data sets.

The library is written as a project for a course in TAU, instructed by Dr. Morrison Adam.
The library's implementation follows the described in the paper [paper](http://dl.acm.org/citation.cfm?id=2908112&CFID=728652129&CFTOKEN=92618137): A. Spiegelman, G. Golan-Gueta and I. Keidar: Transactional Data Structure Libraries. PLDI 2016.
And highly leaned on the Java [implementation](https://github.com/HagarMeir/transactionLib).

## Usage

```cpp
TX tx;
LinkedList<size_t, size_t>& LL1;
LinkedList<size_t, size_t>& LL2;
while (true) {
    try {
        tx->TXbegin();
        LL1.put(3, 7);
        LL2.put(5, 5);
        LL2.put(6, 10);
        auto ret = LL1.remove(1);
        break;
    } catch (TxAbortException& e) {
        continue;
    }
}
```
