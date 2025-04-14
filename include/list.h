#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/* Circular doubly linked list (same one used in Linux kernel) */
struct list_head {
    struct list_head *prev, *next; // initial structure definition still needs to use full `struct list_head` syntax to refer to itself 
};
typedef struct list_head list_head; // give list_head data type an alias, so we don't have to keep writing "struct" in front (like in C++)

/* Macro for defining a variable with the head and tail of the list */
#define LIST_HEAD(name) \
    list_head name = { &(name), &(name) }

/* Initialize a new list head. */
static inline void list_init(list_head *head) {
    head->prev = head->next = head; 
}

/* Add new element at head of the list */
static inline void list_add(list_head *head, list_head *newp) {
    head->next->prev = newp;
    newp->next = head->next;
    head->next = newp;
    newp->prev = head;
}

/* Add new element at tail of the list */
static inline void list_add_tail(list_head *head, list_head *newp) {
    newp->prev = head->prev; // head->prev is tail node
    head->prev->next = newp; // tail node's next becomes newp
    head->prev = newp;
    newp->next = head;  
}

/* Remove element from list */
static inline void __list_del(list_head *prev, list_head *next) { // pass the elements that come before and after the delete elem    
    prev->next = next;
    next->prev = prev;
}

/* Remove element from list */
static inline void list_del(list_head *elem) {
    __list_del(elem->prev, elem->next);

    elem->prev = NULL;
    elem->next = NULL;
}

/* Get pointer to the containing struct from a pointer to a member within the containing struct */
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member))) // take address of ptr node, determines how far that node is from start of its containing struct, substract that offset to get the base address of the full struct

/* Get pointer to the containing list struct from the list linkage which is within the containing list. So we can access other data fields */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/* Get first entry from a list. Assumes we pass in  */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/* Iterate through each element in the list */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/* Iterate through each element in the list but list elements can be removed from the list. pos is current list_head elem being processed, n is the next list_head elem (saved in advance) */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; \
            pos != (head); \
            pos = n, n = pos->next)

/* Check if a list is empty */
static inline int list_empty(list_head *head) {
    return head->next == head;
}

#endif /* LIST_H */