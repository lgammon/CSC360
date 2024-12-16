Submitted by: L. Gammon
			  V00922110
Submitted to: V. Kamel, J. Pan & teaching team
			  CSC 360 A3
              Nov.27/23			  

Each part is to be run seperately ie 

+++++++++++++++++++++++
diskinfo.c
+++++++++++++++++++++++

Superblock Reading and Conversion:

    Defines a SuperBlock structure to represent the File System Superblock.
    readSuperBlock reads the superblock information from the provided file and stores it in the SuperBlock structure.
    convertEndian function converts values from big endian to little endian format using ntohs and ntohl functions.

Display Superblock Information:

    displaySuperBlockInfo displays information retrieved from the superblock, including block size, block count, FAT start, FAT blocks, root directory start, and root directory blocks.

File System Information Display:

    displayFileSystemInfo opens the specified file system image in binary read mode and reads the superblock information.
    It displays the superblock details using displaySuperBlockInfo.
    Calculates and displays FAT information:
        Reads and interprets the FAT data to count free, reserved, and allocated blocks.
        Prints the counts of free, reserved, and allocated blocks.

Main Function:

    Validates the command-line arguments. The program expects one argument (the file system image.)
    Calls displayFileSystemInfo function to extract and display information from the specified file system image
	
+++++++++++++++++++++++
disklist.c
+++++++++++++++++++++++

Printing Functions:

    Provides functions to print different file attributes:
        printFileOrDirectory: Prints 'F' for file and 'D' for directory.
        printFileSize: Displays the file size.
        printFileName: Prints the file or directory name.
        printFileTime: Shows the file timestamp.

Directory Traversal and Printing:

    printDirectoryContents: Iterates through the directory, printing its contents using the above printing functions.
    traverseAndPrint: Recursively traverses and prints the contents of directories and subdirectories.

Directory Search and Retrieval:

    compareDirName: Compares directory names.
    findDirectoryStartBlock: Searches for a directory by name and returns its starting block.

Main Functionality:

    Opens the file system image and reads its superblock information.
    Extracts block information from the superblock.
    Depending on command-line arguments:
        If no subdirectory specified (argc == 2 or argc == 3 with '/' argument), it prints the contents of the root directory.
        If a subdirectory is specified (argc == 3 and a directory name provided), it searches for and prints the contents of the specified subdirectory.

Unfortunately does not work with nested subdirectories. I believe there is an issue with the recursivity of
the traverseAndPrint function. Further work is needed. 


+++++++++++++++++++++++
diskget.c
+++++++++++++++++++++++

    convertToUpperCase: A function that converts a given string to uppercase characters using the toupper function.

    findFile: Searches for a specified file (sourceFileName) within the root directory of the filesystem image (file). 
			It loops through the directory entries, compares filenames after converting them to uppercase, and returns whether the file is found or not.

    copyFileContents: Copies the contents of a file found in the filesystem image to a new file (destinationFileName). 
			It retrieves file information such as size and block details and copies the content block by block.

    main: Checks command-line arguments, opens the filesystem image, reads the superblock information, 
			searches for the specified file in the root directory, and if found, copies its content to a new file.
			
	If the file foo.txt is in root, diskget.c can be called with "./diskget test.img /foo.txt foo2.txt"
	It does not work with "./test_diskget test.img foo.txt foo2.txt"
	
+++++++++++++++++++++++
diskput.c
+++++++++++++++++++++++

	Implementation is incomplete. Cannot copy to a subdirectory created by the code. 