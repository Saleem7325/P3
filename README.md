# Description
- C program that wraps characters to match specified line width

- When given a file and a valid(positive) line width:
  -  ww wraps the characters in the input file such that each line in the file contains at most line width number of characters. 
  -  If a word contains more characters than the input line width, the word is included a line and an error message is printed.
  -  Prints the wrapped file to STDOUT.

- When given a directory and a valid line width:
  - ww wraps each file in the input directory as specified in file scenario above. 
  - For each file that is wrapped in the input directory, a new file is created in that directory with the name <code>wrap.file_name</code> where file_name is the name of pertaining file.
  - If the first arg passed to ww is <code>-r</code>, ww will wrap files in sub-directories.
  
- When given only a valid line width:
  - ww reads characters from STDIN
  - prints wrapped text to STDOUT
  
# Using Word Wrap
- <code>ww.c</code> can be compiled by running <code>make</code> command in the same directory as <code>ww.c</code>
  - <code>ww</code> the executable file created from running <code>make</code>, is ran as follows:

- <code>./ww line_size</code> 
  - line_size must be a positive integer.
  - Reads from STDIN, writes to STDOUT after pressing <code>CTRL-D</code> to indicate EOF

- <code>./ww line_size file_path</code>
  - file_path must be a valid file.
  - Reads from file, writes wrapped characters to STDOUT.

- <code>./ww line_size dir_path</code>
  - dir_path must be a valid directory.
  - Reads every file in directory dir_path, writes wrapped characters for each file to file <code>wrap.file_name</code> in dir_path.
    - <code>file_name</code> is the name of the original non-wrapped file.
  
- <code>./ww -r line_size dir_path</code>
  - Reads every file in dir_path and sub-directories in dir_path
  - Writes wrapped characters to file <code>wrap.file_name</code> in pertaining sub-directories.

- <code>./ww -rN line_size dir_path</code>
  - N is a positive integer indicating the number of threads to create for reading/wrapping files in dir_path and its sub-directories.

- <code>./ww -rM,N line_size dir_path</code>
  - M is a positive integer indicating the number of threads to create for reading files/directories in dir_path and its sub-directories.
  - N is a positive integer indicating the number of threads to create for reading/wrapping files in dir_path and its sub-directories.

# Test Plan
- Primary stategy for testing the output of ww, was to run <code>wcheck</code> on output for different test cases.

- <code>wcheck.c</code> is a program that recieves a line size and file name as arguments, and reports if any lines in input file are greater than input line size. 

- For smaller files where errors can be seen, along with wcheck, using a character counter to ensure lines are maximum width and less than or equal to line size.

- Visually observing output files for consistency among spacing(articles/books that have spacing structure which includes '\n')

- Smaller file test cases:  (tested at various arbitrary line sizes)
	- Empty file
	- File with one character
	- p2.pdf example
	- p2.pdf example with white spaces before the first character
	- p2.pdf example with a sequence of white spaces characters(not including ' ' or '\n') before the two consective newlines(empty line 6) and following the two consecutive newlines(empty line 6 still present)
	- p2.pdf example wrapped to line size 1(to observe if the two consecutive newlines get cut out, which they don't)
	- p2.pdf example with random sequences of white spaces(not including ' ' or '\n') between words(many variations of this)  
	- File containing only white spaces(empty file created) 
	- File/Directory with one character name

- Large files test cases:  (tested at various arbitrary line sizes) 
	- Of Mice and Men
	- The Pearl
	- Animal Farm
	- Harry Potter and the Sorcerers Stone
	- Harry Potter and the Prisoner of Azkaban 
	- Alice's Adventures in Wonderland
	- Random Latin text generated online
	- Various random news articles

- General Tests include: (tested at various arbitrary line sizes)  
	- wrapping test files, re-wrapping the output, then comparing the re-wrapped output with the original output using cmp
	- re-wrapping already wrapped text in dynamic buffer without memoization(included memoization after)
	- running every test including -r tests using valgrind to detect memory leaks

- Larger files contained so many bytes that we could not visually observe the validity of each line hence we used wcheck
	- For all files in our test cases the only error reported by wcheck was line to big, for each line reported we checked if that particular line only contained non white space characters

- <code>-r</code> test cases: (tested at various arbitrary file/directory thread counts)
  - Directories with an arbitary number of sub-directories where each directory/sub-directory contained every test file.
    - Ensured the same line sizes were reported as too large across various directories for the same wrap files. 
  - Directory with no sub-directories or files.
  - Directories with an arbitrary number of sub-directories and no files.
  
## Notes
- Default number of file/directory threads is 1.

- Upper bound on threads depends on how many <code>pthread_t</code>'s can be stored in memory at runtime, this varies depending on system/OS.

- All testing was done on Rutgers iLab machines.

  
  
