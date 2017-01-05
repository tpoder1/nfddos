
#include <stdio.h>
#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include <fcntl.h>
//#include "dataflow.h"

#include "nfddos.h"
#include "msgs.h"

#define QUEUE_FILE_FMT  "%s/%03d"

/* shift queue */
int nfd_queue_shift(nfd_options_t *opt) {

	char buf[MAX_STRING];
	char buf2[MAX_STRING];
	int i;

	/* is the handler open? */
	if (opt->queue_file != NULL) {
		lnf_close(opt->queue_file);

		/* rotate files */
	}

	msg(MSG_DEBUG, "Rotating queue files");
	for (i = opt->queue_num; i > 0; i--) {
		snprintf(buf,  MAX_STRING, QUEUE_FILE_FMT, opt->queue_dir, i - 1 );		
		snprintf(buf2, MAX_STRING, QUEUE_FILE_FMT, opt->queue_dir, i);
//		msg(MSG_DEBUG, "Renaming file %s -> %s", buf, buf2);
		rename(buf, buf2);
	}

	/* open new file handler */
	snprintf(buf, MAX_STRING, QUEUE_FILE_FMT, opt->queue_dir, 0);
	msg(MSG_DEBUG, "Opening queue file %s", buf);
	if (lnf_open(&opt->queue_file, buf, LNF_WRITE, "nfddos") != LNF_OK) {
		msg(MSG_ERROR, "Can not open queue file %s", buf);
		opt->queue_file = NULL;
		return 0;
	}

	return 1;
}

/* shift queue */
int nfd_queue_add_flow(nfd_options_t *opt, lnf_file_t *output, lnf_filter_t *filter, int backtrack) {

	int i;
	char buf[MAX_STRING];
	lnf_rec_t *recp;
	lnf_file_t *filep;

	if (backtrack > opt->queue_num) {
		backtrack = opt->queue_num;
	}

	if (lnf_rec_init(&recp) != LNF_OK) {
		msg(MSG_ERROR, "Can not init rec %s", __FILE__, __LINE__ );
		return 0;
	}

	for (i = backtrack; i > 0; i--) {

		snprintf(buf, MAX_STRING, QUEUE_FILE_FMT, opt->queue_dir, i);
		msg(MSG_DEBUG, "Reading queudata from %s", buf);
		if (lnf_open(&filep, buf, LNF_READ, "") == LNF_OK) {

			while (lnf_read(filep, recp) == LNF_OK) {

				if (filter == NULL || lnf_filter_match(filter, recp)) {
					lnf_write(output, recp);
				}

			}

			lnf_close(filep);

		} else {
			msg(MSG_INFO, "Can not open queue file %s", buf);
		}

	}	

	return 1;
}


