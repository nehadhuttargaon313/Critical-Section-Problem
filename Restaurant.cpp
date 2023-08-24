#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <semaphore>
#define T 10
#define MAX 1000

using namespace std;

chrono::time_point<std::chrono::steady_clock> start_time;

class Diner;
class Restaurant{
    private:
    bool front_gate;
    bool back_gate;
    int seats;
    int diners;
    counting_semaphore<> semaphore;
    mutex back_mtx;
    mutex front_mtx;
    mutex diner_mtx;
    condition_variable front_open;
    condition_variable front_closed;
    condition_variable back_open;
    condition_variable back_closed;
    atomic_bool close_restaurant;

    void open_front_gate();
    void open_back_gate();
    void close_front_gate();
    void close_back_gate();

    public:
    Restaurant(int N);
    ~Restaurant();
    void enter(Diner* diner);
    void leave(Diner* diner);
    void serve_a_batch();
    void run();
    void close();
};

class Diner{
    private:
    int id;
    Restaurant* r;
    atomic_bool stop_visiting;

    public:
    Diner(Restaurant* res, int id);
    void dine();
    int get_id();
    void stop();
};

mutex exit_mtx;
bool close;

/**
 * Opens front gate and wakes up all the threads waiting for it to open.
 */
void Restaurant::open_front_gate(){
    cout << "opening front gate\n";
    front_gate = true;
    front_open.notify_all();
}

/**
 * Opens back gate and wakes up all the threads waiting for it to open.
 */
void Restaurant::open_back_gate(){
    cout << "opening back gate\n";
    back_gate = true;
    back_open.notify_all();
}

/**
 * Closes front gate and wakes up all the threads waiting for it to close.
 */
void Restaurant::close_front_gate(){
    cout << "closing front gate\n";
    front_gate = false;
    front_closed.notify_all();
}

/**
 * Closes back gate and wakes up all the threads waiting for it to close.
 */
void Restaurant::close_back_gate(){
    cout << "closing back gate\n";
    back_gate = false;
    back_closed.notify_all();
}

/**
 * Constructor for Restaurant Class
 * @param N Number of seats in the restaurant
 */
Restaurant::Restaurant(int N) : seats(N), semaphore(N){
    front_gate = false;
    back_gate = false;
    diners = 0;
    close_restaurant = false;
}

/**
 * Destructor for Restaurant class.
 */
Restaurant::~Restaurant(){
    back_mtx.~mutex();
    front_mtx.~mutex();
    diner_mtx.~mutex();
}
    
/**
 * function to enter the restaurant. Blocks until semaphore can be acquired and the front gate is open.
 * 
 * @param diner Pointer to diner object entering the restaurant.
 */

void Restaurant::enter(Diner* diner){
    auto curr_time = chrono::steady_clock::now();
    auto elapsed_time = chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time);
    auto remaining_time = T*1000 - elapsed_time.count();

    // wait until semaphore is greater than 0 or time up.
    semaphore.try_acquire_for(chrono::milliseconds(remaining_time));

    if(close_restaurant){
        diner->stop();
        return;
    }

    //block if front gate is closed.
    unique_lock<mutex> lock(front_mtx);
    while(!close_restaurant && !front_gate) {
        //release semaphore so that it can be acquired by other diners.
        semaphore.release();
        front_open.wait(lock);
        lock.unlock();
        if(close_restaurant){
            diner->stop();
            return;
        }
        curr_time = chrono::steady_clock::now();
        elapsed_time = chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time);
        remaining_time = T*1000 - elapsed_time.count();
        //reacquire semaphore just given up.
        if(!semaphore.try_acquire_for(chrono::milliseconds(remaining_time))){
            diner->stop();
            return;
        }
        lock.lock();        
    }
    if(close_restaurant){                
        diner->stop();
        return;
    }
    unique_lock<mutex> d_lock(diner_mtx);
    //increment the number of diners currently in the restaurant
    diners++;
    cout << "Diner " << diner->get_id() << " entered\n";
    if(diners == seats){
        //if the seats have been filled close the front gate.
        cout << "All diners have arrived\n";
        close_front_gate();
    }
    d_lock.unlock();
    lock.unlock();
}

/**
 * function to leave the restaurant. Blocks until the back gate is open.
 * 
 * @param diner Pointer to diner object leaving the restaurant. The diner must have entered initially.
 */
void Restaurant::leave(Diner* diner){
    unique_lock<mutex> lock(back_mtx);
    //block if back gate is closed
    while(!close_restaurant && !back_gate){
        back_open.wait(lock);
        if(close_restaurant){
            semaphore.release();
            lock.unlock();
            diner->stop();
            return;
        }
    }
    if(close_restaurant){
        semaphore.release();
        lock.unlock();
        diner->stop();
        return;
    }
    unique_lock<mutex> d_lock(diner_mtx);
    //decrement the diners currently in the restaurant.
    diners--;
    cout << "Diner " << diner->get_id()<< " left\n";
    if(diners == 0){
        //if all the diners have left, close the back gate.
        cout << "All diners have left\n";
        close_back_gate();
    }
        
    d_lock.unlock();
    lock.unlock();

    //release the semphore the diner helds
    semaphore.release();
}

/**
 * function to serve a batch in the restaurant. Blocks until the seats have been filled. 
 * 
 * @param diner Pointer to diner object leaving the restaurant. The diner must have entered initially.
 */
void Restaurant::serve_a_batch(){
    //open front gate so that the diners can enter
    unique_lock<mutex> front_lock(front_mtx);
    open_front_gate();
    front_lock.unlock();

    this_thread::sleep_for(chrono::microseconds(1000));

    //Wait until all the diners have arrived
    front_lock.lock();
    if(!close_restaurant && front_gate)
        front_closed.wait(front_lock);
    front_lock.unlock();
    
    if(close_restaurant){
        cout << "Restaurant closed \n"; 
        return;
    }

    //simulate serving. sleep for a ms.
    cout << "serving...\n";

    this_thread::sleep_for(chrono::microseconds(1000));

    unique_lock<mutex>  back_lock(back_mtx);

    //open back gate so that the diners can leave.
    open_back_gate();
    back_lock.unlock();

    this_thread::sleep_for(chrono::microseconds(1000));

    back_lock.lock();
    //wait until all the diners have left
    if(!close_restaurant && back_gate)
        back_closed.wait(back_lock);
    back_lock.unlock();
    if(close_restaurant){
        cout << "Restaurant closed \n"; 
        return;
    }
}

/**
 * function to repeatedly serve a batch until closed. 
 * 
 */
void Restaurant::run(){
    int b = 1;
    while(!close_restaurant){
        cout << "\nbatch " << b << endl;
        this->serve_a_batch();
        b++;
    }
    cout << "restaurant closing\n";
}

/**
 * function to close the restaurant. Notifies all the waiting diner threads that the restaurant has been closed. 
 * 
 */
void Restaurant::close(){
    close_restaurant = true;
    unique_lock<mutex> front_lock(front_mtx);
    close_front_gate();
    front_lock.unlock();

    unique_lock<mutex> back_lock(back_mtx);
    close_back_gate();
    back_lock.unlock();

    front_open.notify_all();
    front_closed.notify_all();
    back_open.notify_all();
    back_closed.notify_all();
    semaphore.release(MAX);
}

/**
 * Diner Class constructor
 * 
 * @param res Pointer to restaurant object the diner is dining.
 * @param id integer id
 */
Diner::Diner(Restaurant* res, int id): r(res), id(id){
    stop_visiting = false;
}

/**
 * repeatedly visits the restaurant till stop() method is called 
 * 
 */
void Diner::dine(){
    while(!stop_visiting){
        r->enter(this);
        if(stop_visiting)
            break;
        this_thread::sleep_for(chrono::microseconds(1000));
        r->leave(this);
    }
    cout << "Diner " << id << " exiting\n";
}


/**
 * stop the diner from visiting the restaurant
 * 
 */
void Diner::stop(){
    stop_visiting = true;
}

int Diner::get_id(){
    return id;
}

int main(int argc, char* argv[]){
    if(argc < 3){
        cerr << "Enter number of seats, diners\n";
        exit(EXIT_FAILURE);
    }

    int seats = atoi(argv[1]);
    int diners = atoi(argv[2]);

    //Create a restaurant instance
    Restaurant *r = new Restaurant(seats);
    start_time = chrono::steady_clock::now();

    //Create a thread that executes run method of the created instance
    thread res_thread(&Restaurant::run, r);

    //Create a vector to hold the diners.
    vector<Diner*> diners_arr;
    for(int i = 0; i < diners; i++){
        diners_arr.push_back(new Diner(r, i+1));
    }

    vector<thread> din_threads(diners);
    //Create threads that executes dine method of each of the diner instances.
    for(int i = 0; i < diners; i++){
        din_threads[i] = thread(&Diner::dine, diners_arr[i]);
    }

    //sleep for T seconds
    this_thread::sleep_for(chrono::microseconds(1000000*T));

    //close the restaurant
    r->close();

    //wait until all the threads have finished execution.
    for(int i = 0; i < diners; i++){
        din_threads[i].join();
    }

    res_thread.join();

}
