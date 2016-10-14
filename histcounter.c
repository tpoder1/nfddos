
#include <histcounter.h>


/* rubg buffer structure */
typedef ringbuf_s {

	int 	size;		/* number of allocated time slots */
	uint64_t *data;		/* timeslots for counters */

} ringbuf_t;


typedef histc_s {

	int 	numslots;		/* number of allocated time slots */
	uint64_t *tslots;		/* timeslots for counters */
	uint64_t end_time;	/* unixtimestamp (in miliseconds) of the first timeslot */
	uint64_t slot_size;		/* size of each slot in miliseconds */
	uint64_t end_slot;	/* number of the first slot */

} histc_t;


/* initialize ring buffer */
int ringbuf_init(ringbuf_t *rb, int size) {
	
	rb.data = malloc(size * sizeof(uint64_t)); 
	rb.size = size;

	if (ht.tslots == NULL) {
		return 0;
	}

	return 1;
}

/* shift positions in ring buffer and delete the oldest one */
int ringbuf_shift(ringbuf_t *rb, int numshift) {

	int i = 0;

	if (numshift > rb.size) {
		return 0;
	}

	/* very, very stupid aproach ! */
	for ( i = 0; i < rb.size; i++ ) {

		if (i + numshift < rb.size) {
			/* data on position i is taken from position i + numshift */ 
			rb->data[i] = rb->data[i + numshift];
		} else {
			/* tail remaining tail is filled with 0 */
			rb->data[i] = 0;
		}
	}

	return 1;
}


/* add value on  on positions from start_pos to end_pos */
int ringbuf_add(ringbuf_t *rb, uint64_t value, int start_pos, int end_pos) {

	int i;

	if (start_pos > rb->size || end_pos > rb->size || start_pos > end_pos) {
		return 0;	
	}	

	for (i = start_pos; i <= end_pos; i++) {
		rb->data[i] += value;
	}

	return 1;

}

/* get max/avg value from positions from start_pos to end_pos */
int ringbuf_getval(ringbuf_t *rb, uint64_t value, int start_pos, int end_pos) {

	int i;

	if (start_pos > rb->size || end_pos > rb->size || start_pos > end_pos) {
		return 0;	
	}	

	for (i = start_pos; i <= end_pos; i++) {
		rb->data[i] += value;
	}

	return 1;

}


/* initalise new histogramcounter */
int histc_init(histc_t ht, int numsots, uint64_t slot_size) {

	ht.tslots = malloc(numslots * sizeof(uint64_t)); 

	if (ht.tslots == NULL) {
		return 0;
	}

	ht.numslots = numslots;
	ht.slot_size = slot_size;

	return 1;
	
}


/* insert data into histogram counter */
int histc_add(histc_t ht, uint64_t value, uint64_t stat_time, uint64_t duration) {

	/* is the reason to shift start/start time in ht */
	if ( start_time + duration > ht.end_time ) { 
		
	}

	/* short duration - use only one slot */
	if ( duration <= ht.slot_size ) {
		
	} 
	
}

