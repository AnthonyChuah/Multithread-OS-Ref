/******************************************************************
 * The helper file that contains the following helper functions:
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_attach - Attach semaphore array to the program
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 ******************************************************************/

# include "helper.h"

int check_arg (char *buffer)
{
  // Checks if command line input is a number and returns it. Use by taking argv[N] and calling check_arg on it.
  // Command line args should be: size_of_queue  num_jobs_each_prod  num_prods  num_cons
  int i, num = 0, temp = 0;
  if (strlen (buffer) == 0)
    return -1;
  for (i=0; i < (int) strlen (buffer); i++)
  {
    temp = 0 + buffer[i];
    if (temp > 57 || temp < 48)
      return -1; // Not a char from '0' to '1'.
    num += pow (10, strlen (buffer)-i-1) * (buffer[i] - 48);
  }
  return num;
}

int sem_attach (key_t key)
{
  // semget(key,nsems,semflg) is a system call. Returns semaphore set identifier associated with key.
  // nsems is the number of semaphores: but only works when key has value IPC_PRIVATE or if no existing
  // semaphore set is associated with key and IPC_CREAT is specified in semflg.
  // Returns a non-neg int which is the semaphore set identifier. Returns -1 if error.
  // See: https://linux.die.net/man/2/semget
  int id;
  if ((id = semget (key, 1,  0)) < 0)
    return -1;
  return id;
}

int sem_create (key_t key, int num)
{
  // Creates a new semaphore using semget(key,nsems,semflg) system call. Returns the semaphore set identifier.
  // OK: this set of helper functions create semaphore sets with only ONE semaphore.
  int id;
  if ((id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL)) < 0)
    return -1;
  return id;
}

int sem_init (int id, int num, int value)
{
  // id is the sem_id, num is always 0.
  // semctl(id,num,setval,semun). id is the semaphore set id you wish to do semctl upon.
  // semnum is the Nth semaphore in the semaphore set corresponding to id.
  // SETVAL is the name of the Linux command, sets a value of a semaphore.
  // semun is the final argument and it is a special C union (similar to struct with a difference).
  // Important thing is int val field in that union. That's what you use to set the semaphore value.
  union semun semctl_arg;
  semctl_arg.val = value;
  if (semctl (id, num, SETVAL, semctl_arg) < 0)
    return -1;
  return 0;
}

void sem_wait (int id, short unsigned int num)
{
  // sembuf is a special thing taken by the semop() function.
  // semop is the general-purpose semaphore controller.
  // This is instructing the semaphore to wait (and to undo if something goes wrong).
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  semop (id, op, 1);
}

void sem_timewait (int id, short unsigned int num)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  struct timespec tspec;
  tspec.tv_sec = 20;
  tspec.tv_nsec = 0;
  // 20 secs, 0 nanosecs.
  semtimedop (id, op, 1, &tspec);
}

void sem_signal (int id, short unsigned int num)
{
  // See how this is exactly the same as sem_wait but with 1 instead of -1? It frees a slot to "signal" that
  // the proc is done.
  struct sembuf op[] = {
    {num, 1, SEM_UNDO}
  };
  semop (id, op, 1);
}

int sem_close (int id)
{
  // Basically destroys a semaphore set, by its id. IPC_RMID is a special command which tells semctl to remove the
  // semaphore set.
  if (semctl (id, 0, IPC_RMID, 0) < 0)
    return -1;
  return 0;
}
