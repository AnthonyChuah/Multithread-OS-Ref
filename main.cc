/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id);
void *consumer (void *id);

int main (int argc, char **argv)
{
  pthread_t producerid;
  int parameter = 5;
  cout << "Arguments: ";
  for (int i = 1; i < argc; i++) {
    cout << *argv[i] << " ";
  }
  cout << "\n";
  
  // pthread_create arguments:
  // thread: unique id for new thread
  // attr: you may set thread attributes. Here NULL means default
  // start_routine is the function that will be called
  // arg is the single argument that can be passed to start_routine
  // Here, we are passing the address of parameter to the producer(*id) function.

  pthread_create (&producerid, NULL, producer, (void *) &parameter);

  pthread_join (producerid, NULL);

  return 0;
}

void *producer(void *parameter) 
{

  // TODO

  int *param = (int *) parameter;

  cout << "Parameter = " << *param << endl;

  pthread_exit(0);
}

void *consumer (void *id) 
{
  // TODO 

  pthread_exit (0);

}
