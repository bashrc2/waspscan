/*
  Copyright (C) 2015-2016 Bob Mottram
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

#include "waspscan.h"

/* length of the light curve used for transit detection */
#define DETECT_CURVE_LENGTH 256

/* maximum percentage of the light curve which may be missing */
#define MISSING_THRESHOLD   0

/* number of seconds in a day */
#define DAY_SECONDS (1.0f / (60.0f*60.0f*24.0f))

/**
 * @brief Detects the starting and ending indexes of active
 *        data sections within a time series
 * @param timestamp A series of timestamps
 * @param series_length The number of entries in the series
 * @param endpoints An array of returned start and end indexes
 * @returns The number of data sections within the series
 */
int detect_endpoints(float timestamp[], int series_length,
                     int endpoints[])
{
    int start_index = 0;
    float variance;
    float threshold, dt, av_dt = 0, dt_variance = 0;
    int i,ctr=0;

    for (i = 1; i < series_length; i++)
        av_dt += timestamp[i] - timestamp[i-1];
    av_dt /= (float)(series_length-1);

    for (i = 1; i < series_length; i++) {
        variance = (timestamp[i] - timestamp[i-1]) - av_dt;
        dt_variance += variance*variance;
    }
    dt_variance = (float)sqrt(dt_variance/(float)(series_length-1));
    threshold = dt_variance*10;

    for (i = 1; i < series_length-1; i++) {
        dt = timestamp[i] - timestamp[i-1];
        if (dt > threshold) {
            endpoints[ctr*2] = start_index;
            endpoints[ctr*2+1] = i-1;
            ctr++;
            start_index = i+1;
        }
    }
    endpoints[ctr*2] = -1;
    return ctr;
}

/**
 * @brief Calculates the amount of variance for a light curve
 * @param period_days Orbital period in days
 * @param timestamp
 */
static float light_curve_variance(float period_days,
                                  float timestamp[],
                                  float series[],
                                  int series_length,
                                  float curve[], int curve_length)
{
    int i, index;
    float days, variance = 0;
    float mult = (float)curve_length / period_days;

    for (i = series_length-1; i >= 0; i--) {
        days = timestamp[i] * DAY_SECONDS;
        index = (int)(fmod(days,period_days) * mult);
        variance += (series[i] - curve[index])*(series[i] - curve[index]);
    }
    return (float)sqrt(variance/(float)series_length);
}

/**
 * @brief Returns an array containing a light curve for the given orbital period_days
 * @param timestamp Array of imaging times
 * @param series Array containing magnitudes
 * @param series_length The length of the data series
 * @param period_days The expected orbital period
 * @param curve Returned light curve Array
 * @param density Density of samples
 * @param curve_length The number of buckets within the curve
 */
static void light_curve_base(float timestamp[],
                             float series[], int series_length,
                             float period_days,
                             float curve[], float density[], int curve_length)
{
    int i, index;
    float days, max_samples=0;
    float mult = (float)curve_length / period_days;

    memset(curve,0,curve_length*sizeof(float));
    memset(density,0,curve_length*sizeof(float));

    for (i = series_length-1; i >= 0; i--) {
        days = timestamp[i] * DAY_SECONDS;
        index = (int)(fmod(days,period_days) * mult);
        curve[index] += series[i];
        density[index]++;
    }

    for (i = curve_length-1; i >= 0; i--) {
        if (density[i] > 0) curve[i] /= density[i];
        if (density[i] > max_samples) max_samples = density[i];
    }

    /* normalise */
    for (i = curve_length-1; i >= 0; i--) density[i] /= max_samples;
}

/**
 * @brief Returns a value indicating the density of samples within
 *        the region of the dip expected to be vacant
 * @param start_index Starting index for the dip within the light curve
 * @param end_index Ending index for the dip within the light curve
 * @param series Array containing magnitudes
 * @param series_length The length of the data series
 * @param period_days The expected orbital period
 * @param curve Existing light curve Array
 * @param curve_length The number of buckets within the curve
 * @returns Density of samples within expected vacant area in the range 0.0-1.0
 */
static float dip_vacancy(int start_index, int end_index,
                         float timestamp[],
                         float series[], int series_length,
                         float period_days,
                         float curve[],
                         int curve_length)
{
    int i, index;
    float days, max_samples = 0;
    float density = 0, curve_average_mag = 0;
    float curve_variance = 0, min_curve_mag;
    float den[DETECT_CURVE_LENGTH];
    float mult = (float)curve_length / period_days;

    /* get the average magnitude */
    for (i = curve_length-1; i >= 0; i--) curve_average_mag += curve[i];
    memset(den,0,curve_length*sizeof(float));
    curve_average_mag /= (float)curve_length;

    /* get the variance */
    for (i = curve_length-1; i >= 0; i--)
        curve_variance +=
            (curve[i] - curve_average_mag)*(curve[i] - curve_average_mag);

    curve_variance = (float)sqrt(curve_variance / (float)curve_length);
    min_curve_mag = curve_average_mag - (curve_variance*2.0f);

    /* find the number of points within the dip region which are
       within the expected vacancy area */
    for (i = series_length-1; i >= 0; i--) {
        days = timestamp[i] * DAY_SECONDS;
        index = (int)(fmod(days,period_days) * mult);
        den[index]++;
        if ((index >= start_index) && (index < end_index)) {
            if (series[i] > min_curve_mag) {
                density++;
            }
        }
    }

    /* get the maximum density samples */
    for (i = curve_length-1; i >= 0; i--)
        if (den[i] > max_samples) max_samples = den[i];

    if (max_samples > 0)
        return density / (max_samples * ((end_index - start_index)+1));
    return 1.0;
}

/**
 * @brief Resamples a light curve within bounds.
 *        This is used to disguard outliers which otherwise
 *        cause distraction.
 * @param min_value Minimum value
 * @param max_value Maximum value
 * @param timestamp Array of imaging times
 * @param series Array containing magnitudes
 * @param series_length The length of the data series
 * @param period_days The expected orbital period
 * @param curve Returned light curve Array
 * @param density Density of samples
 * @param curve_length The number of buckets within the curve
 */
static void light_curve_resample(float min_value, float max_value,
                                 float timestamp[],
                                 float series[], int series_length,
                                 float period_days,
                                 float curve[], int curve_length)
{
    int i, index;
    float days;
    int hits[512];
    float mult = (float)curve_length / period_days;

    memset(curve,0,curve_length*sizeof(float));
    memset(hits,0,curve_length*sizeof(int));

    for (i = series_length-1; i >= 0; i--) {
        if ((series[i] < min_value) ||
            (series[i] > max_value)) {
            continue;
        }
        days = timestamp[i] * DAY_SECONDS;
        index = (int)(fmod(days,period_days) * mult);
        curve[index] += series[i];
        hits[index]++;
    }

    for (i = curve_length-1; i >= 0; i--)
        if (curve[i] > 0) curve[i] /= hits[i];

    /* fill any holes */
    curve[0] = curve[curve_length-1];
    for (i = 1; i < curve_length; i++) {
        if (curve[i] != 0) continue;
	curve[i] = curve[i-1];
    }
}

/**
 * @brief Returns the average value for all data points
 * @param series Array containing the data points
 * @param series_length Length of the array
 * @returns Average value
 */
float detect_av(float series[], int series_length)
{
    int i;
    double av = 0;

    for (i = series_length-1; i >= 0; i--) av += series[i];
    return (float)(av/(float)series_length);
}

/**
 * @brief Returns the variance for all data points in a series
 * @param series Array containing data points
 * @param series_length Length of the Array
 * @param av Pre-calculated average value
 * @returns Variance value
 */
float detect_variance(float series[], int series_length, float av)
{
    int i;
    double variance = 0;

    for (i = series_length-1; i >= 0; i--)
        variance += (series[i] - av)*(series[i] - av);

    return (float)sqrt(variance/(float)series_length);
}

/**
 * @brief returns the number of buckets within a light curve for
 *        which no data exists
 * @param density The number of data samples within each bucket
 * @param The length of the light curve (number of buckets)
 * @return The number of buckets having zero data samples
 */
int missing_data(float density[], int curve_length)
{
    int i, missing = 0;

    for (i = curve_length-1; i >= 0; i--)
        if (density[i] == 0) missing++;

    return missing;
}

/**
 * @brief Returns an array containing a light curve for the given orbital period_days
 * @param timestamp Array of imaging times
 * @param series Array containing magnitudes
 * @param series_length The length of the data series
 * @param period_days The expected orbital period
 * @param curve Returned light curve Array
 * @param density Density of samples
 * @param curve_length The number of buckets within the curve
 * @return zero on success
 */
int light_curve(float timestamp[],
                float series[], int series_length,
                float period_days,
                float curve[], float density[], int curve_length)
{
    float av, variance;

    light_curve_base(timestamp, series, series_length,
                     period_days, curve, density, curve_length);

    if (missing_data(density, curve_length)*100/curve_length >
        MISSING_THRESHOLD)
        return -1;

    av = detect_av(series, series_length);
    variance = detect_variance(series, series_length, av);

    light_curve_resample(av - variance, av + variance,
                         timestamp, series, series_length,
                         period_days, curve, curve_length);
    return 0;
}

/**
 * @brief Detects the array index of the centre of the transit
 * @param curve Array containing light curve magnitudes
 * @param curve_length Length of the array
 * @returns Array index of the centre of the transit
 */
int detect_phase_offset(float curve[], int curve_length)
{
    int i, j, l, offset = 0;
    float minimum = 0, v;
    int search_radius = curve_length*5/100;

    for (i = 0; i < curve_length; i++) {
        v = 0;
        for (j = i-search_radius; j <= i+search_radius; j++) {
            l = j;
            if (l < 0) l += curve_length;
            if (l >= curve_length) l -= curve_length;
            v += curve[l];
        }
        if ((v < minimum) || (minimum == 0)) {
            minimum = v;
            offset = i;
        }
    }
    return offset;
}

/**
 * @brief Adjust the light curve so that the transit is at the centre
 * @param curve Array containing light curve magnitudes
 * @param curve_length Length of the array
 * @param offset Array index of the centre of the transit
 */
void adjust_curve(float curve[], int curve_length, int offset)
{
    int i, index;
    int adjust = (curve_length/2) - offset;
    float new_curve[512];

    for (i = 0; i < curve_length; i++) {
        index = i + adjust;
        if (index < 0) index += curve_length;
        if (index >= curve_length) index -= curve_length;
        new_curve[index] = curve[i];
    }

    memcpy(curve,new_curve,curve_length*sizeof(float));
}

/**
 * @brief Attempts to detect the orbital period via the transit method.
 *        This tries many possible periods and looks for a dip in
 *        magnitude.
 * @param timestamp Times for observations
 * @param series Magnitude observations
 * @param series_length Length of the Array
 * @param min_period_days The minimum orbital period in days
 * @param max_period_days The maximum orbital period in days
 * @param increment_days The time increment used within the min/max range
 * @params min_dipped_density fraction of the maximum point density below
 *                            which a dip will be considered to be anomalous
 * @params max_dipped_percent Maximum percent of points which are dipped
 * @params min_intermediate_percent Minimum percent of samples between
 *                                  dipped and non-dipped
 * @params max_intermediate_percent Maximum percent of samples between
 *                                  dipped and non-dipped
 * @params expected_dip_radius_percent Expected dip radius as a percentage of
 *                                     the orbital period
 * @params peak_threshold Threshold above the average beyond which to
 *                        disguard the curve
 * @params max_vacancy_density Maximum density within the area of the dip
 *                             expected to be vacant
 * @params dip_threshold A threshold above which the profile will be
 *                       considered to be in the dipped state
 * @returns The best candidate orbital period, or zero if no transit found
 */
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
                            float dip_threshold)
{
    float period_days=0;
    float max_response = 0;
    int expected_width =
        (int)(DETECT_CURVE_LENGTH*expected_dip_radius_percent/100.0f);
    int max_dipped = (int)(DETECT_CURVE_LENGTH*max_dipped_percent/100.0f);
    int max_intermediates =
        (int)(DETECT_CURVE_LENGTH*max_intermediate_percent/100.0f);
    int min_intermediates =
        (int)(DETECT_CURVE_LENGTH*min_intermediate_percent/100.0f);
    int step = 0;
    int steps = (int)((max_period_days - min_period_days)/increment_days);
    float response[MAX_SEARCH_STEPS];

    if (steps > MAX_SEARCH_STEPS) {
        printf("Maximum number of time steps exceeded\n");
        return 0;
    }

    /* Try different orbital periods in parallel */
#pragma omp parallel for
    for (step = 0; step < steps; step++) {
        float curve[DETECT_CURVE_LENGTH];
        float density[DETECT_CURVE_LENGTH];
        float orbital_period_days = min_period_days + (step*increment_days);

        if (light_curve(timestamp, series, series_length,
                        orbital_period_days,
                        curve, density, DETECT_CURVE_LENGTH) != 0)
            continue;

        float variance =
            light_curve_variance(orbital_period_days,
                                 timestamp, series,
                                 series_length,
                                 curve, DETECT_CURVE_LENGTH);

        /* calculate the av */
        float av = 0;
        int hits = 0;
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if (curve[j] <= 0) continue;
            av += curve[j];
            hits++;
        }
        /* there should be no gaps in the series */
        if (hits < DETECT_CURVE_LENGTH) {
            response[step] = 0;
            continue;
        }
        av /= (float)hits;

        /* average density of samples */
        float av_density = 0;
        hits = 0;
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if (density[j] <= 0) continue;
            av_density += density[j];
            hits++;
        }
        av_density /= (float)hits;

        /* variation in the density of samples */
        float density_variance = 0;
        hits = 0;
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if (density[j] > 0) {
                density_variance +=
                    (density[j] - av_density)*
                    (density[j] - av_density);
                hits++;
            }
        }
        density_variance = (float)(density_variance / (float)hits);

        /* find the minimum */
        float minimum = 0;
        for (int j = 0; j < DETECT_CURVE_LENGTH; j++) {
            float v = 0;
            hits = 0;
            for (int k = j-expected_width; k <= j+expected_width; k++) {
                int l = k;
                if (l < 0) l += DETECT_CURVE_LENGTH;
                if (l >= DETECT_CURVE_LENGTH) l -= DETECT_CURVE_LENGTH;
                if (curve[l] > 0) {
                    v += curve[l];
                    hits++;
                    if (k == j) {
                        v += curve[l];
                        hits++;
                    }
                }
            }
            if (hits > 0) {
                v /= hits;
                if ((v < minimum) || (minimum == 0)) {
                    minimum = v;
                }
            }
        }

        /* start and end indexes of the dip */
        int start_index = -1;
        int end_index = -1;

        /* How much difference from the av? */
        int dipped = 0;
        float dipped_density = 0;
        float threshold_dipped = minimum + ((av-minimum)*dip_threshold);
        for (int j = 0; j < DETECT_CURVE_LENGTH; j++) {
            if (curve[j] < threshold_dipped) {
                if (start_index == -1) start_index = j;
                end_index = j;
                dipped++;
                if (dipped > max_dipped) {
                    break;
                }
                dipped_density += density[j];
            }
        }

        /* there should be a beginning and end to the dipped area */
        if ((start_index == -1) && (end_index == -1)) {
            response[step] = 0;
            continue;
        }

        /* dipped area should not be too wide */
        if (end_index - start_index > DETECT_CURVE_LENGTH*10/100) {
            response[step] = 0;
            continue;
        }

        /* we only expect a small percentage
           of the curve to be dipped */
        if (dipped > max_dipped) {
            dipped = 0;
        }
        if (dipped == 0) {
            response[step] = 0;
            continue;
        }
        dipped_density /= (float)dipped;
        if (dipped_density < min_dipped_density) {
            response[step] = 0;
            continue;
        }

        /* peaks above the av are an indicator that this isn't a transit  */
        int peaked = 0;
        float threshold_peaked = av + ((av-minimum)*peak_threshold);
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if (curve[j] > threshold_peaked) {
                peaked++;
                break;
            }
        }
        if (peaked > 0) {
            response[step] = 0;
            continue;
        }

        /* How much difference from the av? */
        int nondipped = 0;
        float threshold_upper = av - ((av-minimum)*0.2);
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if ((curve[j] < threshold_upper) &&
                (curve[j] > threshold_dipped)) {
                nondipped++;
                if (nondipped > max_intermediates) {
                    break;
                }
            }
        }
        if ((nondipped < min_intermediates) ||
            (nondipped > max_intermediates)) {
            response[step] = 0;
            continue;
        }

        variance = 0;
        hits = 0;
        for (int j = DETECT_CURVE_LENGTH-1; j >= 0; j--) {
            if (curve[j] > 0) {
                variance += (curve[j] - av)*(curve[j] - av);
                hits++;
            }
        }
        response[step] = 0;
        if (hits > 0) {
            response[step] =
                (av-minimum) / (float)sqrt(variance/hits);
            response[step] =
                (av-minimum)*dipped*100/(av*(1+nondipped));
            response[step] /= (density_variance*variance);

            /* check the density within the area of the dip which
               is expected to be vacant */
            float vacancy_density =
                dip_vacancy(start_index, end_index,
                            timestamp,
                            series, series_length,
                            orbital_period_days,
                            curve,
                            DETECT_CURVE_LENGTH);
            if (vacancy_density > max_vacancy_density) {
                response[step] = 0;
            }
            else {
                response[step] /= (1.0f + (vacancy_density*10));
            }
        }
    }

    for (int i = steps-1; i >= 0; i--) {
        if (response[i] > max_response) {
            max_response = response[i];
            period_days = min_period_days + (i*increment_days);
        }
    }

    return period_days;
}
