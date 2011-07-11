// $Id: extract.cpp 2828 2010-02-01 16:07:58Z hieuhoang1972 $
// vim:tabstop=2

/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <cstdio>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <sstream>
#include "extract.h"
#include "InputFileStream.h"
#include "Lattice.h"

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

using namespace std;

int main(int argc, char* argv[]) 
{
  cerr << "Extract v2.0, written by Philipp Koehn\n"
       << "rule extraction from an aligned parallel corpus\n";
  //time_t starttime = time(NULL);
	
	Global *global = new Global();
	g_global = global;

		
	if (argc < 5) {
		cerr << "syntax: extract corpus.target corpus.source corpus.align extract "
		     << " [ --Hierarchical | --Orientation"
				 << " | --GlueGrammar FILE | --UnknownWordLabel FILE"
				 << " | --OnlyDirect"
					
					<< " | --MinHoleSpanSourceDefault[" << global->minHoleSpanSourceDefault << "]"
					<< " | --MaxHoleSpanSourceDefault[" << global->maxHoleSpanSourceDefault << "]"
					<< " | --MinHoleSpanSourceSyntax[" << global->minHoleSpanSourceSyntax << "]"
					<< " | --MaxHoleSpanSourceSyntax[" << global->maxHoleSpanSourceSyntax << "]"

				<< " | --MaxSymbolsSource[" << global->maxSymbolsSource << "]"
				 << " | --MaxNonTerm[" << global->maxNonTerm << "]"
		     << " | --SourceSyntax | --TargetSyntax" 
				<<	" | --UppermostOnly[" << g_global->uppermostOnly << "]"
				<< endl;
		exit(1);
	}
  char* &fileNameT = argv[1];
  char* &fileNameS = argv[2];
  char* &fileNameA = argv[3];
	string fileNameGlueGrammar;
 	string fileNameUnknownWordLabel;
	string fileNameExtract = string(argv[4]);

	int optionInd = 5;

  for(int i=optionInd;i<argc;i++) 
	{
		if (strcmp(argv[i],"--MinHoleSpanSourceDefault") == 0) {
			global->minHoleSpanSourceDefault = atoi(argv[++i]);
			if (global->minHoleSpanSourceDefault < 1) {
				cerr << "extract error: --minHoleSourceDefault should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--MaxHoleSpanSourceDefault") == 0) {
			global->maxHoleSpanSourceDefault = atoi(argv[++i]);
			if (global->maxHoleSpanSourceDefault < 1) {
				cerr << "extract error: --maxHoleSourceDefault should be at least 1" << endl;
				exit(1);
			}
		}
		else  if (strcmp(argv[i],"--MinHoleSpanSourceSyntax") == 0) {
			global->minHoleSpanSourceSyntax = atoi(argv[++i]);
			if (global->minHoleSpanSourceSyntax < 1) {
				cerr << "extract error: --minHoleSourceSyntax should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--UppermostOnly") == 0) {
			global->uppermostOnly = atoi(argv[++i]);
		}
		else if (strcmp(argv[i],"--MaxHoleSpanSourceSyntax") == 0) {
			global->maxHoleSpanSourceSyntax = atoi(argv[++i]);
			if (global->maxHoleSpanSourceSyntax < 1) {
				cerr << "extract error: --maxHoleSourceSyntax should be at least 1" << endl;
				exit(1);
			}
		}
		
		// maximum number of words in hierarchical phrase
		else if (strcmp(argv[i],"--MaxSymbolsSource") == 0) {
			global->maxSymbolsSource = atoi(argv[++i]);
			if (global->maxSymbolsSource < 1) {
				cerr << "extract error: --MaxSymbolsSource should be at least 1" << endl;
				exit(1);
			}
		}
		// maximum number of non-terminals
		else if (strcmp(argv[i],"--MaxNonTerm") == 0) {
			global->maxNonTerm = atoi(argv[++i]);
			if (global->maxNonTerm < 1) {
				cerr << "extract error: --MaxNonTerm should be at least 1" << endl;
				exit(1);
			}
		}		
		// allow consecutive non-terminals (X Y | X Y)
    else if (strcmp(argv[i],"--TargetSyntax") == 0) {
      global->targetSyntax = true;
    }
    else if (strcmp(argv[i],"--SourceSyntax") == 0) {
      global->sourceSyntax = true;
    }
		// do not create many part00xx files!
    else if (strcmp(argv[i],"--NoFileLimit") == 0) {
      // now default
    }
		else if (strcmp(argv[i],"--GlueGrammar") == 0) {
			global->glueGrammarFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --GlueGrammar requires a file name" << endl;
				exit(0);
			}
			fileNameGlueGrammar = string(argv[i]);
			cerr << "creating glue grammar in '" << fileNameGlueGrammar << "'" << endl;
    }
		else if (strcmp(argv[i],"--UnknownWordLabel") == 0) {
			global->unknownWordLabelFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --UnknownWordLabel requires a file name" << endl;
				exit(0);
			}
			fileNameUnknownWordLabel = string(argv[i]);
			cerr << "creating unknown word labels in '" << fileNameUnknownWordLabel << "'" << endl;
		}
		// TODO: this should be a useful option
    //else if (strcmp(argv[i],"--ZipFiles") == 0) {
    //  zipFiles = true;
    //}
		// if an source phrase is paired with two target phrases, then count(t|s) = 0.5
    else if (strcmp(argv[i],"--Mixed") == 0) {
			global->mixed = true;
    }
		else if (strcmp(argv[i],"--AllowDefaultNonTermEdge") == 0) {
			global->allowDefaultNonTermEdge = atoi(argv[++i]);
		}
		
    else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

	// open input files
	Moses::InputFileStream tFile(fileNameT);
	Moses::InputFileStream sFile(fileNameS);
	Moses::InputFileStream aFile(fileNameA);

	// open output files
  string fileNameExtractInv = fileNameExtract + ".inv";
  string fileNameExtractOrientation = fileNameExtract + ".o";
  extractFile.open(fileNameExtract.c_str());
  
  
	// loop through all sentence pairs
  int i=0;
  while(true) {
    i++;

    //if (i%1000 == 0) cerr << "." << flush;
    //if (i%10000 == 0) cerr << ":" << flush;
    //if (i%100000 == 0) cerr << "!" << flush;
    string targetString;
    string sourceString;
    string alignmentString;
		
		bool ok = getline(tFile, targetString);
		if (!ok)
			break;
		getline(sFile, sourceString);
		getline(aFile, alignmentString);
    
		cerr << i << " ";

		//cerr << endl << targetString << endl << sourceString << endl << alignmentString << endl;

		//time_t currTime = time(NULL);
		//cerr << "A " << (currTime - starttime) << endl;

    SentenceAlignment sentencePair;
    if (sentencePair.Create( targetString, sourceString, alignmentString, i, *global )) 
		{			
			//cerr << sentence.sourceTree << endl;
			//cerr << sentence.targetTree << endl;

			sentencePair.FindTunnels(*g_global);
			//cerr << "C " << (time(NULL) - starttime) << endl;
			//cerr << sentencePair << endl;
			
			sentencePair.CreateLattice(*g_global);
			//cerr << "D " << (time(NULL) - starttime) << endl;
			//cerr << sentencePair << endl;

			sentencePair.CreateRules(*g_global);
			//cerr << "E " << (time(NULL) - starttime) << endl;

			//cerr << sentence.lattice->GetRules().GetSize() << endl;
			extractFile << sentencePair.GetLattice().GetRules();
    }

  }
	
  tFile.Close();
  sFile.Close();
  aFile.Close();
	
	delete global;
}
 

