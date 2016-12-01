#include <iostream>

#define NLOOPS 1000000000
long long sum = 0;

void* parallel_count(void* arg) {
  int offset = *((int *) arg);
  for (int i = 0; i < NLOOPS; i++) {
    sum += offset;
  }
  return NULL;
}

int main(void)
{
  int offset1 = 1;
  parallel_count(&offset1);
  int offset2 = -1;
  parallel_count(&offset2);
  std::cout << "Sum is " << sum << "\n";
  return 0;
}
