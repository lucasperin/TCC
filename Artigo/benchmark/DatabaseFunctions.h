#include <mysql/mysql.h>
#include "ByteArray.h"
#include "RandomString.h"
#include "CryptoFunctions.h"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>


long DBtimevaldiff2(struct timeval *startTime, struct timeval *finishTime)
{
 long msec;
 msec = (finishTime->tv_sec - startTime->tv_sec) * 1000000.0;
 msec += (finishTime->tv_usec - startTime->tv_usec);
// if(msec == 0){
//	 float usec;
//	 usec = (float)(finishTime->tv_usec - startTime->tv_usec);
//	 return usec/1000.0;
// }
 return msec;
}

long DBbenchmark2( char* msg, char* query, MYSQL* connection )
{
	struct timeval startTime;
	struct timeval finishTime;

	gettimeofday(&startTime, 0);
	if (mysql_query(connection, query)) {
	  printf("QUERY: %s\n", query);
	  printf("ERROR: %s::ErrNum %u: %s\n",msg, mysql_errno(connection), mysql_error(connection));
	}
	gettimeofday(&finishTime, 0);

	return DBtimevaldiff2(&startTime, &finishTime);
}

MYSQL* connectToMysql()
{

	MYSQL* connection = 0;

	connection = mysql_init(0);

	  if (connection == 0) {
	      printf("connectToMysql::Error %d: %s\n", mysql_errno(connection), mysql_error(connection));
	      return 0;
	  }

	  if (mysql_real_connect(connection, "localhost", "root", "root", 0, 0, 0, 0) == 0) {
	      printf("connectToMysql::Error %d: %s\n", mysql_errno(connection), mysql_error(connection));
	      return 0;
	  }
	  return connection;
}

void closeMysqlConnection(MYSQL* connection)
{
	 mysql_close(connection);
}

void createDb(MYSQL* connection)
{
	if (mysql_query(connection, "CREATE DATABASE benchmark")) {
		printf("createDb::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

void dropDb(MYSQL* connection)
{
	if (mysql_query(connection, "DROP DATABASE benchmark")) {
		printf("dropDb::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

void createDefaultTable(MYSQL* connection)
{
char* defaultTableQuery = "create table benchmark.user ("
		"id INT(11) not null auto_increment,"
		"name varchar(100) not null,"
		"email varchar(100) not null,"
		"password varchar(100) not null,"
		"hmac blob,"
		"history blob,"
		"primary key (id)"
	");";

	if (mysql_query(connection, defaultTableQuery)) {
		printf("createDefaultTable::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

void createTable(MYSQL* connection, char* tableName)
{
	char query[500];
	sprintf(query, "create table benchmark.%s ("
		"id INT(11) not null auto_increment,"
		"name varchar(100) not null,"
		"email varchar(100) not null,"
		"password varchar(100) not null,"
		"text longtext not null,"
		"hmac blob,"
		"history blob,"
		"primary key (id)"
	");", tableName);
	if (mysql_query(connection, query)) {
//		printf("createTable::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));

		char dropQuery[500];
		sprintf(dropQuery, "drop table benchmark.%s;", tableName);
		mysql_query(connection, dropQuery);
		mysql_query(connection, query);
	}
}

void dropDefaultTable(MYSQL* connection)
{
	if (mysql_query(connection, "DROP TABLE benchmark.user")) {
		printf("dropDefaultTable::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

void dropTable(MYSQL* connection, char* tableName)
{
	char query[200];
	sprintf(query, "DROP TABLE benchmark.%s", tableName);
	if (mysql_query(connection, query)) {
		printf("dropTable::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}
}

struct ByteArray* getHmac(MYSQL* connection, int rowId, char* tablename, long * dbTime)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "SELECT hmac FROM benchmark.%s WHERE id = %d", tablename, rowId);
	*dbTime += DBbenchmark2("getDefaultTableHmac", query, connection);

	result = mysql_store_result(connection);
	if(!(row = mysql_fetch_row(result))){
		return 0;
	}

	ret->data = (unsigned char*)row[0];
	ret->size = (unsigned int)mysql_fetch_lengths(result)[0];
	mysql_free_result(result);
	return ret;
}

struct ByteArray* getSecondToLastHmac(MYSQL* connection, char* tablename)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "SELECT hmac FROM benchmark.%s WHERE id IN (SELECT max(id)-1 from benchmark.%s)", tablename, tablename);
	if (mysql_query(connection, query)) {
		printf("%s", query);
		printf("getDefaultTableHmac::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);
	if(!(row = mysql_fetch_row(result))){
		return 0;
	}

	ret->data = (unsigned char*)row[0];
	ret->size = (unsigned int)mysql_fetch_lengths(result)[0];

	return ret;
}

struct ByteArray* getLastHmac(MYSQL* connection, char* tablename, long* dbTime)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "select hmac from benchmark.%s order by id desc limit 1", tablename);
	//SELECT hmac FROM benchmark.%s WHERE id IN (SELECT max(id) from benchmark.%s)", tablename, tablename);

	*dbTime += DBbenchmark2("getLastHmac", query, connection);

	result = mysql_store_result(connection);
	if(!(row = mysql_fetch_row(result))){
		free(result);
		free(ret);
		return 0;
	}

	ret->data = (unsigned char*)row[0];
	ret->size = (unsigned int)mysql_fetch_lengths(result)[0];
	free(result);
	return ret;
}

struct ByteArray* getFirstHmac(MYSQL* connection, char* tablename, long* dbTime)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	MYSQL_RES *result;
	MYSQL_ROW row;

	char query[200];
	sprintf(query, "SELECT hmac FROM benchmark.%s LIMIT 1", tablename);

	*dbTime += DBbenchmark2("getFirstHmac", query, connection);

	result = mysql_store_result(connection);
	if(!(row = mysql_fetch_row(result))){
		free(result);
		free(ret);
		return 0;
	}

	ret->data = (unsigned char*)row[0];
	ret->size = (unsigned int)mysql_fetch_lengths(result)[0];
	free(result);
	return ret;
}

int lastId(MYSQL* connection, char* tablename)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char* chunk = "SELECT max(id) FROM benchmark.%s";
	char query[100];
	snprintf(query, 100, chunk, tablename);


	if (mysql_query(connection, query)) {
	  printf("lastId::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	return atoi(row[0]);
}

int firstId(MYSQL* connection, char* tableName)
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	char* chunk = "SELECT min(id) FROM benchmark.%s";
	char query[100];
	snprintf(query, 100, chunk, tableName);

	if (mysql_query(connection, query)) {
	  printf("firstId::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	free(result);
	return atoi(row[0]);
}

void insertRandomRecord(MYSQL* connection, char* tableName)
{
	char* message = calloc(11000, sizeof(char));
	char query[11000];
	char* stat = "INSERT INTO benchmark.%s ("
			"name, "
			"email, "
			"password, "
			"text "
			") VALUES ("
			"\'%s\', "
			"\'%s\', "
			"\'%s\', "
			"\'%s\' "
			");";

	char* name = generateRandomSizeString(100);
	char* email = generateRandomSizeString(100);
	char* password = generateRandomSizeString(100);
	char* text = generateRandomSizeString(10000);

	strcat(message, name);
	strcat(message, email);
	strcat(message, password);
	strcat(message, text);

	snprintf(query, 11000, stat, tableName, name, email, password, text);

	if (mysql_query(connection, query)) {
	  printf("I1R::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	free(message);
	free(name);
	free(email);
	free(password);
	free(text);
}

void insertRandomRecordH(MYSQL* connection, char* tableName)
{
	char* message = calloc(11000, sizeof(char));
	char query[13000];
	char* stat = "INSERT INTO benchmark.%s ("
			"name, "
			"email, "
			"password, "
			"text, "
			"hmac "
			") VALUES ("
			"\'%s\', "
			"\'%s\', "
			"\'%s\', "
			"\'%s\', "
			"\'%s\' "
			");";

	char* name = generateRandomSizeString(100);
	char* email = generateRandomSizeString(100);
	char* password = generateRandomSizeString(100);
	char* text = generateRandomSizeString(10000);

	strcat(message, name);
	strcat(message, email);
	strcat(message, password);
	strcat(message, text);

	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	snprintf(query, 13000, stat, tableName, name, email, password, text, chunk);

	if (mysql_query(connection, query)) {
	  printf("I1R::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	free(message);
	free(hmac);
	free(name);
	free(email);
	free(password);
	free(text);
}

void insertRandomRecordHH(MYSQL* connection, char* tableName)
{
	char* message = calloc(13000, sizeof(char));
	char query[13000];
	char* stat = "INSERT INTO benchmark.%s ("
			"name, "
			"email, "
			"password, "
			"text, "
			"hmac, "
			"history "
			") VALUES ("
			"\"%s\", "
			"\"%s\", "
			"\"%s\", "
			"\"%s\", "
			"\"%s\", "
			"\"%s\" "
			");";

	char* name = generateRandomSizeString(100);
	char* email = generateRandomSizeString(100);
	char* password = generateRandomSizeString(100);
	char* text = generateRandomSizeString(10000);

	strcat(message, name);
	strcat(message, email);
	strcat(message, password);
	strcat(message, text);

	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	char chunk[2*(hmac->size) + 1];
	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);

	//Lendo HMAC do primeiro registro
	long dbTime = 0; //Não usado
	struct ByteArray* firstHmac = getFirstHmac(connection, tableName, &dbTime);

	//calculando history
	if(firstHmac != 0){
		struct ByteArray* history = generateHistory(hmac, firstHmac);
		char chunk2[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk2, history->data, history->size);
		snprintf(query, 13000, stat, tableName, name, email, password,text, chunk, chunk2);

		free(history);
	}
	else
	{
		struct ByteArray* history = generateHistory(hmac, hmac);
		char chunk2[2*(history->size) + 1];
		mysql_real_escape_string(connection, chunk2, history->data, history->size);
		snprintf(query, 13000, stat, tableName, name, email, password,text, chunk, chunk2);

		free(history);
	}

	if (mysql_query(connection, query)) {
	  printf("I1RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
	}

	//update do historico do registro anterior
	if(firstHmac != 0){
		struct ByteArray* secondToLastHmac = getSecondToLastHmac(connection, tableName);
		struct ByteArray* secondToLastHistory = generateHistory(hmac, secondToLastHmac);

		char chunk3[2*(secondToLastHistory->size) + 1];
		mysql_real_escape_string(connection, chunk3, secondToLastHistory->data, secondToLastHistory->size);
		snprintf(query, 13000, stat, chunk, chunk3);

		stat = "UPDATE benchmark.%s SET "
				"history =\'%s\' "
				"WHERE id = %d;";
		snprintf(query, 13000, stat, tableName, chunk3, lastId(connection, tableName) - 1);
		if (mysql_query(connection, query)) {
			printf("%s\n", query);
		  printf("I1RHH::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
		}

		free(message);
		free(hmac);
		free(firstHmac);
		free(secondToLastHmac);
		free(secondToLastHistory);
		free(name);
		free(email);
		free(password);
		free(text);
	}

//----------------------------------------------------------------------
//	struct ByteArray* hmac = generateHMAC((unsigned char*) message);
//
//	char chunk[2*(hmac->size) + 1];
//	mysql_real_escape_string(connection, chunk, hmac->data, hmac->size);
//
//	struct ByteArray* history;
//
//	if(row > 1){
//	struct ByteArray* hmacPredecessor = getHmac(connection, row - 1, tableName);
//	history = generateHistory(hmac, hmacPredecessor);
//	}else{
//		history =generateHistory(hmac, hmac);
//	}
//
//	char chunk2[2*(history->size) + 1];
//	mysql_real_escape_string(connection, chunk, history->data, history->size);
//
//	snprintf(query, 13000 , stat, tableName,
//			name, email, password, text,
//			chunk, chunk2);
//
//	if (mysql_query(connection, query)) {
//	  printf("I1R::Error %u: %s\n", mysql_errno(connection), mysql_error(connection));
//	}

}
