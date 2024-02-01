// Version 1.0

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Flags representing the need to free the program's strings
#define DEST_STR_OWNED (1 << 0)
#define DEF_STR_OWNED  (1 << 1)

#define READ_BUFFER_SIZE 1024

// Convenience macro (because I want to try write() over printf()...)
#define ERRLOG(...) { \
    const char* args[] = { __VA_ARGS__ }; \
    for (size_t i = 0; i < sizeof(args) / sizeof(const char*); ++i)\
        write(STDERR_FILENO, args[i], strlen(args[i]));\
    }

// Checks whether the wrapped expression is equal to -1 and if so exits
#define EXIT_ON_NEG1(expression) if (expression == -1) ErrorExit()

// Convenience macro which wraps functions such as memcpy, strcpy, etc.
#define EXIT_ON_NULL(expression) if (!expression) ErrorExit()

// Called at invalid argument usage
void UsageError(const char* progName) {
    ERRLOG("Usage: ", progName, "[-o output] [-d define] [-f] filename\n");
    exit(EXIT_FAILURE);
}

// Logs the error and exits
void ErrorExit() {
    ERRLOG("Error: ", strerror(errno), "\n");
    exit(EXIT_FAILURE);
}

// Generates a default header name
char* GenerateHeaderName(uint8_t* ownershipFlag, const char* source, const size_t length) {

    // Copy source string
    char *header = malloc(length + 3); // +3 bytes for ".h\0"
    if (!header) {
        ErrorExit();
    }
    *ownershipFlag |= DEST_STR_OWNED;

    // Replace all '/' or '.' with '_'
    EXIT_ON_NULL(memcpy(header, source, length));
    for (char* c = header; c < header + length; ++c) {
        if (*c == '.' || *c == '/') {
            *c = '_';
        }
    }

    // Add .h extension and null terminator
    EXIT_ON_NULL(memcpy(header + length, ".h", 3));
    return header;
}

// Generates a default definition name
char* GenerateDefinitionName(uint8_t* ownershipFlag, const char* source, const size_t length) {

    // Copy source string
    char *definition = malloc(6 + length + 1); // +6 bytes for "EMBED_", + 1 for '\0'
    if (!definition) {
        ErrorExit();
    }
    *ownershipFlag |= DEF_STR_OWNED;

    // Begin string with prefix
    EXIT_ON_NULL(memcpy(definition, "EMBED_", 6));

    // Replace all '/' or '.' with '_' and capitalize
    EXIT_ON_NULL(memcpy(definition + 6, source, length));
    for (char* c = 6 + definition; c < definition + 6 + length; ++c) {
        if (*c == '.' || *c == '/') {
            *c = '_';
        }
        else {
            *c = toupper(*c);
        }
    }

    // Add null terminator
    definition[6 + length] = '\0';

    return definition;
}

// Generates the include guard header definition name
char* GenerateIGName(const char* header, const char* nullTerm) {

    // We only care about the local address to the header

    const char* begin = nullTerm;
    for (; begin > header && *(begin - 1) != '/'; --begin) {}

    const size_t length = nullTerm - begin; 
    
    // Copy header string
    char *IGDefinition = malloc(length + 1);
    if (!IGDefinition) {
        ErrorExit();
    }

    // Replace '.' with '_' and capitalize
    EXIT_ON_NULL(memcpy(IGDefinition, begin, length + 1));
    for (char* c = IGDefinition; c < IGDefinition + length; ++c) {
        if (*c == '.') {
            *c = '_';
        }
        else {
            *c = toupper(*c);
        }
    }

    return IGDefinition;
}

char* ByteToHexString(char* ptr, uint8_t byte) {
    static const char* hexadecimal = "0123456789ABCDEF";
    ptr[0] = hexadecimal[(byte & 0xF0) >> 4];
    ptr[1] = hexadecimal[byte & 0x0F];
    return ptr;
}

size_t ByteToDecString(char* ptr, uint8_t byte) {
    static const char* numerals = "0123456789";
    size_t currentChar = 0;
    
    uint8_t digit = byte / 100;
    if (digit) {
        ptr[currentChar] = numerals[digit];
        ++currentChar;
    }

    digit = (byte % 100) / 10;
    if (digit) {
        ptr[currentChar] = numerals[digit];
        ++currentChar;
    }

    ptr[currentChar] = numerals[byte % 10];
    ++currentChar;

    return currentChar;
}

void WriteIncludeGuardHeader(int outputFD, const char* includeGuardStr, size_t includeGuardSize) {
    EXIT_ON_NEG1(write(outputFD, "#ifndef ", 8));
    EXIT_ON_NEG1(write(outputFD, includeGuardStr, includeGuardSize));
    EXIT_ON_NEG1(write(outputFD, "\n#define ", 9));
    EXIT_ON_NEG1(write(outputFD, includeGuardStr, includeGuardSize));
    EXIT_ON_NEG1(write(outputFD, "\n\n", 2));
}

void WriteIncludeGuardFooter(int outputFD) {
    EXIT_ON_NEG1(write(outputFD, "#endif", 6));
}

void EmbedSource(int outputFD, int sourceFD, const char* definitionStr, size_t definitionSize) {
    EXIT_ON_NEG1(write(outputFD, "#define ", 8));
    EXIT_ON_NEG1(write(outputFD, definitionStr, definitionSize));
    EXIT_ON_NEG1(write(outputFD, " ", 1));
        
    int placeComma = 0;
    uint8_t buffer[READ_BUFFER_SIZE];
    char byteStr[3];
    size_t chars;
    while(1) {
        ssize_t bytesRead = read(sourceFD, buffer, READ_BUFFER_SIZE);
        EXIT_ON_NEG1(bytesRead);
        if (!bytesRead) {
            break;
        }
        for (size_t i = 0; i < bytesRead; ++i) {
            if (placeComma) {
                EXIT_ON_NEG1(write(outputFD, ",", 1));
            }
            else {
                placeComma = 1;
            }
            //EXIT_ON_NEG1(write(destFD, "0x", 2));
            chars = ByteToDecString(byteStr, buffer[i]);
            EXIT_ON_NEG1(write(outputFD, byteStr, chars));
        }
    }
    EXIT_ON_NEG1(write(outputFD, "\n\n", 2));
}

int main(int argc, char ** argv) {

    // Expect arguments



    if (argc < 2) {
        UsageError(argv[0]);
    }

    uint8_t ownershipFlag = 0;

    // Declare option arguments
    // The strings do not point to const char, because a pointer to const cannot be freed

    char* output = NULL; // Name of the header
    char* definition = NULL; // Name of the generated macro
    int replaceFlag = O_EXCL; // Whether we should replace an existing destination 

    const char* sourcePath = NULL; // The file to embed

    // Parse arguments

    int opt;
    while ((opt = getopt(argc, argv, "o:d:f")) != -1) {
        switch (opt) {
            case 'o': 
                output = optarg;
                break;
            case 'd':
                definition = optarg;
                break;
            case 'f':
                replaceFlag = 0;
                break;
            case '?':
                UsageError(argv[0]);;
            default:
                UsageError(argv[0]);
        }
    }

    // We expect exactly one optionless argument, which is the source
    // Maybe in the future we can add the possibility of mulitplle sources

    if (optind != argc - 1) {
        UsageError(argv[0]);
    }

    sourcePath = argv[optind];

    // Store length of source once, to avoid redundant calls to strlen
    const size_t sourceLength = strlen(sourcePath);

    // Assign all unspecified variables with defaults

    if (!output) {
        output = GenerateHeaderName(&ownershipFlag, sourcePath, sourceLength);
    }

    if (!definition) {
        definition = GenerateDefinitionName(&ownershipFlag, sourcePath, sourceLength);
    }

    // Generate include guard definition

    char* includeGuard = GenerateIGName(output, output + strlen(output));
    const size_t IGSize = strlen(includeGuard);

    // Open the source file for reading

    int sourceFD = open(sourcePath, O_RDONLY);
    EXIT_ON_NEG1(sourceFD);

    // Open the output file for writing
    
    int outputFD = open(output, O_WRONLY | O_CREAT | O_TRUNC | replaceFlag, S_IRWXU);
    EXIT_ON_NEG1(outputFD);

    // Wtite the file

    WriteIncludeGuardHeader(outputFD, includeGuard, IGSize);
    EmbedSource(outputFD, sourceFD, definition, strlen(definition));
    WriteIncludeGuardFooter(outputFD);

    // Close files

    EXIT_ON_NEG1(close(outputFD));
    EXIT_ON_NEG1(close(sourceFD));

    // Free allocated dynamic memory
    free(includeGuard);
    if (ownershipFlag & DEF_STR_OWNED) {
        free(definition);
        ownershipFlag &= ~DEF_STR_OWNED;
    }
    if (ownershipFlag & DEST_STR_OWNED) {
        free(output);
        ownershipFlag &= ~DEST_STR_OWNED;
    }

    return 0;
}
