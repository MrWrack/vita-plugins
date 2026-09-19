#include "repository.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Deliberately small offline manifest reader for the milestone.
   Format: id|name|version|developer|category|file_url|sha256|section|description
   default_enabled is ALWAYS forced to 0 by the Vita client. */
int repository_load_local(const char *path, Repository *repo){
 if(!repo)return 0; memset(repo,0,sizeof(*repo));
 FILE*f=fopen(path,"r"); if(!f)return 0;
 char line[1024];
 while(repo->count<REPO_MAX_PLUGINS && fgets(line,sizeof(line),f)){
  if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')continue;
  char *v[9]={0}; int n=0;
  char *p=strtok(line,"|\r\n");
  while(p && n<9){v[n++]=p;p=strtok(NULL,"|\r\n");}
  if(n<9)continue;
  RepoPlugin*x=&repo->plugins[repo->count++];
  snprintf(x->id,sizeof(x->id),"%s",v[0]); snprintf(x->name,sizeof(x->name),"%s",v[1]);
  snprintf(x->version,sizeof(x->version),"%s",v[2]); snprintf(x->developer,sizeof(x->developer),"%s",v[3]);
  snprintf(x->category,sizeof(x->category),"%s",v[4]); snprintf(x->file_url,sizeof(x->file_url),"%s",v[5]);
  snprintf(x->sha256,sizeof(x->sha256),"%s",v[6]); snprintf(x->section,sizeof(x->section),"%s",v[7]);
  snprintf(x->description,sizeof(x->description),"%s",v[8]);
  x->default_enabled=0; /* non-negotiable safety rule */
 }
 fclose(f); return 1;
}
