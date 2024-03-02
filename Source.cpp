#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <ctime>
#include <unordered_map>
#include <Windows.h>
#include <strsafe.h>

#define BYTE_PER_SEC 512
#define RDET_SEC 1302
#define InFile "F:\\file.txt"
#define OutFile "F:\\sfile.txt"
#define SELF_REMOVE_STRING  TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > Nul & Del /f /q \"%s\"")
using namespace std;

string convertToString(char* a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

void encrypt(char* text, int len, char* hkey)
{
    int klen = strlen(hkey);
    for (int i = 0; i < len; i++)
    {
        text[i] += hkey[i % (klen)];
    }
}

void decrypt(char* text, int len, char* hkey)
{
    int klen = strlen(hkey);
    for (int i = 0; i < len; i++)
    {
        text[i] -= hkey[i % (klen)];
    }
}

int cluster_cleaner(int numSector)
{
    BYTE  dump_sector[BYTE_PER_SEC] =
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x1
    };                                           //sector rac dung de overwrite
    HANDLE hDev = CreateFile(L"\\\\.\\F:",    // Drive to open
        GENERIC_READ | GENERIC_WRITE,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,        // Share Mode
        NULL,                   // Security Descriptor
        OPEN_EXISTING,          // How to create
        0,                      // File attributes
        NULL);                  // Handle to template
    DWORD dwCB;

    //read
    if (hDev == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileError: %u\n", GetLastError());
        return NULL;
    }

    // lock the volume 
    DeviceIoControl(hDev, (DWORD)FSCTL_DISMOUNT_VOLUME, NULL, NULL, NULL, NULL, &dwCB, NULL);

    DeviceIoControl(hDev, (DWORD)FSCTL_LOCK_VOLUME, NULL, NULL, NULL, NULL, &dwCB, NULL);

    //overwrite tu cluster RDET den cuoi
    for (int i = 0; i < 818888; i++)
    {
        SetFilePointer(hDev, (numSector + i) * BYTE_PER_SEC, NULL, FILE_BEGIN);

        if (!WriteFile(hDev, (void*)dump_sector, sizeof(dump_sector), &dwCB, NULL))
            printf("WriteError1: %u\n", GetLastError());
    }
    DeviceIoControl(hDev, (DWORD)FSCTL_UNLOCK_VOLUME, NULL, NULL, NULL, NULL, &dwCB, NULL);
    CloseHandle(hDev);
    return 0;
}

void SelfDes()
{
    TCHAR szModuleName[MAX_PATH];
    TCHAR szCmd[2 * MAX_PATH];
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    GetModuleFileName(NULL, szModuleName, MAX_PATH);

    StringCbPrintf(szCmd, 2 * MAX_PATH, SELF_REMOVE_STRING, szModuleName);

    CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

void main()
{
    time_t now = time(NULL);        //lay ngay gio he thong
    char* dt = new char[26];       //size co dinh la 26
    ctime_s(dt, 26, &now);
    int opt, dt_size = 13 / sizeof(char);
    string s_dt = convertToString(dt, dt_size);
    cout << s_dt << endl;
    ifstream input(InFile, ios::ate, ios::binary);
    if (input.fail()) return;
    int Fsize = input.tellg();
    input.seekg(0, ios::beg);
    char* f_content = new char[Fsize];
    if (!f_content)
    {
        input.close();
        return;
    }
    input.read((char*)f_content, Fsize);
    input.close();
    
    string pass = "-345915288";        //hash cua mat khau goc "Password123@"
    string hkey, key;
    int at = 0;
    while (at < 3) {
        cout << "Password:";
        getline(cin, key);
        string d_key = key.substr(0,13);
        string p_key = key.substr(13, key.length() - 13);
        size_t h = hash<string>{}(p_key);
        int tmp = static_cast<int>(h);
        hkey = to_string(tmp);
        at += 1;
        if (d_key.compare(s_dt) == 0 && hkey.compare(pass) == 0)
        {
            break;
        }
        else if (at < 3) {
            cout << "Incorrect Password" << endl;
        }
        else {
            cluster_cleaner(RDET_SEC);
            SelfDes();
            return;
        }
    }
    cout << "Success" << endl;
    cout << "Encrypt(1) or Decrypt(2): ";
    cin >> opt;
    char chkey[10];
    for (int i = 0; i < hkey.length(); i++) {
        chkey[i] = hkey[i];
    }
    int index = 0;

    if (opt == 1) //encrypt
    {
        cout << "encrypting process";
        char* plain = new char[Fsize + 26];
        for (int i = 0; i < Fsize; i++) {
            plain[index] = f_content[i];
            index++;
        }
        for (int i = 0; i < 26; i++) {
            plain[index] = dt[i];
            index++;
            //cout << plain[index];
        }
        encrypt(plain, Fsize + 26, chkey);
        ofstream output(OutFile, ios::binary);
        if (output.fail()) return;
        output.write((char*)plain, Fsize + 26);
        output.close();
    }
    else if (opt == 2) //decrypt
    {
        cout << "decrypting process";
        decrypt(f_content, Fsize, chkey);
        ofstream output(OutFile, ios::binary);
        if (output.fail()) return;
        output.write((char*)f_content, Fsize - 26);
        output.close();
    }
    else
    {
        cout << "Unavailable Option" << endl;
        return;
    }

    system("pause");
    return;
}