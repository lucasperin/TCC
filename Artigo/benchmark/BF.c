/***
Boneh & Franklin encryption scheme as presented in the article "Id-based encryption from the Weil pairing"
***/
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<assert.h>
#include<fcntl.h>
#include<math.h>
#include<string.h>
#include<openssl/sha.h>
#include<openssl/err.h>
#include<openssl/ssl.h>
#include<pbc/pbc.h>
#include<pbc/pbc_test.h>
/*
BF key generation, signature & signature verification
*/

int main(int argc, char **argv)
{
	pairing_t pairing;
	element_t P, Ppub, Did, Qid, U, s, r, xt, gid;
	int fd=0,i=0,m_cnt=0,m_len=100; /*message length is a mutliple of 20*/
	char m[100]={0}, mv[100]={0}, c[100]={0}, id[20]="11.22.33.44.55.66", hash[20]={0}, err[80]={0}, *gs=NULL;
	unsigned long long ci=0, cf=0;
	double tt[1000]={0},tg=0,ts=0,tv=0,ti=0,t=0;
	FILE *f_t=NULL, *f_r=NULL;

	/*
	errors strings initialization for SHA1 & clock initialization for times computation
	*/
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	clockinit("");

	/*initialize the message which is going to be signed*/
	fd = open("/dev/urandom",O_RDONLY);
	read(fd, m, m_len);
	close(fd);

	/*pairing function initalization from the input file which contains the pairing parameters*/
	pbc_demo_pairing_init(pairing, argc, argv);
  	if (!pairing_is_symmetric(pairing)) pbc_die("pairing must be symmetric");

	/*initialization of G1 elements*/
	element_init_G1(P, pairing);
	element_init_G1(Ppub, pairing);
	element_init_G1(Qid, pairing);
	element_init_G1(Did, pairing);
	element_init_G1(U, pairing);

	/*initialization of Zr elements*/
	element_init_Zr(s, pairing);
	element_init_Zr(r, pairing);

	/*initialization of GT elements*/
	element_init_GT(gid, pairing);
	element_init_GT(xt, pairing);

	/*PKG generation of P, s and Ppub*/
	element_random(P);
  	element_random(s);
  	element_mul_zn(Ppub, P, s);

	/*key generation*/
	if(SHA1(id, 20, (unsigned char *)hash)==NULL)
			{
			ERR_error_string(ERR_get_error(),err);
			printf("%s\n",err);
			}
	element_from_hash(Qid, hash, strlen(hash));
	element_mul_zn(Did, Qid, s);

	/*encryption*/
	element_random(r);
	element_mul_zn(U, P, r);
	element_pairing(gid, Qid, Ppub);
	element_pow_zn(gid, gid, r);
	gs=malloc(element_length_in_bytes(gid));
	element_to_bytes(gs,gid);
	if(SHA1(gs , 20, (unsigned char *)hash)==NULL)
		{
		ERR_error_string(ERR_get_error(),err);
		printf("%s\n",err);
		}
	for(i=0;i<20;i++)
	{
		for(m_cnt=0;m_cnt<(m_len/20);m_cnt++)
		{c[i+(m_cnt*20)]=m[i+(m_cnt*20)]^hash[i];}
	}
	free(gs);

	/*decryption*/
	element_pairing(xt, Did, U);
	gs=malloc(element_length_in_bytes(xt));
	element_to_bytes(gs,xt);
	if(SHA1(gs , 20, (unsigned char *)hash)==NULL)
		{
		ERR_error_string(ERR_get_error(),err);
		printf("%s\n",err);
		}
	for(i=0;i<20;i++)
	{
		for(m_cnt=0;m_cnt<(m_len/20);m_cnt++)
		{mv[i+(m_cnt*20)]=c[i+(m_cnt*20)]^hash[i];}
	}

	printf("\n%d\n",strncmp(m,mv,m_len));

	/*free mem*/
  	element_clear(P);
  	element_clear(Ppub);
  	element_clear(Qid);
  	element_clear(Did);
  	element_clear(U);
  	element_clear(gid);
  	element_clear(r);
  	element_clear(xt);
	element_clear(s);
	pairing_clear(pairing);
	free(gs);
	ERR_free_strings();


  return 0;
}

