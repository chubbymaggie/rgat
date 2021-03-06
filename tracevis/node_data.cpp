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
#include "stdafx.h"
#include "node_data.h"
#include "b64.h"
#include "graphicsMaths.h"
#include "GUIConstants.h"
#include "traceMisc.h"

//take the a/b/bmod coords, convert to opengl coordinates based on supplied sphere multipliers/size
FCOORD node_data::sphereCoordB(MULTIPLIERS *dimensions, float diamModifier) 
{
	FCOORD result;
	float adjB = vcoord.b + float(vcoord.bMod * BMODMAG);
	sphereCoord(vcoord.a, adjB, &result, dimensions, diamModifier);
	return result;
}

//this fails if we are drawing a node that has been recorded on the graph but not rendered graphically
bool node_data::get_screen_pos(GRAPH_DISPLAY_DATA *vdata, PROJECTDATA *pd, DCOORD *screenPos)
{
	FCOORD graphPos;
	if (!vdata->get_coord(index, &graphPos)) return false;

	gluProject(graphPos.x, graphPos.y, graphPos.z,
		pd->model_view, pd->projection, pd->viewport,
		&screenPos->x, &screenPos->y, &screenPos->z);
	return true;
}

bool node_data::serialise(ofstream *outfile)
{
	*outfile << index << "{";
	*outfile << vcoord.a << "," <<
		vcoord.b << "," <<
		vcoord.bMod << "," <<
		conditional << "," << nodeMod << ",";
	*outfile << address << ",";

	*outfile << executionCount << ",";

	set<unsigned int>::iterator adjacentIt = incomingNeighbours.begin();
	*outfile << incomingNeighbours.size() << ",";
	for (; adjacentIt != incomingNeighbours.end(); ++adjacentIt)
		*outfile << *adjacentIt << ",";

	adjacentIt = outgoingNeighbours.begin();
	*outfile << outgoingNeighbours.size() << ",";
	for (; adjacentIt != outgoingNeighbours.end(); ++adjacentIt)
		*outfile << *adjacentIt << ",";

	*outfile << external << ",";

	if (!external)
		*outfile << ins->mutationIndex;
	else
	{
		*outfile << funcargs.size() << ","; //number of calls
		vector<ARGLIST>::iterator callIt = funcargs.begin();
		ARGLIST::iterator argIt;
		for (; callIt != funcargs.end(); callIt++)
		{
			*outfile << callIt->size() << ",";
			for (argIt = callIt->begin(); argIt != callIt->end(); argIt++)
			{
				string argstring = argIt->second;
				const unsigned char* cus_argstring = reinterpret_cast<const unsigned char*>(argstring.c_str());
				*outfile << argIt->first << "," << base64_encode(cus_argstring, argstring.size()) << ",";
			}
		}
	}
	*outfile << "}";

	return true;
}


int node_data::unserialise(ifstream *file, map <MEM_ADDRESS, INSLIST> *disassembly)
{
	string value_s;

	getline(*file, value_s, '{');
	if (value_s == "}N,D") return 0;

	if (!caught_stoi(value_s, (int *)&index, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, (int *)&vcoord.a, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, (int *)&vcoord.b, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, (int *)&vcoord.bMod, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, (int *)&conditional, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, &nodeMod, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoul(value_s, &address, 10))
		return -1;

	getline(*file, value_s, ',');
	if (!caught_stoul(value_s, &executionCount, 10))
		return -1;

	unsigned int adjacentQty;
	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, &adjacentQty, 10))
		return -1;

	for (unsigned int i = 0; i < adjacentQty; ++i)
	{
		unsigned int adjacentIndex;
		getline(*file, value_s, ',');
		if (!caught_stoi(value_s, &adjacentIndex, 10))
			return -1;
		incomingNeighbours.insert(adjacentIndex);
	}

	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, &adjacentQty, 10))
		return -1;

	for (unsigned int i = 0; i < adjacentQty; ++i)
	{
		unsigned int adjacentIndex;
		getline(*file, value_s, ',');
		if (!caught_stoi(value_s, &adjacentIndex, 10))
			return -1;
		outgoingNeighbours.insert(adjacentIndex);
	}

	getline(*file, value_s, ',');
	if (value_s.at(0) == '0')
	{
		external = false;

		getline(*file, value_s, '}');
		if (!caught_stoi(value_s, (int *)&mutation, 10))
			return -1;

		map<MEM_ADDRESS, INSLIST>::iterator addressIt = disassembly->find(address);
		if ((addressIt == disassembly->end()) || (mutation >= addressIt->second.size()))
			return -1;

		ins = addressIt->second.at(mutation);
		return 1;
	}

	external = true;

	int numCalls;
	getline(*file, value_s, ',');
	if (!caught_stoi(value_s, &numCalls, 10))
		return -1;

	vector <ARGLIST> funcCalls;
	for (int i = 0; i < numCalls; ++i)
	{
		int argidx, numArgs = 0;
		getline(*file, value_s, ',');
		if (!caught_stoi(value_s, &numArgs, 10))
			return -1;
		ARGLIST callArgs;

		for (int i = 0; i < numArgs; ++i)
		{
			getline(*file, value_s, ',');
			if (!caught_stoi(value_s, &argidx, 10))
				return -1;
			getline(*file, value_s, ',');
			string decodedarg = base64_decode(value_s);
			callArgs.push_back(make_pair(argidx, decodedarg));
		}
		if (!callArgs.empty())
			funcCalls.push_back(callArgs);
	}
	if (!funcCalls.empty())
		funcargs = funcCalls;

	file->seekg(1, ios::cur);
	return 1;
}