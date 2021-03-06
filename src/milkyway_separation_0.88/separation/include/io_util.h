/*
Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _IO_UTIL_H_
#define _IO_UTIL_H_

#include "milkyway_util.h"
#include "separation_types.h"

void fwrite_double_array(FILE* file, const char* array_name, real* array_t, size_t size);
void fwrite_int_array(FILE* file, const char* array_name, int* array_t, size_t size);

real* fread_double_array(FILE* file, const char* array_name, unsigned int* sizeOut);
int* fread_int_array(FILE* file, const char* array_name, unsigned int* sizeOut);

void printIntegralArea(const IntegralArea* ia);
void printAstronomyParameters(const AstronomyParameters* ap);
void printNuConstants(const NuConstants* c, unsigned int n);
void printStreamGauss(const StreamGauss* c, unsigned int n);
void printStreamConstants(const StreamConstants* c, unsigned int n);
void printSeparationResults(const SeparationResults* results, unsigned int numberStreams);

SeparationResults* readReferenceResults(const char* refFile, unsigned int nStream);

#endif /* _IO_UTIL_H_ */

