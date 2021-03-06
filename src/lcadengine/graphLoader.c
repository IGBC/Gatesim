/*
 * This file is part of the L-CAD project
 * Copyright (c) 2016  Ashley Brown, Katharina Sabel
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "graphLoader.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define LOGMODULE "LOADER"
#include "utils/lcadLogger.h"

typedef struct connRecord {
	size_t src;
	size_t drn;
	struct connRecord *prev;
} connRecord;

void readGate(graph *ctx, char* str, connRecord **last) {
	char *end_token;
	char *sstr = malloc(strlen(str)+1);
	strcpy(sstr, str);

	enum readstate {id, gt, input};
	enum readstate state = id;

	size_t ID;
	gateInputType inputMode;
	bool nin;

	//get next token
	char *token = strtok_r(str, " ", &end_token);
	while (token != NULL) {
		if (token[0] == '#') break;
		char *loc;
		connRecord *rec;
		switch(state) {
		case id:
			ID = atol(token);
			state = gt;
			break;
		case gt:
			loc = token;
			if (loc[0] == '!' || loc[0] == 'N') {
				nin = true;
				loc++;
			}
			LOG(TRACE, "ID:%i Read type as %s", ID, loc)
			if (!strcmp(loc, "AND")) inputMode = AND;
			if (!strcmp(loc, "OR")) inputMode = OR;
			if (!strcmp(loc, "XOR")) inputMode = XOR;
			if (!strcmp(loc, "BUF")) inputMode = UNITY;
			if (!strcmp(loc, "OT")) inputMode = UNITY;
			if (!strcmp(loc, "IN")) inputMode = INPUT;
			if (!strcmp(loc, "OUT")) inputMode = OUTPUT;
			state = input;
			graphAddGLI(ctx, inputMode, nin, ID, 0);
			break;
		case input:
		    rec = (connRecord*) malloc(sizeof(connRecord));
			rec->src = atol(token);
			rec->drn = ID;
			rec->prev = *last;
			
			*last = rec;
			
			break;
		}
		token = strtok_r(NULL, " ", &end_token);
	}
	free(sstr);
};

graph *loaderLoadFromStr(char *str) {
	LOG(INFO1, "Creating Graph from string");
	// Create a graph if this fails then give up
	graph *ctx = graphCreate();
	if (!ctx) return NULL;
	
	char * end_token;
	char *sstr = (char*) malloc(strlen(str)+1);
	strcpy(sstr, str);
	connRecord *recordList = NULL;
	char *line = strtok_r(sstr, "\n", &end_token);

	while (line != NULL) {
		readGate(ctx, line, &recordList);
		line = strtok_r(NULL, "\n", &end_token);
	}

    free(sstr);    
    
	while (recordList != NULL) {
	    connRecord* last = recordList->prev;
		graphAddConnection(ctx, recordList->src, recordList->drn);
		free(recordList);
		recordList = last;
	}
	return ctx;
}

graph *loaderLoadFromFile(char *filename) {
	LOG(INFO1, "Loading from file: %s", filename);
	// Open File;
	FILE *fp;
	fp = fopen(filename, "r");

	char *str;
	size_t length;
	if (!fp) {
		LOG(ERROR, "File: %s not found", filename);
		return NULL;
	}
	// Get length of file
	fseek (fp, 0, SEEK_END);
	length = ftell (fp);
	fseek (fp, 0, SEEK_SET);
	// Allocate buffer;
	str = malloc (length);
	if (!str) {
	  LOG(ERROR, "Malloc Failed during file loading");
	  return NULL;
	}
	// Read into buffer;
	fread (str, 1, length, fp);
	fclose (fp);

	LOG(DEBUG, "File Contents: %s", str);
	//TODO: strip all '/r''s

	graph *g = loaderLoadFromStr(str);
	free(str);
	return g;
}
