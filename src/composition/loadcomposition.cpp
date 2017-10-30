/*
Copyright (C) 2013 Bruno Golosio

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
///////////////////////////////////
//     loadcomposition.cpp       //
//        31/10/2017             //
//   author : Bruno Golosio      //
///////////////////////////////////
// Load sample phases composition and density
//

#include <sstream>
#include <iostream>
#include <string>
#include "xrmc.h"
#include "xrmc_composition.h"
#include "xrmc_gettoken.h"
#include "xrmc_exception.h"
#include <xraylib.h>
#include <cstring>

using namespace std;
using namespace gettoken;

int composition::Load(istream &fs)
{
  /* Loads sample phases composition and density */
  int n_comp;
  int n_elem;
  int z_elem;
  double w, rho;
  string comm, comp;
  struct compoundData *cd;

  cout << "Composition file\n";

 // get a command/variable name from input file
  while (GetToken(fs, comm)) {
    // parse the command and decide what to do
    if (comm=="End") break;
    else if(comm=="Phase") { // read parameters for a new phase
      phase *new_ph = new phase;
      int i_phase = Ph.size();
      Ph.push_back(*new_ph);
      MapPhase(fs, i_phase);
      GetToken(fs, comm);
      if (comm != "NElem")
	throw xrmc_exception("NElem variable initialization not found"); 
      GetIntToken(fs, &n_comp); // read number of elements in the phase
      //Ph[i_phase].NElem = n_elem;
      //Ph[i_phase].W = new double[n_elem];  // initializes weight fraction,
      //Ph[i_phase].Z = new int[n_elem];     // atomic numbers and absorption
      //Ph[i_phase].MuAtom = new double[n_elem]; // coefficient arrays
      
      n_elem = 0;
      int elem;
      int elem_found;
      int Z;
      for(int comp_idx=0; comp_idx<n_comp; comp_idx++) { //read element Z and w
	GetToken(fs, comp);
	GetDoubleToken(fs, &w);
	istringstream buffer(comp);
	if ((buffer >> elem)) {
		//classic mode: integer found
		elem_found = 0;
		for (Z = 0 ; Z < n_elem ; Z++) {
			if (elem == Ph[i_phase].Z[Z]) {
				elem_found = 1;
				break;
			}
		}
		if (elem_found) {
			Ph[i_phase].Z[Z] = elem;
			Ph[i_phase].W[Z] += w/100.0;
		}
		else {
			Ph[i_phase].Z = (int*) realloc(Ph[i_phase].Z, sizeof(int)*++n_elem);
			Ph[i_phase].W = (double *) realloc(Ph[i_phase].W, sizeof(double)*n_elem);
			Ph[i_phase].Z[n_elem-1] = elem;
			Ph[i_phase].W[n_elem-1] = w/100.0;
		}
	}
	else {
		//modern mode: chemical formula
		if ((cd = CompoundParser(comp.c_str())) == NULL){
   			throw xrmc_exception("Cannot parse compound\n");
		}
		int Zcd;
		for (Zcd = 0 ; Zcd < cd->nElements ; Zcd++) {
			elem_found = 0;
			for (Z = 0 ; Z < n_elem ; Z++) {
				if (cd->Elements[Zcd] == Ph[i_phase].Z[Z]) {
					elem_found = 1;
					break;
				}	
			}
			if (elem_found) {
				Ph[i_phase].W[Z] += w*cd->massFractions[Zcd]/100.0;
			}
			else {
				Ph[i_phase].Z = (int *) realloc(Ph[i_phase].Z, sizeof(int)*++n_elem);
				Ph[i_phase].W = (double *) realloc(Ph[i_phase].W, sizeof(double)*n_elem);
				Ph[i_phase].Z[n_elem-1] = cd->Elements[Zcd];
				Ph[i_phase].W[n_elem-1] = w*cd->massFractions[Zcd]/100.0;
			}
		}
		FreeCompoundData(cd);
	}	
      }
      Ph[i_phase].MuAtom = (double *) malloc(sizeof(double)*n_elem);
      Ph[i_phase].NElem = n_elem;
      cout << "Num. of compounds: " << n_elem << endl;
      cout << "\tZ\tweight fract.\n";
      for (int elem_idx = 0 ; elem_idx < n_elem ; elem_idx++) {
		cout << "\t" << Ph[i_phase].Z[elem_idx] << "\t"
	     	<< Ph[i_phase].W[elem_idx] << endl;
      }
      GetToken(fs, comm);
      if (comm != "Rho")
	throw xrmc_exception("Rho variable initialization not found"); 
      GetDoubleToken(fs, &rho); // read mass density of the phase in g/c3
      Ph[i_phase].Rho = rho;
      cout << "\tRho: " << Ph[i_phase].Rho << endl;    
    }
    else {
      throw xrmc_exception("Syntax error in composition file\n");
      // unrecognized command
    }
  }

  return 0;
}

// insert name and pointer to the phase in the phase map
int composition::MapPhase(istream &fs, int i_phase)
{
  string ph_name;
  phase_map_insert_pair insert_pair;

  if (!GetToken(fs, ph_name))
    throw xrmc_exception(string("Syntax error reading phase name\n"));
  // insert name and pointer to the phase in the phase map
  insert_pair = PhaseMap.insert(phase_map_pair(ph_name, i_phase));
  if(insert_pair.second == false) // check that it was not already inserted
    throw xrmc_exception(string("Phase ") + ph_name + 
			 " already inserted in phase map\n");
  cout << "Phase: " << ph_name << endl; 

  return 0;
}

//////////////////////////////////////////////////////////////////////
// set default values for composition parameters
int composition::SetDefault()
{
  phase_map_insert_pair insert_pair;

  Ph.clear();

  // initializes phase 0 as vacuum
  phase *new_phase = new phase;
  Ph.push_back(*new_phase);
  Ph[0].NElem = 0;
  Ph[0].Rho = 0;
  string ph_name="Vacuum";
  insert_pair = PhaseMap.insert(phase_map_pair(ph_name, 0));

  //add NIST compound database
  struct compoundDataNIST *cdn;
  char **list = GetCompoundDataNISTList(NULL);

  for (int i = 0 ; list[i] != NULL ; i++) {
	cdn = GetCompoundDataNISTByIndex(i);
	cout << cdn->name << endl;
  	xrlFree(list[i]);
	int i_phase = Ph.size();
	cout << i_phase << endl;
	/*
	Ph.push_back(*new_phase);
	Ph[i_phase].NElem = cdn->nElements;
	Ph[i_phase].Z = (int *) malloc(sizeof(int)*cdn->nElements);
	memcpy(Ph[i_phase].Z, cdn->Elements, sizeof(int)*cdn->nElements);
	Ph[i_phase].W = (double *) malloc(sizeof(double)*cdn->nElements);
	memcpy(Ph[i_phase].W, cdn->massFractions, sizeof(double)*cdn->nElements);
	Ph[i_phase].Rho = cdn->density;
	Ph[i_phase].MuAtom = (double *) malloc(sizeof(double)*cdn->nElements);
  	insert_pair = PhaseMap.insert(phase_map_pair(cdn->name, i_phase));
	*/
	FreeCompoundDataNIST(cdn);	
  }
  xrlFree(list);
  return 0;
}
