/*
  Copyright (C) 2015 Bob Mottram
  This is based on the example from Rosetta Code
  http://rosettacode.org/wiki/Fast_Fourier_transform#C
  which was originally under GNU Free Documentation License 1.2
  bob@robotics.uk.to

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

#include "waspscan.h"

/**
 * @brief Turns a filename into a name usable as a title
 * @param filename The filename
 * @param result The returned scan name
 */
void scan_name(char * filename, char * result)
{
    int start_index, end_index, ctr=0, i;

    for (start_index = strlen(filename)-1; start_index > 0; start_index--) {
        if (filename[start_index] == '/') {
            start_index++;
            break;
        }
    }

    end_index = strlen(filename)-1;
    for (i = strlen(filename)-3; i > 1; i--) {
        if (filename[i] == '.') {
            if ((filename[i+1] == 't') &&
                (filename[i+2] == 'b') &&
                (filename[i+3] == 'l')) {
                end_index = i-1;
            }
            if ((filename[i+1] == 'f') &&
                (filename[i+2] == 'i') &&
                (filename[i+3] == 't')) {
                end_index = i-1;
            }
        }
    }

    for (i = start_index; i <= end_index; i++) {
        result[ctr++] = filename[i];
    }
    result[ctr]=0;
}
