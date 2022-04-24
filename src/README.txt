P3 TEST PLAN
------------

- Our primary stategy for testing the output of ww, our multithreaded word wrapping program was to run wcheck on our wrap files for our different test cases

- For smaller files where errors can be easily detected using gdb, along with wcheck, I used a character counter to ensure the program was not just making every line smaller than the provided line size

- Visually observing output of files for consistency among spacing(articles/book that have spacing structure which includes '\n')

- Smaller file test cases include: (tested at various arbitrary line sizes)
	- Empty file
	- File with one character
	- p2.pdf example
	- p2.pdf example with white spaces before the first character
	- p2.pdf example with a sequence of white spaces characters(not including ' ' or '\n') before the two consective newlines(empty line 6) and following the two consecutive newlines(empty line 6 still present)
	- p2.pdf example wrapped to line size 1(to observe if the two consecutive newlines get cut out, which they don't)
	- p2.pdf example with random sequences of white spaces(not including ' ' or '\n') between words(many variations of this)  
	- File containing only white spaces(empty file created) 
	- File with one character name(ensure string methods wont cause memory leak)
	
- Large files include: (tested at various arbitrary line sizes) 
	- Of Mice and Men
	- The Pearl
	- Animal Farm
	- Harry Potter and the Sorcerers Stone
	- Harry Potter and the Prisoner of Azkaban 
	- Alice's Adventures in Wonderland
	- Random Latin text generated online
	- Various random news articles

- General Tests include: (tested at various arbitrary line sizes)  
	- wrapping our test files, re-wrapping the output, then comparing the re-wrapped output with the original output using cmp
	- re-wrapping/re-normalizing already wrapped/normalized text in dynamic buffer without memoization(included memoization after)
	- running every test including -r tests using valgrind to detect memory leaks

- Larger files contained so many bytes that we could not visually observe the validity of each line hence we used wcheck
	- For all files in our test cases the only error reported by wcheck was line to big, for each line reported we checked if that particular line only contained non white space characters 

-r Scenario
-----------
 
- Created a tests directory which contained 10 directories, each one of those directories contained 3 directories, and a level down each one of those directories contained 3 more directories, followed by one of level containing 3 more directories.

- Each directory within tests contained every one of our file tests.

- We ran ww with an arbitrary number of threads, each time ensuring wcheck didn't report errors for any of our wrap files

- Since wcheck reported line size error for our larger files, we ensured the same line sizes were reported as too large across various directories for the same wrap files

Error Reporting
---------------

- ww prints an invalid argument error when the line size it too long in a file with the specified path of the file

- ww prints an operation not permitted error when the output file cannot be created or when an input file cant be read

- ww prints an invalid argument error when the arguments passed to ww do match the specifications in p3.pdf and p2.pdf

- To test error reporting:
	- Tested files with word sizes longer than the line size passed to ww
	- Passed invalid file and directory arguments to ww
	- Could not test output file case because all the files passed to ww either created output files if a file name was valid otherwise invalid argument error was printed.


