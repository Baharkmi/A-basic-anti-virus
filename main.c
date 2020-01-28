#include<stdlib.h>
#include<stdio.h>
#include"md5.h"
#include<dirent.h>
#include<windows.h>

void listFilesRecursively(char *basePath);
int Search_in_File(char *fname, char *str);
void writefile(FILE* fptr, MD5_CTX res);
void phase1();
void phase2();

int main(){

    int n;
    printf("Select a phase: 1.phase1 2.phase2 : ");
    scanf("%d", &n);

    if(n==1)
        phase1();
    else if(n==2)
        phase2();

    return 0;
}

void phase2(){
    char* filename = "hi.exe";

    FILE* dlls = fopen("dlls.txt", "w");

    HANDLE hFile,hFileMap;
    DWORD dwImportDirectoryVA,dwSectionCount,dwSection=0,dwRawOffset;
    LPVOID lpFile;
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNtHeaders;
    PIMAGE_SECTION_HEADER pSectionHeader;
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor;
    PIMAGE_THUNK_DATA pThunkData;

    hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(hFile==INVALID_HANDLE_VALUE)
       ExitProcess(1);

    // a handle to a file, inherited, protecction, high-order DWORD of the maximum size of the file mapping object
    // low-order DWORD of the maximum size of the file mapping object , name
    hFileMap = CreateFileMapping(hFile,0,PAGE_READONLY,0,0,0);

    // handle to filemappingobj , protection , high-order DWORD of the file offset where the view begins
    // low-order DWORD of the file offset where the view begins , number of bytes to map to the view
    // the return value is the starting address of the mapped view.
    lpFile = MapViewOfFile(hFileMap,FILE_MAP_READ,0,0,0);

    pDosHeader = (PIMAGE_DOS_HEADER)lpFile;

    // Get pointer to NT header
    //e_lfanew is the offset to the IMAGE_NT_HEADERS
    pNtHeaders = (PIMAGE_NT_HEADERS)((DWORD)lpFile+pDosHeader->e_lfanew);

    // size of nt headers
    dwSectionCount = pNtHeaders->FileHeader.NumberOfSections;

    // relational virtual address of import table
    dwImportDirectoryVA = pNtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress;

    pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pNtHeaders+sizeof(IMAGE_NT_HEADERS));

    for(;dwSection < dwSectionCount && pSectionHeader->VirtualAddress <= dwImportDirectoryVA;pSectionHeader++,dwSection++);
    pSectionHeader--;

    dwRawOffset = (DWORD)lpFile+pSectionHeader->PointerToRawData;

    // rva to va
    pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(dwRawOffset+(dwImportDirectoryVA-pSectionHeader->VirtualAddress));

    for( ; pImportDescriptor->Name!=0 ; pImportDescriptor++)
    {
        printf("\nDLL Name : %s\n\n",dwRawOffset+(pImportDescriptor->Name-pSectionHeader->VirtualAddress));
        //fprintf (dlls, "%s\n", dwRawOffset+(pImportDescriptor->Name-pSectionHeader->VirtualAddress));
        char* str = dwRawOffset+(pImportDescriptor->Name-pSectionHeader->VirtualAddress);

        Search_in_File("hash.txt", &str);

        pThunkData = (PIMAGE_THUNK_DATA)(dwRawOffset+(pImportDescriptor->FirstThunk-pSectionHeader->VirtualAddress));
        for(;pThunkData->u1.AddressOfData != 0;pThunkData++)
        printf("\tFunction : %s\n",(dwRawOffset+(pThunkData->u1.AddressOfData-pSectionHeader->VirtualAddress+2)));
    }
    UnmapViewOfFile(lpFile);
    CloseHandle(hFileMap);
    CloseHandle(hFile);
    fclose(dlls);
}

void phase1(){
    char* path;
    printf("Enter Directory: ");
    scanf("%s", &path);

    char* malware;
    printf("Enter file name: ");
    scanf("%s", &malware);

    listFilesRecursively(&path);

	char temp[32];
    FILE* hashes = fopen("hash.txt", "r");
	while(fgets(temp, 32, hashes) != NULL) {
        Search_in_File(&malware, &temp);
	}
    fclose(hashes);
}

void listFilesRecursively(char *basePath)
{
    char path[1000];
    char name[1000];

    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir)
        return;

    FILE *hashes;
    hashes = fopen("hash.txt", "w");

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            FILE* f;
            strcpy(name, basePath);
            strcat(name, "/");
            strcat(name, dp->d_name);
            if((f = fopen(name, "r")) != NULL) {
                printf("%s\t\t", dp->d_name);
                MD5_CTX res = MDFile (name);

                writefile(hashes, res);

                int i;
                for (i = 0; i < 16; i++)
                    printf ("%02x", res.digest[i]);
                printf("\n");
            }
            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            listFilesRecursively(path);
        }
    }

    fclose(hashes);
}

int Search_in_File(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	char temp[32];

	//gcc users
	if((fp = fopen(fname, "r")) == NULL) {
		return(-1);
	}

	//Visual Studio users
	/*if((fopen_s(&fp, fname, "r")) != NULL) {
		return(-1);
	}*/

	while(fgets(temp, 32, fp) != NULL) {
		if((strstr(temp, str)) != NULL) {
			printf("A match found on line: %d\n", line_num);
			printf("\n%s\n", temp);
			find_result++;
		}
		line_num++;
	}

	//Close the file if still open.
	if(fp) {
		fclose(fp);
	}

	if(find_result == 0) {
		printf("\nSorry, couldn't find a match.\n");
		return 0;
	}else
        return 1;
}


void writefile(FILE* fptr, MD5_CTX res){

   if(fptr == NULL)
   {
      printf("Error!");
      exit(1);
   }

    int i;
    for (i = 0; i < 16; i++)
        fprintf (fptr, "%02x", res.digest[i]);

    fprintf(fptr, "%s", "\n");
}
