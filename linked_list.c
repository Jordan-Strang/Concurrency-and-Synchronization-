#include <stdlib.h>
#include "linked_list.h"

/* Name 1: Jordan Strang
*  Psu ID: XJS5074
*  
*  Name 2: Javier Strang
*  Psu ID: JJS7850
*/

// Creates and returns a new list
list_t* list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
  //Uses malloc to allocate the memory needed for the list structure
  list_t* newlst = malloc(sizeof(*newlst)); 
    
  //Intializes a new list
  *newlst = (list_t) {
    //Initializes the head and tail to NULL
    .head = NULL, .tail = NULL,
    //Initializes the count of elements to zero
    .count = 0};

  //Returns the newly created intialized list
  return newlst; 
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
  //If the list head node is not an empty list
  if (list -> head != NULL) 
  {
    //Saves the pointer to the next node 
    list_node_t* nextNode = list -> head -> next;
    //Frees the memory for the current head node
    free(list -> head);
    //the pointer changes from the head of the list to the next node
    list -> head = nextNode;
    //Loop the list destroy to keep freeing nodes updating after each loop
    list_destroy(list); 
  } 
  //If the list head node is an empty list
  else if (list -> head == NULL)
  {
    //Frees the memory for the list
    free(list);
  }
}

// Returns head of the list
list_node_t* list_head(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    //If the list is not null
    if (list != NULL) 
    {
      //Return the head of the list
      return list -> head;
    }
    
    //Returns null if the list is null
    return NULL;
}

// Returns tail of the list
list_node_t* list_tail(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    //If the list is not null
    if (list != NULL) 
    {
      //Return the tail of the list
      return list -> tail;
    }
    
    //Returns null if the list is null
    return NULL;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    //If the node is not null
    if (node != NULL)
    {
      //Return the next node in the list
      return node -> next;
    }
    
    //Returns null if the node is null
    return NULL;
}

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return NULL;
}

// Returns end of the list marker
list_node_t* list_end(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return NULL;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    //If the node is not null
    if (node != NULL)
    {
      //Return the data from the node
      return node -> data;
    }
    
    //Return null if the node is null
    return NULL;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
    //If the list is not null
    if (list != NULL)
    {
      //Return the total number of elements in the list
      return list -> count;
    }
    
    //Return 0 if the list is null
    return 0;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
  //If the list head is Null
  if (list -> head == NULL) 
  {
    //Return Null
    return NULL;
  }
  //If the list head data is the same as the data
  else if (list -> head -> data == data) 
  {
    //Return the list head, first node
    return list -> head;
  }
  
  //If the data does not match, move to the next node in the list
  list_t nextlst = {list -> head -> next};
  
  //Recursive search for the data in the list
  return list_find(&nextlst, data);
}

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data)
{
      /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
      
  //Uses malloc to allocate the memory needed for the new node
  list_node_t* newNode = malloc(sizeof(list_node_t));
  
  //Initializes a new node
  *newNode = (list_node_t) {
      .data = data, 
      .next = NULL, 
      .prev = NULL};
  
  // Adds one to the count to keep the count right as one node is added
  list->count += 1;
  
  //If the list head is null
  if (list->head == NULL) {
      //Sets the new node as both the head and tail
      list->head = list->tail = newNode;
      //Returns the new inserted node
      return newNode;
  }

  //If the list head is not null
  else {
      //Sets the tail to next to the new node
      list->tail->next = newNode;
      //Sets the new node's previous pointer to the tail
      newNode->prev = list->tail;
      //Changes the tail pointer to the new node
      list->tail = newNode;
  }
  
  //Returns the new inserted node
  return newNode;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    
      //Keeps the next node
  list_node_t* next = node -> next;
  //Keeps the previous node
  list_node_t* prev = node -> prev;
  
  //Frees the memory for the node removed
  free(node);
  
  //Removes one from the count to keep the count right as one node is removed
  list -> count -= 1;

  //If the node being removed is the tail node
  if (next == NULL) 
  {
    //Change the tail node to the previous node
    list -> tail = prev;
  }
  else 
  {
    //Else, link the next node to the previous node
    next -> prev = prev;
  }    
    
  //If the node being removed is the head node
  if (prev == NULL) 
  {
    //Change the head node to the next node
    list -> head = next;
  } 
  else 
  {
    //Else, likn the previous node to the next node
    prev -> next = next;
  }
}