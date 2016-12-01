#include <stdio.h>
#include <pthread.h>
#include <iostream>

#define NLOOPS 10000000
long long sum = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* parallel_count(void* arg) {
  int offset = *((int *) arg); // Cast to int pointer, then dereference.
  for (int i = 0; i < NLOOPS; i++) {
    // Start critical section.
    pthread_mutex_lock(&mutex);
    sum += offset;
    // End critical section.
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
  return NULL;
}

int main(void)
{
  pthread_t proc1;
  int offset1 = 1;
  pthread_create(&proc1, NULL, parallel_count, &offset1);

  pthread_t proc2;
  int offset2 = -1;
  pthread_create(&proc2, NULL, parallel_count, &offset2);

  // Wait for threads to finish.
  pthread_join(proc1, NULL);
  pthread_join(proc2, NULL);

  std::cout << "Sum is " << sum << "\n";
  return 0;
}
