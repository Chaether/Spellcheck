// Implements a spell-checker
#include <cs50.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdlib.h>
//Include string.h - for using string functions
#include <string.h>

#define LENGTH 45
#undef calculate
#undef getrusage

// Prototype for time calculation
double calculate(const struct rusage *b, const struct rusage *a);

// Define a node in the hash table, with a variable for the string and a pointer
typedef struct node
{
    char word[LENGTH + 1];
    struct node *next;
}
node;

// Number of bins in hash table start off with 26*26 (676)
const unsigned int N = 676;

// Hash table - has length of constant integer defined above
node *table[N];

int dictionarysize;

//Initialize functions
bool load(const char *dictionary);
unsigned int size(void);
bool unload(void);
unsigned int hash(const char *word);
bool check(const char *word);


//Initialize table nodes to null
void init(void){
    int j;
    //memset(hash_table,0,sizeof(hash_table));
    for(j=0; j<N; j++){
        table[j]->next = NULL;
    }
}
// Default dictionary (in remote IDE server this is the location of the dictionary to use)
#define DICTIONARY "dictionaries/large"

// Prototype for timing calculation
double calculate(const struct rusage *b, const struct rusage *a);


//Main part of program - checks initially entered arguments - either "Spellcheck dictionary text" or "Spellcheck text" are accepted
//Then loads list of words into hash table, and then checks the text file, reporting any mismatches
int main(int argc, char *argv[])
{
    // Check for correct number of args
    if (argc != 2 && argc != 3)
    {
        printf("Usage: ./speller [DICTIONARY] text\n");
        return 1;
    }

    // Structures for timing data
    struct rusage before, after;

    // Benchmarks
    double time_load = 0.0, time_check = 0.0, time_size = 0.0, time_unload = 0.0;
    // Determine dictionary to use
    char *dictionary = (argc == 3) ? argv[1] : DICTIONARY;

    // Load dictionary
    getrusage(RUSAGE_SELF, &before);
    bool loaded = load(dictionary);
    getrusage(RUSAGE_SELF, &after);

    // Exit if dictionary not loaded
    if (!loaded)
    {
        printf("Could not load %s.\n", dictionary);
        return 1;
    }


    // Calculate time to load dictionary
    time_load = calculate(&before, &after);




        // Try to open text
    char *text = (argc == 3) ? argv[2] : argv[1];
    FILE *file = fopen(text, "r");
    if (file == NULL)
    {
        printf("Could not open %s.\n", text);
        unload();
        return 1;
    }

    // Prepare to report misspellings
    printf("\nMISSPELLED WORDS\n\n");

    // Prepare to spell-check
    int index = 0, misspellings = 0, words = 0;
    char word[LENGTH + 1];

    // Spell-check each word in text
    for (int c = fgetc(file); c != EOF; c = fgetc(file))
    {
        // Allow only alphabetical characters and apostrophes
        if (isalpha(c) || (c == '\'' && index > 0))
        {
            // Append character to word
            word[index] = c;
            index++;

            // Ignore alphabetical strings too long to be words
            if (index > LENGTH)
            {
                // Consume remainder of alphabetical string
                while ((c = fgetc(file)) != EOF && isalpha(c));

                // Prepare for new word
                index = 0;
            }
        }

        // Ignore words with numbers
        else if (isdigit(c))
        {
            // Consume remainder of alphanumeric string
            while ((c = fgetc(file)) != EOF && isalnum(c));

            // Prepare for new word
            index = 0;
        }

        // Must find a whole word
        else if (index > 0)
        {
            // Terminate current word
            word[index] = '\0';

            // Update counter
            words++;

            // Check word's spelling
            getrusage(RUSAGE_SELF, &before);
            bool misspelled = !check(word);
            getrusage(RUSAGE_SELF, &after);

            // Update benchmark
            time_check += calculate(&before, &after);

            // Print word if misspelled
            if (misspelled)
            {
                printf("%s\n", word);
                misspellings++;
            }

            // Prepare for next word by resetting index counter
            index = 0;
        }
    }

    // Check whether there was an error
    if (ferror(file))
    {
        fclose(file);
        printf("Error reading %s.\n", text);
        unload();
        return 1;
    }

    // Close text
    fclose(file);














    // Determine dictionary's size
    getrusage(RUSAGE_SELF, &before);
    unsigned int n = size();
    getrusage(RUSAGE_SELF, &after);

    // Calculate time to determine dictionary's size
    time_size = calculate(&before, &after);

    // Unload dictionary
    getrusage(RUSAGE_SELF, &before);
    bool unloaded = unload();
    getrusage(RUSAGE_SELF, &after);

    // Abort if dictionary not unloaded
    if (!unloaded)
    {
        printf("Could not unload %s.\n", dictionary);
        return 1;
    }

    // Calculate time to unload dictionary
    time_unload = calculate(&before, &after);

        // Report benchmarks
    printf("\nWORDS MISSPELLED:     %d\n", misspellings);
    printf("WORDS IN DICTIONARY:  %d\n", n);
    printf("WORDS IN TEXT:        %d\n", words);
    printf("TIME IN load:         %.2f\n", time_load);
    printf("TIME IN check:        %.2f\n", time_check);
    printf("TIME IN size:         %.2f\n", time_size);
    printf("TIME IN unload:       %.2f\n", time_unload);
    printf("TIME IN TOTAL:        %.2f\n\n",
           time_load + time_check + time_size + time_unload);

    // Success
    return 0;
}

// Hashes word to a number (use same formula when loading the words from dictionary into memory and when checking)
unsigned int hash(const char *word)
{
    //First, initialize and set two integers to equal the first two characters of the word
    int char1 = (int) word[0];
    int char2;
    //Initialize binnumber
    int binnumber;

    //Think of cases like "I", "I'll" etc. -- > set char2 to 0 if the word is only 1 letter long or if it is less than 0
    if (word[1] != '\0')
    {
        char2 = (int) word[1];
        if ((((char2) - (int) 'a') < 0) || (((char2) - (int) 'a') > 26))
        {
            char2 = 0;
            binnumber = (26 * (char1 - (int) 'a'));
        }
        else
        {
            binnumber = ((26 * (char1 - (int) 'a')) + (char2 - (int) 'a'));
        }
    }
    else
    {
        //Set character 2 to null terminator (or 0, should be the same)
        char2 = 0;
        binnumber = (26 * (char1 - (int) 'a'));
    }

    //Formula for the bin: Bin# = ((26*(char1 - (int) "a")) + (char2 - (int) "a")) (int) "a" is 97
    //For three bins, total number is 17576, formula is: int binnumber = ((676*((int) char2 - (int) "a")) + (26*((int) char2 - (int) "a")) + ((int) char3 - (int) "a"));
    //printf("Char1 is %i, Char2 is %i\n", char1, char2);
    //int binnumber = ((26*(char1 - (int) 'a')) + (char2 - (int) 'a'));
    //printf("First is %c, second is %c, whole is %s, binnumber is %i\n", word[0], word[1], word, binnumber);
    return binnumber;
}

// Loads dictionary into memory, returning true if successful else false
bool load(const char *dictionary)
{
    //Initialize a counter here
    int counter = 0;

    // Store filename for input dictionary file and open input (dictionary)
    FILE *file = fopen(dictionary, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", dictionary);
        return 4;
    }
    //For each character in each word in the dictionary, if the character is not nextline, add it to the word (use the first two characters to decide where the word fits (think of cases for single letter words))
    //First define an array of chars, with the size of the max length of a word
    char word[LENGTH + 1];

    //Use fgets to read file --> usage = fgets(str, int, stream) --> it reads each line from the input stream into the string specified by str until int-1 characters, end of file or newline
    while (fgets(word, sizeof(word), file))
    {
        //Don't have to worry about uppercase characters here, as all words in input dictionary will be lowercase
        if (!isalpha(word[0]))
        {
            continue;
        }
        //Once word is finished reading, make sure that it terminates with a null character
        //Strcpsn calculates the length of the first string inputted until the character from the second string is matched
        //- so when the newline is found, its place in the char array will be replaced with the null terminator
        word[strcspn(word, "\n")] = 0;

        //Use function hash to calculate a binnumber for the word
        int binnumber = hash(word);

        node *n = malloc(sizeof(node));
        if (n == NULL)
        {
            return 1;
        }
        strcpy(n->word, word);
        //printf("Word size is %lu\n", sizeof(n->word));
        //printf("Word %s stored in table as %s\n", word, n->word);
        n->next = NULL;
        //printf("Word %s stored in table as %s\n", word, n->word);
        if (table[binnumber] == '\0')
        {
            table[binnumber] = n;
            counter++;
        }
        else
        {
            for (node *temp = table[binnumber]; temp != NULL; temp = temp->next)
            {
                if (temp->next != NULL)
                {
                    continue;
                }
                else
                {
                    temp->next = n;
                    counter++;
                    break;
                }
            }
        }
        //printf("Word %s stored in table as %s\n", word, table[binnumber]->word);
    }

    //If not loaded correctly then return false, if one or more words loaded then return true
    dictionarysize = counter;
    if (counter > 0)
    {
        fclose(file);
        return true;
    }
    else
    {
        fclose(file);
        return false;
    }
}
// Function for checking words, returns true if word is in dictionary else false
bool check(const char *word)
{
    //Input is a constant - want to copy to another string so we can convert to lowercase.
    //First, check to make sure it is all lower case (use isupper and tolower), and there are no characters missing
    char *copy = malloc(LENGTH + 1);
    strcpy(copy, word);
    //printf("The word sent to hash is %s\n", copy);

    int i = 0;

    while (copy[i])
    {

        if (isalpha(copy[i]) && isupper(copy[i]))
        {
            copy[i] = (tolower(copy[i]));
        }
        i++;
    }

    //Calculate binnumber using hash function to calculate bin that it's in
    int binnumber2 = hash(copy);

    //Initialize integer to serve as match counter
    int matchcounter = 0;

    //Then iterate through nodes in bin and use strcpy to check if it is the same. If none match (increment match counter, then reset at the end) then return false, otherwise return true
    //Next, compare the word with the words in the binnumber recursively, incrementing counter if successful
    for (node *tempnode = table[binnumber2]; tempnode != NULL; tempnode = tempnode->next)
    {
        if (strcmp(copy, tempnode->word) == 0)
        {
            matchcounter++;
            break;
        }
    }

    //Free the word after comparison is completed
    free(copy);

    //Return true if a match is found, otherwise return false
    if (matchcounter > 0)
    {
        return true;
    }
    else
    {
        return false;
    }

}

// Returns number of words in dictionary if loaded else 0 if not yet loaded
unsigned int size(void)
{
    return dictionarysize;
}

// Unloads dictionary from memory, returning true if successful else false
bool unload(void)
{
    // TODO
    //Initialize counter
    int count = 0;

    //For every bin in the hash table, follow the list and free all values
    for (int i = 0; i < N; i++)
    {
        while (table[i] != NULL)
        {
            node *temp = table[i]->next;
            free(table[i]);
            count++;
            table[i] = temp;

        }

    }
    //If anything was unloaded, count will be greater than 0, so return true, otherwise return false.
    if (count > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


// Returns number of seconds between b and a
double calculate(const struct rusage *b, const struct rusage *a)
{
    if (b == NULL || a == NULL)
    {
        return 0.0;
    }
    else
    {
        return ((((a->ru_utime.tv_sec * 1000000 + a->ru_utime.tv_usec) -
                  (b->ru_utime.tv_sec * 1000000 + b->ru_utime.tv_usec)) +
                 ((a->ru_stime.tv_sec * 1000000 + a->ru_stime.tv_usec) -
                  (b->ru_stime.tv_sec * 1000000 + b->ru_stime.tv_usec)))
                / 1000000.0);
    }
}
