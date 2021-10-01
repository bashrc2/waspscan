/*
  WASPscan: Detection of exoplanet transits
  Copyright (C) 2015 Bob Mottram
  bob@libreserver.org

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

/* field index */
#define TMID         0  /* Mid-time of exposure (sec) */
#define FLUX2        1  /* Processed flux (micro Vega) */
#define FLUX2_ERR    2  /* Processed flux error (micro Vega) */
#define TAMFLUX2     3  /* TAMUZ corrected processed flux (micro Vega) */
#define TAMFLUX2_ERR 4  /* TAMUZ flux error (micro Vega) */
#define IMAGEID      5  /* Unique image ID */
#define CCDX         6  /* X position on the CCD (1/16th of pixel) */
#define CCDY         7  /* Y position on the CCD (1/16th of pixel) */
#define FLAG         8  /* Bitmask */
#define HJD          9  /* Date */
#define MAG2         10

#include "waspscan.h"

/**
 * @brief Loads a WASP log file containing times and magnitudes
 *        for a given star
 * @param filename Log filename
 * @param timestamp Returned array containing the time of each data point
 * @param series Returned array containing magnitudes
 * @param max_series_length The maximum number of data points to be returned
 * @param time_field_index Index of the table column which contains the time
 * @param flux_field_index Index of the table column which contains the
 *        photon flux
 * @returns The number of data points loaded
 */
int logfile_load(char * filename, float timestamp[],
                 float series[], int max_series_length,
				 int time_field_index, int flux_field_index)
{
    FILE * fp;
    char linestr[512], valuestr[512];
    char * retval = NULL;
    int i,ctr=0,field_index;
    int series_length=0;

    fp = fopen(filename,"r");
    if (!fp) return -1;

    while (!feof(fp)) {
        retval = fgets(linestr,511,fp);
        if (retval != NULL) {
            if (strlen(linestr) < 1) continue;
            if ((linestr[0]=='\\') ||
                (linestr[0]=='|')) {
                continue;
            }
            field_index = 0;
            ctr=0;
            for (i = 0; i < strlen(linestr); i++) {
                if (!((linestr[i]==' ') || (linestr[i]=='\t'))) {
                    valuestr[ctr++] = linestr[i];
                }
                else {
                    if (ctr > 0) {
                        valuestr[ctr]=0;
                        if (field_index == time_field_index) {
                            timestamp[series_length] = atof(valuestr);
                        }
                        if (field_index == flux_field_index) {
                            series[series_length++] = atof(valuestr);
                            if (series_length >= max_series_length) {
                                fclose(fp);
                                return(series_length);
                            }
                            break;
                        }
                        ctr = 0;
                        field_index++;
                    }
                }
            }
        }
    }

    fclose(fp);
    return series_length;
}
