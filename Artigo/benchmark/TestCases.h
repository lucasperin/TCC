#ifndef TESTCASES_H
#define TESTCASES_H

/*
 * Database Integrity Test Cases.
 * All CRUD functions are used for benchmark tests
 */

#include "DatabaseFunctions.h"
#include "CryptoFunctions.h"
#include <stdlib.h>
/* GLOBAL */
MYSQL* connection = 0;


/* BENCHMARK FUNCTIONS */
/*
 * Calculates interval value of two 'timeval' structures.
 */
long DBtimevaldiff(struct timeval *startTime, struct timeval *finishTime)
{
 long msec;
 msec = (finishTime->tv_sec - startTime->tv_sec) * 1000000.0;
 msec += (finishTime->tv_usec - startTime->tv_usec);
 return msec;
}

long DBbenchmark( char* msg, char* query )
{
	struct timeval startTime;
	struct timeval finishTime;

	gettimeofday(&startTime, 0);
	if (mysql_query(connection, query)) {
	  printf("QUERY: %s\n", query);
	  printf("ERROR: %s::ErrNum %u: %s\n",msg, mysql_errno(connection), mysql_error(connection));
	}
	gettimeofday(&finishTime, 0);

	return DBtimevaldiff(&startTime, &finishTime);
}

/*
 * INSERT
 *
 * InsertOneRecordWithoutProtection         – Tempo de insert de 1 registro, sem uso de HMAC e ECMAC;
 * InsertOneRecordWithHMAC                  – Tempo de insert de 1 registro, calculando HMAC;
 * InsertOneRecordWithHMACAndECMAC          – Tempo de insert de 1 registro, calculando HMAC e ECMAC;
 * InsertOneRecordWithHMACAndCircularECMAC  – Tempo de insert de 1 registro, calculando HMAC e ECMAC circular;
 */

long InsertOneRecordWithoutProtection()
{
	char* query = "INSERT INTO benchmark.user ("
			"name, "
			"email, "
			"password"
			") VALUES ("
			"\'Lucas Pandolfo Perin\', "
			"\'lucasperin@inf.ufsc.br\', "
			"\'naoVouTeDizer\' "
			");";

	return DBbenchmark("InsertOneRecordWithoutProtection", query);
}

long InsertOneRecordWithHMAC()
{
	char query[1000];
	char* message = "Lucas Pandolfo Perinlucasperin@inf.ufsc.brnaoVouTeDizer";

	struct ByteArray* hmac = generateHMAC((unsigned char*)message);

	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	char* stat = "INSERT INTO benchmark.user ("
			"name, "
			"email, "
			"password, "
			"hmac"
			") VALUES ("
			"\'Lucas Pandolfo Perin\', "
			"\'lucasperin@inf.ufsc.br\', "
			"\'naoVouTeDizer\', "
			"\'%s\');";

	snprintf(query, 1000 , stat, chunk);
	free(hmac);
	return DBbenchmark("InsertOneRecordWithHMAC", query);
}

long InsertOneRecordWithHMACAndECMAC()
{
	char query[1000];
	long dbTime = 0;

	char* stat = "INSERT INTO benchmark.user ("
			"name, "
			"email, "
			"password, "
			"hmac, "
			"history"
			") VALUES ("
			"\'Lucas Pandolfo Perin\', "
			"\'lucasperin@inf.ufsc.br\', "
			"\'naoVouTeDizer\', "
			"\'%s\', "
			"\'%s\');";

	//calculando hmac do registro a ser inserido
	char* message = "Lucas Pandolfo Perinlucasperin@inf.ufsc.brnaoVouTeDizer";
	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	//Lendo HMAC do primeiro registro
	struct ByteArray* lastHmac = getLastHmac(connection, "user", &dbTime);

	//calculando history
	struct ByteArray* history;
	if(lastHmac != 0){
		history = generateHistory(hmac, lastHmac);
		char chunk2[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk2, history->data, history->size);
		snprintf(query, 1000, stat, chunk, chunk2);
	}
	else
	{
		history = generateHistory(hmac, hmac);
		char chunk2[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk2, history->data, history->size);
		snprintf(query, 1000, stat, chunk, chunk2);
	}

	dbTime +=  DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);
	free(hmac);
	free(lastHmac);
	free(history);
	return dbTime;
}

long InsertOneRecordWithHMACAndCircularECMAC()
{
	char query[1000];
	long dbTime = 0;

	char* stat = "INSERT INTO benchmark.user ("
			"name, "
			"email, "
			"password, "
			"hmac, "
			"history"
			") VALUES ("
			"\'Lucas Pandolfo Perin\', "
			"\'lucasperin@inf.ufsc.br\', "
			"\'naoVouTeDizer\', "
			"\'%s\', "
			"\'%s\');";

	//calculando hmac do registro a ser inserido
	char* message = "Lucas Pandolfo Perinlucasperin@inf.ufsc.brnaoVouTeDizer";
	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	struct ByteArray* lastHmac = getLastHmac(connection, "user", &dbTime);

	//calculando history
	struct ByteArray* history = generateHistory(hmac, lastHmac);
	char chunk2[2*(history->size) + 1];
	mysql_real_escape_string(connection, chunk2, history->data, history->size);
	snprintf(query, 1000, stat, chunk, chunk2);

	dbTime += DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);

	//update do historico do primeiro registro
	struct ByteArray* firstHmac = getFirstHmac(connection, "user", &dbTime);
	struct ByteArray* firstECMAC = generateHistory(firstHmac, hmac);

	char chunk3[2*(firstECMAC->size) + 1];
	mysql_real_escape_string(connection, chunk3, firstECMAC->data, firstECMAC->size);
	snprintf(query, 1000, stat, chunk, chunk3);

	stat = "UPDATE benchmark.user SET "
			"history =\'%s\' "
			"WHERE id = %d;";
	snprintf(query, 1000, stat, chunk3, firstId(connection, "user"));
	dbTime += DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);
	free(hmac);
	free(lastHmac);
	free(history);
	free(firstECMAC);
	free(firstHmac);

	return dbTime;
}



long InsertOneRecordWithHMACAndCachedCircularECMAC()
{
	char query[1000];
	long dbTime = 0;

	char* stat = "INSERT INTO benchmark.user ("
			"name, "
			"email, "
			"password, "
			"hmac, "
			"history"
			") VALUES ("
			"\'Lucas Pandolfo Perin\', "
			"\'lucasperin@inf.ufsc.br\', "
			"\'naoVouTeDizer\', "
			"\'%s\', "
			"\'%s\');";

	//calculando hmac do registro a ser inserido
	char* message = "Lucas Pandolfo Perinlucasperin@inf.ufsc.brnaoVouTeDizer";
	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	struct ByteArray* lastHmac = getLastHmac(connection, "user", &dbTime);

	//calculando history
	struct ByteArray* history = generateHistory(hmac, lastHmac);
	char chunk2[2*(history->size) + 1];
	mysql_real_escape_string(connection, chunk2, history->data, history->size);
	snprintf(query, 1000, stat, chunk, chunk2);

	dbTime += DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);

	//update do historico do primeiro registro (usando o lastHmac como dummie, simulando o cached HMAC)
	struct ByteArray* firstECMAC = generateHistory(lastHmac, hmac);

	char chunk3[2*(firstECMAC->size) + 1];
	mysql_real_escape_string(connection, chunk3, firstECMAC->data, firstECMAC->size);
	snprintf(query, 1000, stat, chunk, chunk3);

	stat = "UPDATE benchmark.user SET "
			"history =\'%s\' "
			"WHERE id = %d;";
	snprintf(query, 1000, stat, chunk3, firstId(connection, "user"));
	dbTime += DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);
	free(hmac);
	free(lastHmac);
	free(history);
	free(firstECMAC);

	return dbTime;
}
/*
 * UPDATE
 *
 * UpdateOneRecordWithoutProtection         – Tempo de update de 1 registro, sem uso de HMAC e ECMAC;
 * UpdateOneRecordWithHMAC                  – Tempo de update de 1 registro, calculando HMAC;
 * UpdateOneRecordWithHMACAndECMAC          – Tempo de update de 1 registro, calculando HMAC e calculando ECMAC;
 * UpdateOneRecordWithHMACAndCircularECMAC  – Tempo de update de 1 registro, calculando HMAC e ECMAC circular;
*/

long UpdateOneRecordWithoutProtection(int id)
{
	char query1[1000];
	char query2[1000];
	long dbTime = 0;
//	unsigned int id = 1 + random()%9000;

	char* stat1 = "UPDATE benchmark.user SET "
			"name = \'Marcelo Carlomagno\', "
			"email = \'carlomagno@inf.ufsc.br\',"
			"password = \'OutraSenhaQueTambemNaoVouDizer\'"
			"WHERE "
			"id = %d ;";
	snprintf(query1, 1000 , stat1, id);

	char* stat2 = "SELECT id, name, email, password FROM benchmark.user WHERE id = %d ;";
	snprintf(query2, 1000 , stat2, id);

	dbTime += DBbenchmark("UpdateOneRecordWithoutProtection1", query2);
	mysql_free_result(mysql_store_result(connection));

	dbTime += DBbenchmark("UpdateOneRecordWithoutProtection2", query1);
	mysql_free_result(mysql_store_result(connection));

	return dbTime;
}

long UpdateOneRecordWithHMAC(int id)
{
	long dbTime = 0;
//	unsigned int id = 1 + random()%9000;
	char query2[1000];

	char* stat2 = "SELECT id, name, email, password FROM benchmark.user WHERE id = %d ;";
	snprintf(query2, 1000 , stat2, id);
	dbTime += DBbenchmark("UpdateOneRecordWithHMAC",query2);
	mysql_free_result(mysql_store_result(connection));

	char query[1000];
	char* message = "Marcelo Carlomagnocarlomagno@inf.ufsc.brOutraSenhaQueTambemNaoVouDizer";

	struct ByteArray* hmac = generateHMAC((unsigned char*)message);

	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	char* stat = "UPDATE benchmark.user SET "
			"name = \'Marcelo Carlomagno\', "
			"email = \'carlomagno@inf.ufsc.br\', "
			"password = \'OutraSenhaQueTambemNaoVouDizer\', "
			"hmac = \'%s\' "
			"WHERE "
			"id = %d;";

	snprintf(query, 1000 , stat, chunk, id);
	dbTime += DBbenchmark("UpdateOneRecordWithHMAC", query);
	return dbTime;
}

long UpdateOneRecordWithHMACAndECMAC(int id)
{
	char query[1000];
	long dbTime = 0;

//	unsigned int id = 1 + random()%9000;
	char query2[1000];

	char* stat2 = "SELECT id, name, email, password FROM benchmark.user WHERE id = %d ;";
	snprintf(query2, 1000 , stat2, id);
	dbTime += DBbenchmark("UpdateOneRecordWithHMAC",query2);
	mysql_free_result(mysql_store_result(connection));

	//calculando hmac do registro a ser inserido
	char* message = "Marcelo Carlomagnocarlomagno@inf.ufsc.brOutraSenhaQueTambemNaoVouDizer";

	struct ByteArray* hmac = generateHMAC((unsigned char*)message);

	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	//Lendo HMAC do registro anterior
	struct ByteArray* hmacPredecessor = getHmac(connection, id-1, "user", &dbTime);

	//calculando history
	struct ByteArray* history = generateHistory(hmac, hmacPredecessor);
	char chunk2[2*(history->size) + 1];
	mysql_real_escape_string(connection, chunk2, history->data, history->size);

	char* stat = "UPDATE benchmark.user SET "
			"name = \'Marcelo Carlomagno\', "
			"email = \'carlomagno@inf.ufsc.br\', "
			"password = \'OutraSenhaQueTambemNaoVouDizer\', "
			"hmac = \'%s\', "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";

	snprintf(query, 1000, stat, chunk, chunk2, id);

	dbTime += DBbenchmark("UpdateOneRecordWithHMACAndECMAC", query);

	struct ByteArray* hmacSucessor = getHmac(connection, id+1, "user", &dbTime);
	struct ByteArray* updateHistory = generateHistory(hmac, hmacSucessor);
	char chunk3[2*(updateHistory->size) + 1];
	mysql_real_escape_string(connection, chunk3, updateHistory->data, updateHistory->size);
	snprintf(query, 1000, stat, chunk, chunk3);

	stat = "UPDATE benchmark.user SET "
			"history =\'%s\' "
			"WHERE id = %d;";
	snprintf(query, 1000, stat, chunk3, id+1);

	if (mysql_query(connection, query)) {
		printf("%s\n", query);
	  printf("InsertOneRecordWithHMACAndECMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
	dbTime += DBbenchmark("InsertOneRecordWithHMACAndECMAC", query);

	free(hmac);
	free(hmacPredecessor);
	free(history);
	free(hmacSucessor);
	free(updateHistory);

	return dbTime;
}

long UpdateOneRecordWithHMACAndCircularECMAC(int id)
{
	return UpdateOneRecordWithHMACAndECMAC(id);
}

long UpdateOneRecordWithHMACAndECMACOptimized(int id){

	char query[1000];
	char query2[1000];
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	long dbTime = 0;
//	unsigned int id = 3 + rand()%9000;

	//buscando registros n-1, n e n+1
	char* stat2 = "SELECT id, name, email, password, hmac, history FROM benchmark.user WHERE id IN (%d, %d, %d);";
	snprintf(query2, 1000, stat2, id-1, id, id+1);

	dbTime += DBbenchmark("UpdateOneRecordWithHMACAndECMACOptimized", query2);

	result = mysql_store_result(connection);

	//buscando hmac do registro n-1
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);
	struct ByteArray* hmac2 = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	hmac2->data = (unsigned char*)row[4];
	hmac2->size = (unsigned int)lengths[4];

	//calculando hmac do registro n
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);

	int i;
	char newMessage[1000] = "";
	//strcat(newMessage, "Lucas Perin");
	for (i = 1; i < 4; i++){
		strcat(newMessage, row[i]); /*simulação da leitura da mas colunas*/
	}
	struct ByteArray* hmac3 = generateHMAC((unsigned char*)newMessage);

	//calculando emac do registro n
	struct ByteArray* history3 = generateHistory(hmac2, hmac3);

	//calculando emac do registro n+1
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);
	struct ByteArray* hmac4 = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	hmac4->data = (unsigned char*)row[4];
	hmac4->size = (unsigned int)lengths[4];
	struct ByteArray* history4 = generateHistory(hmac3, hmac4);

	//update do registron n e n+1
	char chunk3[2*(hmac3->size) + 1];
	mysql_real_escape_string(connection, chunk3, hmac3->data, hmac3->size);
	char chunk3h[2*(hmac3->size) + 1];
	mysql_real_escape_string(connection, chunk3h, history3->data, history3->size);

	char chunk4h[2*(hmac3->size) + 1];
	mysql_real_escape_string(connection, chunk4h, history4->data, history4->size);

	mysql_free_result(result);
	char* stat = "UPDATE benchmark.user SET name = CASE id "
	                "WHEN %d THEN \'Lucas Perin\' "
	            "END, "
	            "email = CASE id "
	                "WHEN %d THEN \'carlomagno@inf.ufsc.br\' "
	            "END, "
	            "password = CASE id "
	                "WHEN %d THEN \'OutraSenhaQueTambemNaoVouDizer\' "
	            "END, "
	            "hmac = CASE id "
	                "WHEN %d THEN \'%s\' "
	            "END, "
	            "history = CASE id "
	                "WHEN %d THEN \'%s\' "
	                "WHEN %d THEN \'%s\' "
	            "END "
	            "WHERE id IN (%d, %d);";

	snprintf(query, 1000, stat, id, id, id, id, chunk3, id, chunk3h, id+1, chunk4h, id, id+1);

	dbTime += DBbenchmark("UpdateOneRecordWithHMACAndECMACOptimized", query);

	free(hmac2);
	free(hmac3);
	free(hmac4);
	free(history3);
	free(history4);
	return dbTime;
}

/*
 * DELETE
 *
 * DeleteOneRecordWithoutProtection         – Tempo de delete de 1 registro, sem uso de HMAC e ECMAC;
 * DeleteOneRecordWithHMAC                  – Tempo de delete de 1 registro, calculando HMAC, sem ECMAC;
 * DeleteOneRecordWithHMACAndECMAC          – Tempo de delete de 1 registro, calculando HMAC, calculando ECMAC;
 * DeleteOneRecordWithHMACAndCircularECMAC  – Tempo de delete de 1 registro, calculando HMAC e ECMAC circular;
*/
long DeleteOneRecordWithoutProtection(int id)
{
	char query[1000];
	char* stat = "DELETE FROM benchmark.user WHERE id = %d";
	snprintf(query, 1000, stat, id);

	return DBbenchmark("DeleteOneRecordWithoutProtection", query);
}

long DeleteOneRecordWithHMAC(int id)
{
	return DeleteOneRecordWithoutProtection(id);
}

long DeleteOneRecordWithHMACAndECMAC(int id)
{
	char query[1000];
	char query2[1000];
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	long dbTime = 0;

	char* stat = "DELETE FROM benchmark.user WHERE id = %d";
	snprintf(query, 1000, stat, id);

	dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query);

//	//buscando registros n-1, n e n+1
//	char* stat2 = "SELECT hmac FROM benchmark.user WHERE id IN (%d, %d);";
//	snprintf(query2, 1000, stat2, id-1, id+1);
//
//	dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query2);
//
//
//	result = mysql_store_result(connection);
//
//	//buscando hmac do registro n-1
//	row = mysql_fetch_row(result);
//	lengths = mysql_fetch_lengths(result);
//	struct ByteArray* hmacPredecessor = (struct ByteArray*) malloc(sizeof(struct ByteArray));
//	hmacPredecessor->data = (unsigned char*)row[0];
//	hmacPredecessor->size = (unsigned int)lengths[0];
//
//	//buscando hmac do registro n+1
//	row = mysql_fetch_row(result);
//	lengths = mysql_fetch_lengths(result);
//	struct ByteArray* hmacSuccessor = (struct ByteArray*) malloc(sizeof(struct ByteArray));
//	hmacSuccessor->data = (unsigned char*)row[0];
//	hmacSuccessor->size = (unsigned int)lengths[0];

	struct ByteArray* hmacPredecessor = getHmac(connection, id-1, "user", &dbTime);
	struct ByteArray* hmacSuccessor = getHmac(connection, id+1, "user", &dbTime);

	struct ByteArray* history = generateHistory(hmacPredecessor, hmacSuccessor);

	char chunk[2*(history->size) + 1];
	mysql_real_escape_string(connection, chunk, history->data, history->size);

	stat = "UPDATE benchmark.user SET "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";

	snprintf(query, 1000, stat, chunk, id+1);

	dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query);

	free(hmacPredecessor);
	free(hmacSuccessor);
	free(history);

	return dbTime;
}

long DeleteOneRecordWithHMACAndCircularECMAC(int id)
{
	return DeleteOneRecordWithHMACAndECMAC(id);
}

long DeleteOneRecordWithHMACAndECMACOptimized(int id)
{

	//TODOOO!!!
	char query[1000];
		char query2[1000];
		MYSQL_RES *result;
		MYSQL_ROW row;
		unsigned long *lengths;
		long dbTime = 0;

		char* stat = "DELETE FROM benchmark.user WHERE id = %d";
		snprintf(query, 1000, stat, id);

		dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query);

		//buscando registros n-1, n e n+1
		char* stat2 = "SELECT hmac FROM benchmark.user WHERE id IN (%d, %d);";
		snprintf(query2, 1000, stat2, id-1, id+1);

		dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query2);

		result = mysql_store_result(connection);

		//buscando hmac do registro n-1
		row = mysql_fetch_row(result);
		lengths = mysql_fetch_lengths(result);
		struct ByteArray* hmacPredecessor = (struct ByteArray*) malloc(sizeof(struct ByteArray));
		hmacPredecessor->data = (unsigned char*)row[0];
		hmacPredecessor->size = (unsigned int)lengths[0];

		//buscando hmac do registro n+1
		row = mysql_fetch_row(result);
		lengths = mysql_fetch_lengths(result);
		struct ByteArray* hmacSuccessor = (struct ByteArray*) malloc(sizeof(struct ByteArray));
		hmacSuccessor->data = (unsigned char*)row[0];
		hmacSuccessor->size = (unsigned int)lengths[0];

	//	struct ByteArray* hmacPredecessor = getHmac(connection, id-1, "user", &dbTime);
	//	struct ByteArray* hmacSuccessor = getHmac(connection, id+1, "user", &dbTime);

		struct ByteArray* history = generateHistory(hmacPredecessor, hmacSuccessor);

		char chunk[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk, history->data, history->size);

		stat = "UPDATE benchmark.user SET "
				"history = \'%s\' "
				"WHERE "
				"id = %d;";

		snprintf(query, 1000, stat, chunk, id+1);

		dbTime += DBbenchmark("DeleteOneRecordWithHMACAndECMAC", query);

		free(hmacPredecessor);
		free(hmacSuccessor);
		free(history);

		return dbTime;
}

/*
 * SELECT
 *
 * SelectOneRecordWithoutProtection         – Tempo de select de 1 registro, sem verificacao de HMAC e ECMAC;
 * SelectOneRecordWithHMAC                  – Tempo de select de 1 registro, verificando HMAC, sem ECMAC;
 * SelectOneRecordWithHMACAndECMAC          – Tempo de select de 1 registro, verificando HMAC, verificando ECMAC;
 * SelectOneRecordWithHMACAndCircularECMAC  – Tempo de select de 1 registro, calculando HMAC e ECMAC circular;
*/

long SelectOneRecordWithoutProtection()
{
	long dbTime = 0;
	dbTime = DBbenchmark("SelectOneRecordWithoutProtection","SELECT id, name, email, password FROM benchmark.user WHERE id = 3");
	mysql_free_result(mysql_store_result(connection));
	return dbTime;
}

long SelectOneRecordWithHMAC()
{
	long dbTime = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	dbTime = DBbenchmark("SelectOneRecordWithHMAC", "SELECT id, name, email, password, hmac FROM benchmark.user WHERE id = 3");
	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	int i = 1;
	char message[1000] = "";
	for (; i < 4; i++){
		strcat(message, row[i]);
	}
	int valid = validadeHMAC(message, (unsigned char*) row[4]);
	mysql_free_result(result);
	return dbTime;
}

long SelectOneRecordWithHMACAndECMAC(int id)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	long dbTime = 0;

//	int id = 3;
	char query[1000];
	char* stat = "SELECT id, name, email, password, hmac, history FROM benchmark.user WHERE id = %d;";

	snprintf(query, 1000, stat, id);

	dbTime += DBbenchmark("SelectOneRecordWithHMACAndECMAC", query);

	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);


	int i = 1;
	char message[1000] = "";
	for (; i < 4; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* hmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* history = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmac->data = (unsigned char*)row[4];
	hmac->size = (unsigned int)lengths[4];

	history->data = (unsigned char*)row[5];
	history->size = (unsigned int)lengths[5];

	validadeHMAC(message, (unsigned char*) row[4]);//AQUI
	mysql_free_result(result);

	struct ByteArray* hmacPredecessor = getHmac(connection, id-1, "user", &dbTime);

	validateHistory(history, hmac, hmacPredecessor);

	free(hmac);
	free(history);
	free(hmacPredecessor);

	return dbTime;
}

long SelectOneRecordWithHMACAndCircularECMAC(int id)
{
	return SelectOneRecordWithHMACAndECMAC(id);
}

long SelectOneRecordWithHMACAndECMACOptimized(int id){
	printf("iteracao %d\n", id);
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	long dbTime = 0;

//	int id = 3;
	char query[1000];
	char* stat = "SELECT id, name, email, password, hmac, history FROM benchmark.user WHERE id IN ( %d, %d );";
	snprintf(query, 1000, stat, id-1, id);

	dbTime += DBbenchmark("SelectOneRecordWithHMACAndECMAC", query);

	result = mysql_store_result(connection);

	//Hmac do registro n-1
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);
	struct ByteArray* hmac1 = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	hmac1->data = (unsigned char*)row[4];
	hmac1->size = (unsigned int)mysql_fetch_lengths(result)[4];


	//Hmac do registro n
	row = mysql_fetch_row(result);
	lengths = mysql_fetch_lengths(result);


	int i = 1;
	char message[1000] = "";
	for (; i < 4; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* hmac2 = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* history = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmac2->data = (unsigned char*)row[4];
	hmac2->size = (unsigned int)lengths[4];

	history->data = (unsigned char*)row[5];
	history->size = (unsigned int)lengths[5];

	mysql_free_result(result);

	validadeHMAC(message, (unsigned char*) row[4]);
	validateHistory(history, hmac1, hmac2);

	free(hmac1);
	free(hmac2);
	free(history);

	return dbTime;

}

/* SELECT
 * S100R    – Tempo de select de 100 registros, sem verificacao de HMAC e ECMAC;
 * S100RH   – Tempo de select de 100 registros, verificando HMAC, sem ECMAC;
 * S100RHH  – Tempo de select de 100 registros, verificando HMAC, verificando ECMAC;
*/

void S100R()
{
	if (mysql_query(connection, "SELECT id, name, email, password, text FROM benchmark.user100")) {
	  printf("S100R::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

void S100RH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac FROM benchmark.user100 ORDER BY ID")) {
	  printf("S100RH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs
	result = mysql_store_result(connection);
	int cont = 1;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		int valid = validadeHMAC(message, (unsigned char*) row[5]);
//		printf("%d validação do HMAC: %d\n",cont, valid);
		cont++;
		free(message);
	}

}

void S100RHH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac, history FROM benchmark.user100 ORDER BY ID")) {
	  printf("S100RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs e Historicos
	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	char* message = calloc(12000, sizeof(char));
	lengths = mysql_fetch_lengths(result);

	int i = 1;
	for (; i < 5; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* firstHistory = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmacAnterior->data = (unsigned char*)row[5];
	hmacAnterior->size = (unsigned int)lengths[5];
	firstHmac->data = (unsigned char*)row[5];
	firstHmac->size = (unsigned int)lengths[5];

	int hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
	free(message);
//	printf("primeiro Hmac = %d\n", hmacValid);


	firstHistory->data = (unsigned char*)row[6];
	firstHistory->size = (unsigned int)lengths[6];


	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		lengths = mysql_fetch_lengths(result);
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}


		hmacAtual->data = (unsigned char*)row[5];
		hmacAtual->size = (unsigned int)lengths[5];

		hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
		free(message);
//		printf("%d Hmac = %d\n", count, hmacValid);

		int historyValid = validateHistory(firstHistory, hmacAtual, hmacAnterior);

//		printf("%d History = %d\n", count -1, historyValid);;

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		firstHistory->data = (unsigned char*)row[6];
		firstHistory->size = (unsigned int)lengths[6];
		count ++;
	}

	int historyValid = validateHistory(firstHistory, hmacAnterior, firstHmac);

//	printf("%d History = %d\n", count - 1, historyValid);;

	free(firstHmac);
	free(hmacAnterior);
	free(firstHistory);
	free(hmacAtual);

}
/* SELECT
 * V1kH     – Tempo de verificacao de todos os registros do banco, verificando HMAC, sem ECMAC; (1.000 registros)
 * V1mH     – Tempo de verificacao de todos os registros do banco, verificando HMAC, sem ECMAC; (1.000.000 registros)
 * V1kHH    – Tempo de verificacao de todos os registros do banco, verificando HMAC, verificando ECMAC; (1.000 registros)
 * V1mHH    – Tempo de verificacao de todos os registros do banco, verificando HMAC, verificando ECMAC; (1.000.000 registros)
*/

void V1kH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac FROM benchmark.user1k ORDER BY ID")) {
	  printf("V1kH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs
	result = mysql_store_result(connection);
	int cont = 1;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		int valid = validadeHMAC(message, (unsigned char*) row[5]);
//		printf("%d validação do HMAC: %d\n",cont, valid);
		free(message);
		cont++;
	}

}

void V1mH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac FROM benchmark.user1kk ORDER BY ID")) {
	  printf("SelectOneRecordWithoutProtection::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs
	result = mysql_store_result(connection);
	int cont = 1;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		int valid = validadeHMAC(message, (unsigned char*) row[5]);
//		printf("%d validação do HMAC: %d\n",cont, valid);
		free(message);
		cont++;
	}

}

void V1kHH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac, history FROM benchmark.user1k ORDER BY ID")) {
	  printf("S100RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs e Historicos
	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	char* message = calloc(12000, sizeof(char));
	lengths = mysql_fetch_lengths(result);

	int i = 1;
	for (; i < 5; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* firstHistory = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmacAnterior->data = (unsigned char*)row[5];
	hmacAnterior->size = (unsigned int)lengths[5];
	firstHmac->data = (unsigned char*)row[5];
	firstHmac->size = (unsigned int)lengths[5];

	int hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
	free(message);
//	printf("primeiro Hmac = %d\n", hmacValid);


	firstHistory->data = (unsigned char*)row[6];
	firstHistory->size = (unsigned int)lengths[6];


	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		lengths = mysql_fetch_lengths(result);
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}


		hmacAtual->data = (unsigned char*)row[5];
		hmacAtual->size = (unsigned int)lengths[5];

		hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
		free(message);
//		printf("%d Hmac = %d\n", count, hmacValid);

		int historyValid = validateHistory(firstHistory, hmacAtual, hmacAnterior);

//		printf("%d History = %d\n", count -1, historyValid);;

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		firstHistory->data = (unsigned char*)row[6];
		firstHistory->size = (unsigned int)lengths[6];
		count ++;
	}

	int historyValid = validateHistory(firstHistory, hmacAnterior, firstHmac);

//	printf("%d History = %d\n", count - 1, historyValid);;

	free(firstHmac);
	free(hmacAnterior);
	free(firstHistory);
	free(hmacAtual);
}

void V1mHH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;

	if (mysql_query(connection, "SELECT id, name, email, password, text, hmac, history FROM benchmark.user1kk ORDER BY ID")) {
	  printf("S100RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs e Historicos
	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	char* message = calloc(12000, sizeof(char));
	lengths = mysql_fetch_lengths(result);

	int i = 1;
	for (; i < 5; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* firstHistory = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmacAnterior->data = (unsigned char*)row[5];
	hmacAnterior->size = (unsigned int)lengths[5];
	firstHmac->data = (unsigned char*)row[5];
	firstHmac->size = (unsigned int)lengths[5];

	int hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
	free(message);
//	printf("primeiro Hmac = %d\n", hmacValid);

	firstHistory->data = (unsigned char*)row[6];
	firstHistory->size = (unsigned int)lengths[6];

	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		lengths = mysql_fetch_lengths(result);
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}

		hmacAtual->data = (unsigned char*)row[5];
		hmacAtual->size = (unsigned int)lengths[5];

		hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
		free(message);
//		printf("%d Hmac = %d\n", count, hmacValid);

		int historyValid = validateHistory(firstHistory, hmacAtual, hmacAnterior);

//		printf("%d History = %d\n", count -1, historyValid);;

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		firstHistory->data = (unsigned char*)row[6];
		firstHistory->size = (unsigned int)lengths[6];
		count ++;
	}

	int historyValid = validateHistory(firstHistory, hmacAnterior, firstHmac);

//	printf("%d History = %d\n", count - 1, historyValid);;

	free(firstHmac);
	free(hmacAnterior);
	free(firstHistory);
	free(hmacAtual);
}

void UxRH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (mysql_query(connection, "SELECT id, name, email, password, text FROM benchmark.user100 ORDER BY ID")) {
	  printf("S100RH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	// Validar todos os 100 Hmacs
	result = mysql_store_result(connection);
	int count = 1;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		struct ByteArray* hmac = generateHMAC(message);
		char chunk[2*(hmac->size) + 1];
		mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

		char* stat = "UPDATE benchmark.user100 SET "
				"hmac = \'%s\' "
				"WHERE "
				"id = %d;";
		char query[1000];
		snprintf(query, 1000 , stat, chunk, count);


		if (mysql_query(connection, query)) {
		  printf("UpdateOneRecordWithHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
		}

		count++;
		free(message);
	}

}

void UxRHH()
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (mysql_query(connection, "SELECT id, name, email, password, text FROM benchmark.user100 ORDER BY ID")) {
	  printf("S100RH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	char* stat = "UPDATE benchmark.user100 SET "
			"hmac = \'%s\' "
			"WHERE "
			"id = %d;";

	char* stat2 = "UPDATE benchmark.user100 SET "
			"hmac = \'%s\', "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";
	char* stat3 = "UPDATE benchmark.user100 SET "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";


	result = mysql_store_result(connection);
	struct ByteArray* hmacAnterior;
	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual;
	struct ByteArray* history;

	row = mysql_fetch_row(result);
	int i = 1;
	char* message = calloc(12000, sizeof(char));
	for (; i < 5; i++){
		strcat(message, row[i]);
	}
	hmacAnterior = generateHMAC((unsigned char*)message);
	char chunk[2*(hmacAnterior->size) + 1];
	mysql_real_escape_string(connection, chunk, hmacAnterior->data, hmacAnterior->size);
	char query[1000];
	snprintf(query, 1000 , stat, chunk, 1);
	firstHmac->data = hmacAnterior->data;
	firstHmac->size = hmacAnterior->size;

	if (mysql_query(connection, query)) {
	  printf("UpdateOneRecordWithHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}


	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		hmacAtual = generateHMAC(message);
		history = generateHistory(hmacAnterior, hmacAtual);

		char chunk1[2*(hmacAtual->size) + 1];
		mysql_real_escape_string(connection, chunk1, hmacAtual->data, hmacAtual->size);
		char chunk3[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk3, history->data, history->size);

		snprintf(query, 1000 , stat2, chunk1, chunk3, count);

		if (mysql_query(connection, query)) {
		  printf("UpdateOneRecordWithHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
		}

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		count++;
		free(message);
		free(hmacAtual);
		free(history);
	}
}

void insertHMAC(MYSQL* connection, char* tableName)
{
        MYSQL_RES *result;
        MYSQL_ROW row;

        char query[1000];
        char* stat = "SELECT id, name, email, password, text FROM benchmark.%s ORDER BY ID";
        snprintf(query, 1000 , stat, tableName);

        if (mysql_query(connection, query)) {
          printf("insertHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
        }

        stat = "UPDATE benchmark.%s SET "
                        "hmac = \'%s\' "
                        "WHERE "
                        "id = %d;";

        result = mysql_store_result(connection);
        struct ByteArray* hmacAtual;

        while ((row = mysql_fetch_row(result)))
        {
                int i = 1;
                char* message = calloc(12000, sizeof(char));
                for (; i < 5; i++){
                        strcat(message, row[i]);
                }
                hmacAtual = generateHMAC(message);

                char chunk1[2*(hmacAtual->size) + 1];
                mysql_real_escape_string(connection, chunk1, hmacAtual->data, hmacAtual->size);

                snprintf(query, 1000 , stat, tableName, chunk1, row[0]);

                if (mysql_query(connection, query)) {
                        printf("insertHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
                }

                free(message);
                free(hmacAtual);
        }
        mysql_free_result(result);
        free(result);
}

void insertECMAC(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[1000];
	char* stat = "SELECT id, name, email, password, text FROM benchmark.%s ORDER BY ID";
	snprintf(query, 1000 , stat, tableName);

	if (mysql_query(connection, query)) {
	  printf("S100RH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	char* stat2 = "UPDATE benchmark.%s SET "
			"hmac = \'%s\', "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";

	result = mysql_store_result(connection);
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual;
	struct ByteArray* history;

	int count = 0;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		hmacAtual = generateHMAC(message);

		//Como verificar se hmacAnterior ja foi "preenchido"?
		if (count > 0) {
			history = generateHistory(hmacAnterior, hmacAtual);
		} else {
			history = generateHistory(hmacAtual, hmacAtual);
		}

		char chunk1[2*(hmacAtual->size) + 1];
		mysql_real_escape_string(connection, chunk1, hmacAtual->data, hmacAtual->size);
		char chunk3[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk3, history->data, history->size);

		snprintf(query, 1000 , stat2, tableName, chunk1, chunk3, row[1]);

		if (mysql_query(connection, query)) {
		  printf("insertECMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
		}

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		free(message);

		count++;
	}

	free(hmacAtual);
	free(hmacAnterior);
	free(history);
    mysql_free_result(result);
}

void insertCircularECMAC(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[1000];
	char* stat = "SELECT id, name, email, password, text FROM benchmark.%s ORDER BY ID";
	snprintf(query, 1000 , stat, tableName);

	if (mysql_query(connection, query)) {
	  printf("S100RH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	char* stat2 = "UPDATE benchmark.%s SET "
			"hmac = \'%s\', "
			"history = \'%s\' "
			"WHERE "
			"id = %d;";

	result = mysql_store_result(connection);
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual;
	struct ByteArray* history;

	int count = 0;
	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}
		hmacAtual = generateHMAC(message);

		//Como verificar se hmacAnterior ja foi "preenchido"?
		if (count > 0) {
			history = generateHistory(hmacAnterior, hmacAtual);

			char chunk1[2*(hmacAtual->size) + 1];
			mysql_real_escape_string(connection, chunk1, hmacAtual->data, hmacAtual->size);
			char chunk3[2*(history->size) + 1];
			mysql_real_escape_string(connection, chunk3, history->data, history->size);

			snprintf(query, 1000 , stat2, tableName, chunk1, chunk3, row[1]);

			if (mysql_query(connection, query)) {
			  printf("insertECMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
			}

		} else {
			firstHmac->data = hmacAtual->data;
			firstHmac->size = hmacAtual->size;
		}

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		free(message);

		count++;
	}

	history = generateHistory(firstHmac, hmacAtual);

	char chunk1[2*(hmacAtual->size) + 1];
	mysql_real_escape_string(connection, chunk1, hmacAtual->data, hmacAtual->size);
	char chunk3[2*(history->size) + 1];
	mysql_real_escape_string(connection, chunk3, history->data, history->size);

	snprintf(query, 1000 , stat2, tableName, chunk1, chunk3, firstId(connection, tableName));

	free(hmacAtual);
	free(hmacAnterior);
	free(history);
    mysql_free_result(result);
}

void validateHMAC(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[1000];
	char* stat = "SELECT id, name, email, password, text, hmac FROM benchmark.%s ORDER BY ID";
	snprintf(query, 1000 , stat, tableName);

	if (mysql_query(connection, query)) {
		printf("validateHMAC::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);

	while ((row = mysql_fetch_row(result)))
	{
		int i = 1;
		char* message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}

		int valid = validadeHMAC(message, (unsigned char*) row[5]);

		free(message);
	}
    mysql_free_result(result);
}

void validateECMAC(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;

	char query[1000];
	char* stat = "SELECT id, name, email, password, text, hmac, history FROM benchmark.%s ORDER BY ID";
	snprintf(query, 1000 , stat, tableName);

	if (mysql_query(connection, query)) {
		printf("S100RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	char* message = calloc(12000, sizeof(char));
	lengths = mysql_fetch_lengths(result);

	int i = 1;
	for (; i < 5; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* history = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmacAnterior->data = (unsigned char*)row[5];
	hmacAnterior->size = (unsigned int)lengths[5];

	hmacAtual->data = (unsigned char*)row[5];
	hmacAtual->size = (unsigned int)lengths[5];

	history->data = (unsigned char*)row[6];
	history->size = (unsigned int)lengths[6];

	int hmacValid = validadeHMAC(message, hmacAnterior->data);
	int historyValid = validateHistory(history, hmacAtual, hmacAnterior);

	free(message);

	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		lengths = mysql_fetch_lengths(result);
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}

		hmacAtual->data = (unsigned char*)row[5];
		hmacAtual->size = (unsigned int)lengths[5];

		hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
		free(message);

		history->data = (unsigned char*)row[6];
		history->size = (unsigned int)lengths[6];

		int historyValid = validateHistory(history, hmacAtual, hmacAnterior);

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		count ++;
	}

	free(hmacAnterior);
	free(hmacAtual);
	free(history);
    mysql_free_result(result);
}

void validateCircularECMAC(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;

	char query[1000];
	char* stat = "SELECT id, name, email, password, text, hmac, history FROM benchmark.%s ORDER BY ID";
	snprintf(query, 1000 , stat, tableName);

	if (mysql_query(connection, query)) {
		printf("S100RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	char* message = calloc(12000, sizeof(char));
	lengths = mysql_fetch_lengths(result);

	int i = 1;
	for (; i < 5; i++){
		strcat(message, row[i]);
	}

	struct ByteArray* firstHmac = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAnterior = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* hmacAtual = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* firstHistory = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	struct ByteArray* history = (struct ByteArray*) malloc(sizeof(struct ByteArray));

	hmacAnterior->data = (unsigned char*)row[5];
	hmacAnterior->size = (unsigned int)lengths[5];
	firstHmac->data = (unsigned char*)row[5];
	firstHmac->size = (unsigned int)lengths[5];

	int hmacValid = validadeHMAC(message, hmacAnterior->data);
	free(message);

	firstHistory->data = (unsigned char*)row[6];
	firstHistory->size = (unsigned int)lengths[6];

	int count = 2;
	while ((row = mysql_fetch_row(result)))
	{
		lengths = mysql_fetch_lengths(result);
		int i = 1;
		message = calloc(12000, sizeof(char));
		for (; i < 5; i++){
			strcat(message, row[i]);
		}

		hmacAtual->data = (unsigned char*)row[5];
		hmacAtual->size = (unsigned int)lengths[5];

		hmacValid = validadeHMAC(message, (unsigned char*) row[5]);
		free(message);

		history->data = (unsigned char*)row[6];
		history->size = (unsigned int)lengths[6];

		int historyValid = validateHistory(history, hmacAtual, hmacAnterior);

		hmacAnterior->data = hmacAtual->data;
		hmacAnterior->size = hmacAtual->size;

		count ++;
	}

	int historyValid = validateHistory(firstHistory, hmacAnterior, firstHmac);

	free(firstHmac);
	free(hmacAnterior);
	free(firstHistory);
	free(hmacAtual);
	free(history);
    mysql_free_result(result);
}

#endif
