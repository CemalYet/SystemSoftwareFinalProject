/**
 * \author Cemal Yetismis
 */

#include "dplist.h"

/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
	void *(*element_copy)(void *src_element),
	void (*element_free)(void **element),
	int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {

     if(*list== NULL){
        return;
    }
 
  dplist_node_t *temp;
  
  while((*list)->head != NULL) {

    temp = (*list)->head;
    (*list)->head = temp->next;
    if(free_element==true && temp->element != NULL){
        (*list)->element_free(&temp->element);
    }
    free(temp->element);
    free(temp);
  }
  free(*list);
  *list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {

    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;

    list_node = malloc(sizeof(dplist_node_t));
    
    if(insert_copy==true){
       list_node->element=list->element_copy(element);
       //list->free_element(&element);
    }else{
    list_node->element = element;
    }
    // pointer drawing breakpoint
    if (list->head == NULL) { // covers case 1
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        // pointer drawing breakpoint
    } else if (index <= 0) { // covers case 2
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        // pointer drawing breakpoint
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        // pointer drawing breakpoint
        if (index < dpl_size(list)) { // covers case 4
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
            // pointer drawing breakpoint
        } else { // covers case 3
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
            // pointer drawing breakpoint
        }
    }
    return list;

}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {

    
    dplist_node_t *to_delete_node,*list_node;
  
    if (list == NULL) return NULL;
    if(list->head == NULL) return list;
  
    list_node=list->head;
    to_delete_node=dpl_get_reference_at_index(list,index);

   
    if(list->head->next==NULL){
        list->head=NULL;
   
    }
    //Case 1 head
    else if(index <= 0){
        
        list->head=list_node->next;
        list_node->next->prev= NULL;  

    }
    //Case 2 between
    else if(index< dpl_size(list)-1) {
    	to_delete_node->next->prev=to_delete_node->prev;
    	to_delete_node->prev->next=to_delete_node->next;
	}
    //Case 3 tail

    else{
        
        to_delete_node->prev->next=NULL;
	}

    if(free_element==true){
        
        list->element_free(&(to_delete_node->element));
    }
    free(to_delete_node->element);    
    free(to_delete_node);
   
    to_delete_node=NULL;
    return list;

}


int dpl_size(dplist_t *list) {
	dplist_node_t *temp_node;
	int count=0;
    if (list== NULL) return -1;
    temp_node=list->head;
    
    while(temp_node!=NULL){
        count++; 
        temp_node=temp_node->next;
        
    }
    return count;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {

    dplist_node_t *current_node = dpl_get_reference_at_index(list,index);
    if(current_node == NULL) {
        return 0;
    }
    return current_node->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
   
    if (list == NULL ) return -1;
   
    dplist_node_t *temp_node = list->head;
    if(temp_node==NULL) return -1;
    
    int index=0;
    while(temp_node!=NULL){
        if(element==NULL && temp_node->element==NULL){
            return index;
        }else if(temp_node->element==NULL){
            temp_node=temp_node->next;
            index++;
        }else if(element==NULL){
            temp_node=temp_node->next;
            index++;
        }else{
            if(list->element_compare(temp_node->element,element)==0)  return index;
            temp_node=temp_node->next;
            index++;
        }
    }
    return -1;
}


dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {

    if (list == NULL) return NULL;
    int count;
    dplist_node_t *dummy;
    
    if (list->head == NULL) return NULL;
    for (dummy = list->head, count = 0; dummy->next != NULL; dummy = dummy->next, count++) {
        if (count >= index) return dummy;
    }
    return dummy;

}

    
           //*** HERE STARTS THE EXTRA SET OF OPERATORS ***//

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {

    if(list ==NULL || reference==NULL || list->head==NULL)return NULL;
   
    dplist_node_t *temp_node=list->head;
    while(temp_node!=NULL){
    if(temp_node==reference) return temp_node->element;
       temp_node=temp_node->next;
    }
    return NULL;
}

int dpl_get_index_of_reference(dplist_t *list, dplist_node_t *reference){

	if(list ==NULL || reference==NULL || list->head==NULL)return -1;
	int count=0;
	dplist_node_t *temp_node=list->head;

	while(temp_node!=NULL){
		if(temp_node==reference) return count;
		temp_node=temp_node->next;
		count++;
	}
	return -1;
}

dplist_node_t *dpl_get_first_reference(dplist_t *list){
    if(list==NULL) return NULL;
    if(list->head==NULL) return NULL;
    return list->head;
}


dplist_node_t *dpl_get_last_reference(dplist_t *list){
    
    if(list==NULL) return NULL;
    if(list->head==NULL) return NULL;

    dplist_node_t *last_reference;
    int index = dpl_size(list)-1;
    last_reference = dpl_get_reference_at_index(list,index);
    return last_reference;
}


dplist_node_t *dpl_get_next_reference(dplist_t *list, dplist_node_t *reference){

    if(list==NULL || list->head==NULL || reference==NULL) return NULL;
    dplist_node_t *temp_node=list->head;
    while(temp_node!=NULL){
        if(temp_node==reference) return temp_node->next;
        temp_node=temp_node->next;
    }
    return NULL;
}


dplist_node_t *dpl_get_previous_reference(dplist_t *list, dplist_node_t *reference){

	if(list==NULL || list->head==NULL || reference==NULL) return NULL;
    dplist_node_t *temp_node=list->head;
    while(temp_node!=NULL){
        if(temp_node==reference) return temp_node->prev;
        temp_node=temp_node->next;
    }
    return NULL;
}

dplist_node_t *dpl_get_reference_of_element(dplist_t *list, void *element){

	if(list==NULL || list->head==NULL) return NULL;

	int index= dpl_get_index_of_element(list,element);
	if(index == -1) return NULL;
	return (dpl_get_reference_at_index(list,index));
}


dplist_t *dpl_insert_at_reference(dplist_t *list, void *element, dplist_node_t *reference, bool insert_copy){

	if(list==NULL) return NULL;
	if(reference==NULL) return NULL;
	
	int index_of_refrence=dpl_get_index_of_reference(list,reference);
	
	if(index_of_refrence==-1) return list;
	return dpl_insert_at_index(list,element,index_of_refrence,insert_copy);


}


dplist_t *dpl_insert_sorted(dplist_t *list, void *element, bool insert_copy){

    if(list==NULL) return NULL;
    if(list->head==NULL) return dpl_insert_at_index(list,element,0,insert_copy);
    dplist_node_t *current_node;
    current_node=list->head;
    int index=0;
    int compare;
    if(element==NULL) return dpl_insert_at_index(list,element,index,insert_copy);

    while(current_node!=NULL){
        if(current_node->element==NULL) compare=1;
        else{
        	compare=list->element_compare(element,current_node->element);
    	}

        if(compare==1) {
            index++;
            current_node=current_node->next;
          
        }else if(compare<=0){
       
            return dpl_insert_at_index(list,element,index,insert_copy);
        }
    }
    return dpl_insert_at_index(list,element,index,insert_copy);
}


dplist_t *dpl_remove_at_reference(dplist_t *list, dplist_node_t *reference, bool free_element){

    if(list==NULL) return NULL;
    if(list->head == NULL) return list;
    if(reference==NULL) return NULL;
    int index=dpl_get_index_of_reference(list,reference);
    
    if(index==-1) return list;
    return dpl_remove_at_index(list,index,free_element);
}

dplist_t *dpl_remove_element(dplist_t *list, void *element, bool free_element){

    if(list==NULL) return NULL;
    if(list->head == NULL) return list;

    int index=dpl_get_index_of_element(list,element);
    if(index==-1) return list;
    return dpl_remove_at_index(list,index,free_element);
}