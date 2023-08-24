#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/sem.h>

// These are used for formatting output text
#define BLUE "\033[0;34m"
#define GREEN "\033[0;32m"
#define PURPLE "\033[0;35m"
#define CYAN  "\033[0;36m"
#define END "\033[0m"

// constants/parameters
// Can be adjusted based on requirement
#define MAXWAIT 50			//Maximum arrival delay (seconds)
#define CROSSINGTIME 4	    //Time taken to cross the bridge (seconds)

#define MAXPEOPLE 5   		//number of people allowed on bridge concurrently
#define SOUTHBOUND 25	    //number of person travelling south
#define NORTHBOUND 25		//number of person travelling north

// These are used to 'lock' variables during thread operations
// used to lock the  direction counters
int southCount;
int northCount;
//to track people in each direction
int southPeople;
int northPeople;
//limits number of people concurrently on bridge
int southLimiter;
int northLimiter;
//locks the bridge
int lock;

//takes semaphore ID and an operation
static void semAction(int semaphoreID, int op) 
{
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = op;
  sb.sem_flg = 0;
  if(semop(semaphoreID, &sb, 1) == -1 )
    printf("semop");
}

// semaphore 'wait' or locked
void down(int semaphoreID) 
{
  semAction(semaphoreID, -1);
}

// semaphore 'up' or 'lock off'
void up(int semaphoreID) 
{
  semAction(semaphoreID, 1);
}

// takes semaphore ID, delete the semaphore
void semDelete(int semaphoreID) 
{
  (void)semctl(semaphoreID, 0, IPC_RMID, 0);
}

//takes a semaphore key, returns the ID for that semaphore
int semtran(int key)
{	
  int semaphoreID;
  if((semaphoreID = semget((key_t)key, 1, 0666 | IPC_CREAT)) == -1)
    printf("semget");
  return(semaphoreID);
}


//deletes all the semaphores
void deleteAllSemaphores(void)
{
	semDelete(lock);
	semDelete(southLimiter);
	semDelete(northLimiter);
	semDelete(southCount);
	semDelete(northCount);
	exit(1);
}


//South person can follow South people heading in the same direction
//there can be any amount of people up to the MAXPEOPLE limit on the bridge simultaneously
void *SouthPerson(void * arg)
{
	int *selfID = (int *) arg;

	//arrival delay of the person
	sleep(rand() % MAXWAIT);
	
	//Cross bridge if enough space
	down(southLimiter);
	//disable counter
	down(southCount);

	//increment counter
	southPeople++;
	//this is first Person on bridge; lock people traveling other way
	if(southPeople == 1)
		down(lock);
	printf(""BLUE"(North)  ---> Person S%d going south on the bridge (South) [People on Bridge : %d]\n"END"", *selfID,southPeople );
	
	
	//unlock counter
	up(southCount);

	sleep( CROSSINGTIME );
	
	//block counter
	down(southCount);
	//A person has left bridge
	southPeople--;
	//this person was last on bridge so unlock everything
	if(southPeople == 0)
		up(lock);
	printf(""BLUE"(North)  Person S%d going south off the bridge--->  (South) [People on Bridge : %d]\n"END"", *selfID,southPeople );

	
	//unlock counter and weight limit
	up(southCount);
	up(southLimiter);

	pthread_exit(NULL);
}

void *NorthPerson(void * arg)
{
	int *selfID = (int *) arg;

  //random arrival delay for each person
	sleep(rand() % MAXWAIT);

	//Cross bridge if enough space
	down(northLimiter);
	//disable counter
	down(northCount);

	//increment counter
	northPeople++;
	//this is first Person on bridge; lock people traveling other way
	if(northPeople == 1)
		down(lock);
	
	printf(""CYAN"(North)  Person N%d going north on the bridge <---  (South)[People on Bridge : %d]\n"END"", *selfID ,northPeople);
	
	//unlock counter 
	up(northCount);

	sleep(CROSSINGTIME);
	
	//block counter
	down(northCount);
	//A person has left bridge
	northPeople--;
	//this person was last on bridge so unlock everything
	if(northPeople == 0)
		up(lock);
	printf(""CYAN"(North)  <--- Person N%d going north off the bridge  (South)[People on Bridge : %d]\n"END"", *selfID ,northPeople);;

	
	//unlock counter and weight limit	
	up(northCount);
	up(northLimiter);

	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////
/// Main function. 
////////////////////////////////////////////////////////////////////////////
int main(void)
{

	//initilise the semaphores, and set binary semaphores to 'up' positions
	lock = semtran(IPC_PRIVATE);	
	southCount = semtran(IPC_PRIVATE);
	southLimiter = semtran(IPC_PRIVATE);
	northCount = semtran(IPC_PRIVATE);
	northLimiter = semtran(IPC_PRIVATE);
	up(southCount);
	up(northCount);
	up(lock);

	//initialise variables, and arrays
	int i;
	
	//number of people simultaneously on bridge
	southPeople = 0;
	northPeople = 0;

	//arrays of Person IDs
	int IDSouthPerson[SOUTHBOUND];
	int IDNorthPerson[NORTHBOUND];

	//arrays of Person thread IDs
	pthread_t tIDSouthPerson[SOUTHBOUND];
	pthread_t tIDNorthPerson[NORTHBOUND];

	//assign IDs to People
	for(i = 0; i < NORTHBOUND; i++)
		IDNorthPerson[i] = i;
	for(i = 0; i < SOUTHBOUND; i++)
		IDSouthPerson[i] = i;

	//set the limits for people simultaneously on bridge
	for(i = 0; i < MAXPEOPLE; i++)
		up(southLimiter);
	for(i = 0; i < MAXPEOPLE; i++)
		up(northLimiter);

	//create threads
	for(i = 0; i < NORTHBOUND; i++)
	{
		if(pthread_create(&tIDNorthPerson[i], NULL, NorthPerson, (void *)&IDNorthPerson[i] ))
		{
			perror("error creating North person thread");
			exit(EXIT_FAILURE);
		}
	}
	for(i = 0; i < SOUTHBOUND; i++)
	{
		if(pthread_create(&tIDSouthPerson[i], NULL, SouthPerson, (void *)&IDSouthPerson[i] ))
		{
			perror("error creating South person thread");
			exit(EXIT_FAILURE);
		}
	}
	//join threads when done
	for(i = 0; i < NORTHBOUND; i++)
	{
		if(pthread_join(tIDNorthPerson[i], NULL ))
		{
			perror("error joining North person thread");
			exit(EXIT_FAILURE);		
		}
	}
	for(i = 0; i < SOUTHBOUND; i++)
	{
		if(pthread_join(tIDSouthPerson[i], NULL ))
		{
			perror("error joining South person thread");
			exit(EXIT_FAILURE);		
		}
	}	
	
	deleteAllSemaphores();
	return 0;
}
