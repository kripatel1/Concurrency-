
// kripatel1




#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

/* TODO */
/* Add your synchronization variables here */

/* Basic information about simulation.  They are printed/checked at the end
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the
 * code that you develop.
 */
 /************************************************************************/
sem_t seat_wait;

sem_t student_A; // controls who enters the office
sem_t student_B; // this is how to stop too many consec students

sem_t classA_REQUEST;
sem_t classB_REQUEST;

sem_t requestCompleted;


pthread_mutex_t A_REQUESTS;
pthread_mutex_t B_REQUESTS;


pthread_mutex_t class_A_COUNTER_UP;
pthread_mutex_t class_B_COUNTER_UP;

pthread_mutex_t class_A_COUNTER_DOWN;
pthread_mutex_t class_B_COUNTER_DOWN;



static int A_consec = 0;
static int B_consec = 0;

static int A_request = 0;
static int B_request = 0;

static int student_A_B =-1;

static int prof_loop=0;


/************************************************************************/



static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;


typedef struct
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename)
{


  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and
   * other variables you might use) here
   */
   sem_init(&seat_wait,0,3);
   sem_init(&student_A,0,0);
   sem_init(&student_B,0,0);


   sem_init(&classA_REQUEST,0,0);
   sem_init(&classB_REQUEST,0,0);

   sem_init(&requestCompleted,0,0);




  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL)
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) &&
           i < MAX_STUDENTS )
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break
 * You do not need to add anything here.
 */
static void take_break()
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk)
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */

  int requestCounter_A = 0;
  int requestCounter_B = 0;
  int completedSignal = 0;

  int tempStudents_in_Office =0;
  int tempClassB_in_Office =-1;
  int tempClassA_in_Office =-1;


// Professor waits here unless there are students
  while( (requestCounter_A == 0) && (requestCounter_B == 0))
  {
    pthread_mutex_lock(&A_REQUESTS);
    sem_getvalue(&classA_REQUEST,&requestCounter_A);
    pthread_mutex_unlock(&A_REQUESTS);

    pthread_mutex_lock(&B_REQUESTS);
    sem_getvalue(&classB_REQUEST,&requestCounter_B);
    pthread_mutex_unlock(&B_REQUESTS);
  }


  while(1)
  {

    // student A will enter
    if( (requestCounter_A > 0) && ( A_consec < 5)
     && (students_since_break < 10) && (classb_inoffice == 0))
    {
      A_consec++;
      B_consec = 0;
      sem_post(&student_A);
      sem_wait(&requestCompleted);
    }
    // student B will enter
    else if( (requestCounter_B > 0) && ( B_consec < 5)
          && (students_since_break < 10) && (classa_inoffice == 0))
    {
      A_consec = 0;
      B_consec++;
      sem_post(&student_B);
      sem_wait(&requestCompleted);

    }
    // FORCE STUDENT A to enter
    else if( (B_consec >=5) && (requestCounter_A > 0)
          && (students_since_break < 10))
    {
      // wait for office to empty
      pthread_mutex_lock(&class_B_COUNTER_UP);
      while(classb_inoffice)
      {}
      pthread_mutex_unlock(&class_B_COUNTER_UP);
      tempClassB_in_Office = -1; // reset value for the next time;
      A_consec++;
      B_consec = 0;
      sem_post(&student_A);
      sem_wait(&requestCompleted);

    }
    //FORCE STUDENT B to enter
    else if ( (A_consec >= 5) && (requestCounter_B > 0)
           && (students_since_break < 10))
    {
      // wait for office to empty
      pthread_mutex_lock(&class_A_COUNTER_UP);
      while(classa_inoffice)
      {}
      pthread_mutex_unlock(&class_A_COUNTER_UP);

      tempClassA_in_Office = -1; // reset value for the next time;
      A_consec = 0;
      B_consec++;
      sem_post(&student_B);
      sem_wait(&requestCompleted);

    }
    //ONLY HAVE A LINE OF STUDENTS A
    else if ( (A_consec >=5) && (requestCounter_B == 0)
           && (students_since_break < 10))
    {
      A_consec++;
      B_consec = 0;
      sem_post(&student_A);
      sem_wait(&requestCompleted);

    }
    //ONLY HAVE A LINE OF STUDENTS B
    else if ( (B_consec >= 5) && (requestCounter_A == 0)
           && (students_since_break < 10))
    {
      sem_getvalue(&requestCompleted,&completedSignal);


      A_consec = 0;
      B_consec++;
      sem_post(&student_B);
      sem_wait(&requestCompleted);

    }
    // REGARDS B STUDENT------
    // this is needed because no more students are of certain type are there
    // this prof function will loop too many times causing errors
    // this catches the problem drastically cuts the iteration
    else if((requestCounter_B == 0) && (classb_inoffice > 0)
          && (students_since_break < 10))
    {
      while(students_in_office)
      {
        // constantly reading values for any changes
        pthread_mutex_lock(&A_REQUESTS);
        sem_getvalue(&classA_REQUEST,&requestCounter_A);
        pthread_mutex_unlock(&A_REQUESTS);

        pthread_mutex_lock(&B_REQUESTS);
        sem_getvalue(&classB_REQUEST,&requestCounter_B);
        pthread_mutex_unlock(&B_REQUESTS);
        if(requestCounter_B > 0)
        {
          break; // exit this loop and let outer loop what should be done
        }
      }
    }

    // REGARDS A STUDENT------
    // this is needed because no more students are of certain type are there
    // this prof function will loop too many times causing errors
    // this if catches the problem drastically cuts the iteration
    else if((requestCounter_A == 0) && (classa_inoffice > 0)
          && (students_since_break < 10))
    {
      while(students_in_office)
      {
        // constantly reading values for any changes
        pthread_mutex_lock(&A_REQUESTS);
        sem_getvalue(&classA_REQUEST,&requestCounter_A);
        pthread_mutex_unlock(&A_REQUESTS);

        pthread_mutex_lock(&B_REQUESTS);
        sem_getvalue(&classB_REQUEST,&requestCounter_B);
        pthread_mutex_unlock(&B_REQUESTS);
        if(requestCounter_A > 0)
        {
          break; // exit this loop and let outer loop what should be done
        }
      }
    }
    // FORCE BREAK
    else if( students_since_break >=10)
    {
      pthread_mutex_lock(&class_A_COUNTER_UP);
      pthread_mutex_lock(&class_B_COUNTER_UP);
      while(students_in_office);
      take_break();
      pthread_mutex_unlock(&class_B_COUNTER_UP);
      pthread_mutex_unlock(&class_A_COUNTER_UP);
    }


    pthread_mutex_lock(&class_A_COUNTER_UP);
    pthread_mutex_lock(&class_A_COUNTER_DOWN);
    pthread_mutex_lock(&class_B_COUNTER_UP);
    pthread_mutex_lock(&class_B_COUNTER_DOWN);
    tempStudents_in_Office = students_in_office;
    pthread_mutex_unlock(&class_B_COUNTER_DOWN);
    pthread_mutex_unlock(&class_B_COUNTER_UP);
    pthread_mutex_unlock(&class_A_COUNTER_DOWN);
    pthread_mutex_unlock(&class_A_COUNTER_UP);
    while(tempStudents_in_Office == 3) // pause execution if office is full
    {
      pthread_mutex_lock(&class_A_COUNTER_UP);
      pthread_mutex_lock(&class_A_COUNTER_DOWN);
      pthread_mutex_lock(&class_B_COUNTER_UP);
      pthread_mutex_lock(&class_B_COUNTER_DOWN);
      tempStudents_in_Office = students_in_office;
      pthread_mutex_unlock(&class_B_COUNTER_DOWN);
      pthread_mutex_unlock(&class_B_COUNTER_UP);
      pthread_mutex_unlock(&class_A_COUNTER_DOWN);
      pthread_mutex_unlock(&class_A_COUNTER_UP);

    }
    // dumps whats in memory if this function runs while
    // if(prof_loop>500000)
    // {
    //   printf("%d value in students_in_office\n"
    //           "%d value in classa_inoffice\n"
    //           "%d value in classb_inoffice\n"
    //           "%d value in requestCounter_A\n"
    //           "%d value in requestCounter_B\n"
    //           "%d value in A_consec\n"
    //           "%d value in B_consec\n",
    //           students_in_office,
    //           classa_inoffice,
    //           classb_inoffice,
    //           requestCounter_A,
    //           requestCounter_B,
    //           A_consec,
    //           B_consec);
    //       exit(1);
    // }
    // time to re-read values
    pthread_mutex_lock(&A_REQUESTS);
    sem_getvalue(&classA_REQUEST,&requestCounter_A);
    pthread_mutex_unlock(&A_REQUESTS);

    pthread_mutex_lock(&B_REQUESTS);
    sem_getvalue(&classB_REQUEST,&requestCounter_B);
    pthread_mutex_unlock(&B_REQUESTS);
    while( (requestCounter_A == 0) && (requestCounter_B == 0))
    {
      pthread_mutex_lock(&A_REQUESTS);
      sem_getvalue(&classA_REQUEST,&requestCounter_A);
      pthread_mutex_unlock(&A_REQUESTS);

      pthread_mutex_lock(&B_REQUESTS);
      sem_getvalue(&classB_REQUEST,&requestCounter_B);
      pthread_mutex_unlock(&B_REQUESTS);
    }
    prof_loop++; // allowed me to debug the number of times this loop ran
  }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter()
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */

  // This counts the number B_Students
  pthread_mutex_lock(&A_REQUESTS);
  A_request++;
  sem_post(&classA_REQUEST);
  pthread_mutex_unlock(&A_REQUESTS);



  sem_wait(&student_A);
  sem_wait(&seat_wait);

  pthread_mutex_lock(&class_A_COUNTER_UP);
  classa_inoffice += 1;
  students_in_office += 1;
  students_since_break += 1;
  pthread_mutex_unlock(&class_A_COUNTER_UP);
  pthread_mutex_lock(&A_REQUESTS);
  A_request--; // removes request
  sem_wait(&classA_REQUEST); // remove request
  pthread_mutex_unlock(&A_REQUESTS);

  // waits for the student to properly enter before professor
  // processes another request.
  sem_post(&requestCompleted);



}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter()
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */


// This counts the number B_Students
  pthread_mutex_lock(&B_REQUESTS);
  B_request++;
  sem_post(&classB_REQUEST);
  pthread_mutex_unlock(&B_REQUESTS);



// Students only entered if allowed by professor
  sem_wait(&student_B);
  sem_wait(&seat_wait);
  pthread_mutex_lock(&class_B_COUNTER_UP);
  classb_inoffice += 1;
  students_in_office += 1;
  students_since_break += 1;
  pthread_mutex_unlock(&class_B_COUNTER_UP);
  pthread_mutex_lock(&B_REQUESTS);
  B_request--;// removes request
  sem_wait(&classB_REQUEST);
  pthread_mutex_unlock(&B_REQUESTS);

  // waits for the student to properly enter before professor
  // processes another request.
  sem_post(&requestCompleted);

}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.
 */
static void ask_questions(int t)
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave()
{
  /*
   *  TODO
   *  YOUR CODE HERE.
   */

  pthread_mutex_lock(&class_A_COUNTER_DOWN);
  students_in_office -= 1;
  classa_inoffice -= 1;
  pthread_mutex_unlock(&class_A_COUNTER_DOWN);

  sem_post(&seat_wait);


}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave()
{
  /*
   * TODO
   * YOUR CODE HERE.
   */

  pthread_mutex_lock(&class_B_COUNTER_DOWN);
  students_in_office -= 1;
  classb_inoffice -= 1;
  pthread_mutex_unlock(&class_B_COUNTER_DOWN);

  sem_post(&seat_wait);

}

/* Main code for class A student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si)
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );

  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si)
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );


  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * 
 */
int main(int nargs, char **args)
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2)
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0)
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result)
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++)
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);

    student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result)
    {
      printf("officehour: thread_fork failed for student %d: %s\n",
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++)
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done. \n");

    return 0;
}
