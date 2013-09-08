#if !defined FILEOPS_H
#define FILEOPS_H


int send_putfile_messages(const char* localfilename, const char* remotefilename, int destfd);
int writefiledata(int argc, char** argv, int fd);

#endif // !defined FILEOPS_H

