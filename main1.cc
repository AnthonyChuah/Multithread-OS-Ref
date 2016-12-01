#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFSIZE 10
#define NTHREADS 4
#define JOBS_PER_PROD 4
#define NPRODS 5
#define NCONS 5
#define JOBS_PER_CONS 4

int buffer[BUFFSIZE]; // This is the maximum number of jobs allowed in the jobs queue.
int buffer_index; // This thing is like a permanent iterator going up and down the buffer to allow for insertion
// and extraction of jobs.
int job = 0; // Simply a job ID so that I can see the trace of jobs completed by this multithreaded program.

pthread_mutex_t mutex;

sem_t full_sem; // This semaphore starts off having a high counter equal to buffer size.
// It will prevent producer from inserting into a full buffer, but because buffer starts empty, it starts off
// by being permissive and allowing BUFFSIZE number of jobs to come in.
sem_t empty_sem; // This semaphore starts off at 0. It prevents consumer from extracting a job from the buffer.

void addjob(int job) {
  if (buffer_index < BUFFSIZE) {
    buffer[buffer_index] = job;
    buffer_index++;
  } else {
    std::cout << "Buffer overflow.\n";
  }
}

int dojob() {
  if (buffer_index > 0) {
    int jobnum = buffer[buffer_index];
    buffer_index--;
    return jobnum;
  } else {
    std::cout << "Buffer underflow.\n";
  }
  return 0;
}

int isempty() {
  if (buffer_index == 0) return 1;
  return 0;
}

int isfull() {
  if (buffer_index == BUFFSIZE) return 1;
  return 0;
}

void* producer(void* thread_n) {
  int threadid = (*(int*) thread_n);
  for (int i = 0; i < JOBS_PER_PROD; i++) {
    sleep(1);
    job = job + 1;
    pthread_mutex_lock(&mutex);
    do {
      pthread_mutex_unlock(&mutex); // Release the mutex lock before waiting.
      sem_wait(&full_sem); // It will decrement the semaphore counter. If counter is 0, it will wait until up.
      pthread_mutex_lock(&mutex); // Hold the mutex lock.
    } while (isfull()); // While the buffer is full, you have to wait. (Lampson Monitor style)
    // How does this work? If the buffer is full, the producer needs to be careful. She wants to produce, but she
    // needs the buffer to free up space. So she waits. Once there is space, she leaps into action by locking the
    // mutex. HOWEVER: if the buffer fills up again before she proceeds! She needs to keep waiting. This makes sure
    // that you don't have a situation where you overflow the buffer because two producers simultaneously leapt on
    // a single opening in the buffer.
    addjob(job);
    std::cout << "Producer " << threadid << " added job " << job << " to the buffer.\n";
    pthread_mutex_unlock(&mutex);
    sem_post(&empty_sem); // Increments the counter on empty_sem, indicating that one place in the jobs queue has
    // now been taken up!
  }
  pthread_exit(0);
  return NULL;
}

void* consumer(void* thread_n) {
  int threadid = (*(int*) thread_n);
  int jobdone = 0;
  for (int i = 0; i < JOBS_PER_CONS; i++) {
    pthread_mutex_lock(&mutex);
    do {
      pthread_mutex_unlock(&mutex);
      sem_wait(&empty_sem);
      pthread_mutex_lock(&mutex);
    } while (isempty()); // Lampson Monitor style method to wait when buffer empty.
    // Works in the opposite way from the producer: when buffer is EMPTY, consumer has to wait. He wants to consume,
    // but he needs a producer to put a job on the buffer. Once there is a job, he jumps at it by locking the mutex.
    jobdone = dojob();
    std::cout << "Consumer " << threadid << " completed job " << jobdone << " from the buffer.\n";
    pthread_mutex_unlock(&mutex);
    sem_post(&full_sem);
  }
  pthread_exit(0);
  return NULL;
}

int main(int argc, char **argv) {
  buffer_index = 0;
  pthread_mutex_init(&mutex, NULL);
  sem_init(&full_sem, 0, BUFFSIZE); // Full semaphore needs to be initialized to buffer size.
  // Because the buffer starts empty, it can accept BUFFSIZE number of new jobs on the queue.
  sem_init(&empty_sem, 0, 0); // Empty semaphore needs to be initialized to counter 0.
  // Because the buffer starts empty, it cannot have any removal from BUFFER at the start.
  pthread_t thread[NTHREADS];
  int thread_id[NTHREADS];
  for (int i = 0; i < NTHREADS; ) {
    thread_id[i] = i;
    pthread_create(&thread[i], NULL, producer, &thread_id[i]);
    i++;
    thread_id[i] = i;
    pthread_create(&thread[i], NULL, consumer, &thread_id[i]);
    i++;
  }
  for (int i = 0; i < NTHREADS; i++)
    pthread_join(thread[i], NULL);
  pthread_mutex_destroy(&mutex);
  sem_destroy(&full_sem);
  sem_destroy(&empty_sem);
  return 0;
}
