# CS3013 Project 4

## Building the project
To build the project, simple run `make proj4`.

## Examples
```bash
# Demonstrate default chunking behavior
$ proj4 src/main.c print
File size: 8191 bytes.
Occurrences of the string "print": 24

# Demonstrates chunking behavior with chunk size overriden
$ proj4 src/main.c print 4096
File size: 8191 bytes.
Occurrences of the string "print": 24

# Demonstrates chunking behavior when the user tries to set the chunk size to
# an invalid value
$ proj4 src/main.c print 9999
warning: 9999 bytes exceeds the maximum chunk size of 8192 bytes; the maximum chunk size will be used instead
File size: 8191 bytes.
Occurrences of the string "print": 24

# Demonstrates default mmap behavior (single-threaded)
$ proj4 src/main.c print mmap
File size: 8191 bytes.
Occurrences of the string "print": 24

# Which is the same as...
$ proj4 src/main.c print p1
File size: 8191 bytes.
Occurrences of the string "print": 24

# Demonstrates multithreaded mmap behavior
$ proj4 src/main.c print p4
File size: 8191 bytes.
Occurrences of the string "print": 24
```

## Notes
There are some circumstances where the multithreaded and chunked searches will
yield different results, based on the simplistic search algorithm. Take for
instance a file containing the string "bababy". Performing a search of the
file for the string "baby" would return 0 counts if it was singlethreaded.
However, if it was multithreaded, and, for example, one thread started its
search at "baby", the second thread would count one occurrence, and thus one
would be counted overall.