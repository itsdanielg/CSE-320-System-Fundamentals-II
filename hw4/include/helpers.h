void start_imp();
void list_help();
void split_string(char* inputString, char** arrayList, int wordCount);
int word_count(char* inputString);
char* get_extension(char* fileName);
int conversion_search(char* currentFileType, char* endFileType, int conversionCount, char*** allConversions);
int check_jobs();
int conversion_pipeline(char* currentFileType, char* endFileType, int conversionCount, char*** allConversions, JOB* currentJob);