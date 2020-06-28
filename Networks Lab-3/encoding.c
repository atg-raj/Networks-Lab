#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include<string.h>
#include<errno.h>
#include<math.h>
#include<errno.h>


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;  //table used for decoding
static int rem[] = {0, 2, 1};   //table used for deciding no of '=' characters to be appended

/* Function to encode message */
char *encode( char *data)
{
	int in=strlen(data);
	int out=4*((in+2)/3);  //output length of the encoded message
	int i,j=0,a,b,c;
    char *encoding = malloc(out);
    if (encoding == NULL) return NULL;

    for ( i = 0; i < in;) {

/* Extracting 3 characters at a time for encoding */
        if(i<in)
		a=data[i++];
	else
		a=0;
	if(i<in)
		b=data[i++];
	else
		b=0;
	if(i<in)
		c=data[i++];
	else
		c=0;

        uint32_t triple = (a << 16) + (b << 8) + c;

/* Dividing the 24 bits string into four groups of 6 bits each and encoding them according to encoding table */
        encoding[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoding[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoding[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoding[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    for (int i = 0; i < rem[in % 3]; i++)	//appending additional characters
        encoding[out - 1 - i] = '=';

    encoding[out]='\0';

    return encoding;

}

char *decode( char *data) 
{

    if (decoding_table == NULL)   /* Building necessary decoding table for the corresponding encoded string  */
    {
	decoding_table = malloc(256);
    	for (int i = 0; i < 64; i++)
        	decoding_table[(unsigned char) encoding_table[i]] = i;
    }
    int in=strlen(data);
    if (in % 4 != 0) return NULL;
    int i,j=0,a,b,c,d;
    int out = in / 4 * 3;
    if (data[in - 1] == '=') (out)--;	//Removing the additional characters
    if (data[in - 2] == '=') (out)--;
     char *decoding = malloc(out);
    if (decoding == NULL) return NULL;

    for ( i = 0; i < in;) {

/* Extracting 4 characters at a time for decoding  */
	if(data[i]!='=')
		a=decoding_table[data[i++]];
	else{
		a=0;i++;}
	if(data[i]!='=')
		b=decoding_table[data[i++]];
	else{
		b=0;i++;}
	if(data[i]!='=')
		c=decoding_table[data[i++]];
	else{
		c=0;i++;}
	if(data[i]!='=')
		d=decoding_table[data[i++]];
	else{
		d=0;i++;}

        uint32_t quad = (a << 3 * 6)
        + (b << 2 * 6)
        + (c << 1 * 6)
        + d;
/* Dividing the 24 bits string into groups of three and decoding them to get the original characters */
        if (j < out) decoding[j++] = (quad >> 2 * 8) & 0xFF;
        if (j < out) decoding[j++] = (quad >> 1 * 8) & 0xFF;
        if (j < out) decoding[j++] = (quad >> 0 * 8) & 0xFF;
    }

    decoding[out]='\0';
    
    return decoding;
}
