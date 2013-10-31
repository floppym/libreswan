#define LEAK_DETECTIVE
#define AGGRESSIVE 1
#define PRINT_SA_DEBUG 1
#include "../../../lib/libswan/alg_info.c"

char *progname;

void exit_tool(int stat)
{
	exit(stat);
}

main(int argc, char *argv[]){
	int i;
	struct alg_info *aie;
	const char *err;
	char algbuf[256];

	progname = argv[0];

	tool_init_log();

	err = "no error";

	aie = (struct alg_info *)alg_info_esp_create_from_str(
		"3des-sha1;modp1024", &err);
	passert(aie != NULL);
	alg_info_snprint(algbuf, 256, aie);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	aie =  (struct alg_info *)alg_info_esp_create_from_str("3des-sha1",
							       &err);
	passert(aie != NULL);
	alg_info_snprint(algbuf, 256, aie);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	aie =  (struct alg_info *)alg_info_esp_create_from_str("aes256-sha1",
							       &err);
	passert(aie != NULL);
	alg_info_snprint(algbuf, 256, aie);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	aie =  (struct alg_info *)alg_info_esp_create_from_str("aes-sha2",
							       &err);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	aie =  (struct alg_info *)alg_info_ah_create_from_str("md5", &err);
	alg_info_snprint(algbuf, 256, aie);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	aie =  (struct alg_info *)alg_info_ah_create_from_str("vanityhash1",
							      &err);
	printf("1 err = %s alg=%s\n", err, algbuf);
	alg_info_free(aie);

	report_leaks();
	tool_close_log();
	exit(0);
}
