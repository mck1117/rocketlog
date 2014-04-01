

#include <Wire.h>
#include <SD.h>
#include <SFE_BMP180.h>

// ***********************
//        Settings
// ***********************

// Datapoint averaging
//#define ENABLE_DATA_AVERAGE
#define DATA_AVERAGE_POINTS 10

// Error printing
//#define ENABLE_ERROR_PRINT

// ***********************
//      SD Card Setup
// ***********************

#define chipSelect SS

static File dataFile;

// Fix support for non-leonardo/32u4 based devices
#ifndef TXLED1
#define LEDPIN 7
#define TX_RX_LED_INIT pinMode(LEDPIN, OUTPUT);
#define TXLED1 digitalWrite(LEDPIN, HIGH)
#define TXLED0 digitalWrite(LEDPIN, LOW)
#endif

struct datapoint
{
	uint32_t timestamp;
	double pressure;
};


// Disablable error printing, saves space
#ifdef ENABLE_ERROR_PRINT
#define ERRPRINT(s)	Serial.println(s)
#else
#define ERRPRINT(s)
#endif

// ***********************
//    Barometer Setup
// ***********************
SFE_BMP180 barometer;

void failBlink(int t)
{
	while (true)
	{
		TXLED1;
		delay(t);
		TXLED0;
		delay(t);
	}
}

void setup()
{
	TX_RX_LED_INIT;

#if ENABLE_ERROR_PRINT
	Serial.begin(115200);
#endif


	ERRPRINT("Initializing SD...");

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		ERRPRINT("Card failed, or not present");
		// don't do anything more:
		failBlink(100);
	}
	ERRPRINT("Card initialized!");


	bool success = false;
	int n = 0;

	char c[20];

	// Find the first file in the sequence {0.BIN, 1.BIN, 2.BIN, ..., n.BIN} that doesn't exist, and create it
	do
	{
		itoa(n++, c, 10);
		strcat(c, ".BIN");

		if (!SD.exists(c))		// If it doesn't extist, create.
		{
			dataFile = SD.open(c, FILE_WRITE);
			success = true;
		}
	} while (!success);


	if (barometer.begin() == 0)
	{
		ERRPRINT("BMP180 Init Fail");

		failBlink(1000);
	}

	TXLED1;
}

#define DATA_LENGTH 64
#define DATA_BUFFER_LENGTH (DATA_LENGTH * sizeof(datapoint))
int dataIdx = 0;
datapoint data[DATA_LENGTH];

void loop()
{
	// Set the timestamp on our datapoint
	data[dataIdx].timestamp = millis();

#ifdef ENABLE_DATA_AVERAGE
	double sum = 0;

	// Sum a set of points for the average
	for (byte i = 0; i < DATA_AVERAGE_POINTS; i++)
	{
		sum += getPressure();
	}

	// Set the pressure of this datapoint to the average that we calculate
	data[dataIdx].pressure = sum / DATA_AVERAGE_POINTS;
#else
	data[dataIdx].pressure = getPressure();
#endif

	// Increment index to the next datapoint in the array
	dataIdx++;

	if (dataIdx == DATA_LENGTH)		// The struct is full, time to flush.
	{
		// Reset index
		dataIdx = 0;

		TXLED1;
		if (dataFile)
		{
			if (dataFile.write((const uint8_t*)data, DATA_BUFFER_LENGTH) != DATA_BUFFER_LENGTH)
			{
				ERRPRINT("Error writing datalog.txt");
				failBlink(2000);
			}

			dataFile.flush();
		}
		TXLED0;
	}
}

double getPressure()
{
	char status;
	double T, P, p0, a;

	// You must first get a temperature measurement to perform a pressure reading.

	// Start a temperature measurement:
	// If request is successful, the number of ms to wait is returned.
	// If request is unsuccessful, 0 is returned.

	status = barometer.startTemperature();

	if (status == 0)
	{
		ERRPRINT("error starting temperature measurement\n");
		goto cleanup;
	}

	// Wait for the measurement to complete:
	delay(status);


	// Retrieve the completed temperature measurement:
	// Note that the measurement is stored in the variable T.
	// Use '&T' to provide the address of T to the function.
	// Function returns 1 if successful, 0 if failure.

	status = barometer.getTemperature(T);

	if (status == 0)
	{
		ERRPRINT("error retrieving temperature measurement\n");
		goto cleanup;
	}


	// Start a pressure measurement:
	// The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
	// If request is successful, the number of ms to wait is returned.
	// If request is unsuccessful, 0 is returned.

	status = barometer.startPressure(3);

	if (status == 0)
	{
		ERRPRINT("error starting pressure measurement\n");
		goto cleanup;
	}

	// Wait for the measurement to complete:
	delay(status);

	// Retrieve the completed pressure measurement:
	// Note that the measurement is stored in the variable P.
	// Use '&P' to provide the address of P.
	// Note also that the function requires the previous temperature measurement (T).
	// (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
	// Function returns 1 if successful, 0 if failure.

	status = barometer.getPressure(P, T);

	if (status == 0)
	{
		ERRPRINT("error retrieving pressure measurement\n");
		goto cleanup;
	}

	return P;

cleanup:
	return 0;
}