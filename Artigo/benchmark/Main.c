//#include "EncryptFunctions.h"
#include "BenchmarkFunctions.h"
#include "RandomString.h"
#include <mysql/mysql.h>

int main(int argc, char **argv) {

	/*Inicializando*/
	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_ciphers();

	unsigned int salt[] = {12345, 54321};
	if (aes_init(key, sizeof(key), (unsigned char *)&salt, &en, &de)) {
	printf("Couldn't initialize AES cipher\n");
	return -1;
	}

		init_benchmark();
		/*TODO
		 *	testes insert/select 1k-1kk - refatorar a chamada dos testes
		 * 	teste update - criar o teste update que dada uma tabela sem proteção adiciona HMAC ou HMAC + EMAC
		 */
	return 0;
}


