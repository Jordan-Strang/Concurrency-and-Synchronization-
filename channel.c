#include "channel.h"
/* Name 1: Jordan Strang
*  Psu ID: XJS5074
*  
*  Name 2: Javier Strang
*  Psu ID: JJS7850
*/

// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */
   
    //Malloc used to allocate memory for the channel
    channel_t *chann = malloc(sizeof(channel_t));
   
    //Creates a buffer size
    chann -> buffer = buffer_create(size);
   
    //If the size is not zero, the channel starts as empty
    if(size != 0){
        chann -> stat = CHANNEL_EMPTY;
    //If the size is zero, the channel is full
    }else if(size == 0){
        chann -> stat = CHANNEL_FULL;
    //Failcase, if an issue happens free the memory and return null
    }else{
        free(chann);
        return NULL;
    }  
   
    //Initializes the mutex
    int mutexInitial = pthread_mutex_init(&chann -> mutex, NULL);
    //If the initialization fails return null
    if (mutexInitial != 0) {
        return NULL;
    }

    //Initializes the condCon
    int condConInitial = pthread_cond_init(&chann -> condCon, NULL);
    //if initialization fails destroy the mutex and free memory
    if (condConInitial != 0) {
        pthread_mutex_destroy(&chann -> mutex);
        free(chann);
        return NULL;
    }
   
    //Initializes the condGen
    int condGenInitial = pthread_cond_init(&chann -> condGen, NULL);
    //If initialization fails destory mutex and condcon and free memory
    if (condGenInitial != 0) {
        pthread_mutex_destroy(&chann -> mutex);
        pthread_cond_destroy(&chann -> condCon);
        free(chann);
        return NULL;
    }
   
    //Initilize the waitGen
    int waitGenInitial = pthread_cond_init(&chann -> waitGen, NULL);
    //If initialization fails destroy mutex, condgen, condcon, and free memory
    if (waitGenInitial != 0) {
        pthread_mutex_destroy(&chann -> mutex);
        pthread_cond_destroy(&chann -> condGen);
        pthread_cond_destroy(&chann -> condCon);
        free(chann);
        return NULL;
    }    
   
    //Initialize the buffer size
    chann -> buffSize = size;
    //Initialize the wait count
    chann -> waitCon = 0;
   
    //Initialize the lists for receieve and send
    chann -> recSel = list_create();
    chann -> sendSel = list_create();
   
    //Return the initialized channel
    return chann;
}

//Helper Function
//Signal to all threads for condition variables
void signal_threads(list_t* list)
{
    //Go through all of the nodes in the list
    for (list_node_t *n = list_head(list);  n != NULL; n = list_next(n)) {
        //Get the object from node
        select_t* sel = (select_t*)list_data(n);
        //Lock mutex
        pthread_mutex_lock(sel -> mutex);
        //Signal the condition variable to a waiting thread
        pthread_cond_signal(sel -> cond);
        //Unlock mutex
        pthread_mutex_unlock(sel -> mutex);
    }
}

//Helper function
//Updating thread status' during data transfer based on its direction
void channelDirection(channel_t *channel, void* data, enum direction dir)
{  
    //If the buffer size is not zero 
    if (channel -> buffSize != 0) {
        
        //If the direction is to send
        if (dir == SEND) {
            //Add the data 
            buffer_add(channel -> buffer, data);
            
            //If the buffer has no more space change the status to be full
            if (buffer_current_size(channel -> buffer) == channel -> buffSize) {
                channel -> stat = CHANNEL_FULL;
            } 
            //If the buffer is empty change its status to open
            else if (channel->stat == CHANNEL_EMPTY) {
                channel->stat = CHANNEL_OPEN;
            }

            //Signal that the data was added
            signal_threads(channel -> recSel);
            //Signal that the data is accessible
            pthread_cond_signal(&channel -> condCon);

        } 
        //If the direction is to receive
        else if (dir == RECV) {
            //Removed the data
            buffer_remove(channel->buffer, data);

            //If buffer is now empty change its status to be empty
            if (buffer_current_size(channel->buffer) == 0) {
                channel->stat = CHANNEL_EMPTY;
            } 
            //If the buffer was previously full but now has space change its status to open
            else if (channel->stat == CHANNEL_FULL) {
                channel->stat = CHANNEL_OPEN;
            }

            //Signal that the data was removed
            signal_threads(channel->sendSel); 

            //If the buffer size is zero signal to wait
            if (channel->buffSize == 0) {
                pthread_cond_signal(&channel->waitGen);
            }
            //If the buffer size is not zero signal not to wait
            else if (channel -> buffSize != 0) {
                pthread_cond_signal(&channel->condGen); 
            }
        }
    }
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    //Lock mutex
    pthread_mutex_lock(&channel -> mutex);

    //If the channel is full wait for space to become available in the buffer
    while (channel -> stat == CHANNEL_FULL || (channel -> buffSize == 0 && buffer_current_size(channel -> buffer) == 1)) {
        pthread_cond_wait(&channel -> condGen, &channel -> mutex);
    }

    //If the channel is closed return a closed error
    if (channel -> stat == CHANNEL_CLOSED) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //Use helper function so send data
    channelDirection(channel, data, SEND);

    //Unlock mutex
    pthread_mutex_unlock(&channel -> mutex);
    //Return that the send was a success
    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{    
    //Lock mutex
    pthread_mutex_lock(&channel -> mutex);

    //If the channel is not empty or full or has data available
    while (channel-> stat == CHANNEL_EMPTY || (channel -> buffSize == 0 && channel -> stat == CHANNEL_FULL)) {
        //If the buffer has data in it wait for the signaled condition
        if (channel -> buffSize != 0) {
            pthread_cond_wait(&channel -> condCon, &channel -> mutex);
        }
        //Otherwise increment waitcon and signal for other threads to send data
        else {
            channel -> waitCon = channel -> waitCon + 1;
            signal_threads(channel -> sendSel);
        }
    }

    //If the channel is closed return a closed error
    if (channel -> stat == CHANNEL_CLOSED) {
        //Unlock mutex
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
    
    //Use helper function to recieve data
    channelDirection(channel, data, RECV);

    //Unlock mutex
    pthread_mutex_unlock(&channel->mutex);
    //Return that the recieve was successful
    return SUCCESS;
}

//Helper function
//Function that helps the non blocking send
enum channel_status nonBlockSend(channel_t* channel, void* data)
{
    //If the channel status is closed return a closed error
    if (channel -> stat == CHANNEL_CLOSED) {
        return CLOSED_ERROR;
    }

    //if the bugger size is zero and buffer is full or receive list is empty or if the
    //buffer is full and the channel status is full.
    if ((channel -> buffSize == 0 && (buffer_current_size(channel -> buffer) == 1 ||
          (channel -> waitCon == 0 && list_count(channel -> recSel) == 0))) ||
        (channel -> buffSize > 0 && channel -> stat == CHANNEL_FULL)) {
        //If either of the above conditions are met, return channel is full
        return CHANNEL_FULL;
    }

    //Make the channel direction SEND and call the channeldirection function
    channelDirection(channel, data, SEND);
   
    //Return a success if the data was successfully sent in the direction of SEND
    return SUCCESS;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
   
    //If the channel is NULL or data is NULl, return a generic error
    if (channel == NULL || data == NULL) {
        return GENERIC_ERROR;
    }
   
    //If the lock fails, return a generic error
    if (pthread_mutex_lock(&channel -> mutex) != 0) {
        return GENERIC_ERROR;
    }
   
    //If the channel status is closed
    if (channel -> stat == CHANNEL_CLOSED) {
        //Unlock mutex and return a closed error
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
   
    //If the channel status is full
    if (channel -> stat == CHANNEL_FULL) {
        //Unlock mutex and return channel is full
        pthread_mutex_unlock(&channel -> mutex);
        return CHANNEL_FULL;
    }
   
    //Send the data to the helper function
    enum channel_status sendStat = nonBlockSend(channel, data);
   
    //Unlock mutex after the call to helper function is complete
    pthread_mutex_unlock(&channel -> mutex);
   
    //Return the result of the send
    return sendStat;
}

//Helper Function
//Function that helps the non block receive
enum channel_status nonBlockRec(channel_t* channel, void* data)
{
    //If the channel status is closed return a closed error
    if (channel->stat == CHANNEL_CLOSED) {
        return CLOSED_ERROR;
    }
    
    //If the buffer size is zero and the is no data in the buffer
    if (channel -> buffSize == 0 && buffer_current_size(channel -> buffer) == 0) {
        //If the channel status is full and send is waiting
        if (channel->stat == CHANNEL_FULL && list_count(channel -> sendSel) != 0) {
            //Increase the wait condition count by 1
            channel -> waitCon = channel -> waitCon + 1;
            //Signal threads for waiting send
            signal_threads(channel -> sendSel);
            //Wait for the condition signal, call from signal threads
            pthread_cond_wait(&channel -> condCon, &channel -> mutex);
            //Decrease the wait condition count by 1, afte the signal
            channel -> waitCon = channel -> waitCon - 1;
        }
        //If no send is waitng and there is no data to be received
        else {
            //Return a channel empty status
            return CHANNEL_EMPTY;
        }
    }
    
    //If the channel status is empty, return channel is empty
    if (channel -> stat == CHANNEL_EMPTY) {
        return CHANNEL_EMPTY;
    }
    
    //Make the channel direction RECV and call the channeldirection function
    channelDirection(channel, data, RECV);
    
    //Return the result of the send
    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    
    //If the channel is NULL or data is NULl, return a generic error
    if (channel == NULL || data == NULL) {
        return GENERIC_ERROR;
    }

    //If the lock fails, return a generic error
    if (pthread_mutex_lock(&channel -> mutex) != 0) {
        return GENERIC_ERROR;
    }

    //If the channel status is closed
    if (channel -> stat == CHANNEL_CLOSED) {
        //Unlock mutex and return a closed error
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
    
    //If the channel status is empty
    if (channel -> stat == CHANNEL_EMPTY) {
        //Unlock mutex and return channel is empty
        pthread_mutex_unlock(&channel -> mutex);
        return CHANNEL_EMPTY;
    }
     
    //Receive the data using the helper function
    enum channel_status recStat = nonBlockRec(channel, data);
    
    //Unlock mutex after the call to helper function is complete
    pthread_mutex_unlock(&channel -> mutex);

    //Return the result of the receive
    return recStat;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    //If the channel is already closed return the Closed error
    if (channel -> stat == CHANNEL_CLOSED) {
        return CLOSED_ERROR;
    }
    //If the channel is not already closed
    else if (channel -> stat != CHANNEL_CLOSED) {
     
        //Lock mutex
        pthread_mutex_lock(&channel -> mutex);
       
        //Change the status to closed
        channel -> stat = CHANNEL_CLOSED;
   
        //Broadcasat to all the threads that channel is closed
        pthread_cond_broadcast(&channel -> condGen);
        pthread_cond_broadcast(&channel -> condCon);
   
        //Signal all threads that the send and recieve operations will cease to function using helper function
        signal_threads(channel -> sendSel);
        signal_threads(channel -> recSel);
   
        //Unlock mutex
        pthread_mutex_unlock(&channel -> mutex);
        //Return that the close was successful
        return SUCCESS;
    }
    //All else fails return a Generic error
    else {
        return GENERIC_ERROR;
    }
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{

    //If the channel has no status, NULL, or if the status is not closed return the Destroy error
    if (channel == NULL || channel -> stat != CHANNEL_CLOSED) {
        return DESTROY_ERROR;
    }
    //If the channel has a status and is closed
    else if (channel != NULL && channel -> stat == CHANNEL_CLOSED) {
   
        //Free the buffer on the channel
        buffer_free(channel -> buffer);    
   
        //Destroy the receive list
        list_destroy(channel -> recSel);
   
        //Destroy the send list
        list_destroy(channel -> sendSel);
       
        //Destroy the condCon
        int condConDestroy = pthread_cond_destroy(&channel -> condCon);
        //If the destroy fails return a destroy error
        if (condConDestroy != 0) {
            return DESTROY_ERROR;
        }
   
        //Destroy the condGen
        int condGenDestroy = pthread_cond_destroy(&channel -> condGen);
        //If destroy fails return a destroy error
        if (condGenDestroy != 0) {
            return DESTROY_ERROR;
        }
       
        //Destroy the waitGen
        int waitGenDestroy = pthread_cond_destroy(&channel -> waitGen);
        //If the destroy fails return a destroy error
        if (waitGenDestroy != 0) {
            return DESTROY_ERROR;
        }
       
        //Destroy the mutex
        int mutexDestroy = pthread_mutex_destroy(&channel -> mutex);
        //If destroy fails return a destroy error
        if (mutexDestroy != 0) {
            return DESTROY_ERROR;
        }        
       
        //Free the memory that was allocated to the channel
        free(channel);
   
        //Return success if no destroy error is triggered, meaning that the destroy was succesful
        return SUCCESS;
    }
    //All else fails return generic error
    else{
        return GENERIC_ERROR;
    }
} 

//Helper Function
//Function loops through the list of channels ensuring that channels that refer to each
//other are all unlocked if one is unlocked
void unlock_channel(select_t* channel_list, size_t unlockCount)
{
    //Tracker to track which channesl are unlocked, zero if locked and 1 if unlocked
    int unlockChann[unlockCount];  
   
    //Sets the unlockChann array to locked, zero
    memset(unlockChann, 0, sizeof(unlockChann));  
   
    //sets the index to 0 for current channel
    size_t currChann = 0;
   
    //Loop the channel list until locked is found
    while (currChann < unlockCount) {
       
        //If the current channel is unlocked
        if (unlockChann[currChann]) {
            //Move to the next channel
            currChann = currChann + 1;  
            continue;
        }

        //Unlock the mutex on the current channel
        pthread_mutex_unlock(&channel_list[currChann].channel -> mutex);

        //Makes the current channel unlocked
        unlockChann[currChann] = 1;

        //move to the next channel
        size_t dupChann = currChann + 1;
       
        //Loop through the left over channels
        while (unlockCount > dupChann) {
           
            //If there are channels that refer to each other, unlock them
            if (channel_list[dupChann].channel == channel_list[currChann].channel) {
                unlockChann[dupChann] = 1;
            }
            //move to the next channel
            dupChann =  dupChann + 1;
        }
       
        //Move to the next channel in other loop
        currChann = currChann + 1;  
    }
}

//Helper Function
//If the mutex cant be locked the function will unlock previous locked channel and
//keep trying to lock all channels
void try_lock_channels(select_t* channel_list, size_t countChann)
{
    //A loop to lock all the channels if a mutex cannot be locked
    while (1) {
        //Tracks if the all of the locks are succesful
        size_t lockTrue = 1;
       
        //Allocate an arrat to track channels that refer to each other
        int* duppChann = (int*)calloc(countChann, sizeof(int));
       
        //Loop through the channels and lock
        for (size_t currChann = 0; countChann > currChann; currChann = currChann + 1) {
           
            //If the channel is already marked as a channel that refers to another, move on
            if (duppChann[currChann]) continue;

            //loop to find channels that refer to each other and marke them
            for (size_t prevChann = 0; currChann > prevChann; prevChann = prevChann + 1) {
                //if the current channel and previous channel are the same    
                if (channel_list[currChann].channel == channel_list[prevChann].channel) {
                    //mark it and move on
                    duppChann[currChann] = 1;
                    break;
                }
            }

            //if the channel does not refer to another
            if (!duppChann[currChann]) {
                //try to lock the current channel, if already locked return non zero
                if (pthread_mutex_trylock(&channel_list[currChann].channel -> mutex) != 0) {
                    //Mark that not all the locks were successful
                    lockTrue = 0;
                   
                    //unlock all the channels that were successful and move on
                    unlock_channel(channel_list, currChann);
                    break;
                }
            }
        }
       
        //Free the memory allocated for the array
        free(duppChann);
       
        //if the lock fails retry locking
        if (!lockTrue) continue;
       
        //If the all the locks were succesful, exit the loop
        return;  
    }
}

//Helper function
//Clear and unlock channels
void cleanup_channel(pthread_mutex_t* mutex, pthread_cond_t* cond, select_t* channel_list, size_t channel_count) {
    //Unlock channels
    unlock_channel(channel_list, channel_count);  
    //Destroy mutex and condition
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(cond);
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index) {
    //Initiate condition and mutex     
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;     
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      

    //Create index variable
    size_t indexChan = 0; 
    //Infinitely loop
    while (1) {         
               
        //
        try_lock_channels(channel_list, channel_count);          

        //Reset index variable        
        indexChan = 0; 
        //Loop through all channels
        while (indexChan < channel_count) {  
            //Get the list from either the send or recieve           
            list_t* listChan = (channel_list[indexChan].dir == SEND) ? channel_list[indexChan].channel->sendSel : channel_list[indexChan].channel->recSel;
            //Find the node in the list             
            list_node_t* node = list_find(listChan, channel_list);
            
            //If the node is found remove it from the list             
            if (node != NULL) {                  
                list_remove(listChan, node);             
            }    
            //Index to the next channel         
            indexChan = indexChan + 1; 
        }          

        //Reset index variable
        indexChan = 0;
        //Loop through all channels
        while (indexChan < channel_count) {             
            enum channel_status status; 
            
            //Call nonBlock functions for the correct direction            
            if (channel_list[indexChan].dir == SEND) { 
                //Nonblock call for send                
                status = nonBlockSend(channel_list[indexChan].channel, channel_list[indexChan].data);             
            } 
            else {    
                //Nonblock call for recieve             
                status = nonBlockRec(channel_list[indexChan].channel, &(channel_list[indexChan].data));             
            }              

            //If the given operation is successful        
            if ((channel_list[indexChan].dir == SEND && status != CHANNEL_FULL) || (channel_list[indexChan].dir == RECV && status != CHANNEL_EMPTY)) {                 
                *selected_index = indexChan;
                //Call helper function to clear channel                 
                cleanup_channel(&mutex, &cond, channel_list, channel_count);
                //Return its status                 
                return status;             
            }          
            //Index to the next channel   
            indexChan = indexChan + 1; 
        }          

        //Lock mutex        
        pthread_mutex_lock(&mutex); 
        
        //Get mutex and condition for current channel        
        channel_list->mutex = &mutex;         
        channel_list->cond = &cond;          

        //Reset index variable
        indexChan = 0;
        //Loop through all channels
        while (indexChan < channel_count) {   
            //Get the list from either send or recieve          
            list_t* target_list = (channel_list[indexChan].dir == RECV) ? channel_list[indexChan].channel->recSel : channel_list[indexChan].channel->sendSel;
            //Insert the channel into the list             
            list_insert(target_list, channel_list);  
            //Index to the next channel           
            indexChan = indexChan + 1; 
        }          

        //Unlock channel list
        unlock_channel(channel_list, channel_count); 
        //Wait for condition signal        
        pthread_cond_wait(&cond, &mutex);
        //Unlock mutex         
        pthread_mutex_unlock(&mutex);     
    } 
}


