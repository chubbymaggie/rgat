/*
Copyright 2016 Nia Catlin

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
Class describing each node
*/
#pragma once
#include "stdafx.h"
#include "graph_display_data.h"
#include "traceStructs.h"
#include "traceConstants.h"
#include "mathStructs.h"

class node_data
{
public:
	node_data() {};
	~node_data() {};

	bool serialise(ofstream *file);
	//takes a file with a pointer next to a node entry, loads it into the node
	int unserialise(ifstream *file, map <MEM_ADDRESS, INSLIST> *disassembly);

	bool get_screen_pos(GRAPH_DISPLAY_DATA *vdata, PROJECTDATA *pd, DCOORD *screenPos);
	FCOORD sphereCoordB(MULTIPLIERS *dimensions, float diamModifier);

	unsigned int index = 0;
	VCOORD vcoord;
	int conditional = 0;
	INS_DATA* ins = NULL;
	bool external = false;
	int nodeMod;
	BLOCK_IDENTIFIER mutation;
	//list of lists of arg number, contents
	vector<ARGLIST> funcargs;
	unsigned long calls = 1;
	//number of external functions called
	unsigned childexterns = 0;
	MEM_ADDRESS address = 0; //this is only used in externs. bit big?
	unsigned int parentIdx = 0;

	unsigned long executionCount = 0;
	unsigned long chain_remaining_in = 0;
	unsigned long chain_remaining_out = 0;
	DWORD heat_run_marker;

	set<unsigned int> incomingNeighbours;
	set<unsigned int> outgoingNeighbours;
};

