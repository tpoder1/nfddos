
#include "histcounter.h"
#include <stdint.h>
#include <stdlib.h>


/* initalise new histogramcounter */
int histc_init(histc_t *ht, int num_slots, uint64_t slot_size) {


	ht->slots = (void *)calloc(num_slots, sizeof(histslot_t)); 

	if (ht->slots == NULL) {
		return 0;
	}


	ht->num_slots = num_slots;
	ht->slot_size = slot_size;

	return 1;
	
}


/* insert data into histogram counter */
int histc_add(histc_t *ht, uint64_t bytes, uint64_t pkts, uint64_t start_time, uint64_t duration) {

	int num_slots, global_slot_number, slot_number, i; 
	uint64_t bytes_per_slot, pkts_per_slot, slot_ts;


	if (duration > 0) {
		/* get number of timeslots */
		num_slots = duration / ht->slot_size;

		if ( duration % ht->slot_size > 0) {
			num_slots++;
		}
	} else {
		num_slots = 1;
	}


	/* divide value amongst all slots */
	bytes_per_slot = bytes / num_slots;
	pkts_per_slot = pkts / num_slots;

	/* 

		The time interval is divided into time slots. The absolute slot number is determined as: 

			global_slot_number = timestamp / slot_size

		The slot number within the specific time window (slot_size * num_slots) is: 

			slot_number = absolute_slot_number  % numslots
		

	*/


	/* add value in all slots in interval */
	for ( slot_ts = start_time; slot_ts < start_time + duration; slot_ts += ht->slot_size ) {

		/* get slot number and position in slot  */
		global_slot_number = slot_ts / ht->slot_size; 
		slot_number = global_slot_number  % ht->num_slots; 

		/* if the slot number changed then reset value to 0 */
		if ( ht->slots[slot_number].global_slot_number != global_slot_number ) {

			/* global statistics - remove previous value */
			ht->bytes -= ht->slots[slot_number].bytes;
			ht->pkts -= ht->slots[slot_number].pkts;

			ht->slots[slot_number].global_slot_number = global_slot_number;

			ht->slots[slot_number].bytes = 0;
			ht->slots[slot_number].pkts = 0;

			/* if the current slot is same as peak slot we have to find a new peak slot */
			if (ht->peak_slot_bytes == slot_number) {
				for (i = 0; i < ht->num_slots; i++) {
					if (ht->slots[i].bytes >= ht->slots[ht->peak_slot_bytes].bytes) {
						ht->peak_slot_bytes =  i;
					}
				}
			}

			if (ht->peak_slot_pkts == slot_number) {
				for (i = 0; i < ht->num_slots; i++) {
					if (ht->slots[i].pkts >= ht->slots[ht->peak_slot_pkts].pkts) {
						ht->peak_slot_pkts =  i;
					}
				}
			}
		}


		/* add statistics to the slot */
		ht->slots[slot_number].bytes += bytes_per_slot;
		ht->slots[slot_number].pkts += pkts_per_slot;

		/* increase global statistics */
		ht->bytes += bytes_per_slot;
		ht->pkts += pkts_per_slot;
		ht->time += slot_ts;

		/* set this slot as peak slot if the value in the current slot is bigger than in the peak slot */
		if (ht->slots[slot_number].bytes >= ht->slots[ht->peak_slot_bytes].bytes) {
			ht->peak_slot_bytes = slot_number;
		}

		if (ht->slots[slot_number].pkts >= ht->slots[ht->peak_slot_pkts].pkts) {
			ht->peak_slot_pkts = slot_number;
		}

	}

	return 1;
}


int histc_get_avg(histc_t *ht, uint64_t *bps, uint64_t *pps, uint64_t *time) {

	*bps = (ht->bytes / (ht->slot_size * ht->num_slots)) * 1000 * 8;
	*pps = (ht->pkts / (ht->slot_size * ht->num_slots)) * 1000;
	*time = ht->time;


	/* print all slots */
	/*
	{ 
	int i ;
	for (i = 0; i < ht->num_slots; i++) {
		printf("%d:%d ", i, ht->slots[i].bytes);
	}

	printf("\n");
	}
	*/

	return 1;	
}

int histc_get_peak(histc_t *ht, uint64_t *bps, uint64_t *pps, uint64_t *time) {

	*bps = (ht->slots[ht->peak_slot_bytes].bytes / ht->slot_size) * 1000 * 8;
	*pps = (ht->slots[ht->peak_slot_pkts].pkts / ht->slot_size) * 1000;
	*time = ht->slots[ht->peak_slot_pkts].global_slot_number * ht->slot_size;

	return 1;	
}


