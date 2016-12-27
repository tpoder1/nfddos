
#include <stdio.h>
#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include <fcntl.h>
//#include "dataflow.h"

//#include <pcap.h>
//#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include "nfddos.h"
#include "msgs.h"
#include "daemonize.h"
//#include <netinet/tcp.h>
#include <pthread.h>

#define SCAN_TIME 10*1000		/* scan loop - every 10 msecs */
#define UPDATES_IN_STEP 50		/* max number of updates (new/del) in one cycle */


//nfd_options_t *active_opt;


/*
int exec_cmd(options_t *opt, action_t action) {

	char cmd[MAX_STRING];
	FILE *fh;

	switch (action) {
		case ACTION_INIT: snprintf(cmd, MAX_STRING, "%s %s", opt->exec_init, "init"); break;
		case ACTION_FINISH: snprintf(cmd, MAX_STRING, "%s %s", opt->exec_finish, "finish"); break;
		default: return 0; break;
	}

	fh = popen(cmd, "w");

	if (fh == NULL) {
		msg(MSG_ERROR, "Can execute external command %s.", cmd);
		return 0;
	}

	fprintf(fh, "device: %s\n", opt->device);
	fflush(fh);

	if (opt->debug > 5) {
		fprintf(stdout, "Trying to execute command %s.\n", cmd);
	}

	if ( pclose(fh) != 0 ) {
		msg(MSG_ERROR, "External command %s was not executed.", cmd);
		return 0;
	}

	return 1;
}
*/


// Kopie zbyvajicich argumantu do rednoho retezce
char *copy_argv(char *argv[]) {
  char **arg;
  char *buf;
  int total_length = 0;

  for (arg = argv; *arg != NULL; arg++)
    total_length += (strlen(*arg) + 1); /* length of arg plus space */

  if (total_length == 0)
    return NULL;

  total_length++; /* add room for a null */

  buf = (char *)malloc(total_length);

  *buf = 0;
  for (arg = argv; *arg != NULL; arg++) {
    strcat(buf, *arg);
    strcat(buf, " ");
  }

  return buf;
}

int mkpidfile(nfd_options_t *opt) {

	FILE * fp;
	int pid;

	fp = fopen(opt->pid_file, "w");

	if (fp == NULL) {
		msg(MSG_INFO, "Can't create PID file %s (%s)", opt->pid_file, strerror(errno));
		return 0;
	}

	pid = getpid();
	fprintf(fp,"%d\n", pid);

	fclose(fp);

	return 1;

}

/* eval all tracks in profile and execute action of conditions are meet  */
/*
int nfd_profile_eval_profile_tracks(nfd_profile_t *profp) { 

	nfd_track_t *trackp;
	lnf_rec_t *recp;
	lnf_mem_cursor_t *cursor;

	trackp = profp->root_track;

	lnf_rec_init(&recp);

	while (trackp != NULL) {
		if (trackp->mem != NULL && trackp->filter != NULL) {
			msg(MSG_DEBUG, "Evaluating track in profile: %s, track: %s", profp->name, trackp->fields);
	
			lnf_mem_first_c(trackp->mem, &cursor);	

			while (cursor != NULL) {

				lnf_mem_read_c(trackp->mem, cursor, recp);
				if (lnf_filter_match(trackp->filter, recp)) {
					msg(MSG_DEBUG, "Matched condition in profile: %s, track: %s", profp->name, trackp->fields);


					
				}

				lnf_mem_next_c(trackp->mem, &cursor);
			}

		}

		trackp = trackp->next_track;
	}

	return 1;
}
*/

void nfd_format_rule(nfd_options_t *opt, FILE *fh, char *buf1, char *buf2, nfd_counter_t *c) { 

	fprintf(fh, "%-7s %-35s %10lld %10lld %10lld %12lld %12lld %10lld   %c\n",
			buf1, buf2,
			(LLUI)c->bytes / opt->window_size,
			(LLUI)c->pkts / opt->window_size,
			(LLUI)c->flows / opt->window_size,
			(LLUI)c->total_bytes,
			(LLUI)c->total_pkts,
			(LLUI)c->total_flows, 
			c->active ? 'Y' : 'N' );

}

int nfd_dump_profile(nfd_options_t *opt, FILE *fh, nfd_profile_t *profp) {

	char *ptr;
	lnf_ip_t *ip;
	nfd_counter_t *c;
	char buf[MAX_STRING];
	char buf2[MAX_STRING];

	c = &profp->counters;

	nfd_format_rule(opt, fh, "static", profp->name, c);	

	c->bytes = 0;
	c->pkts = 0;
	c->flows = 0;

	/* dump data from hash table */
	hash_table_sort(&profp->hash_table);	
	ptr =  hash_table_first(&profp->hash_table);	

	while (ptr) {

		hash_table_fetch(&profp->hash_table, ptr, (char **)&ip, (char **)&c);
			
		/* print data */
		inet_ntop(AF_INET6, ip, (char *)&buf, MAX_STRING);

		sprintf(buf2, "%s/%s", profp->name, buf);
		nfd_format_rule(opt, fh, "dynamic", buf2, c);	
		
		ptr =  hash_table_next(&profp->hash_table, ptr);	
	}

	hash_table_clean(&profp->hash_table);	
		
	return 1;
}

int nfd_dump_profiles(nfd_options_t *opt) {

	nfd_profile_t *profp;
	FILE *fh;

	/* open dbdump file */
	fh = fopen(opt->status_file, "w"); 

	if (fh == NULL) {
		msg(MSG_ERROR, "Can not open file %s.", opt->status_file);
		return 0;
	}

	fprintf(fh, "TYPE    RULE                                       b/s        p/s     flow/s        bytes         pkts      flows act\n");

	profp = opt->root_profile;

	while (profp != NULL) {

		nfd_dump_profile(opt, fh, profp);

		profp = profp->next_profile;
	}	
	
	fclose(fh);	
	return 1;
}

/* callback function for router */
void aggr_callback(char *key, char *hval, char *uval, void *p) {

	((nfd_counter_t *)hval)->bytes += ((nfd_counter_t *)uval)->bytes;
	((nfd_counter_t *)hval)->pkts += ((nfd_counter_t *)uval)->pkts;
	((nfd_counter_t *)hval)->flows += ((nfd_counter_t *)uval)->flows;

	((nfd_counter_t *)hval)->total_bytes += ((nfd_counter_t *)uval)->total_bytes;
	((nfd_counter_t *)hval)->total_pkts += ((nfd_counter_t *)uval)->total_pkts;
	((nfd_counter_t *)hval)->total_flows += ((nfd_counter_t *)uval)->total_flows;
}

int sort_callback(char *key1, char *val1, char *key2, char *val2, void *p) {

	return ((nfd_counter_t *)val1)->bytes > ((nfd_counter_t *)val2)->bytes;
}




/* add flow into pfifiles */
int nfd_profile_add_flow(nfd_profile_t *profp, lnf_rec_t *recp) {

	lnf_brec1_t brec1;
	nfd_counter_t c; 
	nfd_counter_t *pc; 

	if (profp->input_filter != NULL) {
		/* record do not match profile filter */
		if (!lnf_filter_match(profp->input_filter, recp)) {
			return 0;
		}
	} 

	/* get flow data */
	lnf_rec_fget(recp, LNF_FLD_BREC1, &brec1);

	/* add to profile histogram */
//	histc_add(&profp->hcounter, brec1.bytes, brec1.pkts, brec1.first, brec1.last - brec1.first);

	pc = &profp->counters;
	pc->bytes += brec1.bytes;
	pc->pkts += brec1.pkts;
	pc->flows += brec1.flows;
	pc->total_bytes += brec1.bytes;
	pc->total_pkts += brec1.pkts;
	pc->total_flows += brec1.flows;

	c.bytes = brec1.bytes;
	c.pkts = brec1.pkts;
	c.flows = brec1.flows;
	c.total_bytes = brec1.bytes;
	c.total_pkts = brec1.pkts;
	c.total_flows = brec1.flows;
	c.active = 0;
	c.last_updated = brec1.last / 1000 / 1000;
/*
			inet_ntop(AF_INET6, &(brec1.dstaddr), (char *)&buf, MAX_STRING);

			printf("IP1: %s %d %d %d\n", buf, 
				(LLUI)counter.bytes,
				(LLUI)counter.pkts,
				(LLUI)counter.flows);
*/
	/* add to hash table -> per detination IP */
	if (profp->per_dst_ip) { 
		hash_table_insert_hash(&profp->hash_table, (char *)&(brec1.dstaddr), (char *)&c);
	}

	return 1;
}

int read_data_loop(nfd_options_t *opt) {

	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	nfd_profile_t *profp;
	time_t	tm;


	if (lnf_ring_init(&ringp, opt->shm, 0) != LNF_OK) {
		msg(MSG_ERROR, "Can not open file '%s'", opt->shm);
		return 0;
	}

	lnf_rec_init(&recp);

	msg(MSG_INFO, "Reading data from shm/ringbuf %s", opt->shm);


	while (lnf_ring_read(ringp, recp) != LNF_EOF) {

		profp = opt->root_profile;

		while (profp != NULL) {
			nfd_profile_add_flow(profp, recp);
			profp = profp->next_profile;
		}	

		tm = time(NULL);
		if (opt->tm_display + opt->window_size < tm) {
			nfd_dump_profiles(opt);
			opt->tm_display = tm;
		}
		
	}

	lnf_ring_free(ringp);

	return 1;

}


int main(int argc, char *argv[]) {
    extern int optind;
    char op;
//    int pflag = 0;

//	pthread_t read_thread;

	nfd_options_t opt = { 
		.debug = 0, 
		.slot_size = 10, 
		.window_size = 60, 
		.stop_delay = 60, 
		.num_slots = 30, 
		.hash_buckets = 50000, 
		.treshold = 0.8 };


	strcpy(opt.config_file, "./nfddos.conf");	
	strcpy(opt.pid_file, "/var/run/nfddos.pid");	
	strcpy(opt.exec_start, "./nfddos-start.sh");	
	strcpy(opt.exec_stop, "./nfddos-stop.sh");	
	strcpy(opt.status_file, "./nfddos.status");	
	strcpy(opt.flow_queue_file, "./nfddos-queue.pcap");	
	strcpy(opt.shm, "libnf-shm");	


    /*  process options */
	while ((op = getopt(argc, argv, "i:c:d:p:F?h")) != -1) {
		switch (op) {
			case 'c' : strncpy(opt.config_file,optarg, MAX_STRING); break;
			case 'p' : strncpy(opt.pid_file,optarg, MAX_STRING); opt.pid_file_fromarg = 1; break;
			case 'd' : opt.debug = atoi(optarg); opt.debug_fromarg = 1; break;
			case 'F' : opt.foreground = 1; if (!opt.debug_fromarg) opt.debug = 1; break;
			case 'h' :
			case '?' : opt.debug = 1; opt.foreground = 1; msg_init(opt.debug); help(); break;
			} // konec switch op 
    } // konec while op...

	msg_init(opt.debug);

	if (opt.foreground == 0) {
		if ( !daemonize() )  {
			fprintf(stderr, "Can not daemonize process\n");
			exit(1);
		}
	}

	mkpidfile(&opt);


	if (!nfd_parse_config(&opt)) {
		exit(1);
	}

	read_data_loop(&opt);

	return 0;
}

