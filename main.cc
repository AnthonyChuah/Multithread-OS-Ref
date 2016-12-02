 /******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

// Basically this is a thread-safe Buffer class. It contains queue of integers which represent job time.
class Buffer {
public:
  queue<int> buffer;
  pthread_mutex_t mutex;
  int sem_id;
  int maxsize;
  Buffer(int _key, int _buffsize) {
    maxsize = _buffsize;
    pthread_mutex_init(&mutex, NULL);
    sem_id = sem_create(_key, 2);
    sem_init(sem_id, 0, _buffsize); // First semaphore in the set is always the semaphore for a full buffer.
    sem_init(sem_id, 1, 0); // Second semaphore in the set is for empty buffer: starts at 0.
  }
  ~Buffer() {
    sem_close(sem_id);
    pthread_mutex_destroy(&mutex);
  }
  void push(int _jobtime) {
    // Insertion is DECREMENT (wait) the first, INCREMENT (signal) the second.
    sem_wait(sem_id, 0); // OCCUPY / DECREMENT first semaphore, which is for the FULL BUFFER.
    // Mnemonic: pushing makes it MORE FULL. DECREMENTING is OCCUPYING RESOURCE. INCREMENT is RELEASE.
    // Structure:
    // 1. Decrement the First (Fullness) semaphore using wait.
    // 2. Decrement (lock) the mutex.
    // 3. Insert the jobtime 
    pthread_mutex_lock(&mutex);
    buffer.push(_jobtime); // Critical section.
    pthread_mutex_unlock(&mutex);
    sem_signal(sem_id, 1);
  }
  int extract() {
    // Extraction is INCREMENT (signal) the first, DECREMENT (wait) the second.
    // while (buffer.size() == 0) {
    //   sem_timewait(sem_id, 1);
    // }
    // Structure:
    // 1. Decrement the Second (Emptiness) semaphore using wait, but with a timer.
    // 2. Decrement (lock) the mutex.
    // 3. Pop from queue and read the jobtime. Sleep for that jobtime.
    // 4. Increment (unlock) the mutex.
    // 5. Increment the First (Fullness) semaphore using signal.
    if (sem_timewait(sem_id, 1)) {
      return -1;
    }
    // Waits on second semaphore, but will time out after 20 secs.
    // If it times out, then it will return -1 and not do anything else.
    // If no time out, it will proceed to lock the mutex and consume the job.
    pthread_mutex_lock(&mutex);
    int jobtime = buffer.front();
    buffer.pop();
    sleep(jobtime);
    pthread_mutex_unlock(&mutex);
    sem_signal(sem_id, 0); // The semaphore for FULL buffer should free up / increment counter.
    return jobtime;
  }
};

struct Arguments {
  Arguments(int _njobs, int _tid, Buffer* _ptr) : njobs(_njobs), threadid(_tid), buffer_ptr(_ptr) {}
  int njobs;
  int threadid;
  Buffer* buffer_ptr;
};

void *producer (void *id);
void *consumer (void *id);

int main (int argc, char **argv)
{
  // Command line args are size_of_queue  num_jobs_each_prod  num_prods  num_cons.
  pthread_t producerid;
  cout << "Arguments: ";
  for (int i = 0; i < argc; i++) {
    cout << *argv[i] << " ";
  }
  cout << "\n";
  int queue_size, num_jobs_per_prod, num_prods, num_cons, nthreads;
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
  nthreads = num_prods + num_cons;
  Buffer JobQueue(SEM_KEY, queue_size);

  pthread_t thread[nthreads]; // no of threads in this program, each thread is an element of the array.

  // Illustrative example: 5 threads, 2 producers, 3 consumers.
  // First launch all producers.
  for (int i = 0; i < num_prods; i++) {
    Arguments args(num_jobs_per_prod, i, &JobQueue);
    pthread_create(&thread[i], NULL, producer, &args); // Launch a producer.
  }
  // Next, launch all consumers.
  for (int j = num_prods; i < (num_prods + num_cons); j++) {
    Arguments args(num_jobs_per_prod, i, &JobQueue);
    pthread_create(&thread[j], NULL, consumer, &args);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_join(thread[i], NULL);
  }

  // The Buffer class destructor automatically takes care of cleaning up the semaphores and mutexes.

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



void *producer(void* _args)
{
  // Input argument is an Arguments struct containing threadid, njobs, and Buffer, passed by reference.
  // Producer will Buffer::insert() a job into the buffer with a random jobtime parameter.
  // Make it sleep a tiny bit between each job insertion, e.g. 1 second, so we can see Consumers snatch up jobs
  // before new jobs are inserted into queue.
  Arguments* args = (Arguments*) _args; // Need to cast the void pointer to Arguments type to use it.
  int njobs = args->njobs; // Number of jobs per producer.
  int threadid = args->threadid; // The thread number.
  // Loop however many times njobs says you have to, and make that number of jobs, each with its random jobtime.
  // Do a random jobtime calculation (spec says between 1 to 10 seconds).
  for (int i = 0; i < njobs; i++) {
    jobtime = rand() % 10;
    args.buffer_ptr->push(jobtime);
    cout << "Producer in thread " << threadid << " has pushed a new job " << i << " of duration " << jobtime
	 << " to the buffer.\n";
  }
  pthread_exit(0);
  return NULL;
}

void *consumer (void *args)
{
  // Input argument is an Arguments struct containing threadid, njobs, and Buffer, passed by reference.
  // Consumer should Buffer::extract() a job from the buffer, read the time taken, and sleep for that many seconds.
  // Consumer should do this indefinitely until it fails to read any job for 20 seconds.
  // Then it should break the loop and proceed to close the thread, and print out "No more jobs left".
  Arguments* args = (Arguments*) _args;
  int threadid = args->threadid;
  int duration = 0;
  while (duration != -1) {
    duration = args.buffer_ptr->extract();
    cout << "Consumer in thread " << threadid << " has completed a job of duration " << duration << ".\n";
  } // Repeat until there is a timeout.
  // Insert a while loop here that breaks only after 20 seconds pass. Use semtimedop() for this. See:
  // https://linux.die.net/man/2/semop
  // This means that the extraction member function of Buffer should use sem_timewait which I added.
  // Ended up making this inside the member function of Buffer.
  cout << "Consumer in thread " << threadid << " has timed out and closed thread because no more jobs.\n";
  pthread_exit (0);
  return NULL;
}
