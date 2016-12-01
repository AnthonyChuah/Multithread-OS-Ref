/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

void insertbuffer(int val, int bufsize);
int extractbuffer();
void *producer (void *id);
void *consumer (void *id);

void insertbuffer(int val, int bufsize) {
  if (buffer_index < bufsize) {
    buffer[buffer_index] = val;
  } else {
    cout << "Buffer overflow\n";
  }
}

int extractbuffer() {
  if (buffer_index > 0) {
    return buffer[--buffer_index];
  } else {
    cout << "Buffer underflow\n";
  }
}

int main (int argc, char **argv)
{
  // Command line args are size_of_queue  num_jobs_each_prod  num_prods  num_cons.
  pthread_t producerid;
  int parameter = 5;
  cout << "Arguments: ";
  for (int i = 0; i < argc; i++) {
    cout << *argv[i] << " ";
  }
  cout << "\n";
  int queue_size, num_jobs_per_prod, num_prods, num_cons;
  if (check_arg(argv[1]) >= 0) {
    queue_size = *argv[1]; // This also means BUFFER SIZE.
  }
  if (check_arg(argv[2]) >= 0) {
    num_jobs_per_prod = *argv[2];
  }
  if (check_arg(argv[3]) >= 0) {
    num_prods = *argv[3];
  }
  if (check_arg(argv[4]) >= 0) {
    num_cons = *argv[4];
  }

  pthread_mutex_t bufferlock;
  int buffer[queue_size];
  int buffer_index;

  // Initialize semaphores.
  sem_t full_sem;
  sem_t empty_sem;
  sem_init(&full_sem, 0, queue_size); // The buffer will be empty to start with, so the counter should
  // be initialized to the size of buffer.
  sem_init(&empty_sem, 0, 0); // This buffer is empty to start with, so counter should start at 0 to reflect that
  // the buffer is empty now. Consumers cannot consume anything now.

  pthread_t thread[parameter]; // paramter is no of threads in this program, each thread is an element of the array.
  int thread_index[parameter];

  for (int i = 0; i < parameter; ) {
    thread_index[i] = i;
    pthread_create(&thread[i], NULL, producer, &thread_index[i]);
    i++;
    thread_index[i] = i;
    pthread_create(&thread[i], NULL, consumer, &thread_index[i]);
    i++;
  }

  for (int i = 0; i < parameter; i++) {
    pthread_join(thread[i], NULL);
  }

  pthread_mutex_destroy(&bufferlock);
  sem_destroy(&full_sem);
  sem_destroy(&empty_sem);

  // pthread_create arguments:
  // thread: unique id for new thread
  // attr: you may set thread attributes. Here NULL means default
  // start_routine is the function that will be called
  // arg is the single argument that can be passed to start_routine
  // Here, we are passing the address of parameter to the producer(*id) function.
  // Parameter represents the NUMBER OF THREADS.

  // pthread_create (&producerid, NULL, producer, (void *) &parameter);

  // pthread_join (producerid, NULL);

  return 0;
}



void *producer(void *parameter) 
{

  // TODO

  int *param = (int *) parameter; // Cast to int.

  cout << "Parameter = " << *param << endl;

  pthread_exit(0);
}

void *consumer (void *id) 
{
  // TODO 

  pthread_exit (0);

}
