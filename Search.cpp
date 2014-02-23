/*
 * EECS 3540 Project 1 - Search
 * Author: Dylan Drake Rush
 * Date: February 4, 2014
 *
 * This program takes a string as input, and searches though every file
 * in the directory, as well as every subdirectory in the main directory
 * that the program was initially ran from.  If the input string is found,
 * the full path to the file that it was found in is printed out, as well
 * as the line number the string was found on, and the line contents.
 * Nothing is printed out if the string is not found.
 */

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct entry
{
    std::string fileName;
    mode_t fileType;
};

enum mode_type
{
    RegularFile,
    Directory,
    CharacterSpecialDevice,
    BlockSpecialDevice,
    SymbolicLink,
    FIFO,
    UnixDomainSocket,
    Unknown
};

// Prototypes
mode_type           decodeMode      (mode_t mode);
std::vector<entry>  GetFileList     (DIR *directory);
void                SearchDirectory (std::vector<entry> fileList);
void                SearchFile      (std::vector<entry> fileList);

// Global
std::string searchString;

int main (int argCount, char* argVaules[])
{
    DIR  *directory;
    char *arguments[] = {argVaules[1]};
    char *envp[] = { NULL };
    int conditionCode;
    std::vector<entry> files = std::vector<entry>();
    if (argCount == 1)
    {
        std::cout << "Run the program with a string to search for..."
                  << std::endl;
    }

    else
    {
        searchString = argVaules[1];
        directory = opendir(".");
        if (directory != NULL)
        {
            files = GetFileList(directory);
            SearchFile(files);
            SearchDirectory(files);
        }
    }
}

/*
 * Returns the type of a file.
 */
mode_type decodeMode(mode_t mode)
{
    switch (mode & S_IFMT)
    {
        case S_IFREG:
            return RegularFile;
        case S_IFDIR:
            return Directory;
        case S_IFCHR:
            return CharacterSpecialDevice;
        case S_IFBLK:
            return BlockSpecialDevice;
        case S_IFLNK:
            return SymbolicLink;
        case S_IFIFO:
            return FIFO;
        case S_IFSOCK:
           return UnixDomainSocket;
        default:
           return Unknown;
    }
}

/*
 * Puts all the objects in a directory into a vector.  All of the vector
 * entries include both the objects name, as well as data indicating
 * the type of the object (i.e. file, directory, etc.)
 */
std::vector<entry> GetFileList(DIR *directory)
{
    struct dirent *dirEntry;
    struct stat fileInfo;
    std::string fileName;
    std::vector<entry> files = std::vector<entry>();
    int i = 0;
    while ((dirEntry = readdir(directory)) != NULL)
    {
        fileName = "./";
        fileName = fileName + dirEntry->d_name;
        lstat(fileName.c_str(), &fileInfo);

        /* The following lines work with C++09 */
        //files.push_back(entry());
        //files[i].fileName = fileName;
        //files[i].fileType = decodeMode(fileInfo.st_mode);
        //i++;

        /* The following line works with C++ll, but not C++09... */
        files.push_back({fileName, decodeMode(fileInfo.st_mode)});
    }
    return files;
}

/*
 * Searches though all the objects in a directory for additional
 * directories.  If there are additional directories, fork a child
 * process and the child will go into the subdirectory and search
 * the files in the subdirectory for the search string, and if
 * there are additional directories, spawn more children.  The
 * child process is ended once all of the subdirectories under
 * the child's directory have been searched.
 */
void SearchDirectory(std::vector<entry> fileList)
{
    DIR *subDirectory;
    std::vector<entry> subDirFileList = std::vector<entry>();
    pid_t childID;

    for (int i = 0; i < fileList.size(); i++)
    {
        if (fileList[i].fileType == Directory)
        {
            if (fileList[i].fileName != "./."
            && fileList[i].fileName != "./..")
            {
                // Only a child process should go into a directory
                childID = fork();
                if (childID == 0) // I am the child
                {
                    chdir((fileList[i].fileName).c_str());
                    subDirectory = opendir(".");
                    if (subDirectory != NULL)
                    {
                        subDirFileList = GetFileList(subDirectory);
                        SearchFile(subDirFileList);
                        SearchDirectory(subDirFileList);
                        exit(0);
                    }
                }
                else if (childID > 0) // I am the parent
                {
                    waitpid(childID, NULL, 0); // Wait for child to finish
                }
                else
                {
                    std::cout << "There was some kind of error : [" 
                              << std::endl;
                }
            }
        }
    }
}


/*
 * Open up any file in a directory and search though said file
 * for the specified string.  Prints out the path to the file,
 * the line number, and the contents of the line the string
 * was found on.
 */
void SearchFile(std::vector<entry> fileList)
{
    std::ifstream fileInput;
    std::string cleanFileName, line;
    char cCurrentPath[FILENAME_MAX];

    getcwd(cCurrentPath, sizeof(cCurrentPath));
    cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';

    for (int i = 0; i < fileList.size(); i++)
    {
        if (fileList[i].fileType == RegularFile)
        {
            cleanFileName = fileList[i].fileName;   // Remove the ./ before 
            cleanFileName.erase(0, 1);              // the file name.
            
            fileInput.open((fileList[i].fileName).c_str());
            int curLine = 0;
            while(std::getline(fileInput, line))
            {
                curLine++;
                if (line.find(searchString, 0) != std::string::npos)
                {
                    std::cout << cCurrentPath  << cleanFileName << "\t"
                              << ": Line: "    << curLine       << " "
                              << line          << std::endl;
                }
            }
            fileInput.close();
        }
    }
}

