#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

std::atomic<bool> x, y;
std::atomic<int> z;

// sequential consisten view std::memory_order_cstmt
// x is written before y: w(x) < w(y) means y=TRUE => x=TRUE => z = 1
//
// std::memory_order_relaxed
// no guarantee that ordering is as given in writer thread. So both is possible
// w(x)<w(y) OR w(y)<w(x) => Z=0 OR Z=1
// 

void write_x_then_y() {
  x.store(true, std::memory_order_relaxed);
  y.store(true, std::memory_order_relaxed);
}

void read_y_then_x() {
  while (!y.load(std::memory_order_relaxed))
    ; // Wait until thread sees y updated to TRUE
  if (x.load(std::memory_order_relaxed)) {
    z++; // Increment Z if X is TRUE
  }
}

int main(int argc, char* argv[])
{
  x = false;
  y = false;
  z = 0;

  std::thread writer_thread(write_x_then_y);
  std::thread reader_thread(read_y_then_x);

  writer_thread.join();
  reader_thread.join();

  assert(z != 0);

  return 0;
}

