 /******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

// Make a global mutex to manage writing to the stdout stream.
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

// Basically this is a thread-safe Buffer class. It contains queue of integers which represent job time.
class Buffer {
public:
  queue<int> buffer;
  pthread_mutex_t mutex;
  int sem_id;
  int maxsize;
  int sem_id_full;
  int sem_id_empty;
  Buffer(int _keyfull, int _keyempty, int _buffsize) {
    maxsize = _buffsize;
    pthread_mutex_init(&mutex, NULL);
    sem_id_full = sem_create(_keyfull, 1);
    sem_id_empty = sem_create(_keyempty, 1);
    cout << "Buffer Constructor found a full semaphore ID " << sem_id_full << ".\n";
    cout << "Buffer Constructor found an empty semaphore ID " << sem_id_empty << ".\n";
    if (sem_id_full == -1 || sem_id_empty == -1) {
      cout << "At least one semaphore could not be created. Abort now.\n";
      exit(1);
    }
    sem_init(sem_id_full, 0, _buffsize); // First semaphore in the set is always the semaphore for a full buffer.
    sem_init(sem_id_empty, 0, 0); // Second semaphore in the set is for empty buffer: starts at 0.
    cout << "Buffer Constructor initialized semaphores for Fullness and Emptiness at "
	 << _buffsize << " and 0.\n";
  }
  ~Buffer() {
    sem_close(sem_id);
    pthread_mutex_destroy(&mutex);
  }
  void push(int _jobtime) {
    // Insertion is DECREMENT (wait) the full, INCREMENT (signal) the empty.
    sem_wait(sem_id_full, 0); // OCCUPY / DECREMENT first semaphore, which is for the FULL BUFFER.
    // Mnemonic: pushing makes it MORE FULL. DECREMENTING is OCCUPYING RESOURCE. INCREMENT is RELEASE.
    // Structure:
    // 1. Decrement the First (Fullness) semaphore using wait.
    // 2. Decrement (lock) the mutex.
    // 3. Insert the jobtime into the buffer.
    pthread_mutex_lock(&mutex);
    buffer.push(_jobtime); // Critical section.
    // cout << "Buffer::push new job on buffer: writing buffer size: " << buffer.size() << ".\n";
    pthread_mutex_unlock(&mutex);
    sem_signal(sem_id_empty, 0);
    sleep(1);
  }
  int extract() {
    // Extraction is INCREMENT (signal) the full (do this later, after this->), DECREMENT (wait) the empty.
    // while (buffer.size() == 0) {
    //   sem_timewait(sem_id, 1);
    // }
    // Structure:
    // 1. Decrement the Second (Emptiness) semaphore using wait, but with a timer.
    // 2. Decrement (lock) the mutex.
    // 3. Pop from queue and read the jobtime. Sleep for that jobtime.
    // 4. Increment (unlock) the mutex.
    // 5. Increment the First (Fullness) semaphore using signal.
    // if (sem_timewait(sem_id, 1)) {
    //   return -1;
    // }
    sem_timewait(sem_id_empty, 0);
    if (errno == EAGAIN) {
      pthread_mutex_lock(&stdout_mutex);
      cout << "Semaphore timed out.\n";
      pthread_mutex_unlock(&stdout_mutex);
      return -1;
    }
    // Waits on second semaphore, but will time out after 20 secs.
    // If it times out, then it will return -1 and not do anything else.
    // If no time out, it will proceed to lock the mutex and consume the job.
    pthread_mutex_lock(&mutex);
    int jobtime = buffer.front();
    buffer.pop();
    // cout << "Buffer::extract job off buffer: decremented buffer size to: " << buffer.size() << ".\n";
    pthread_mutex_unlock(&mutex);
    sem_signal(sem_id_full, 0); // The semaphore for FULL buffer should free up / increment counter.
    sleep(jobtime);
    return jobtime;
  }
};

struct Arguments {
  Arguments() : njobs(0), threadid(0), buffer_ptr(NULL) {}
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
  int queue_size, num_jobs_per_prod, num_prods, num_cons, nthreads;
  if (check_arg(argv[1]) >= 0) {
    queue_size = check_arg(argv[1]); // This also means BUFFER SIZE.
  }
  if (check_arg(argv[2]) >= 0) {
    num_jobs_per_prod = check_arg(argv[2]);;
  }
  if (check_arg(argv[3]) >= 0) {
    num_prods = check_arg(argv[3]);
  }
  if (check_arg(argv[4]) >= 0) {
    num_cons = check_arg(argv[4]);
  }
  nthreads = num_prods + num_cons;
  cout << "Queue size: " << queue_size << "\n";
  cout << "Number of jobs per producer: " << num_jobs_per_prod << "\n";
  cout << "Number of producers: " << num_prods << "\n";
  cout << "Number of consumers: " << num_cons << "\n";
  cout << "Number of threads: " << nthreads << "\n";
  Buffer JobQueue(SEM_KEY_FULL, SEM_KEY_EMPTY, queue_size);
  pthread_t thread[nthreads]; // no of threads in this program, each thread is an element of the array.
  // First launch all producers.
  Arguments argarray[nthreads];
  for (int i = 0; i < num_prods; i++) {
    argarray[i].njobs = num_jobs_per_prod;
    argarray[i].threadid = i;
    argarray[i].buffer_ptr = &JobQueue;
    pthread_create(&thread[i], NULL, producer, &argarray[i]); // Launch a producer.
    pthread_mutex_lock(&stdout_mutex);
    cout << "Launched thread " << i << " creating a producer.\n";
    pthread_mutex_unlock(&stdout_mutex);
  }
  // Next, launch all consumers.
  for (int j = num_prods; j < (num_prods + num_cons); j++) {
    argarray[j].njobs = (num_jobs_per_prod * num_prods) / num_cons + 1;
    argarray[j].threadid = j;
    argarray[j].buffer_ptr = &JobQueue;
    pthread_create(&thread[j], NULL, consumer, &argarray[j]);
    pthread_mutex_lock(&stdout_mutex);
    cout << "Launched thread " << j << " creating a consumer.\n";
    pthread_mutex_unlock(&stdout_mutex);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_join(thread[i], NULL);
  }

  // The Buffer class destructor automatically takes care of cleaning up the semaphores and mutexes.
  return 0;
}

void* producer(void* _args)
{
  // Input argument is an Arguments struct containing threadid, njobs, and Buffer, passed by reference.
  // Producer will Buffer::insert() a job into the buffer with a random jobtime parameter.
  // Make it sleep a tiny bit between each job insertion, e.g. 1 second, so we can see Consumers snatch up jobs
  // before new jobs are inserted into queue.
  int njobs = ((Arguments*) _args)->njobs; // Number of jobs per producer.
  int threadid = ((Arguments*) _args)->threadid; // The thread number.
  // Loop however many times njobs says you have to, and make that number of jobs, each with its random jobtime.
  // Do a random jobtime calculation (spec says between 1 to 10 seconds).
  for (int i = 0; i < njobs; i++) {
    int jobtime = rand() % 10 + 1;
    ((Arguments*) _args)->buffer_ptr->push(jobtime);
    pthread_mutex_lock(&stdout_mutex);
    cout << "Producer in thread " << threadid << " has pushed a new job " << i << " of duration " << jobtime
	 << " to the buffer.\n";
    pthread_mutex_unlock(&stdout_mutex);
  }
  pthread_exit(0);
  return NULL;
}

void* consumer (void* _args)
{
  // Input argument is an Arguments struct containing threadid, njobs, and Buffer, passed by reference.
  // Consumer should Buffer::extract() a job from the buffer, read the time taken, and sleep for that many seconds.
  // Consumer should do this indefinitely until it fails to read any job for 20 seconds.
  int threadid = ((Arguments*) _args)->threadid;
  int duration = 0;
  while (true) {
    duration = ((Arguments*) _args)->buffer_ptr->extract();
    if (duration == -1) {
      break;
    }
    pthread_mutex_lock(&stdout_mutex);
    cout << "Consumer in thread " << threadid << " extracted a job of duration " << duration << ".\n";
    pthread_mutex_unlock(&stdout_mutex);
  }
  pthread_mutex_lock(&stdout_mutex);
  cout << "Consumer in thread " << threadid << " has timed out and closed thread because no more jobs.\n";
  pthread_mutex_unlock(&stdout_mutex);
  pthread_exit (0);
  return NULL;
}
