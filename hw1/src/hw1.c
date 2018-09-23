#include <stdlib.h>

#include "debug.h"
#include "hw1.h"
#include "hw1helper.c"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the content of three frames of audio data and
 * two annotation fields have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 1 if validation succeeds and 0 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variables "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 1 if validation succeeds and 0 if validation fails.
 * Refer to the homework document for the effects sof this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {

    if (argc == 0) return 0;

    unsigned long tempGlobal = 0x0;
    char **arrAdd = argv;
    int requiredParam = 1;
    int flagCheck = 0;
    int multFCheck = 0;
    int multKCheck = 0;

    /* Flag Checks
    0 = -h
    1 = -u
    2 = -d
    3 = -c
    */

    // Positional Argument Check
    arrAdd++; //Start from flag input, not "bin/audible"
    char *charPointer = *arrAdd;
    if (*charPointer == '-') {
        charPointer++;
        if (*charPointer == 'h') {
            if (*(charPointer + 1) != '\0') return 0;
            global_options = 0x8000000000000000;
            return 1;
         }
        else if (*charPointer == 'u') {
            if (*(charPointer + 1) != '\0') return 0;
            tempGlobal = 0x4000000000000000;
            flagCheck = 1;
        }
        else if (*charPointer == 'd') {
            if (*(charPointer + 1) != '\0') return 0;
            tempGlobal = 0x2000000000000000;
            flagCheck = 2;
        }
        else if (*charPointer == 'c') {
            if (*(charPointer + 1) != '\0') return 0;
            tempGlobal = 0x1000000000000000;
            requiredParam = 0;
            flagCheck = 3;
        }
        else return 0;
    }
    else return 0;

    // Optional Argument Check
    arrAdd++;
    int i = 2;
    while (i < argc) {
        charPointer = *arrAdd;
        if (*charPointer != '-') return 0;
        else {
            charPointer++;
            if (*charPointer == 'f') {
                if (*(charPointer + 1) != '\0') return 0;
                if (multFCheck == 1) return 0;
                arrAdd++;
                i++;
                charPointer = *arrAdd;
                if (charPointer == NULL) return 0;
                else {
                    unsigned long charToDec = 0;
                    while (*charPointer >= '0' && *charPointer <= '9') {
                        charToDec = charToDec * 10 + (*charPointer - 48);
                        charPointer++;
                    }
                    if (charToDec < 1 || charToDec > 1024) return 0;
                    else {
                        if (flagCheck == 1 || flagCheck == 2) {
                            unsigned long factMod = (charToDec - 1) << 48;
                            tempGlobal = tempGlobal | factMod;
                        }
                        else return 0;
                    }
                    multFCheck = 1;
                }
            }
            else if (*charPointer == 'k') {
                if (*(charPointer + 1) != '\0') return 0;
                if (multKCheck == 1) return 0;
                arrAdd++;
                i++;
                charPointer = *arrAdd;
                if (charPointer == NULL) return 0;
                if (flagCheck == 3) {
                    requiredParam = 1;
                    int charNumCheck = 0;
                    unsigned long keyHex = 0x0;
                    while (*charPointer != '\0') {
                        if (charNumCheck > 8) return 0;
                        keyHex = keyHex << 4;
                        unsigned long charToHex = 0x0;
                        if (*charPointer >= '0' && *charPointer <= '9') {
                            charToHex = *charPointer - 48;
                        }
                        else if (*charPointer >= 'A' && *charPointer <= 'F') {
                            charToHex = *charPointer - 55;
                        }
                        else if (*charPointer >= 'a' && *charPointer <= 'f') {
                            charToHex = *charPointer - 87;
                        }
                        else return 0;
                        keyHex += charToHex;
                        charPointer++;
                        charNumCheck++;
                    }
                    if (charNumCheck == 0) return 0;
                    tempGlobal = tempGlobal | keyHex;
                }
                else return 0;
                multKCheck = 1;
            }
            else if (*charPointer == 'p') {
                if (*(charPointer + 1) != '\0') return 0;
                unsigned long pBits = 0x800000000000000;
                tempGlobal = tempGlobal | pBits;
            }
            else return 0;
        }
        arrAdd++;
        i++;
    }
    if (requiredParam == 0) return 0;

    global_options = tempGlobal;
    return 1;
}

/**
 * @brief  Recodes a Sun audio (.au) format audio stream, reading the stream
 * from standard input and writing the recoded stream to standard output.
 * @details  This function reads a sequence of bytes from the standard
 * input and interprets it as digital audio according to the Sun audio
 * (.au) format.  A selected transformation (determined by the global variable
 * "global_options") is applied to the audio stream and the transformed stream
 * is written to the standard output, again according to Sun audio format.
 *
 * @param  argv  Command-line arguments, for constructing modified annotation.
 * @return 1 if the recoding completed successfully, 0 otherwise.
 */
int recode(char **argv) {

    AUDIO_HEADER header;
    AUDIO_HEADER* hp;
    hp = &header;

    int returnCheck = 0;

    // HEADER
    returnCheck = read_header(hp);
    if (returnCheck == 0) return 0;

    // ANNOTATIONS
    int annotationExists;
    int pExists = 0;
    unsigned int dataOffset = reverseNumHelper(header.data_offset);
    unsigned int size = dataOffset - sizeof(AUDIO_HEADER);
    if (size != 0) {
        if (dataOffset < 32) return 0;
        returnCheck = read_annotation(input_annotation, size);
        if (returnCheck == 0) return 0;
        annotationExists = 1;
    }
    else {
        if (dataOffset < 24) return 0;
        annotationExists = 0;
    }

    // FRAMES
    int channels = reverseNumHelper(header.channels);
    int bytes_per_sample = reverseNumHelper(header.encoding) - 1;
    int bytesPerFrame = channels * bytes_per_sample;
    int data_size = reverseNumHelper(header.data_size);
    int totalFrames = data_size / bytesPerFrame;
    unsigned int factor = 1;
    int upOrDown = 0;
    int cryptCheck = 0;

    char **arrAdd = argv;

    arrAdd++; //Start from flag input, not "bin/audible"
    char *charPointer = *arrAdd;
    charPointer++;
    if (*charPointer == 'h') {
        return 1;
    }
    else if (*charPointer == 'u') {
        arrAdd++;
        charPointer = *arrAdd;
        while (charPointer != NULL) {
            charPointer++;
            if (*charPointer == 'f') {
                arrAdd++;
                charPointer = *arrAdd;
                factor = 0;
                while (*charPointer != '\0') {
                    factor = factor * 10 + (*charPointer - 48);
                    charPointer++;
                }
                upOrDown = 0;
                header.data_size = reverseNumHelper((totalFrames + 1) / factor * bytesPerFrame);
            }
            else if (*charPointer == 'p') {
                pExists = 1;
                pGiven(input_annotation, output_annotation);
            }
            arrAdd++;
            charPointer = *arrAdd;
        }
    }
    else if (*charPointer == 'd') {
        arrAdd++;
        charPointer = *arrAdd;
        while (charPointer != NULL) {
            charPointer++;
            if (*charPointer == 'f') {
                arrAdd++;
                charPointer = *arrAdd;
                factor = 0;
                while (*charPointer != '\0') {
                    factor = factor * 10 + (*charPointer - 48);
                    charPointer++;
                }
                upOrDown = 1;
                header.data_size = reverseNumHelper(totalFrames * factor * bytesPerFrame);
            }
            else if (*charPointer == 'p') {
                pExists = 1;
                pGiven(input_annotation, output_annotation);
            }
            arrAdd++;
            charPointer = *arrAdd;
        };
    }
    else if (*charPointer == 'c') {
        arrAdd++;
        charPointer = *arrAdd;
        while (charPointer != NULL) {
            charPointer++;
            if (*charPointer == 'k') {
                unsigned long mask =  0x00000000ffffffff;
                unsigned int key = global_options & mask;
                cryptCheck = 1;
                mysrand(key);
            }
            else if (*charPointer == 'p') {
                pExists = 1;
                pGiven(input_annotation, output_annotation);
            }
            arrAdd++;
            charPointer = *arrAdd;
        }
    }
    else return 0;

    if (pExists == 0) {
        int append;
        unsigned int* sizeP;
        sizeP = &size;
        if (annotationExists == 0) {
            append = noAnn(output_annotation, argv, sizeP);
            header.data_offset = reverseNumHelper(dataOffset + append);
        }
        else {
            append = yesAnn(input_annotation, output_annotation, argv, sizeP);
            header.data_offset = reverseNumHelper(append);
        }
    }

    write_header(hp);
    write_annotation(output_annotation, size);
    if (upOrDown == 0) {
        int frameCount;
        int n = factor;
        for (frameCount = 0; frameCount < totalFrames; frameCount++) {
            returnCheck = read_frame((int*)input_frame, channels, bytes_per_sample);
            if (returnCheck == 0) return 0;
            if (factor == n) {
                inOutFrame(input_frame, output_frame, channels, bytes_per_sample);
                write_frame((int*)output_frame, channels, bytes_per_sample);
                n = 1;
            }
            else n++;
        }
    }
    else if (upOrDown == 1) {
        int frameCount;
        int n = factor;
        for (frameCount = 0; frameCount < totalFrames; frameCount++) {
            returnCheck = read_frame((int*)input_frame, channels, bytes_per_sample);
            if (returnCheck == 0) return 0;
            inOutFrame(input_frame, output_frame, channels, bytes_per_sample);
            while (n < factor) {
                write_frame((int*)output_frame, channels, bytes_per_sample);
                n++;
            }
            n = 1;
        }
    }
    if (cryptCheck == 1) {
        int frameCount;
        for (frameCount = 0; frameCount < totalFrames; frameCount++) {
            returnCheck = read_frame((int*)input_frame, channels, bytes_per_sample);
            if (returnCheck == 0) return 0;
            crypt((int*)input_frame, (int*)output_frame, channels, bytes_per_sample);
            write_frame((int*)output_frame, channels, bytes_per_sample);
        }
    }

    return 1;
}

/**
 * @brief Read the header of a Sun audio file and check it for validity.
 * @details  This function reads 24 bytes of data from the standard input and
 * interprets it as the header of a Sun audio file.  The data is decoded into
 * six unsigned int values, assuming big-endian byte order.   The decoded values
 * are stored into the AUDIO_HEADER structure pointed at by hp.
 * The header is then checked for validity, which means:  no error occurred
 * while reading the header data, the magic number is valid, the data offset
 * is a multiple of 8, the value of encoding field is one of {2, 3, 4, 5},
 * and the value of the channels field is one of {1, 2}.
 *
 * @param hp  A pointer to the AUDIO_HEADER structure that is to receive
 * the data.
 * @return  1 if a valid header was read, otherwise 0.
 */
int read_header(AUDIO_HEADER *hp){

    unsigned int hexNum;
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    int i;

    for (i = 0; i < 6; i++) {
        hexNum = 0;
        a = getchar();
        b = getchar();
        c = getchar();
        d = getchar();

        hexNum += d;
        hexNum = hexNum << 8;
        hexNum += c;
        hexNum = hexNum << 8;
        hexNum += b;
        hexNum = hexNum << 8;
        hexNum += a;

        unsigned int realVal = reverseNumHelper(hexNum);

        if (i == 0) {
            if (realVal != 0x2e736e64) return 0;
            else hp->magic_number = hexNum;
        }
        else if (i == 1) {
            if (realVal % 8 != 0) return 0;
            hp->data_offset = hexNum;
        }
        else if (i == 2) {
            hp->data_size = hexNum;
        }
        else if (i == 3) {
            if (realVal < 2 || realVal > 5) return 0;
            else hp->encoding = hexNum;
        }
        else if (i == 4) {
            hp->sample_rate = hexNum;
        }
        else {
            if (realVal < 1 || realVal > 2) return 0;
            else hp->channels = hexNum;
        }
    }
    if (reverseNumHelper(hp->data_size) %
        ((reverseNumHelper(hp->encoding) - 1) * reverseNumHelper(hp->channels) != 0)) {
            return 0;
    }

    return 1;
}

/**
 * @brief  Write the header of a Sun audio file to the standard output.
 * @details  This function takes the pointer to the AUDIO_HEADER structure passed
 * as an argument, encodes this header into 24 bytes of data according to the Sun
 * audio file format specifications, and writes this data to the standard output.
 *
 * @param  hp  A pointer to the AUDIO_HEADER structure that is to be output.
 * @return  1 if the function is successful at writing the data; otherwise 0.
 */
int write_header(AUDIO_HEADER *hp){

    char* bytePointer = (char*) hp;
    char val = 0;

    unsigned char c;

    for (int i = 0; i < 24; i++) {
        c = *bytePointer;
        val = putchar(c);
        if (val == EOF) return 0;
        bytePointer++;
    }

    return 1;
}

/**
 * @brief  Read annotation data for a Sun audio file from the standard input,
 * storing the contents in a specified buffer.
 * @details  This function takes a pointer 'ap' to a buffer capable of holding at
 * least 'size' characters, and it reads 'size' characters from the standard input,
 * storing the characters read in the specified buffer.  It is checked that the
 * data read is terminated by at least one null ('\0') byte.
 *
 * @param  ap  A pointer to the buffer that is to receive the annotation data.
 * @param  size  The number of bytes of data to be read.
 * @return  1 if 'size' bytes of valid annotation data were successfully read;
 * otherwise 0.
 */
int read_annotation(char *ap, unsigned int size){

    unsigned char c;
    int nullCount = 0;
    int i = 0;

    if (size >= ANNOTATION_MAX) return 0;
    if (size == 0) return 1;

    while (i < size) {
        c = getchar();
        if (c == '\0') {
            *ap = '\0';
            nullCount = 1;
            break;
        }
        *ap = c;
        ap++;
        i++;
    }
    if (nullCount == 0) return 0;

    return 1;
}

/**
 * @brief  Write annotation data for a Sun audio file to the standard output.
 * @details  This function takes a pointer 'ap' to a buffer containing 'size'
 * characters, and it writes 'size' characters from that buffer to the standard
 * output.
 *
 * @param  ap  A pointer to the buffer containing the annotation data to be
 * written.
 * @param  size  The number of bytes of data to be written.
 * @return  1 if 'size' bytes of data were successfully written; otherwise 0.
 */
int write_annotation(char *ap, unsigned int size){

    unsigned char c;
    char val;

    for (int i = 0; i < size; i++) {
        c = *ap;
        val = putchar(c);
        if (val == EOF) return 0;
        ap++;
    }

    return 1;
}

/**
 * @brief Read, from the standard input, a single frame of audio data having
 * a specified number of channels and bytes per sample.
 * @details  This function takes a pointer 'fp' to a buffer having sufficient
 * space to hold 'channels' values of type 'int', it reads
 * 'channels * bytes_per_sample' data bytes from the standard input,
 * interpreting each successive set of 'bytes_per_sample' data bytes as
 * the big-endian representation of a signed integer sample value, and it
 * stores the decoded sample values into the specified buffer.
 *
 * @param  fp  A pointer to the buffer that is to receive the decoded sample
 * values.
 * @param  channels  The number of channels.
 * @param  bytes_per_sample  The number of bytes per sample.
 * @return  1 if a complete frame was read without error; otherwise 0.
 */
int read_frame(int *fp, int channels, int bytes_per_sample){

    char* charPointer = (char*) fp;
    int frameBytes = bytes_per_sample * channels;

    unsigned char c;
    int i = 0;

    while (i < frameBytes) {
        c = getchar();
        if (c == EOF) return 0;
        if (i - 1 == bytes_per_sample) {
            fp++;
            charPointer = (char*) fp;
        }
        *charPointer = c;
        charPointer++;
        i++;
    }

    return 1;
}

/**
 * @brief  Write, to the standard output, a single frame of audio data having
 * a specified number of channels and bytes per sample.
 * @details  This function takes a pointer 'fp' to a buffer that contains
 * 'channels' values of type 'int', and it writes these data values to the
 * standard output using big-endian byte order, resulting in a total of
 * 'channels * bytes_per_sample' data bytes written.
 *
 * @param  fp  A pointer to the buffer that contains the sample values to
 * be written.
 * @param  channels  The number of channels.
 * @param  bytes_per_sample  The number of bytes per sample.
 * @return  1 if the complete frame was written without error; otherwise 0.
 */
int write_frame(int *fp, int channels, int bytes_per_sample){

    char* charPointer = (char*) fp;
    int frameBytes = bytes_per_sample * channels;
    unsigned char c;
    char val;
    int i;

    for (i = 0; i < frameBytes; i++) {
        c = *charPointer;
        val = putchar(c);
        if (val == EOF) return 0;
        charPointer++;
    }

    return 1;
}

unsigned int reverseNumHelper(unsigned int hexNum) {

    unsigned int realVal = 0;

    unsigned int mask;
    int i;

    mask = 0x000000ff;
    realVal = (hexNum & mask) << 24;

    mask = 0x0000ff00;
    realVal = realVal | ((hexNum & mask) << 8);

    mask = 0x00ff0000;
    realVal = realVal | ((hexNum & mask) >> 8);

    mask = 0xff000000;
    realVal = realVal | ((hexNum & mask) >> 24);

    return realVal;
}

int argCount(char** argv) {
    char **arrAdd = argv;
    int count = 0;
    while (*arrAdd != NULL) {
        count++;
        arrAdd++;
    }
    return count;
}

int pGiven(char* input_annotation, char* output_annotation) {

    unsigned char c;

    c = *input_annotation;
    while (c != '\0') {
        *output_annotation = c;
        output_annotation++;
        input_annotation++;
        c = *input_annotation;
    }
    *output_annotation = c;

    return 1;
}

int noAnn(char* output_annotation, char** argv, unsigned int* sizeP){

    char **arrAdd = argv;
    char *charPointer = *arrAdd;
    unsigned char c;

    int totalBytes = 0;

    while (charPointer != NULL) {
        c = *charPointer;
        if (c == '\0') {
            *output_annotation = ' ';
            arrAdd++;
            charPointer = *arrAdd;
        }
        else  {
            *output_annotation = c;
            charPointer++;
        }
        totalBytes++;
        output_annotation++;
    }
    output_annotation--;
    totalBytes--;
    *output_annotation = '\n';
    totalBytes++;
    output_annotation++;

    while (totalBytes % 8 != 0) {
        totalBytes++;
        *output_annotation = '\0';
        output_annotation++;
    }

    if (totalBytes > ANNOTATION_MAX) return 0;
    *sizeP = totalBytes;

    return totalBytes;
}

int yesAnn(char* input_annotation, char* output_annotation, char** argv, unsigned int* sizeP){

    char **arrAdd = argv;
    char *charPointer = *arrAdd;
    unsigned char c;

    int totalBytes = 0;

    while (charPointer != NULL) {
        c = *charPointer;
        if (c == '\0') {
            *output_annotation = ' ';
            arrAdd++;
            charPointer = *arrAdd;
        }
        else  {
            *output_annotation = c;
            charPointer++;
        }
        totalBytes++;
        output_annotation++;
    }
    output_annotation--;
    totalBytes--;
    *output_annotation = '\n';
    totalBytes++;
    output_annotation++;

    c = *input_annotation;
    while (c != '\0') {
        *output_annotation = c;
        totalBytes++;
        output_annotation++;
        input_annotation++;
        c = *input_annotation;
    }

    while (totalBytes % 8 != 0) {
        totalBytes++;
        *output_annotation = '\0';
        output_annotation++;
    }

    if (totalBytes > ANNOTATION_MAX) return 0;
    *sizeP = totalBytes;

    return totalBytes;
}

int inOutFrame(char* input_frame, char* output_frame, int channels, int bytes_per_sample){

    unsigned char c;
    int i;

    int frameBytes = bytes_per_sample * channels;

    for (i = 0; i < frameBytes; i++) {
        c = *input_frame;
        *output_frame = c;
        output_frame++;
        input_frame++;
    }

    return 1;
}

int crypt(int* input_frame, int* output_frame, int channels, int bytes_per_sample){

    int randomNum;

    int i = 0;

    while(i < channels) {
        unsigned int input;
        input = *input_frame;
        randomNum = myrand32();
        input = input ^ randomNum;
        *output_frame = input;
        input_frame++;
        output_frame++;
        i++;
    }
    return 1;
}