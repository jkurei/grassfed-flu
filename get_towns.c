
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#define max(x,y) (x>y)?x:y
#define min(x,y) (x<y)?x:y

#define d2r (M_PI / 180.0)

///////////////////////////////////////////////////////////////////////////////
// definitions

// TWEETS
typedef struct 
{
	uint  * timestamp;
	float * lon;
	float * lat;
	uint  * town_id;
	char ** rest;
} Tweets_t;

// TOWNS
typedef struct 
{
	uint  * id;
	float * lon;
	float * lat;
	char ** provincia;
} Towns_t;

Tweets_t Tweets_init(uint n);
void Tweets_free(Tweets_t t, uint n);

Towns_t Towns_init(uint n);
void Towns_free(Towns_t t, uint n);

double haversine_km(double long1, double lat1, double long2, double lat2);
double haversine_nounit(double long1, double lat1, double long2, double lat2);

void InitPrintProgress(uint total);
void PrintProgress(uint done);

///////////////////////////////////////////////////////////////////////////////
// MAIN

int main (int argc, char ** argv) 
{
	///////////////////////////////////
	// parameters and initialization

	if (argc < 5) {
		fprintf(stderr, 
		        "wrong invocation\n\n./program num_tweets tweets_file num_towns towns_file > output\n\nfor example:\n\t./get_towns `wc -l s4.201308.unsorted` `wc -l data/geonames/T2.csv` > data/s5.201308.unsorted\n");
		return -1;
	}

	int num_tweets  = atoi(argv[1]);
	char* tweets_fn = argv[2];
	int num_towns   = atoi(argv[3]);
	char* towns_fn  = argv[4];

	printf("get_towns:\n");
	printf("\tnum_tweets=%d\n", num_tweets);
	printf("\ttweets_fn =%s\n", tweets_fn);
	printf("\tnum_towns =%d\n", num_towns);
	printf("\ttowns_fn  =%s\n", towns_fn);

	Towns_t towns   = Towns_init(num_towns);
	Tweets_t tweets = Tweets_init(num_tweets);

	///////////////////////////////////
	// input
	// 
	//   s4:
	//       1        2   3   4      5        6        7            8               9
	//       timestamp|lat|lon|userid|username|hashtags|mentions(id)|mentiones(name)|my_id
	//   
	//   t2:
	//       1         2         3         4
	//       geonameid|provincia|latitude|longitude
	
	fprintf(stderr, "reading...\n");
	
	FILE *ftweets, *ftowns;

	ftweets = fopen(tweets_fn, "r");
	ftowns = fopen(towns_fn, "r");

	uint rownum=0;
	char * line = NULL;
	size_t len = 0;
	size_t read;

	while ((read = getline(&line, &len, ftweets)) != -1) {
		tweets.rest[rownum] = calloc(strlen(line), sizeof(char));
		sscanf(line, "%d|%f|%f|%s", &(tweets.timestamp[rownum]), 
		                            &(tweets.lat[rownum]), 
		                            &(tweets.lon[rownum]),
		                            tweets.rest[rownum]);
		++ rownum;
	}

	rownum=0;
	while ((read = getline(&line, &len, ftowns)) != -1) {
		towns.provincia[rownum] = calloc(3, sizeof(char));
		sscanf(line, "%d|%f|%f|%s", &(towns.id[rownum]), 
		                            &(towns.lat[rownum]), 
		                            &(towns.lon[rownum]), 
		                            towns.provincia[rownum]);
		++ rownum;
	}

	if (line) {
		free(line);
	}

	///////////////////////////////////
	// find closest town for each tweet
	
	fprintf(stderr, "now the real shit begins...\n");

	uint i,j;
	float d, min_d;

	InitPrintProgress(num_tweets);

	for (i=0; i<num_tweets; i++) {
		
		uint min_geoid=-1;
		min_d = UINT_MAX;

		for (j=0; j<num_towns; j++) {
			d = haversine_nounit(tweets.lon[i], tweets.lat[i], towns.lon[j], towns.lat[j]);
			if (d < min_d) { 
				min_d = d; 
				min_geoid = towns.id[j]; 
			}
		}
		
		tweets.town_id[i] = min_geoid;

		PrintProgress(i);
	}

	///////////////////////////////////
	// output
	// 
	fprintf(stderr, "about to start printing the output\n");
	
	for (rownum=0; rownum<num_tweets; rownum++) {
		// s5 (sale de get_towns.c): 
  	    // 1        2   3   4         5         6      7        8        9            10              11
  	    // timestamp|lat|lon|geonameid|provincia|userid|username|hashtags|mentions(id)|mentiones(name)|my_id
		printf("%d|%f|%f|%d|%s|%s\n", 
		       			 tweets.timestamp[rownum],
                         tweets.lat[rownum],
                         tweets.lon[rownum],
                         tweets.town_id[rownum],
                         "OH",
                         tweets.rest[rownum]
           );
	}

	///////////////////////////////////
	// cleanup
	// 
	fprintf(stderr, "all done, i'll clean up this mess and go home...\n");

	Tweets_free(tweets, num_tweets);
	Towns_free(towns, num_towns);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Function definitions

int printstep  = 0;
int printtotal = 0;
void InitPrintProgress(uint total) {
	printtotal = total;

	if (total > 100000) {
		printstep = 20000;
	} else {
		printstep = total/20;
	}
}
void PrintProgress(uint done) {
	if (done % (printstep) == 0) {
		fprintf(stderr, "%.2f%% (%d/%d)\r ", ((float)(done*100))/printtotal, done, printtotal);
		//printf("\a");
	}
}

Tweets_t Tweets_init(uint n) {
	Tweets_t t;
	t.timestamp = calloc(n, sizeof(uint));
	t.lon       = calloc(n, sizeof(float));
	t.lat       = calloc(n, sizeof(float));
	t.town_id   = calloc(n, sizeof(uint));
	t.rest      = calloc(n, sizeof(char*));
	return t;
}

void Tweets_free(Tweets_t t, uint n) {
	uint rownum=0;
	for(;rownum<n; rownum++) {
		free(t.rest[rownum]);
	}

	free(t.lon);
	free(t.lat);
	free(t.timestamp);
	free(t.town_id);
	free(t.rest);
}

Towns_t Towns_init(uint n) {
	Towns_t t;
	t.id        = calloc(n, sizeof(int));
	t.lon       = calloc(n, sizeof(float));
	t.lat       = calloc(n, sizeof(float));
	t.provincia = calloc(n, sizeof(char*));
	return t;
}

void Towns_free(Towns_t t, uint n) {
	uint rownum=0;
	for(;rownum<n; rownum++) {
		free(t.provincia[rownum]);
	}
	free(t.provincia);
	free(t.id);
	free(t.lon);
	free(t.lat);
}

//calculate haversine distance for linear distance
double haversine_km(double long1, double lat1, double long2, double lat2)
{
	double dlong = (long2 - long1) * d2r;
	double dlat = (lat2 - lat1) * d2r;
	double a = pow(sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
	double c = 2 * atan2(sqrt(a), sqrt(1-a));
	double d = 6367 * c;

	return d;
}

double haversine_nounit(double long1, double lat1, double long2, double lat2)
{
	double dlong = (long2 - long1) * d2r;
	double dlat = (lat2 - lat1) * d2r;
	double a = pow(sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
	
	// since atan and product are order-preserving operations, I can stop here

	return a;
}
