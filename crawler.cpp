#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include<fstream>

#define HASH_SIZE 100

#define Url_Length 1000

//#define SEED_URL "http://www.chitkara.edu.in"

#define MAX_URL_PER_PAGE 1000

#define INTERVAL_TIME 10
using namespace std;
char base_url[]="www.chitkara.edu.in";
char *str1,*str2,*str3;
int depth;
int linkdepth = 0;
int id=1;

//structure

typedef struct node
{
    char *url;
    int key;
    int isvisited;
    int depth;
    node *next;
    node *prev;
};


//some extra decalarations
node *hash1[50];



void NormalizeWord(char* word) {
  int i = 0;
  while (word[i]) {
      // NEW
    if (word[i] < 91 && word[i] > 64) // Bounded below so this funct. can run on all urls
      // /NEW
      word[i] += 32;
    i++;
  }
}



int NormalizeURL(char* URL) 
{
  int len = strlen(URL);
  if (len <= 1 )
    return 0;
  //! Normalize all URLs.
  if (URL[len - 1] == '/') 
  {
    URL[len - 1] = 0;
    len--;
  }
  int i, j;
  len = strlen(URL);
  //! Safe check.
  if (len < 2)
    return 0;
  //! Locate the URL's suffix.
  for (i = len - 1; i >= 0; i--)
    if (URL[i] =='.')
      break;
  for (j = len - 1; j >= 0; j--)
    if (URL[j] =='/')
      break;
  //! We ignore other file types.
  //! So if a URL link is to a file that are not in the file type of the following
  //! one of four, then we will discard this URL, and it will not be in the URL list.
  if ((j >= 7) && (i > j) && ((i + 2) < len)) 
  {
    if ((!strncmp((URL + i), ".htm", 4))
        ||(!strncmp((URL + i), ".HTM", 4))
        ||(!strncmp((URL + i), ".php", 4))
        ||(!strncmp((URL + i), ".jsp", 4))
        ) 
    {
      len = len; // do nothing.
    } 
    else 
    {
      return 0; // bad type
    }
  }
  return 1;
}


void removeWhiteSpace(char* html) 
{
  int i;
  char *buffer = (char *)malloc(strlen(html)+1), *p=(char *)malloc (sizeof(char)+1);
  memset(buffer,0,strlen(html)+1);
  for (i=0;html[i];i++) 
  {
    if(html[i]>32)
    {
      sprintf(p,"%c",html[i]);
      strcat(buffer,p);
    }
  }
  strcpy(html,buffer);
  free(buffer); free(p);
}


int GetNextURL(char* html, char* urlofthispage, char* result, int pos) 
{
  char c;
  int len, i, j;
  char* p1;  //!< pointer pointed to the start of a new-founded URL.
  char* p2;  //!< pointer pointed to the end of a new-founded URL.

  // NEW
  // Clean up \n chars
  if(pos == 0) {
    removeWhiteSpace(html);
  }
  // /NEW

  // Find the <a> <A> HTML tag.
  while (0 != (c = html[pos])) 
  {
    if ((c=='<') &&
        ((html[pos+1] == 'a') || (html[pos+1] == 'A'))) {
      break;
    }
    pos++;
  }
  //! Find the URL it the HTML tag. They usually look like <a href="www.abc.com">
  //! We try to find the quote mark in order to find the URL inside the quote mark.
  if (c) 
  {  
    // check for equals first... some HTML tags don't have quotes...or use single quotes instead
    p1 = strchr(&(html[pos+1]), '=');
    
    if ((!p1) || (*(p1-1) == 'e') || ((p1 - html - pos) > 10)) 
    {
      // keep going...
      return GetNextURL(html,urlofthispage,result,pos+1);
    }
    if (*(p1+1) == '\"' || *(p1+1) == '\'')
      p1++;

    p1++;    

    p2 = strpbrk(p1, "\'\">");
    if (!p2) 
    {
      // keep going...
      return GetNextURL(html,urlofthispage,result,pos+1);
    }
    if (*p1 == '#') 
    { // Why bother returning anything here....recursively keep going...

      return GetNextURL(html,urlofthispage,result,pos+1);
    }
    if (!strncmp(p1, "mailto:",7))
      return GetNextURL(html, urlofthispage, result, pos+1);
    if (!strncmp(p1, "http", 4) || !strncmp(p1, "HTTP", 4)) 
    {
      //! Nice! The URL we found is in absolute path.
      strncpy(result, p1, (p2-p1));
      return  (int)(p2 - html + 1);
    } else {
      //! We find a URL. HTML is a terrible standard. So there are many ways to present a URL.
      if (p1[0] == '.') {
        //! Some URLs are like <a href="../../../a.txt"> I cannot handle this. 
	// again...probably good to recursively keep going..
	// NEW
        
        return GetNextURL(html,urlofthispage,result,pos+1);
	// /NEW
      }
      if (p1[0] == '/') {
        //! this means the URL is the absolute path
        for (i = 7; i < strlen(urlofthispage); i++)
          if (urlofthispage[i] == '/')
            break;
        strcpy(result, urlofthispage);
        result[i] = 0;
        strncat(result, p1, (p2 - p1));
        return (int)(p2 - html + 1);        
      } else {
        //! the URL is a absolute path.
        len = strlen(urlofthispage);
        for (i = (len - 1); i >= 0; i--)
          if (urlofthispage[i] == '/')
            break;
        for (j = (len - 1); j >= 0; j--)
          if (urlofthispage[j] == '.')
              break;
        if (i == (len -1)) {
          //! urlofthis page is like http://www.abc.com/
            strcpy(result, urlofthispage);
            result[i + 1] = 0;
            strncat(result, p1, p2 - p1);
            return (int)(p2 - html + 1);
        }
        if ((i <= 6)||(i > j)) {
          //! urlofthis page is like http://www.abc.com/~xyz
          //! or http://www.abc.com
          strcpy(result, urlofthispage);
          result[len] = '/';
          strncat(result, p1, p2 - p1);
          return (int)(p2 - html + 1);
        }
        strcpy(result, urlofthispage);
        result[i + 1] = 0;
        strncat(result, p1, p2 - p1);
        return (int)(p2 - html + 1);
      }
    }
  }    
  return -1;
}


int genereateKey(char key[10000])
{
    int sum = 0;
    for(int i=0;key[i]!='\0';i++)
    {
        sum+=key[i];
    }
    while(!(sum<=50))
    sum = sum/10;
    return sum;
}
node* createLinkList(char *links[100])
{
	node *new_node;
	new_node = (node *)malloc(sizeof(node));
	node *head = new_node;
	node *prev = NULL;
	new_node->prev = prev;
	for(int i=0;i<100;i++)
	{
		if(genereateKey(links[i]) != 0)
		{
			new_node->url = links[i];
			new_node->key = genereateKey(links[i]);
			new_node->isvisited = 0;//initially it is not visited
			new_node->depth = linkdepth;
			prev = new_node;
			new_node = (node *)malloc(sizeof(node));
			prev->next = new_node;
		}
	}
	linkdepth++;
	prev->next = NULL;
	return head;
}

void arrangeinhash(node *head)
{
	node *temp = head,*new_node;
	node *prev;
	while(temp)
	{	
		new_node = (node *)malloc(sizeof(node));
		new_node->url = temp->url;
		new_node->key = temp->key;
		new_node->isvisited = temp->isvisited;
		new_node->depth = temp->depth;
		if(hash1[temp->key]==NULL)
		{
			hash1[temp->key] = new_node;
			new_node->prev = NULL;
			new_node->next = NULL;
		}
		else
		{
			node *addr = hash1[temp->key];
			while(addr->next!=NULL)
			{
				addr = addr->next;
			}
			//cout<<"got the NULL";
			addr->next = new_node;
			new_node->prev = addr;
			new_node->next = NULL;
		}
		temp = temp->next;
	}
	cout<<"successfully created hash table";
}


char** extractlinks(char *path,char *seed)
{
    struct stat st;
    stat(path,&st);
    int file_size = st.st_size;
    ifstream file(path);
    char ch;
    char sourcecode[file_size];
    int it=0;
    while(!file.eof())
    {
        file>>ch;
        sourcecode[it]=ch;
        it++;
    }
    char *m = (char *)malloc(file_size*sizeof(char));
    char **links = (char **)calloc(100,sizeof(char *));
    for(int i=0;i<100;i++)
    {
    		links[i] = (char *)malloc(file_size*sizeof(char));
    }
    int flag = 0;
    int n=GetNextURL(sourcecode,seed,m,0);
    // m is my extracted link from webpage
    
     for(int i=0;i<100;i++)
     {
        for(int i=0;i<100;i++)
        {
            if(strcmp(m,*(links+i)) == 0)
            {
            	flag++;
            	break;
            }
        }
        if(flag == 0)
        {
            strcpy(*(links+i),m);
        }
        else
        continue;
        
        n = GetNextURL(sourcecode,seed,m,n);
     }
     
   	return links;
}

char* copyfile(char *url)
{
    char *path = (char *)malloc(sizeof(char)*100);
    sprintf(path,"store/%d.txt",id);
    id++;
    ifstream file1("store/temp.txt");
    ofstream file2(path);
	/*here we can add extra information about the file if needed*/ 
	file2<<url<<endl;
    char ch;
   while(!file1.eof())
    {
        file1>>ch;
        file2<<ch;
    }
    file2.close();
    file1.close();
     
    return path;
    //node *ptr = movetoram(path,seed);
    //return ptr;
}

void getPage(char url[50])
{
char urlbuffer[Url_Length+300]={0}; 
strcat(urlbuffer, "wget -O ");

strcat(urlbuffer, "store/temp.txt ");

strcat(urlbuffer, url);
system(urlbuffer);

//making new file by copying data from temp file
}
/*---------------------------------------------------------------*/
void testDir(char *dir)
{
  struct stat statbuf;
  if ( stat(dir, &statbuf) == -1 )
  {
    fprintf(stderr, "-----------------\n");
    fprintf(stderr, "Invalid directory\n");
    fprintf(stderr, "-----------------\n");
    exit(1);
  }

  //Both check if there's a directory and if it's writable
  if ( !S_ISDIR(statbuf.st_mode) )
  {
    fprintf(stderr, "-----------------------------------------------------\n");
    fprintf(stderr, "Invalid directory entry. Your input isn't a directory\n");
    fprintf(stderr, "-----------------------------------------------------\n");
    exit(1);
  }

  if ( (statbuf.st_mode & S_IWUSR) != S_IWUSR )
  {
    fprintf(stderr, "------------------------------------------\n");
    fprintf(stderr, "Invalid directory entry. It isn't writable\n");
    fprintf(stderr, "------------------------------------------\n");
    exit(1);
  }
}
void checkDepth()
{

  if(str2[0]>='1'&&str2[0]<='5'&&str2[1]=='\0')
  {
    depth=str2[0]-48;
  }
  else
  {
    printf("Invalid depth");
    exit(1);
  }
}
void validateUrl()
{
char str[strlen(str1)+20]="wget --spider ";
strcat(str,str1);
    if(!system(str))
    {
    printf("Valid URL");
    }
  else
    printf("Invalid URL");
}

// it is a simple crawler not a big thing
// follow me on github aniketchanana05
void checkUrl()
{
int i,flag=0;
for(i=0;base_url[i]!='\0';i++)
{
  if(base_url[i]!=str1[i])
  {
    flag=1;
      printf("Invalid url");
    break;
  }
}
if(flag==0)
{
  validateUrl();
}
}
void checkCreteria(int n)
{
  if(n==4)
  {
checkDepth();
testDir(str3);
checkUrl();
  }
  else
  {
    printf("Invalid input");
  }
}
int main(int argc,char *argv[])
{
  str1=argv[1];//base url
  str2=argv[2];//depth
  str3=argv[3];//store directory path

  checkCreteria(argc);
  
  //working for one link
  
  //fetch the given page from iternet
  getPage(str1);
  
  //transfer the page to given id .txt
  char *loc = copyfile(str1);
  
  //loc is the location of the file we have just stored in our store
  //extractlinks from this stored file
  char **links = extractlinks(loc,str1);
  //these are all the links fetched from the given one url arranged ina linklist
  node *head = createLinkList(links);
  //arrange in global hashtable
  arrangeinhash(head);
  
  
  //todo
  //1. traverse the link list and get the pages from give link list
  //2. extract the links from the page
  //3. now got the links check whether they are repeated or not from hash table
  //if repeated then drop it else put in link list
  //extract the links till depth dosenot overflow
  
  node *start = head;
  while(start!=NULL)
  {
	if(start->isvisited == 0)
	{
		getPage(start->url);
		loc = copyfile(start->url);
		links = extractlinks(loc,start->url);
		node *newhead = createLinkList(links);
		while(head->next != NULL)
		{
			head = head->next;
		}
		head->next = newhead;
		start->isvisited = 1;
	}
		start = start->next;
  }
  
 return 0;
}
