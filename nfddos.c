
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

/* add flow into pfifiles */
int nfd_profile_add_flow(nfd_profile_t *profp, lnf_rec_t *recp) { 

	//nfd_track_t *trackp;
	lnf_brec1_t brec1;
	uint64_t bps, pps, time;

	if (profp->input_filter != NULL) {
		/* record do not match profile filter */
		if (!lnf_filter_match(profp->input_filter, recp)) {
			return 0;
		}
	}

	/* get flow data */
	lnf_rec_fget(recp, LNF_FLD_BREC1, &brec1);


	histc_add(&profp->hcounter, brec1.bytes, brec1.pkts, brec1.first, brec1.last - brec1.first);



	histc_get_avg(&profp->hcounter, &bps, &pps, &time);
	
	printf("AVG Mb/s: %d  p/s: %d\n", bps / 1000 / 1000, pps);

	histc_get_peak(&profp->hcounter, &bps, &pps, &time);
	
	printf("MAX Mb/s: %d  p/s: %d\n\n", bps / 1000 / 1000, pps);
	return 1;

}

int read_data_loop(nfd_options_t *opt) {

	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	nfd_profile_t *profp;

	msg(MSG_DEBUG, "Processing file %s", filename);
	if (lnf_ring_init(&ringp, filename, LNF_READ, NULL) != LNF_OK) {
		msg(MSG_ERROR, "Can not open file '%s'", filename);
		return 0;
	}

	lnf_rec_init(&recp);

	while (lnf_ring_read(ringp, recp) != LNF_EOF) {

		profp = opt->root_profile;

		while (profp != NULL) {
			nfd_profile_add_flow(profp, recp);
			profp = profp->next_profile;
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
		.stop_delay = 60, 
		.num_slots = 30, 
		.treshold = 0.8 };


	strcpy(opt.config_file, "./nfddos.conf");	
	strcpy(opt.input_files, "./nfddos_fifo");	
	strcpy(opt.pid_file, "/var/run/nfddos.pid");	
	strcpy(opt.exec_start, "./nfddos-start.sh");	
	strcpy(opt.exec_stop, "./nfddos-stop.sh");	
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
}

