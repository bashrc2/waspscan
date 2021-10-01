/*
    WASPscan: Detection of exoplanet transits
    Copyright (C) 2015-2016 Bob Mottram
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

void show_help()
{
    printf("WASPscan: Detection of exoplanet transits\n\n");
    printf(" -f  --filename              Log filename\n");
    printf(" -i  --incr                  Search increment in seconds\n");
    printf(" -p  --period                Known orbital period in days\n");
    printf(" -m  --minsamples            Minimum number of data samples\n");
    printf(" -0  --min                   Minimum orbital period in days\n");
    printf(" -1  --max                   Maximum orbital period in days\n");
    printf("     --maxvac                Maximum density within vacancy region\n");
    printf(" -t  --type                  Table type: 0=WASP 1=K2\n");
    printf("     --mindd                 Minimum dipped density (0.0 -> 1.0)\n");
    printf("     --minint                Minimum intermediate samples percent\n");
    printf("     --maxint                Maximum intermediate samples percent\n");
    printf("     --maxd                  Maximum dipped samples percentage\n");
    printf("     --peak                  Peak threshold (0.0 -> 1.0)\n");
    printf("     --dip                   Dip threshold (0.0 -> 1.0)\n");
    printf(" -r  --diprad                Expected dip radius as a percent of orbital period\n");
    printf("     --vscale                Vertical scaling factor\n");
    printf(" -h  --help                  Show help\n");
    printf(" -v  --version               Show version number\n");
}

int main(int argc, char* argv[])
{
    int i, series_length;
    float timestamp[MAX_SERIES_LENGTH];
    float series[MAX_SERIES_LENGTH];
    int endpoints[MAX_SERIES_LENGTH];
    int no_of_sections;
    char log_filename[256];
    char name[256], title[256*2];
    float orbital_period_days;
    int minimum_data_samples = 1000;
    float minimum_period_days = 0;
    float maximum_period_days = 0;
    float known_period_days = 0;
    char light_curve_filename[256*2];
    char light_curve_distribution_filename[256*2];
    int table_type = TABLE_TYPE_WASP;
    int time_field_index=0, flux_field_index=3;
    float vertical_scale = 1.0f;
    float search_increment_seconds = 0.864f;

    /* maximum density within the area of the dip expected to be vacant */
    float max_vacancy_density = 0.008f;

    /* fraction of the maximum point density below which a dip
       will be considered to be anomalous */
    float min_dipped_density = 0.3f;

    /* maximum percentage of samples in the dipped position */
    float max_dipped_percent = 15.0f;

    /* minimum percentage of samples which are neither dipped nor non-dipped */
    float min_intermediate_percent = 2.0f;

    /* maximum percentage of samples which are neither dipped nor non-dipped */
    float max_intermediate_percent = 10.0f;

    /* Expected dip radius as a percentage of the orbital period */
    float expected_dip_radius_percent = 2.0f;

    /* Threshold above the mean beyond which to disguard the curve */
    float peak_threshold = 0.6f;

    /* Threshold above which the profile will be considered to be dipped */
    float dip_threshold = 0.2f;

    /* if no options given then show help */
    if (argc <= 1) {
        show_help();
        return 1;
    }

    /* no filename specified */
    log_filename[0]=0;

    /* parse the options */
    for (i = 1; i < argc; i++) {
        /* log filename */
        if ((strcmp(argv[i],"-f")==0) ||
            (strcmp(argv[i],"--filename")==0)) {
            i++;
            if (i < argc) {
                sprintf(log_filename,"%s",argv[i]);
            }
        }
        /* Minimum data samples */
        if ((strcmp(argv[i],"-m")==0) ||
            (strcmp(argv[i],"--minsamples")==0)) {
            i++;
            if (i < argc) {
                minimum_data_samples = atoi(argv[i]);
            }
        }
        /* Minimum orbital period */
        if (strcmp(argv[i],"--vscale")==0) {
            i++;
            if (i < argc) {
                vertical_scale = atof(argv[i]);
            }
        }
        /* Minimum orbital period */
        if ((strcmp(argv[i],"-0")==0) ||
            (strcmp(argv[i],"--min")==0)) {
            i++;
            if (i < argc) {
                minimum_period_days = atof(argv[i]);
            }
        }
        /* Maximum orbital period */
        if ((strcmp(argv[i],"-1")==0) ||
            (strcmp(argv[i],"--max")==0)) {
            i++;
            if (i < argc) {
                maximum_period_days = atof(argv[i]);
            }
        }
        /* Search increment in seconds */
        if ((strcmp(argv[i],"-i")==0) ||
            (strcmp(argv[i],"--incr")==0)) {
            i++;
            if (i < argc) {
                search_increment_seconds = atof(argv[i]);
            }
        }
        /* Known orbital period */
        if ((strcmp(argv[i],"-p")==0) ||
            (strcmp(argv[i],"--period")==0)) {
            i++;
            if (i < argc) {
                known_period_days = atof(argv[i]);
            }
        }
        /* Maximum density within vacancy region */
        if (strcmp(argv[i],"--maxvac")==0) {
            i++;
            if (i < argc) {
                max_vacancy_density = atof(argv[i]);
            }
        }
        /* Minimum dipped density */
        if (strcmp(argv[i],"--mindd")==0) {
            i++;
            if (i < argc) {
                min_dipped_density = atof(argv[i]);
            }
        }
        /* Maximum dipped percent */
        if (strcmp(argv[i],"--maxd")==0) {
            i++;
            if (i < argc) {
                max_dipped_percent = atof(argv[i]);
            }
        }
        /* Expected dip radius percent percent */
        if ((strcmp(argv[i],"-r")==0) ||
            (strcmp(argv[i],"--diprad")==0)) {
            i++;
            if (i < argc) {
                expected_dip_radius_percent = atof(argv[i]);
            }
        }
        /* Maximum intermediates percent */
        if (strcmp(argv[i],"--maxint")==0) {
            i++;
            if (i < argc) {
                max_intermediate_percent = atof(argv[i]);
            }
        }
        /* Peak threshold */
        if (strcmp(argv[i],"--peak")==0) {
            i++;
            if (i < argc) {
                peak_threshold = atof(argv[i]);
            }
        }
        /*dip threshold */
        if (strcmp(argv[i],"--dip")==0) {
            i++;
            if (i < argc) {
                dip_threshold = atof(argv[i]);
            }
        }
        /* table type */
        if ((strcmp(argv[i],"-t")==0) ||
            (strcmp(argv[i],"--type")==0)) {
            i++;
            if (i < argc) {
                if ((strcmp(argv[i],"wasp")==0) ||
                    (strcmp(argv[i],"WASP")==0)) {
                    table_type = TABLE_TYPE_WASP;
                }
                if ((strcmp(argv[i],"k2")==0) ||
                    (strcmp(argv[i],"K2")==0)) {
                    table_type = TABLE_TYPE_K2;
                }
            }
        }
        /* show help */
        if ((strcmp(argv[i],"-h")==0) ||
                (strcmp(argv[i],"--help")==0)) {
            show_help();
            return 0;
        }
        /* show version number */
        if ((strcmp(argv[i],"-v")==0) ||
            (strcmp(argv[i],"--version")==0)) {
            printf("Version %.2f\n",VERSION);
            return 0;
        }
    }

    if (log_filename[0]==0) {
        printf("No log file specified\n");
        return -1;
    }

    if (known_period_days == 0) {
        if (maximum_period_days == 0) {
            printf("No maximum orbital period specified\n");
            return -2;
        }

        if (maximum_period_days <= minimum_period_days) {
            printf("Maximum orbital period must be greater ");
            printf("than the minimum orbital period\n");
            return -3;
        }
    }

    /* get the name of the scan from the log filename */
    scan_name(log_filename, name);

    /* change the table columns based upon the format type */
    switch(table_type) {
    case TABLE_TYPE_WASP: {
        time_field_index=0;
        flux_field_index=3;
        break;
    }
    case TABLE_TYPE_K2: {
        time_field_index=0;
        flux_field_index=2;
        break;
    }
    }

    /* read the data */
    series_length = logfile_load(log_filename,
                                 timestamp,
                                 series,
                                 MAX_SERIES_LENGTH,
                                 time_field_index, flux_field_index);
    if (series_length < minimum_data_samples) {
        printf("Number of data samples too small: %d\n", series_length);
        return 1;
    }
    printf("%d values loaded\n", series_length);

    no_of_sections = detect_endpoints(timestamp, series_length,
                                      endpoints);
    if (no_of_sections == 0) {
        printf("No sections detected in the time series\n");
        return 2;
    }

    /*orbital_period_days = 1.3382282f;*/

    if (known_period_days == 0) {
        orbital_period_days =
            detect_orbital_period(timestamp,
                                  series, series_length,
                                  minimum_period_days,
                                  maximum_period_days,
                                  search_increment_seconds / (60.0f * 60.0f * 24.0f),
                                  min_dipped_density,
                                  max_dipped_percent,
                                  min_intermediate_percent,
                                  max_intermediate_percent,
                                  expected_dip_radius_percent,
                                  peak_threshold,
                                  max_vacancy_density,
                                  dip_threshold);
        if (orbital_period_days == 0) {
            printf("No transits detected\n");
            return -5;
        }
        printf("orbital_period_days %.6f\n",orbital_period_days);
    }
    else {
        orbital_period_days = known_period_days;
    }

    sprintf(light_curve_filename,"%s.png",name);
    sprintf(light_curve_distribution_filename,"%s_distr.png",name);
    sprintf(title,"SuperWASP Light Curve for %s",name);
    gnuplot_light_curve_distribution(title,
                                     timestamp, series, series_length,
                                     light_curve_distribution_filename,
                                     1024, 640,
                                     0.44,0.93,
                                     "TAMUZ corrected processed flux (micro Vega)",
                                     orbital_period_days,
                                     vertical_scale);
    gnuplot_light_curve(title,
                        timestamp, series, series_length,
                        light_curve_filename,
                        1024, 640,
                        0.44,0.93,
                        "TAMUZ corrected processed flux (micro Vega)",
                        orbital_period_days,
                        vertical_scale);

    gnuplot_tidy();
    return 0;
}
