
#include <stdint.h>
#include <stdlib.h>

/* default values for 5 minute hostogram with 30 slots ( 1 slot is 10 seconds) */
#define HISTC_SLOTS 30
#define HISTC_SIZE  10*1000

/* rubg buffer structure */
typedef struct histslot_s {

	uint64_t 	bytes;		/* slot counter */
	uint64_t 	pkts;		/* slot counter */
	uint64_t 	global_slot_number;	/* slot position  */

} histslot_t;


typedef struct histc_s {

	int 		num_slots;	/* number of allocated time slots */
	uint64_t 	slot_size;	/* size of each slot in miliseconds */
	uint64_t 	bytes;		/* sum of bytes in all slots */
	uint64_t 	pkts;		/* sum of pkts in all slots */
	uint64_t 	time;		/* timestam of last updated slot */
	int 		peak_slot_bytes;		/* slot with peak value */
	int 		peak_slot_pkts;		/* slot with peak value */
	histslot_t	*slots;		/* histogram slots structurre */

} histc_t;



/* initalise new histogramcounter */
int histc_init(histc_t *ht, int num_slots, uint64_t slot_size);


/* insert data into histogram counter */
int histc_add(histc_t *ht, uint64_t bytes, uint64_t pkts, uint64_t start_time, uint64_t duration);

int histc_get_avg(histc_t *ht, uint64_t *bps, uint64_t *pps, uint64_t *time);
int histc_get_peak(histc_t *ht, uint64_t *bps, uint64_t *pps, uint64_t *time);

void histc_free(histc_t *ht);

