/*
 * Libreswan config file writer (confwrite.c)
 * Copyright (C) 2004-2006 Michael Richardson <mcr@xelerance.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <sys/queue.h>

#include "ipsecconf/parser.h"
#include "ipsecconf/confread.h"
#include "ipsecconf/confwrite.h"
#include "ipsecconf/keywords.h"

void confwrite_list(FILE *out, char *prefix, int val, const struct keyword_def *k)
{
	const struct keyword_enum_values *kevs = k->validenum;
	const struct keyword_enum_value  *kev  = kevs->values;
	int i;
	char *sep = "";

	for (i = 0; i < (int)kevs->valuesize; i++) {
		unsigned int mask = kev[i].value;

		if (mask != 0 && (val & mask) == mask) {
			fprintf(out, "%s%s%s", sep, prefix, kev[i].name);
			sep = " ";
		}
	}
}

static void confwrite_int(FILE *out,
		   char   *side,
		   unsigned int context,
		   unsigned int keying_context,
		   knf options,
		   int_set options_set,
		   ksf strings)
{
	const struct keyword_def *k;

	for (k = ipsec_conf_keywords_v2; k->keyname != NULL; k++) {

		if ((k->validity & KV_CONTEXT_MASK) != context)
			continue;
		if (keying_context != 0 && (k->validity & keying_context) == 0)
			continue;

		/* do not output aliases */
		if (k->validity & kv_alias)
			continue;

		/* do not output policy settings handled elsewhere */
		if (k->validity & kv_policy)
			continue;
		if (k->validity & kv_processed)
			continue;

#if 0
		printf("#side: %s  %s validity: %08x & %08x=%08x vs %08x\n",
		       side,
		       k->keyname, k->validity, KV_CONTEXT_MASK,
		       k->validity & KV_CONTEXT_MASK, context);
#endif

		switch (k->type) {
		case kt_string:
		case kt_appendstring:
		case kt_appendlist:
		case kt_filename:
		case kt_dirname:
		case kt_rsakey:

		case kt_percent:
		case kt_ipaddr:
		case kt_subnet:
		case kt_range:
		case kt_idtype:
		case kt_bitstring:
			/* none of these are valid number types */
			continue;

		case kt_time:
			/* special number, but do work later XXX */
			break;

		case kt_bool:
		case kt_invertbool:
			/* special enumeration */
			if (options_set[k->field]) {
				int val = options[k->field];
				if (k->type == kt_invertbool)
					val = !val;

				fprintf(out, "\t%s%s=%s\n", side,
					k->keyname, val ? "yes" : "no");
			}
			continue;

		case kt_enum:
		case kt_loose_enum:
			/* special enumeration */
			if (options_set[k->field]) {
				int val = options[k->field];
				fprintf(out, "\t%s%s=", side, k->keyname);

				if (k->type == kt_loose_enum && val ==
				    LOOSE_ENUM_OTHER) {
					fprintf(out, "%s\n",
						strings[k->field]);
				} else {
					const struct keyword_enum_values *kevs =
						k->validenum;
					const struct keyword_enum_value  *kev  =
						kevs->values;
					int i = 0;

					while (i < (int)kevs->valuesize) {
						if ((int)kev[i].value == val) {
							fprintf(out, "%s",
								kev[i].name);
							break;
						}
						i++;
					}
					fprintf(out, "\n");
				}
			}
			continue;

		case kt_list:
			/* special enumeration */
			if (options_set[k->field]) {
				int val = options[k->field];

				if (val == 0)
					continue;

				fprintf(out, "\t%s%s=\"", side, k->keyname);
				confwrite_list(out, "", val, k);

				fprintf(out, "\"\n");
			}
			continue;

		case kt_number:
			break;

		case kt_comment:
			continue;

		case kt_obsolete:
		case kt_obsolete_quiet:
			continue;
		}

		if (options_set[k->field])
			fprintf(out, "\t%s%s=%d\n", side, k->keyname,
				options[k->field]);
	}
}

static void confwrite_str(FILE *out,
		   char   *side,
		   unsigned int context,
		   unsigned int keying_context,
		   ksf strings,
		   str_set strings_set)
{
	const struct keyword_def *k;

	for (k = ipsec_conf_keywords_v2; k->keyname != NULL; k++) {

		if ((k->validity & KV_CONTEXT_MASK) != context)
			continue;
		if (keying_context != 0 && (k->validity & keying_context) == 0)
			continue;

		/* do not output aliases */
		if (k->validity & kv_alias)
			continue;

		/* do not output policy settings handled elsewhere */
		if (k->validity & kv_policy)
			continue;
		if (k->validity & kv_processed)
			continue;

		switch (k->type) {
		case kt_appendlist:
			if (strings_set[k->field])
				fprintf(out, "\t%s%s={%s}\n", side, k->keyname,
					strings[k->field]);
			continue;

		case kt_string:
		case kt_appendstring:
		case kt_filename:
		case kt_dirname:
			/* these are strings */
			break;

		case kt_rsakey:
		case kt_ipaddr:
		case kt_range:
		case kt_subnet:
		case kt_idtype:
		case kt_bitstring:
			continue;

		case kt_bool:
		case kt_invertbool:
		case kt_enum:
		case kt_list:
		case kt_loose_enum:
			/* special enumeration */
			continue;

		case kt_time:
			/* special number, not a string */
			continue;

		case kt_percent:
		case kt_number:
			continue;

		case kt_comment:
			continue;

		case kt_obsolete:
		case kt_obsolete_quiet:
			continue;
		}

		if (strings_set[k->field]) {
			char *quote = "";

			if (strchr(strings[k->field], ' '))
				quote = "\"";

			fprintf(out, "\t%s%s=%s%s%s\n", side, k->keyname,
				quote,
				strings[k->field],
				quote);
		}
	}
}

static void confwrite_side(FILE *out,
		    struct starter_end *end,
		    char   *side)
{
	switch (end->addrtype) {
	case KH_NOTSET:
		/* nothing! */
		break;

	case KH_DEFAULTROUTE:
		fprintf(out, "\t%s=%%defaultroute\n", side);
		break;

	case KH_ANY:
		fprintf(out, "\t%s=%%any\n", side);
		break;

	case KH_IFACE:
		if (end->strings_set[KSCF_IP])
			fprintf(out, "\t%s=%s\n", side, end->strings[KSCF_IP]);
		break;

	case KH_OPPO:
		fprintf(out, "\t%s=%%opportunistic\n", side);
		break;

	case KH_OPPOGROUP:
		fprintf(out, "\t%s=%%opportunisticgroup\n", side);
		break;

	case KH_GROUP:
		fprintf(out, "\t%s=%%group\n", side);
		break;

	case KH_IPHOSTNAME:
		if (end->strings_set[KSCF_IP])
			fprintf(out, "\t%s=%s\n", side, end->strings[KSCF_IP]);
		break;

	case KH_IPADDR:
		{
			char as[ADDRTOT_BUF];

			addrtot(&end->addr, 0, as, sizeof(as));
			fprintf(out, "\t%s=%s\n", side, as);
		}
		break;
	}

	if (end->strings_set[KSCF_ID] && end->id)
		fprintf(out, "\t%sid=\"%s\"\n",     side, end->id);

	switch (end->nexttype) {
	case KH_NOTSET:
		/* nothing! */
		break;

	case KH_DEFAULTROUTE:
		fprintf(out, "\t%snexthop=%%defaultroute\n", side);
		break;

	case KH_IPADDR:
		{
			char as[ADDRTOT_BUF];

			addrtot(&end->nexthop, 0, as, sizeof(as));
			fprintf(out, "\t%snexthop=%s\n", side, as);
		}
		break;

	default:
		break;
	}

	if (end->has_client) {
		if (!subnetishost(&end->subnet) ||
		     !addrinsubnet(&end->addr, &end->subnet))
		{
			char as[ADDRTOT_BUF];

			subnettot(&end->subnet, 0, as, sizeof(as));
			fprintf(out, "\t%ssubnet=%s\n", side, as);
		}
	}

	if (end->rsakey1 != NULL)
		fprintf(out, "\t%srsasigkey=%s\n", side, end->rsakey1);

	if (end->rsakey2 != NULL)
		fprintf(out, "\t%srsasigkey2=%s\n", side, end->rsakey2);

	if (end->port != 0 || end->protocol != 0) {
		/* it is hoped that any number fits within 32 characters */
		char portstr[32] = "%any";
		char protostr[32] = "%any";

		if (end->port != 0)
			snprintf(portstr, sizeof(portstr), "%u", end->port);
		if (end->protocol != 0)
			snprintf(protostr, sizeof(protostr), "%u", end->protocol);

		fprintf(out, "\t%sprotoport=%s/%s\n", side,
			portstr, protostr);
	}

	if (end->cert)
		fprintf(out, "\t%scert=%s\n", side, end->cert);

	if (!isanyaddr(&end->sourceip)) {
		char as[ADDRTOT_BUF];

		addrtot(&end->sourceip, 0, as, sizeof(as));
		fprintf(out, "\t%ssourceip=%s\n", side, as);
	}

	confwrite_int(out, side,
		      kv_conn | kv_leftright, kv_auto,
		      end->options, end->options_set, end->strings);
	confwrite_str(out, side, kv_conn | kv_leftright, kv_auto,
		      end->strings, end->strings_set);

}

static void confwrite_comments(FILE *out, struct starter_conn *conn)
{
	struct starter_comments *sc, *scnext;

	for (sc = conn->comments.tqh_first;
	     sc != NULL;
	     sc = scnext) {
		scnext = sc->link.tqe_next;

		fprintf(out, "\t%s=%s\n",
			sc->x_comment, sc->commentvalue);
	}
}

static void confwrite_conn(FILE *out,
		    struct starter_conn *conn)
{
	/* short-cut for writing out a field (string-valued, indented, on its own line) */
#	define cwf(name, value)	do fprintf(out, "\t" name "=%s\n", (value)); while (0)

	fprintf(out, "# begin conn %s\n", conn->name);

	fprintf(out, "conn %s\n", conn->name);

	if (conn->alsos) {
		/* handle also= as a comment */

		int alsoplace = 0;

		fprintf(out, "\t#also = ");
		while (conn->alsos[alsoplace] != NULL) {
			fprintf(out, "%s ", conn->alsos[alsoplace]);
			alsoplace++;
		}
		fprintf(out, "\n");
	}
	confwrite_side(out, &conn->left,  "left");
	confwrite_side(out, &conn->right, "right");
	confwrite_int(out, "", kv_conn, kv_auto,
		      conn->options, conn->options_set, conn->strings);
	confwrite_str(out, "", kv_conn, kv_auto,
		      conn->strings, conn->strings_set);
	confwrite_comments(out, conn);

	if (conn->connalias)
		cwf("connalias", conn->connalias);

	{
		const char *dsn = "UNKNOWN";

		switch (conn->desired_state) {
		case STARTUP_IGNORE:
			dsn = "ignore";
			break;

		case STARTUP_POLICY:
			dsn = "policy";	/* ??? no keyword for this */
			break;

		case STARTUP_ADD:
			dsn = "add";
			break;

		case STARTUP_ONDEMAND:
			dsn = "ondemand";
			break;

		case STARTUP_START:
			dsn = "start";
			break;
		}
		cwf("auto=", dsn);
	}

	if (conn->policy) {
		lset_t phase2_policy =
			(conn->policy &
			 (POLICY_AUTHENTICATE | POLICY_ENCRYPT));
		lset_t failure_policy = (conn->policy & POLICY_FAIL_MASK);
		lset_t shunt_policy = (conn->policy & POLICY_SHUNT_MASK);
		lset_t ikev2_policy = (conn->policy & POLICY_IKEV2_MASK);
		lset_t ike_frag_policy = (conn->policy & POLICY_IKE_FRAG_MASK);
		static const char *const noyes[2 /*bool*/] = {"no", "yes"};
		/* short-cuts for writing out a field that is a policy bit.
		 * cwpbf flips the sense of teh bit.
		 */
#		define cwpb(name, p)  do cwf(name, noyes[(conn->policy & (p)) != LEMPTY]); while (0)
#		define cwpbf(name, p)  do cwf(name, noyes[(conn->policy & (p)) == LEMPTY]); while (0)

		switch (shunt_policy) {
		case POLICY_SHUNT_TRAP:
			cwf("type", conn->policy & POLICY_TUNNEL? "tunnel" : "transport");

			cwpb("compress", POLICY_COMPRESS);

			cwpb("pfs", POLICY_PFS);
			cwpbf("ikepad", POLICY_NO_IKEPAD);
			/* ??? the following used to write out "rekey=no  #duplicate?" */
			cwpbf("rekey", POLICY_DONT_REKEY);
			cwpbf("overlapip", POLICY_OVERLAPIP);

			{
				const char *abs = "UNKNOWN";

				switch (conn->policy & POLICY_ID_AUTH_MASK) {
				case POLICY_PSK:
					abs = "secret";
					break;

				case POLICY_RSASIG:
					abs = "rsasig";
					break;

				default:
					abs = "never";
					break;
				}
				cwf("authby", abs);
			}

			{
				const char *p2ps = "UNKNOWN";

				switch (phase2_policy) {
				case POLICY_AUTHENTICATE:
					p2ps = "ah";
					break;

				case POLICY_ENCRYPT:
					p2ps = "esp";
					break;

				case POLICY_ENCRYPT | POLICY_AUTHENTICATE:
					p2ps = "ah+esp";
					break;

				default:
					break;
				}
				cwf("phase2", p2ps);
			}

			{
				const char *fps = "UNKNOWN";

				switch (failure_policy) {
				case POLICY_FAIL_NONE:
					fps = NULL;	/* uninteresting */
					break;

				case POLICY_FAIL_PASS:
					fps = "passthrough";
					break;

				case POLICY_FAIL_DROP:
					fps = "drop";
					break;

				case POLICY_FAIL_REJECT:
					fps = "reject";
					break;

				default:
					fps = "UNKNOWN";
					break;
				}
				if (fps != NULL)
					cwf("failureshunt", fps);
			}

			{
				const char *v2ps = "UNKNOWN";

				switch (ikev2_policy) {
				case 0:
					v2ps = "never";
					break;

				case POLICY_IKEV2_ALLOW:
					/* it's the default, do not print anything */
					/* fprintf(out, "\tikev2=permit\n"); */
					v2ps = NULL;
					break;

				case POLICY_IKEV2_ALLOW | POLICY_IKEV2_PROPOSE:
					v2ps = "never";
					break;

				case POLICY_IKEV1_DISABLE | POLICY_IKEV2_ALLOW | POLICY_IKEV2_PROPOSE:
					v2ps = "insist";
					break;
				}
				if (v2ps != NULL)
					cwf("ikev2", v2ps);
			}

			{
				const char *fps = "UNKNOWN";

				switch (ike_frag_policy) {
				case 0:
					fps = "never";
					break;

				case POLICY_IKE_FRAG_ALLOW:
					/* it's the default, do not print anything */
					fps = NULL;
					break;

				case POLICY_IKE_FRAG_ALLOW | POLICY_IKE_FRAG_FORCE:
					fps = "force";
					break;
				}
				if (fps != NULL)
					cwf("ike_frag", fps);
			}

			break; /* end of case POLICY_SHUNT_TRAP */

		case POLICY_SHUNT_PASS:
			cwf("type", "passthrough");
			break;

		case POLICY_SHUNT_DROP:
			cwf("type", "drop");
			break;

		case POLICY_SHUNT_REJECT:
			cwf("type", "reject");
			break;

		}

#		undef cwpb
#		undef cwpbf
	}

	fprintf(out, "# end conn %s\n\n", conn->name);
#	undef cwf
}

void confwrite(struct starter_config *cfg, FILE *out)
{
	struct starter_conn *conn;

	/* output version number */
	/* fprintf(out, "\nversion 2.0\n\n"); */

	/* output config setup section */
	fprintf(out, "config setup\n");
	confwrite_int(out, "",
		      kv_config, 0,
		      cfg->setup.options, cfg->setup.options_set,
		      cfg->setup.strings);
	confwrite_str(out, "",
		      kv_config, 0,
		      cfg->setup.strings, cfg->setup.strings_set);

	fprintf(out, "\n\n");

	/* output connections */
	for (conn = cfg->conns.tqh_first; conn != NULL;
	     conn = conn->link.tqe_next)
		confwrite_conn(out, conn);
	fprintf(out, "# end of config\n");
}
