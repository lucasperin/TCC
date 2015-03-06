#ifndef BENCHMARKFUNCTIONS_H
#define BENCHMARKFUNCTIONS_H

#include "TestCases.h"
#include "math.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */

/* Structs */

typedef struct Result
{
long elapsedTime;
long dbElapsedTime;
}Result;

typedef struct Sample
{
	Result* resultList;
	int resultListSize;
}Sample;

void releaseSample(Sample* sample){
	free(sample->resultList);
	free(sample);
}

typedef struct DigestedSample{
	float averageElapsedTime;
	float averageDbElapsedTime;

	float varianceElapsedTime;
	float varianceDbElapsedTime;

	float standardElapsedTimeDeviation;
	float standardDbElapsedTimeDeviation;
}DigestedSample;

/* BENCHMARK FUNCTIONS */
/*
 * Calculates interval value of two 'timeval' structures.
 */
long timevaldiff(struct timeval *startTime, struct timeval *finishTime)
{
 long msec;
 msec = (finishTime->tv_sec - startTime->tv_sec) * 1000000;
 msec += (finishTime->tv_usec - startTime->tv_usec);
 return msec;
}

int sort(const void *x, const void *y) {
  return (*(int*)x - *(int*)y);
}

/*
 * Calls the parameter function returning the elapsed time from its start to finish.
 */
long benchmark( void(*func)(void) )
{
	struct timeval startTime;
	struct timeval finishTime;

	gettimeofday(&startTime, 0);
	(*func)();
	gettimeofday(&finishTime, 0);

	return timevaldiff(&startTime, &finishTime);
}

Result benchmark2( long(*func)(void))
{
	struct timeval startTime;
	struct timeval finishTime;
	Result ret;

	gettimeofday(&startTime, 0);
	ret.dbElapsedTime = (*func)();
	gettimeofday(&finishTime, 0);
	ret.elapsedTime = timevaldiff(&startTime, &finishTime);

	return ret;
}

Result benchmark3( long(*func)(int), int i )
{
	struct timeval startTime;
	struct timeval finishTime;
	Result ret;

	gettimeofday(&startTime, 0);
	ret.dbElapsedTime = (*func)(i);
	gettimeofday(&finishTime, 0);
	ret.elapsedTime = timevaldiff(&startTime, &finishTime);

	return ret;
}

long benchmark_load( void(*func)(MYSQL*, char*), MYSQL* con, char* table)
{
	struct timeval startTime;
	struct timeval finishTime;

	gettimeofday(&startTime, 0);
	(*func)(con, table);
	gettimeofday(&finishTime, 0);

	return timevaldiff(&startTime, &finishTime);
}

void printResult(float msec, char* function)
{
	printf("%.0fus - %s.\n", msec, function);
}

void printStructResult(Result result){
	printResult((float)result.elapsedTime, "Total");
	printResult((float)result.dbElapsedTime, "DB");
}

void printSample(Sample* sample, char* function){

	FILE *file;
	file = fopen("results.txt","a+"); /* apend file (add text to a file or create a file if it does not exist.*/
	int i;
	fprintf(file, "%s.\n", function);
	for(i = 0; i < sample->resultListSize; i++){
			fprintf(file, "%.0f ", (float)sample->resultList[i].dbElapsedTime); //DB
			fprintf(file, "%.0f ", (float)sample->resultList[i].elapsedTime - (float)sample->resultList[i].dbElapsedTime); //Client
			fprintf(file, "%.0f\n", (float)sample->resultList[i].elapsedTime); //Total
	}
	fclose(file); /*done!*/
}

void printResultSec(float sec, char* function)
{
	printf("%fs - %s.\n", sec, function);
}

void printResultMSec(float sec, char* function)
{
	printf("%fms - %s.\n", sec, function);
}

void printDigestedSample(DigestedSample digestedSample, char* function)
{
	printf(RED "%s\n" GREEN
			"Tempo médio decorrido total    : %.2fus\n"
			"Tempo médio decorrido SGBD     : %.2fus\n"
//			"Variância do tempo total       : %.2fus\n"\TODO{Citar artigo do Anderson, comentar sobre soluções circular e CACHED}
//			"Variância do tempo do SGBD     : %.2fus\n"
//			"Desvio Padrão do tempo total   : %.2fus\n"
//			"Desvio Padrão do tempo do SGBD : %.2fus\n"
			RESET
			,function
			,digestedSample.averageElapsedTime
			,digestedSample.averageDbElapsedTime
//			,digestedSample.varianceElapsedTime
//			,digestedSample.varianceDbElapsedTime
//			,digestedSample.standardElapsedTimeDeviation
//			,digestedSample.standardDbElapsedTimeDeviation
			);
}

DigestedSample digestSample(Sample* sample){
	DigestedSample digestedSample;

	digestedSample.averageElapsedTime = 0;
	digestedSample.averageDbElapsedTime = 0;

	int i;
	for(i = 0; i < sample->resultListSize; i++)
	{
		digestedSample.averageElapsedTime += sample->resultList[i].elapsedTime;
		digestedSample.averageDbElapsedTime += sample->resultList[i].dbElapsedTime;
	}

	digestedSample.averageElapsedTime /= (float)sample->resultListSize;
	digestedSample.averageDbElapsedTime /= (float)sample->resultListSize;


	digestedSample.varianceElapsedTime = 0;
	digestedSample.varianceDbElapsedTime = 0;

	for(i = 0; i < sample->resultListSize; i++)
	{
		float aux = (float)sample->resultList[i].elapsedTime;
		aux -= digestedSample.averageElapsedTime;
		digestedSample.varianceElapsedTime += aux*aux;

		aux = (float)sample->resultList[i].dbElapsedTime;
		aux -= digestedSample.averageDbElapsedTime;
		digestedSample.varianceDbElapsedTime += aux*aux;
	}

	digestedSample.varianceElapsedTime /= (float)sample->resultListSize;
	digestedSample.varianceDbElapsedTime /= (float)sample->resultListSize;

	digestedSample.standardElapsedTimeDeviation = sqrtf(digestedSample.varianceElapsedTime);
	digestedSample.standardDbElapsedTimeDeviation = sqrtf(digestedSample.varianceDbElapsedTime);

	return digestedSample;
}


void setUpEnvironment(char* tableName){
connection = connectToMysql();
mysql_query(connection, "SET AUTOCOMMIT=0");
mysql_query(connection, "START TRANSACTION");
createTable(connection, tableName);
//Sets up the environment
int j=0;
for (j=0; j<5000; j++) {
	InsertOneRecordWithHMACAndECMAC();
}
mysql_query(connection, "COMMIT");
mysql_close(connection);
}

void deleteEnvironment(char* tableName){
	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");
	dropTable(connection, tableName);
	mysql_query(connection, "COMMIT");
	mysql_close(connection);
}


double run_load_test(void(*function)(MYSQL*, char*), char* tableName, int load)
{
	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");

	createTable(connection, tableName);
	int i;
	for (i = 0; i < load; i++) {
		insertRandomRecord(connection, tableName);
	}
	mysql_query(connection, "COMMIT");
	mysql_close(connection);

	double retorno = 0;

	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");
	retorno = (float)benchmark_load(function, connection, tableName);
	mysql_query(connection, "COMMIT");
	mysql_close(connection);

	connection = connectToMysql();
	dropTable(connection, tableName);
	mysql_close(connection);

	return retorno/1000;
}

double run_load_test_cenario3(void(*function)(MYSQL*, char*), char* tableName, int load)
{
	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");

	createTable(connection, tableName);
	int i;
	for (i = 0; i < load; i++) {
		insertRandomRecordHH(connection, tableName);
	}
	mysql_query(connection, "COMMIT");
	mysql_close(connection);

	double retorno = 0;

	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");
	retorno = (float)benchmark_load(function, connection, tableName);
	mysql_query(connection, "COMMIT");
	mysql_close(connection);

	connection = connectToMysql();
	dropTable(connection, tableName);
	mysql_close(connection);

	return retorno/1000;
}


Sample* run_sample_test(long(*function)(void), char* tableName)
{
	int i = 0;
	int sampleSize = 300;

	Sample* sample = (Sample*) malloc(sizeof(Sample));
	sample->resultListSize = sampleSize;
	sample->resultList = (Result*) malloc(sizeof(Result)*sampleSize);

		setUpEnvironment(tableName);
		connection = connectToMysql();
		mysql_query(connection, "SET AUTOCOMMIT=0");
		mysql_query(connection, "START TRANSACTION");
	for (i=0; i<sampleSize; i++){
		sample->resultList[i] = benchmark2(function);
	}
		mysql_query(connection, "COMMIT");
		mysql_close(connection);
		deleteEnvironment(tableName);

	return sample;
}

Sample* run_sample_test2(long(*function)(int), char* tableName)
{
	int i = 0;
	int sampleSize = 300;

	Sample* sample = (Sample*) malloc(sizeof(Sample));
	sample->resultListSize = sampleSize;
	sample->resultList = (Result*) malloc(sizeof(Result)*sampleSize);

		setUpEnvironment(tableName);
		connection = connectToMysql();
		mysql_query(connection, "SET AUTOCOMMIT=0");
		mysql_query(connection, "START TRANSACTION");
	for (i=0; i<sampleSize; i++){
		sample->resultList[i] = benchmark3(function, (i+1)*5);
	}
		mysql_query(connection, "COMMIT");
		mysql_close(connection);
		deleteEnvironment(tableName);

	return sample;
}

void run_INSERT()
{
		Sample *sample1, *sample2, *sample3, *sample4, *sample5;
		printf(YELLOW"------ Executing tests of one single INSERT ------\n"RESET);
		sample1 = run_sample_test(&InsertOneRecordWithoutProtection, "user");
		sample2 = run_sample_test(&InsertOneRecordWithHMAC, "user");
		sample3 = run_sample_test(&InsertOneRecordWithHMACAndECMAC, "user");
		sample4 = run_sample_test(&InsertOneRecordWithHMACAndCircularECMAC, "user");
		sample5 = run_sample_test(&InsertOneRecordWithHMACAndCachedCircularECMAC, "user");

		printSample(sample1, "Without Protection");
		printDigestedSample(digestSample(sample1), "Without Protection");

		printSample(sample2, "With HMAC");
		printDigestedSample(digestSample(sample2), "With HMAC");

		printSample(sample3, "With HMAC + ECMAC");
		printDigestedSample(digestSample(sample3), "With HMAC + ECMAC");

		printSample(sample4, "With HMAC + Circular ECMAC");
		printDigestedSample(digestSample(sample4), "With HMAC + Circular ECMAC");

		printSample(sample5, "With HMAC + Circular CACHED ECMAC");
		printDigestedSample(digestSample(sample5), "With HMAC + Circular CACHED ECMAC");

		printf(YELLOW"--------------------------------------------------\n"RESET);

		releaseSample(sample1);
		releaseSample(sample2);
		releaseSample(sample3);
		releaseSample(sample4);
		releaseSample(sample5);
}

void run_UPDATE()
{
	Sample *sample1, *sample2, *sample3, *sample4, *sample5;
	printf(YELLOW"------ Executing tests of one single UPDATE ------\n"RESET);

	sample1 = run_sample_test2(&UpdateOneRecordWithoutProtection, "user");
	sample2 = run_sample_test2(&UpdateOneRecordWithHMAC, "user");
	sample3 = run_sample_test2(&UpdateOneRecordWithHMACAndECMAC, "user");
	sample4 = run_sample_test2(&UpdateOneRecordWithHMACAndCircularECMAC, "user");
	sample5 = run_sample_test2(&UpdateOneRecordWithHMACAndECMACOptimized, "user");

	printSample(sample1, "Without Protection");
	printDigestedSample(digestSample(sample1), "Without Protection");

	printSample(sample2, "With HMAC");
	printDigestedSample(digestSample(sample2), "With HMAC");

	printSample(sample3, "With HMAC + ECMAC");
	printDigestedSample(digestSample(sample3), "With HMAC + ECMAC");

	printSample(sample4, "With HMAC + Circular ECMAC");
	printDigestedSample(digestSample(sample4), "With HMAC + Circular ECMAC");

	printSample(sample5, "With HMAC + ECMAC [OPTMIZED]");
	printDigestedSample(digestSample(sample5), "With HMAC + ECMAC [OPTIMIZED]");

	printf(YELLOW"--------------------------------------------------\n"RESET);

	releaseSample(sample1);
	releaseSample(sample2);
	releaseSample(sample3);
	releaseSample(sample4);

}

void run_DELETE()
{
	Sample *sample1, *sample2, *sample3, *sample4, *sample5;
	printf(YELLOW"------ Executing tests of one single DELETE ------\n"RESET);

	sample1 = run_sample_test2(DeleteOneRecordWithoutProtection, "user");
	sample2 = run_sample_test2(DeleteOneRecordWithHMAC, "user");
	sample3 = run_sample_test2(DeleteOneRecordWithHMACAndECMAC, "user");
	sample4 = run_sample_test2(DeleteOneRecordWithHMACAndCircularECMAC, "user");
	sample5 = run_sample_test2(DeleteOneRecordWithHMACAndECMACOptimized, "user");

	printSample(sample1, "Without Protection OR With HMAC");
	printDigestedSample(digestSample(sample1), "Without Protection OR With HMAC");

	printSample(sample2, "With HMAC");
	printDigestedSample(digestSample(sample2), "With HMAC");

	printSample(sample3, "With HMAC + ECMAC");
	printDigestedSample(digestSample(sample3), "With HMAC + ECMAC");

	printSample(sample4, "With HMAC + Circular ECMAC");
	printDigestedSample(digestSample(sample4), "With HMAC + Circular ECMAC");

	printSample(sample5, "With HMAC + ECMAC [OPTMIZED]");
	printDigestedSample(digestSample(sample5), "With HMAC + ECMAC [OPTIMIZED]");

	printf(YELLOW"--------------------------------------------------\n"RESET);

	releaseSample(sample1);
	releaseSample(sample2);
	releaseSample(sample3);
	releaseSample(sample4);
	releaseSample(sample5);
}

void run_SELECT()
{
//	Sample *sample1, *sample2;

	Sample *sample3;
//	Sample *sample4;

	printf(YELLOW"------ Executing tests of one single SELECT ------\n"RESET);

//	sample1 = run_sample_test(SelectOneRecordWithoutProtection, "user");
//	sample2 = run_sample_test(SelectOneRecordWithHMAC, "user");
	sample3 = run_sample_test2(SelectOneRecordWithHMACAndECMAC, "user");
//	sample4 = run_sample_test2(SelectOneRecordWithHMACAndECMACOptimized, "user");

//	printSample(sample1, "Without Protection");
//	printDigestedSample(digestSample(sample1), "Without Protection");
//
//	printSample(sample2, "With HMAC");
//	printDigestedSample(digestSample(sample2), "With HMAC");

	printSample(sample3, "With HMAC + ECMAC");
	printDigestedSample(digestSample(sample3), "With HMAC + ECMAC");

//	printSample(sample4, "With HMAC + ECMAC [OPTMIZED]");
//	printDigestedSample(digestSample(sample4), "With HMAC + ECMAC [OPTIMIZED]");

	printf(YELLOW"--------------------------------------------------\n"RESET);

//	releaseSample(sample1);
//	releaseSample(sample2);
	releaseSample(sample3);
//	releaseSample(sample4);
}

//void run_CENARIO2(int rows)
//{
//	float result = 0;
//	printf("------ Executing CENARIO 2 with %d registries ------\n", rows);
//
//	result = run_load_test(insertHMAC, "user", rows);
//	printResultMSec(result, "With HMAC");
//	result = run_load_test(insertECMAC, "user", rows);
//	printResultMSec(result, "With HMAC + ECMAC");
//	result = run_load_test(insertCircularECMAC, "user", rows);
//	printResultMSec(result, "With HMAC + Circular ECMAC");
//
//	printf("-------------------------------------------------------------\n");
//}
//
//void run_CENARIO3(int rows)
//{
//	float result = 0;
//	printf("------ Executing CENARIO 3 with %d registries ------\n", rows);
//
//	result = run_load_test_cenario3(validateHMAC, "user", rows);
//	printResultMSec(result, "With HMAC");
//	result = run_load_test_cenario3(validateECMAC, "user", rows);
//	printResultMSec(result, "With HMAC + ECMAC");
//	result = run_load_test_cenario3(validateCircularECMAC, "user", rows);
//	printResultMSec(result, "With HMAC + Circular ECMAC");
//
//	printf("-------------------------------------------------------------\n");
//}

void init_benchmark()
{
	/* Teste de conexão */
	connection = connectToMysql();
	if( connection == 0){
		printf("Conexão não foi inicializada!\n");
		exit(1);
	}
	createDb(connection);
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");
	mysql_query(connection, "COMMIT");
	mysql_close(connection);


//	run_INSERT(); //OK

	run_UPDATE(); // ok planilha feita, atualizar imagem no tcc

//	run_DELETE();

//	run_SELECT();

	connection = connectToMysql();
	mysql_query(connection, "SET AUTOCOMMIT=0");
	mysql_query(connection, "START TRANSACTION");
	dropDb(connection);
	mysql_query(connection, "COMMIT");
	mysql_close(connection);
}

#endif
