/* rd_listcartschedulcodes.c
 *
 * Implementation of the ListCartSchedulCodes Rivendell Access Library
 *
 * (C) Copyright 2015 Todd Baker  <bakert@rfa.org>             
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <expat.h>

#include "rd_listcartschedcodes.h"
#include "rd_common.h"

struct xml_data {
  unsigned schedcodes_quan;
  char elem_name[256];
  char strbuf[1024];
  struct rd_schedcodes *schedcodes;
};


static void XMLCALL __ListCartSchedCodesElementStart(void *data, const char *el, 
					     const char **attr)
{
  struct xml_data *xml_data=(struct xml_data *)data;
  if(strcasecmp(el,"schedCode")==0) {    // Allocate a new schedule code entry
    xml_data->schedcodes=realloc(xml_data->schedcodes,
			   (xml_data->schedcodes_quan+1)*sizeof(struct rd_schedcodes));
    (xml_data->schedcodes_quan)++;
  }
  strncpy(xml_data->elem_name,el,256);
  memset(xml_data->strbuf,0,1024);
}

static void XMLCALL __ListCartSchedCodesElementData(void *data,const XML_Char *s,
					    int len)
{
  struct xml_data *xml_data=(struct xml_data *)data;

  memcpy(xml_data->strbuf+strlen(xml_data->strbuf),s,len);
}


static void XMLCALL __ListCartSchedCodesElementEnd(void *data, const char *el)
{
  struct xml_data *xml_data=(struct xml_data *)data;
  struct rd_schedcodes *schedcodes=xml_data->schedcodes+(xml_data->schedcodes_quan-1);

  if(strcasecmp(el,"code")==0) {
    strncpy(schedcodes->code,xml_data->strbuf,10);
  }
  if(strcasecmp(el,"description")==0){
    strncpy(schedcodes->description,xml_data->strbuf,256);
  }
}


size_t __ListCartSchedCodesCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  XML_Parser p=(XML_Parser)userdata;

  XML_Parse(p,ptr,size*nmemb,0);
  
  return size*nmemb;
}


int RD_ListCartSchedCodes(struct rd_schedcodes *scodes[],
		  	const char hostname[],
                  	const char username[],
                  	const char passwd[],
			const unsigned cartnum,
                  	unsigned *numrecs)
{
  char post[1500];
  char url[1500];
  CURL *curl=NULL;
  XML_Parser parser;
  struct xml_data xml_data;
  long response_code;
  char errbuf[CURL_ERROR_SIZE];
  CURLcode res;

   /* Set number of recs so if fail already set */
  *numrecs = 0;

  /*
   * Setup the CURL call
   */
  memset(&xml_data,0,sizeof(xml_data));
  parser=XML_ParserCreate(NULL);
  XML_SetUserData(parser,&xml_data);
  XML_SetElementHandler(parser,__ListCartSchedCodesElementStart,
			__ListCartSchedCodesElementEnd);
  XML_SetCharacterDataHandler(parser,__ListCartSchedCodesElementData);
  snprintf(url,1500,"http://%s/rd-bin/rdxport.cgi",hostname);
  snprintf(post,1500,"COMMAND=27&LOGIN_NAME=%s&PASSWORD=%s&CART_NUMBER=%u",
	   username,passwd,cartnum);
  if((curl=curl_easy_init())==NULL) {
    return -1;
  }
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,parser);
  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,__ListCartSchedCodesCallback);
  curl_easy_setopt(curl,CURLOPT_URL,url);
  curl_easy_setopt(curl,CURLOPT_POST,1);
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post);
  curl_easy_setopt(curl,CURLOPT_NOPROGRESS,1);
  curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,errbuf);
  //  curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK) {
    size_t len = strlen(errbuf);
    fprintf(stderr, "\nlibcurl error: (%d)", res);
    if (len)
        fprintf(stderr, "%s%s", errbuf,
            ((errbuf[len-1] != '\n') ? "\n" : ""));
    else
        fprintf(stderr, "%s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return -1;
  }

/* The response OK - so figure out if we got what we wanted.. */

  curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
  curl_easy_cleanup(curl);
  
  if (response_code > 199 && response_code < 300) { 
    *scodes=xml_data.schedcodes;
    *numrecs = xml_data.schedcodes_quan;
    return 0;
  }
  else {
    fprintf(stderr," Call Returned Error: %s\n",xml_data.strbuf);
    return (int)response_code;
  }
}
