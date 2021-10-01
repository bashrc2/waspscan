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

#ifndef WASPSCAN_H
#define WASPSCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#define VERSION 1.00

/* Maximum number of steps in a search between min/max orbital periods */
#define MAX_SEARCH_STEPS      1000000

/* Maximum length of a series of values loaded from a log file */
#define MAX_SERIES_LENGTH     100000

/* the type of table */
#define TABLE_TYPE_WASP 0
#define TABLE_TYPE_K2   1

float detect_mean(float series[], int series_length);
float detect_variance(float series[], int series_length, float mean);
int logfile_load(char * filename, float timestamp[],
                 float series[], int max_series_length,
                 int time_field_index, int flux_field_index);
int gnuplot_tidy();
int gnuplot_distribution(char * title,
                         float timestamp[],
                         float series[], int series_length,
                         char * image_filename,
                         int image_width, int image_height,
                         float subtitle_indent_horizontal,
                         float subtitle_indent_vertical,
                         char * axis_label);
int gnuplot_light_curve(char * title,
                        float timestamp[],
                        float series[], int series_length,
                        char * image_filename,
                        int image_width, int image_height,
                        float subtitle_indent_horizontal,
                        float subtitle_indent_vertical,
                        char * axis_label,
                        float period_days,
                        float vertical_scale);
int gnuplot_light_curve_distribution(char * title,
                                     float timestamp[],
                                     float series[], int series_length,
                                     char * image_filename,
                                     int image_width, int image_height,
                                     float subtitle_indent_horizontal,
                                     float subtitle_indent_vertical,
                                     char * axis_label,
                                     float period_days,
                                     float vertical_scale);
void fft1D(float series[], int series_length, float freq[]);
int detect_endpoints(float timestamp[], int series_length,
                     int endpoints[]);
int light_curve(float timestamp[],
                float series[], int series_length,
                float period_days,
                float curve[], float density[], int curve_length);
void scan_name(char * filename, char * result);
float detect_orbital_period(float timestamp[],
                            float series[], int series_length,
                            float min_period_days,
                            float max_period_days,
                            float increment_days,
                            float min_dipped_density,
                            float max_dipped_percent,
                            float min_intermediate_percent,
                            float max_intermediate_percent,
                            float expected_dip_radius_percent,
                            float peak_threshold,
                            float max_vacancy_density,
                            float dip_threshold);
int detect_phase_offset(float curve[], int curve_length);
void adjust_curve(float curve[], int curve_length, int offset);

#endif
