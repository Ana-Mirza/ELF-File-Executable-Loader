Name: Mîrza Ana-Maria

Group: 321CA


# Tema 1 - File Executable Loader

Organization
-
In order to implement the loader for statically linked ELF file executables in linux, a handler function was used to treats page faults after intercepting the signal SIGSEGV, with the help of sigaction function. The handler searches the segment, then the specific page  in which the page fault was generated, using an internal structure containing the necessary information about the file. If the page fault occured in an unmmaped page, the handler mapps the page and copies the data from the file in the mapped area. Otherwise, the default handler is called. Mapped pages are efficiently stored for future reference using a bit-like array structure.


***Functions:*** 
* so_seg_t *loadSegment(...)
* void checkPage(...)
* void copyData(...)

***Improvements:*** 
* Some improvements that could be brought to this code are searching for the segment and page generating the page fault more efficiently, such as with a binary search algorithm. 
* Another improvement could be by extending the functionality for dynamically linked files as well.

Implementation
-

* The handler functionality is based on a demand page mechanism, meaning that each page is loaded in the memory when it is needed.
* For each detected page fault, the handler searches for the specific segment and page in the file that generated it.
* If the page is already loaded in memory or the the segment does not exist, the handler sees it as an invalid memory acces and calls the default handler.
* Otherwise, the page is loaded in memory with the use of the 
<span style="color:lightblue">*mmap()*</span> function that allocated the virtual memory at given address, and with given permissions.
* The last step is to copy the data from the file segment into the virtual memory using only POSIX functions, such as <span style="color:lightblue">*open, read, write, close*</span>, and also string functions and protect the mapped page.

How to compile and run?
-
* give the following command in linux bash
```
make run
```
* no arguments are needed, the executable uses included files as input

Bibliography
-

* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-05
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
* https://man7.org/linux/man-pages/