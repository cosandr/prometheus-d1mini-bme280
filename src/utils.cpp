#include <cmath>
#include "utils.h"

// Copied from https://github.com/beegee-tokyo/DHTesp/blob/affec2841444f06b3c64dc023aee8e9ff20bbec0/DHTesp.cpp#L296

static float toFahrenheit(float fromCelcius) { return 1.8 * fromCelcius + 32.0; };
static float toCelsius(float fromFahrenheit) { return (fromFahrenheit - 32.0) / 1.8; };

//boolean isFahrenheit: True == Fahrenheit; False == Celcius
float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit)
{
	// Using both Rothfusz and Steadman's equations
	// http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
	float hi;

	if (!isFahrenheit)
	{
		temperature = toFahrenheit(temperature);
	}

	hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

	if (hi > 79)
	{
		hi = -42.379 +
			 2.04901523 * temperature +
			 10.14333127 * percentHumidity +
			 -0.22475541 * temperature * percentHumidity +
			 -0.00683783 * pow(temperature, 2) +
			 -0.05481717 * pow(percentHumidity, 2) +
			 0.00122874 * pow(temperature, 2) * percentHumidity +
			 0.00085282 * temperature * pow(percentHumidity, 2) +
			 -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

		if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
			hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

		else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
			hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
	}

	return isFahrenheit ? hi : toCelsius(hi);
}

//boolean isFahrenheit: True == Fahrenheit; False == Celcius
float computeDewPoint(float temperature, float percentHumidity, bool isFahrenheit)
{
	// reference: http://wahiduddin.net/calc/density_algorithms.htm
	if (isFahrenheit)
	{
		temperature = toCelsius(temperature);
	}
	double A0 = 373.15 / (273.15 + (double)temperature);
	double SUM = -7.90298 * (A0 - 1);
	SUM += 5.02808 * log10(A0);
	SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1);
	SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1);
	SUM += log10(1013.246);
	double VP = pow(10, SUM - 3) * (double)percentHumidity;
	double Td = log(VP / 0.61078); // temp var
	Td = (241.88 * Td) / (17.558 - Td);
	return isFahrenheit ? toFahrenheit(Td) : Td;
}
