unsigned int reverseNumHelper(unsigned int hexNum);

int argCount(char** argv);

int pGiven(char* input_annotation, char* output_annotation);

int noAnn(char* output_annotation, char** argv, unsigned int* sizeP);

int yesAnn(char* input_annotation, char* output_annotation, char** argv, unsigned int* sizeP);

int inOutFrame(char* input_frame, char* output_frame, int channels, int bytes_per_sample);

int crypt(int* input_frame, int* output_frame, int channels, int bytes_per_sample);