# Task Queue Cpp

## Requirements

- The main thread continuously checks the task queue and run it FIFO
- Task queue will not stop
- When `clear` is called, it clears all tasks in the queue
- Some tasks take way longer than others

## How to build

Run:

```bash
g++ -std=c++17 -pthread ./taskQueue.cpp -o taskQueue
```
