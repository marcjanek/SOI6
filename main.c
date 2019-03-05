/*
 * only 98 C
 */
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include <zconf.h>

#define MAX_FILES       8
#define MAX_FILE_NAME   16

char *v_name;
unsigned long v_size = 0, v_start = 0, v_free_space = 0;
int v_ID;
size_t v_free_slots = MAX_FILES;
char *menu="Dostepne polecenia:\n1=kopiowanie na dysk\n2=kopiowanie z dysku\n3=wyswietla zawartosc\n4=usuwanie pliku\n5=usuwanie konteneara\n0=wyjscie\n";
typedef struct inode_struct
{
    char fileName[MAX_FILE_NAME];
    unsigned long fileSize;
    unsigned long fileBegin;
} inode;

typedef struct node_struct
{
    inode data;
    struct node_struct *next;
} listNode;
listNode *root = NULL;

/*====================================================================================================================================================*/
int chose(char*);

void create_v(void);

void open_v(void);

void download_files_from_out(void);

void save_v_out(void);

int clean_file(void);

void free_file(void);

void enter_name(char *,char *);

listNode *find_file_named(char*);

void free_node_named(char*);

void copy_file_to_v();

int long find_free_space(int long);

listNode *find_prev(int long);

void copy_file_from_v(void);

void map_all_v_space(void);

/*====================================================================================================================================================*/
int main(int argc, char *argv[])
{
    if(argc==3)//create
    {
        v_name =argv[1];
        v_size = (unsigned long)atoi(argv[2]);
        if(v_size<=0)
        {
            printf("zle argumenty!!!");
            return 0;
        }
        create_v();
    }
    else if(argc==2)//open
    {
        v_name =argv[1];
        open_v();
        struct stat size;
        stat(v_name, &size);
        v_size = (unsigned long) (size.st_size);
        download_files_from_out();
    } else
    {
        printf("zle argumenty!!!");
        return 0;
    }
    while (1)
    {
        switch (chose(menu))
        {
            case 0:
                save_v_out();
                close(v_ID);
                return 0;
            case 1:
                if (v_free_slots > 0)
                    copy_file_to_v();
                else
                    printf("brak slotu wolnego:!!!\n");
                break;
            case 2:
                copy_file_from_v();
                break;
            case 3:
                map_all_v_space();
                break;
            case 4:
                free_file();
                break;
            case 5:
                save_v_out();
                close(v_ID);
                if(remove(v_name)==0)
                {
                    printf("udalo sie usunac kontener\n");
                    return 0;
                }
                else
                {
                    printf("nie udalo sie usunac kontenera\n");
                    open_v();
                    download_files_from_out();
                }
                break;
            default:
                printf("nie ma takiej operacji,wprowadz poprawnie\n");
                break;
        }
    }
}

/*====================================================================================================================================================*/
void map_all_v_space(void)
{
    printf("###########\n");
    printf("rozmiar kontenera      : %ld\n", v_size);
    printf("wolne miejsce kontenera: %ld\n", v_free_space);
    printf("wolne sloty w kontenera: %ld\n",v_free_slots);
    printf("###########\n");
    listNode *temp1 = root;
    while (temp1 != NULL)
    {
        printf("-------------\n");
        printf("nazwa   : %s \n", temp1->data.fileName);
        printf("rozmiar : %ld\n", temp1->data.fileSize);
        printf("poczatek: %ld\n", temp1->data.fileBegin);
        printf("koniec  : %ld\n", temp1->data.fileBegin + temp1->data.fileSize-1);
        printf("-------------\n");
        temp1 = temp1->next;
    }
}

/*====================================================================================================================================================*/
void copy_file_from_v(void)
{
    char name[MAX_FILE_NAME],name1[MAX_FILE_NAME];
    enter_name(name,"nazwa pliku w kontenerze: ");
    enter_name(name1,"nowa nazwa pliku do zapisania: ");
    listNode *temp1 = find_file_named(name);
    if (temp1 == NULL)
    {
        printf("nie ma pliku w kontenerze\n");
        return;
    }
    int fileID = creat(name1,O_RDWR|0666|O_EXCL);
    if (fileID < 0)
    {
        fileID=open(name1,O_RDWR|O_EXCL);
        if(fileID<0)
        {
            printf("nie mozna otworzyc pliku\n");
            return;
        }

    }
    lseek(v_ID, temp1->data.fileBegin, 0);
    unsigned long count;
    unsigned long size;
    char buf[1024];

    count =1024;
    size = temp1->data.fileSize;
    while (size > 0)
    {
        if (size < count) count = size;
        read(v_ID, buf, count);
        write(fileID, buf, count);
        size -= count;
    }
    close(fileID);
}

/*====================================================================================================================================================*/
listNode *find_prev(long end)
{
    if ((root != NULL) && (root->next == NULL)) return root;
    listNode *temp1 = root;
    while (temp1 != NULL)
    {
        if (temp1->data.fileBegin + temp1->data.fileSize == end)
            return temp1;
        temp1 = temp1->next;
    }
    return NULL;//nie ma prev temp==null
}

/*====================================================================================================================================================*/
long find_free_space(long size)
{
    if(size>v_free_space){ return -1;}
    if (root == NULL)//pusta
    {
        if ((v_size - v_start) > size) return v_start;
        else return -1;
    }
    //szukaj dalej
    if ((root->data.fileBegin - v_start) >= size) return v_start;//zerowe miejsce
    listNode *temp1 = root;
    while (temp1->next != NULL)//szukaj dalej
    {
        if ((temp1->next->data.fileBegin - (temp1->data.fileBegin + temp1->data.fileSize)) >= size)
            return (temp1->data.fileBegin + temp1->data.fileSize);
        temp1 = temp1->next;
    }//ostatnia przedstrzen
    if ((v_size - temp1->data.fileBegin + temp1->data.fileSize) >= size)
        return (temp1->data.fileBegin + temp1->data.fileSize);
    return -1;
}

/*====================================================================================================================================================*/
void copy_file_to_v(void)
{
    unsigned long size;
    int fileID;
    struct stat stbuf;
    char name[MAX_FILE_NAME],name1[MAX_FILE_NAME];
    listNode *temp1 = root;
    enter_name(name,"podaj nazwe docelowa pliku w kontenerze: ");
    enter_name(name1,"podaj nazwe orginalna pliku w katalogu: ");

    while (temp1 != NULL)
    {
        if (strcmp(temp1->data.fileName, name) == 0)
        {
            printf("w kontenerze jest już plik o takiej nazwie\n");
            return;
        }
        temp1 = temp1->next;
    }
    if ((fileID = open(name1, O_RDONLY|O_EXCL)) < 0)
    {
        printf("nie ma takiego pliku\n");
        return;
    }
    stat(name1, &stbuf);
    size = (unsigned long int) stbuf.st_size;
    unsigned long start = (unsigned long int) (find_free_space(size));
    if (start == -1)
    {
        printf("plik nie miesci sie w kontenerze\n");
        close(fileID);
        return;
    }
    listNode *new = malloc(sizeof(listNode));
    listNode *prev = find_prev(start);
    strncpy(new->data.fileName, name, MAX_FILE_NAME - 1);
    new->data.fileSize = size;
    new->data.fileBegin = start;
    if (prev == NULL)
    {
        if (root == NULL)//zerowy
        {
            root = new;
            new->next = NULL;
        } else//przed pierwszy
        {
            new->next = root;
            root = new;
        }
    } else//pomiedzy
    {
        new->next = prev->next;
        prev->next = new;
    }
    char buf[1024];
    unsigned long count=1024;
    lseek(v_ID, new->data.fileBegin, 0);
    while (size > 0)
    {
        if (size < count) count = size;
        read(fileID, buf, count);
        write(v_ID, buf, count);
        size -= count;
    }
    --v_free_slots;
    v_free_space = v_free_space - new->data.fileSize;
    close(fileID);
}

/*====================================================================================================================================================*/
void free_node_named(char* name)
{
    if (root == NULL) return;
    listNode *find = root, *prev = root;
    while (find != NULL)
    {
        if (strcmp(find->data.fileName, name) == 0)
            break;
        prev = find;
        find = find->next;
    }
    if (find == root)/*gdy tylko jeden element i wiem ze na pewno jest  */
    {
        root = find->next;//if(find==NULL)root==NULL;
    } else
    {
        prev->next = find->next;
    }
    v_free_space = v_free_space + find->data.fileSize;
    ++v_free_slots;
    free(find);
}

/*====================================================================================================================================================*/
listNode *find_file_named(char* name)
{
    listNode *temp = root;
    while (temp != NULL)
    {
        if (strcmp(name, temp->data.fileName) == 0)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;/*==temp*/
}

/*====================================================================================================================================================*/
void enter_name (char *buf,char *to_print)
{
    printf("%s", to_print);
    char c;
    int i = 0;
    for(i=0;i<MAX_FILE_NAME;++i)
    {
        if((c = getchar()) != EOF && c != '\n')
        {
            buf[i]=c;
        }
        else
        {
            break;
        }
    }
    //buf[i]='\n';
    buf[i] = '\0';
}
/*====================================================================================================================================================*/
void free_file(void)
{
    char name[MAX_FILE_NAME];
    enter_name(name,"nazwa pliku do usuniecia: ");
    if (find_file_named(name) == NULL)
    {
        printf("nie ma takiego pliku!\n");
        return;
    }
    free_node_named(name);
}

/*====================================================================================================================================================*/
int clean_file(void)
{
    char *temp = malloc(sizeof(inode) * MAX_FILES);
    int i = 0;
    while (i < sizeof(inode) * MAX_FILES)
    {
        temp[i] = '\0';
        ++i;
    }
    lseek(v_ID, 0, 0);
    if (write(v_ID, temp, sizeof(inode) * MAX_FILES) < 0)
    {
        printf("nie mozna pisac do pliku");
        free(temp);
        return 0;
    }
    free(temp);
    return 1;
}
/*====================================================================================================================================================*/
void save_v_out(void)
{
    listNode *current = root;
    if (clean_file() == 0) return;
    lseek(v_ID, 0, 0);
    while (current != NULL)
    {
        if (write(v_ID, &(current->data), sizeof(inode)) < 0)
            return;
        current = current->next;
    }
}

/*====================================================================================================================================================*/
void download_files_from_out(void)
{

    int i;
    char *temp1 = (char *) malloc(sizeof(inode));
    lseek(v_ID, 0, 0);
    for (i = 0; i < MAX_FILES; ++i)
    {
        if (read(v_ID, temp1, sizeof(inode)) < 0)
        {
            printf("blad podczas czytania z pliku!!!");
            free(temp1);
            listNode *temp2;
            while (v_free_slots < MAX_FILES - 1)
            {
                temp2 = root;
                root = root->next;
                free(temp2);
                ++v_free_slots;
            }
            if (root != NULL)
            {
                free(root);
                v_free_slots++;
            }
            exit(0);
        }
        inode *temp2 = (inode *) (temp1);
        if(temp2->fileSize>0)
        {
            listNode *current = malloc(sizeof(listNode));
            strncpy(current->data.fileName, temp2->fileName, MAX_FILE_NAME);
            current->data.fileSize = temp2->fileSize;
            current->data.fileBegin = temp2->fileBegin;
            current->next = NULL;

            if (root == NULL)
            {
                root = current;
            } else
            {
                listNode *temp = root;
                while (temp->next != NULL)
                {
                    temp = temp->next;
                }
                temp->next = current;
            }
            --v_free_slots;
        }

    }
    v_start = sizeof(inode) * MAX_FILES;
    listNode *temp2 = root;
    long used_space = 0;
    while (temp2 != NULL)
    {
        used_space += temp2->data.fileSize;
        temp2 = temp2->next;
    }
    v_free_space = v_size - v_start - used_space;
    free(temp1);
}

/*====================================================================================================================================================*/
void open_v(void)
{
    v_ID = open(v_name, O_RDWR|O_EXCL);
    if (v_ID < 0)
    {
        printf("otwieranie pliku nie udalo sie!!!");
        exit(0);
    }
}
/*====================================================================================================================================================*/
void create_v(void)
{
    v_start = sizeof(inode) * MAX_FILES;
    v_free_space = v_size - v_start;
    if (v_start > v_size)
    {
        printf("przeznacz więcej miejsca!!!");
        exit(0);
    }
    v_ID=creat(v_name,O_EXCL|0666);/*0666=linux RDWR*/
    close(v_ID);
    open_v();
    unsigned long int i;
    int buf[1024];
    size_t count;

    for (i = 0; i < 1024; i++) buf[i] = '\0';
    i = v_size;
    count = 1024;
    while (i > 0)
    {
        if (i < count)
        {
            count = i;
        }
        write(v_ID, buf, count);
        i -= count;
    }
}

/*====================================================================================================================================================*/
int chose(char* komentarz)
{
    printf("%s",komentarz);
    int opcja = 0, blad, znak = 0, cyfry, c;
    while (znak != 1)
    {
        blad = cyfry = opcja = 0;
        printf("wybieram:");
        while ((c = getchar()))
        {
            if (!blad && (c >= '0') && (c <= '9'))
            {
                opcja = opcja * 10;
                opcja += (c - '0');
                cyfry++;
            } else if (c == '\n')
            {
                znak = 1;
                break;
            } else blad = 1;
        }
        if (blad || cyfry > 1)
        {
            printf("blad!!! wprowadz poprawnie\n");
            printf("%s",komentarz);
            znak = 0;
            opcja = 0;
        }
    }
    return opcja;
}