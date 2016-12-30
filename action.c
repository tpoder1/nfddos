

#include "nfddos.h"
#include "string.h"
#include "time.h"

int nfd_act_init(nfd_actions_t *act, int max_actions) {

	act->actions = (nfd_action_t **)calloc(max_actions, sizeof(nfd_action_t *)); 

	if (act->actions == NULL) {
		msg(MSG_ERROR, "Can not alocate memory for actions, required %d itesm", max_actions);
		return 0;
	}

	act->max_actions = max_actions;

	return 1;

}

/* update or insert new action */
int nfd_act_upsert(nfd_actions_t *act, char *name) {

	nfd_action_t *a;

	int i;

	/* lookup existing action by name */
	a = NULL;
	for (i = 0; i < act->max_actions; i++) {
		if  ( act->actions[i] != NULL && act->actions[i]->name != NULL && strncmp(act->actions[i]->name, name, MAX_STRING) == 0)  {
			a = act->actions[i];
			break;
		}
	}

	/* action with this name was not found - we create new one */
	if (a == NULL) {
		a = calloc(sizeof(nfd_action_t), 1); 

		if (a == NULL) {
			msg(MSG_ERROR, "Can not alocate memory %s:%d", __FILE__, __LINE__);
			return 0;
		}

		/* find empty slot for new created actions */
		for (i = 0; i < act->max_actions; i++) {
			if  ( act->actions[i] == NULL ) {
				act->actions[i] = a;
				break;
			}
		}

		/* no free slots */
		if (i == act->max_actions - 1) {
			msg(MSG_ERROR, "Can find free action slot");
			free(a);
			return 0;
		}

		/* steps performed when a new action is invoked */
		/* XXX TODO */	
		strncpy(a->name, name, MAX_STRING);

		msg(MSG_DEBUG, "NEW PROFILE ACTION FOR %s", name);
	} 

	/* we update information in existing profile */
	a->updated = time(NULL);

	return 1;

}


/* evaluate limits in profile and if limits are reached take action */
int nfd_act_eval_profile(nfd_options_t *opt, nfd_profile_t *profp, char *key, nfd_counter_t *c) {

	int over_limit = 0;
	int w = opt->last_window_size;
	char name[MAX_STRING];
	/* check limits */

	if (profp->limits.bytes != 0 && profp->limits.bytes <= c->bytes / w) {
		over_limit = 1;
	}

	if (profp->limits.pkts != 0 && profp->limits.pkts <= c->pkts / w) {
		over_limit = 1;
	}

	if (profp->limits.flows != 0 && profp->limits.flows <= c->flows / w) {
		over_limit = 1;
	}

	if (!over_limit) {
		/* nothing to do */ 
		return 0;
	}

	if (key == NULL || strncmp(key, "", MAX_STRING) == 0) {
		snprintf(name, MAX_STRING, "%s", profp->name);
	} else {
		snprintf(name, MAX_STRING, "%s/%s", profp->name, key);
	}

	msg(MSG_DEBUG, "Profile %s reached the limit", name);

	nfd_act_upsert(&opt->actions, name);

	return 1;
}
