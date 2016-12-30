#include <math.h>
#include <bzlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MIN2(a, b) (((a)<(b))?(a):(b))

const int BUFFERSIZE = 1024*16;

#define FILTER_LEN  49
float coeffs[FILTER_LEN] = {
	0.0030767705757170916, 0.0032281388994306326, 0.003679626388475299, 0.0044235121458768845, 0.005447071511298418, 
	0.006732794921845198, 0.008258684538304806, 0.009998636320233345, 0.01192287914454937, 0.013998489826917648, 
	0.016189955174922943, 0.018459778279066086, 0.02076912485063076, 0.023078477010130882, 0.025348322466015816, 
	0.027539819478988647, 0.029615474864840508, 0.03153977170586586, 0.03327978029847145, 0.03480572998523712, 
	0.036091506481170654, 0.03711511567234993, 0.03785903751850128, 0.038310542702674866, 0.03846191242337227, 
	0.038310542702674866, 0.03785903751850128, 0.03711511567234993, 0.036091506481170654, 0.03480572998523712, 
	0.03327978029847145, 0.03153977170586586, 0.029615474864840508, 0.027539819478988647, 0.025348322466015816, 
	0.023078477010130882, 0.02076912485063076, 0.018459778279066086, 0.016189955174922943, 0.013998489826917648, 
	0.01192287914454937, 0.009998636320233345, 0.008258684538304806, 0.006732794921845198, 0.005447071511298418, 
	0.0044235121458768845, 0.003679626388475299, 0.0032281388994306326, 0.0030767705757170916};

void errorout(char* str) {
	fprintf(stderr, "%s", str);
	exit(EXIT_FAILURE);
}

char* allocate_buffer(size_t len) {
	char* ret = malloc(len);
	if (ret == NULL) errorout("Failed malloc\n");
	return ret;
}

void read_bzfile(int reset, char* fname, size_t start, size_t end,
		 char* buffer, size_t* length) {

	int bzerror;
	static size_t pos = 0;
	static FILE* infile = NULL;
	static BZFILE* infilebz = NULL;

	if (reset == 1) {
		if (infile != NULL) {
			BZ2_bzReadClose(&bzerror, infilebz);
			fclose(infile);
		}
		infile = fopen(fname, "r");
		if (infile == NULL) errorout("Couldn't open input file\n");
		infilebz = BZ2_bzReadOpen(&bzerror, infile, 0, 0, NULL, 0);
		if (bzerror != BZ_OK) errorout("Error opening bz file\n");
		pos = 0;
	}

	while (pos < start) {
		assert(*length > 0);
		pos += BZ2_bzRead(&bzerror, infilebz, buffer, MIN2(*length, pos-start));
	}
	*length = BZ2_bzRead(&bzerror, infilebz, buffer, MIN2(*length, end-pos));
	pos += *length;
	if ((bzerror != BZ_OK) && (bzerror != BZ_STREAM_END)) {
		errorout("Error during decompression\n");
	}

}

void find_signal_start_end(int reset, double thresh, char* buffer, size_t len,
		           size_t* start, size_t* end) {

	static int found = 0;
	static size_t totalpos = 0;

	if (reset == 1) {
		totalpos = 0;
		found = 0;
	}

	size_t pos = 0;
	while (pos < len) {
		float I = ((float*) buffer)[pos/4];
		float Q = ((float*) buffer)[pos/4+1];
		float Mag = sqrt(I*I+Q*Q);
	
		if ((Mag >= thresh) && (found == 0)) {
			*start = totalpos + pos;
			found = 1;
		}

		if ((Mag < thresh) && (found == 1)) {
			*end = totalpos + pos;
			found = 2;
		}

		if ((Mag >= thresh) && (found == 2)) {
			*end = totalpos + pos;
			found = 2;
		}

		
		pos += 8;
	}
	totalpos += len;

}

void downshift(int reset, double dsfreq, double samprate, char* buffer, size_t len) {

	static double t = 0.0;
	if (reset) t = 0.0;

	size_t pos = 0;
	while (pos < len) {
		float I = ((float*) buffer)[pos/4];
		float Q = ((float*) buffer)[pos/4+1];
		double w = -2.0 * M_PI * dsfreq * t / samprate;
		float A = cos(w);
		float B = sin(w);
		float In = I*A-B*Q;
		float Qn = I*B+Q*A;
		((float*) buffer)[pos/4]   = In;
		((float*) buffer)[pos/4+1] = Qn;
		t += 1.0;
		pos += 8;
	}

}

	
void write_iqdata(int reset, char* fname, char* buffer, size_t len) {

	static FILE* ofile;
	char sbuffer[64];

	if (reset == 1) {
		if (ofile != NULL) fclose(ofile);
		ofile = fopen(fname, "w");
		if (ofile == NULL) errorout("Error opening outfile\n");
	}

	size_t pos = 0;
	while (pos < len) {
		float I = ((float*) buffer)[pos/4];
		float Q = ((float*) buffer)[pos/4+1];
		sprintf(sbuffer, "% .6f%+.6fi\n", I, Q);
		int slen = strlen(sbuffer);
		fwrite(sbuffer, slen, 1, ofile);
		pos += 8;
	}

}


void firFloat( float *coeffs, float *input, float *output,
       int length, int filterLength )
{

    const int MAX_INPUT_LEN  = 80;
    const int MAX_FLT_LEN = 63;
    const int BUFFER_LEN = (MAX_FLT_LEN-1+MAX_INPUT_LEN); 
    float insamp[ BUFFER_LEN ];
    float acc;     // accumulator for MACs
    float *coeffp; // pointer to coefficients
    float *inputp; // pointer to input samples


    static int first = 1;
    if (first) {
	memset(insamp, 0,  BUFFER_LEN * sizeof(double));
    	first = 0;
    }

    // put the new samples at the high end of the buffer
    memcpy( &insamp[filterLength - 1], input,
            length * sizeof(double) );

    // apply the filter to each input sample
    for (int n = 0; n < length; n++ ) {
        // calculate output n
        coeffp = coeffs;
        inputp = &insamp[filterLength - 1 + n];
        acc = 0;
        for (int k = 0; k < filterLength; k++ ) {
            acc += (*coeffp++) * (*inputp--);
        }
        output[n] = acc;
    }
    // shift input samples back in time for next time
    memmove( &insamp[0], &insamp[length],
            (filterLength - 1) * sizeof(double) );

}




void filter(int reset, float* coeffs, int num_coeffs, char* buffer, size_t len) {
	
    len /= 4;
    int pos = 0;
    const int size = 80;
    float  inI[size],  inQ[size];
    float outI[size], outQ[size];

    while (pos < len) {
        // split I and Q
        for (int i=0; i<MIN2(size,(len-pos)/2); i++) {
    	     inI[i] = ((float*) buffer)[pos+i*2+0];
	     inQ[i] = ((float*) buffer)[pos+i*2+1];
        } 

        firFloat( coeffs, inI, outI, MIN2(size, (len-pos)/2), num_coeffs);
        firFloat( coeffs, inQ, outQ, MIN2(size, (len-pos)/2), num_coeffs);

        // merge them again
        for (int i=0; i<MIN2(size, (len-pos)/2); i++) {
    	    ((float*) buffer)[pos+i*2+0] = outI[i];
	    ((float*) buffer)[pos+i*2+1] = outQ[i];
        }

        pos += size*2;
    }

}

int main(int argc, char **argv) {

	char* buffer = allocate_buffer(BUFFERSIZE);

	for (int i=1; i<argc; i++) {
		size_t start = ~0;
		size_t end = 0;
		size_t len = BUFFERSIZE;
		int reset = 1;
		char* fname = argv[i];

		while (len == BUFFERSIZE) {
			read_bzfile(reset, fname, 0, ~0, buffer, &len);
			find_signal_start_end(reset, 0.1, buffer, len, &start, &end);
			reset = 0;
		}
		
		printf("%s %llu %llu\n", fname, start, end);
		size_t lead = (end-start) / 10;
		lead -= lead % 8;

		reset = 1;
		if (start < end) {
			char* newfname = allocate_buffer(strlen(fname) + 16);
			sprintf(newfname, "squelched_%s", fname);
			len = BUFFERSIZE;
			while (len == BUFFERSIZE) {
				read_bzfile(reset, fname, MIN2(start, start-lead), end+lead, buffer, &len);
				newfname[strlen(newfname)-4] = 0; // remove .bz2 from filename
				write_iqdata(reset, newfname, buffer, len);
				reset = 0;
			}
			free(newfname);
		}
		else {
			errorout("No signal found\n");
		}
	}

	free(buffer);
}

