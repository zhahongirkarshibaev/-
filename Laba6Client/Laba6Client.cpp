#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <synchapi.h>
#include <time.h>
#include <string.h>
#include <mswsock.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "27016"
#define SIZE_PART_FILE 1024

WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo* result = NULL, * ptr, hints;
HANDLE resurs = 0;
DWORD WINAPI ThreadRecv(LPVOID LP);

void* fName_Cut(char buf[MAX_PATH], char* cmd, char fNAME[MAX_PATH])
{
    int j = 0;
    int i = strlen(cmd);
    for (i; i < strlen(buf); i++) {
        fNAME[j] = buf[i];
        j++;
    }
    fNAME[j] += '\0';
}

int main(int argc, char** argv)
{
    resurs = CreateMutexA(0, FALSE, 0);

    HANDLE hThread = 0;
    DWORD IPthread = 0;

    int iResult;
    char buffer[SIZE_PART_FILE] = { 0 };

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
        continue;
    }    
    hThread = CreateThread(0, 0, ThreadRecv, 0, 0, &IPthread);
    if (hThread == INVALID_HANDLE_VALUE)
        goto close;

    BOOL isRunning_ = TRUE;
    while (isRunning_ == TRUE) {
        printf("Enter message:\n");
        char* rstr = gets(buffer);
        if (strncmp(buffer, "bye", strlen("bye")) == 0) {
            iResult = send(ConnectSocket, buffer, (int)strlen(buffer), 0);
            isRunning_ = FALSE;
        }
        else if (strncmp(buffer, "file-", strlen("file-")) == 0) {
            WaitForSingleObject(resurs, INFINITE);
            char fNAME[MAX_PATH] = { 0 };
            fName_Cut(buffer, (char*)"file- ", fNAME);
            HANDLE hFile = CreateFileA(fNAME,
                GENERIC_READ,       // считывание и запись файла доступны
                FILE_SHARE_READ,    // никакого обмена
                NULL,               // атрибуты безопасности по умолчанию
                OPEN_ALWAYS,        // открывать файлы всегда
                0,                  // атрибуты по умолчанию
                NULL);              // нет шаблона файла
            if (hFile != INVALID_HANDLE_VALUE) {
                send(ConnectSocket, "file- ", sizeof("file- "), NULL);
                DWORD dwBytesRead = 0;
                int size = GetFileSize(hFile, 0);
                iResult = send(ConnectSocket, (char*)&size, sizeof(size), NULL);
                if (iResult <= 0) {
                    printf("error send file size to server %d", GetLastError());
                    break;
                }
                printf("\n\t\tsize: %d\n", size);
                while (size > 0) {
                    memset(buffer, 0, sizeof(buffer));
                    BOOL right = ReadFile(hFile,
                        buffer,
                        sizeof(buffer),
                        &dwBytesRead,
                        NULL);
                    if (right == TRUE) {
                        printf("\t\tRead file! Size: %d\n", dwBytesRead);
                        for (int j = 0; j < dwBytesRead; j++)
                            printf("%0X", buffer[j]);

                        iResult = send(ConnectSocket, buffer, dwBytesRead, 0);
                        if (iResult < 0) {
                            printf("error send file to server %d", GetLastError());
                            break;
                        }
                        size = size - dwBytesRead;
                        printf("\n");
                    }
                }
            }
            else {
                printf("error created handle of file %s, code: %d\n", buffer, GetLastError());
                continue;
            }
            ReleaseMutex(resurs);
            CloseHandle(hFile);
        }
        else {
            iResult = send(ConnectSocket, buffer, (int)strlen(buffer), 0);
        }
        if (iResult == SOCKET_ERROR)
            isRunning_ = FALSE;
    }

    freeaddrinfo(result);
    printf("Client disconnected!\n");

    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
close:
    CloseHandle(hThread);
    CloseHandle(resurs);

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

DWORD WINAPI ThreadRecv(LPVOID LP)
{
    char* recvbuf[SIZE_PART_FILE] = { 0 };

    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        int iResult = recv(ConnectSocket, recvbuf, sizeof(recvbuf), 0);
        if (iResult > 0) {
            
            if (strncmp(recvbuf, "file-", strlen("file-")) == 0) {
                
                long int size = 0;
                iResult = recv(ConnectSocket, (char*)&size, sizeof(size), 0);
                printf("\n\t\tsize: %d\n", size);
                if (iResult > 0 && size > 0) {
                    char buff_file[SIZE_PART_FILE] = "C:\\Users\\Daniil\\Desktop\\NEW_FILE.exe";
                    // Занимаем мьютекс
                    WaitForSingleObject(resurs, INFINITE);
                    
                    HANDLE newFile = CreateFileA(buff_file,
                        GENERIC_ALL,    // считывание и запись файла доступны
                        0,              // никакого обмена
                        NULL,           // атрибуты безопасности по умолчанию
                        CREATE_NEW,  // создать новый файл
                        0,              // атрибуты по умолчанию 
                        0);             // нет шаблона файла
                    if (newFile == INVALID_HANDLE_VALUE) {
                        printf("failed create file code: %d", GetLastError());
                        continue;
                    }
                    printf("\t\tCreate file!\n");
                    
                    int ReturnCheck = 0;
                    int dwWrite = 0;
                    while (size > 0) {
                        ReturnCheck = recv(ConnectSocket, buff_file, sizeof(buff_file), NULL);
                        if (ReturnCheck == SOCKET_ERROR) {
                            printf("recv failed with error: %d\n", WSAGetLastError());
                            break;
                        }
                        
                        BOOL right = WriteFile(newFile, // дескриптор файла
                            buff_file, // буфер для записи
                            ReturnCheck, // количество байтов
                            &dwWrite, // Записанные байты
                            NULL); // IVERLOPED
                        if (right != TRUE) {
                            printf("failed writing to the file, code: %d\n", GetLastError());
                            break;
                        }
                        SetFilePointer(newFile, NULL, NULL, FILE_END);
                        printf("\t\tWrite file! size: %d\n", ReturnCheck);
                        for (int j = 0; j < dwWrite; j++)
                            printf("%0X", buff_file[j]);

                        size -= dwWrite;
                        memset(buff_file, 0, sizeof(buff_file));
                        ReturnCheck = 0;
                        dwWrite = 0;
                    }
                    ReleaseMutex(resurs);
                    CloseHandle(newFile);
                }
            }
            else {
                printf("New message: %s\n", recvbuf);
            }
        }
        else if (iResult == 0)
            continue;
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
        memset(recvbuf, 0, sizeof(recvbuf));
    }
}