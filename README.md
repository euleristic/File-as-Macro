# File-as-Macro
A Linux/GNU tool which embeds a file into your C/C++ code as a macro which expands to a comma-seperated list of bytes comprising the file. Upon execution it writes an output file (a header) which contains the macro definition. The tool's use is as a placeholder for C23's `#embed`, until it should become available for your project.

## Usage
Compile the main.c. It may be necessary to compile in the environment that you're going to compile the project using the embedded file, for character encoding compatibility reasons. Otherwise you may need to use a third party tool to translate the output of File-as-Macro to the needed encoding.

The output of the compilation will be refered to as File-as-Macro in this document.

### Synopsis

`File-as-Macro [OPTION]... SOURCE`

### Options
* `-o output` specifies the output file of the program. By default its name will be generated from the name of the source file and it's placed in the working directory. For the file `some/example.txt` the generated output name `some_example_txt.h`.

* `-d definition` specifies the name of the macro. By default it is generated from the name of the source file. For the file `some/example.txt`, the the output will define `EMBED_SOME_EXAMPLE_TXT`.

* `-f` "forces" the program to write the output. That is, it will replace any existing file at the path of the output.

## Notes
You, as the software developer, will sacrifice your disk space if you use this tool. It is for the benefit of the user, that, with any decent compiler optimization, should see no net increase size. You may even save some of their disk space, if you start processing the data contained in the file at preprocess or compile time. However, it should always be a runtime performance gain. The way that File-as-Macro is set up at this stage (v.1.0), the output file is going to be roughly 3.5 times the size of the source file. So, whenever `#embed` becomes available for you, *prefer that*.

It is not unfeasable that some size reduction should be possible by use of a compression algorithm, which would decompress upon macro expansion. However, upon consideration I've opted not to implement this (even though it does sound fun), because of the problems that would arise. Any macro symbol that would be used inside the output definition (the embedded file), necessarily needs to be small, so that it actually saves characters. This renders that symbol unusable for the rest of the translation unit, which sounds terrifying. Imagine not being able to use, say, `A` as a symbol. It cannot be `#undef`ed, and it may also override a previous macro. Passing the symbol as an argument to File-as-Macro could be an option. If you know of a clean way to go about such a compression, please let me know. Until then, anyone who wishes to save disk space may use the tool as part of the build process, perhaps, and dispose of the header after.

This project is mostly for-fun. I wrote it to practice my C and use of the Linux/GNU standard C-library (which is why there is no `printf`). Porting it to cross-platform C or C++ should be trivial, but I won't do it unless someone asks me to. In C++, you might even be able to compress it using the language's compile-time utilites (or in a few years, reflection).