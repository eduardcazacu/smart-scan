#include "TrakStarController.h"

#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include "Exceptions.h"

using namespace SmartScan;

//constructor
TrakStarController::TrakStarController(bool mock): mMock {mock}
{
	if (mock)
	{
		/* initialize random seed: */
		srand(time(NULL));
	}
}
//init func
void TrakStarController::Init()
{
	//when in Mock mode do nothing:
	if (mMock)
	{
		return;
	}
	//printf("Initializing ATC3DG system...\n");
	errorCode = InitializeBIRDSystem();
	if (errorCode != BIRD_ERROR_SUCCESS)
	{
		throw ex_trakStar(GetErrorString(errorCode), __func__, __FILE__);
	}
}

void TrakStarController::Config()
{
	//when in mock mode do nothing:
	if (mMock)
	{
		return;
	}
	//system
	errorCode = GetBIRDSystemConfiguration(&ATC3DG.m_config);
	if (errorCode != BIRD_ERROR_SUCCESS) ErrorHandler(errorCode);

	//sensor
	pSensor = new CSensor[ATC3DG.m_config.numberSensors];
	for (i = 0; i < ATC3DG.m_config.numberSensors; i++)
	{
		errorCode = GetSensorConfiguration(i, &(pSensor + i)->m_config);
		if (errorCode != BIRD_ERROR_SUCCESS) ErrorHandler(errorCode);
	}

	//transmitter
	pXmtr = new CXmtr[ATC3DG.m_config.numberTransmitters];
	for (i = 0; i < ATC3DG.m_config.numberTransmitters; i++)
	{
		errorCode = GetTransmitterConfiguration(i, &(pXmtr + i)->m_config);
		if (errorCode != BIRD_ERROR_SUCCESS) ErrorHandler(errorCode);
	}
}

void TrakStarController::AttachSensor()
{
	//when in mock mode do nothing:
	if (mMock)
	{
		return;
	}
	for (id = 0; id < ATC3DG.m_config.numberTransmitters; id++)
	{
		if ((pXmtr + id)->m_config.attached)
		{
			// Transmitter selection is a system function.
			// Using the SELECT_TRANSMITTER parameter we send the id of the
			// transmitter that we want to run with the SetSystemParameter() call
			errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
			if (errorCode != BIRD_ERROR_SUCCESS) ErrorHandler(errorCode);
			break;
		}
	}
}


int TrakStarController::GetNSensors()
{
	//when in mock mode use 4 sensors
	if (mMock)
	{
		return 4;
	}

	if (&ATC3DG)
	{
		return ATC3DG.m_config.numberSensors;
	}
	else
	{
		throw "Sensor config unnavailable.";
	}
}

Point3 TrakStarController::GetRecord(int sensorID)
{
	//when in mock mode, return a random value on a sphere:
	if (mMock)
	{
		return GetMockRecord();
	}

	if (++sensorID > ATC3DG.m_config.numberSensors || sensorID <= 0)
	{
		throw "Sensor ID out if range";
	}

	DOUBLE_POSITION_ANGLES_RECORD record, * pRecord = &record;

	// sensor attached so get record
	errorCode = GetAsynchronousRecord(sensorID, pRecord, sizeof(record));
	if (errorCode != BIRD_ERROR_SUCCESS) { ErrorHandler(errorCode); }

	// get the status of the last data record
	// only report the data if everything is okay
	unsigned int status = GetSensorStatus(sensorID);

	if (status == VALID_STATUS)
	{
		return Point3(record.x, record.y, record.z, record.a, record.e, record.r);
	}
	else
	{
		throw "No valid sensor record found";
	}

	return Point3();
}

void TrakStarController::ReadSensor()
{
	//when in mock mode do nothing:
	if (mMock)
	{
		return;
	}
	DOUBLE_POSITION_ANGLES_RECORD record, * pRecord = &record;

	// Set up time delay for first loop
	// It only makes sense to request a data record from the sensor once per
	// measurement cycle. Therefore we set up a 10ms loop and request a record
	// only after at least 10ms have elapsed.
	//
	goal = wait + clock();

	// collect as many records as specified in the command line
	for (i = 0; i < records; i++)
	{
		// delay 10ms between collecting data
		// wait till time delay expires
		while (goal > clock());
		// set up time delay for next loop
		goal = wait + clock();

		// scan the sensors and request a record
		for (sensorID = 0; sensorID < ATC3DG.m_config.numberSensors; sensorID++)
		{
			// sensor attached so get record
			errorCode = GetAsynchronousRecord(sensorID, pRecord, sizeof(record));
			if (errorCode != BIRD_ERROR_SUCCESS) { ErrorHandler(errorCode); }

			// get the status of the last data record
			// only report the data if everything is okay
			unsigned int status = GetSensorStatus(sensorID);

			if (status == VALID_STATUS)
			{
				// send output to console
				sprintf_s(output, "[%d] %8.3f %8.3f %8.3f: %8.2f %8.2f %8.2f\n",
					sensorID,
					record.x,
					record.y,
					record.z,
					record.a,
					record.e,
					record.r
				);
				numberBytes = strlen(output);
				printf("%s", output);
			}
		}
	}


}

void TrakStarController::StopTransmit()
{
	//when in mock mode do nothing:
	if (mMock)
	{
		return;
	}
	id = -1;
	errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
	if (errorCode != BIRD_ERROR_SUCCESS) ErrorHandler(errorCode);

	delete[] pSensor;
	delete[] pXmtr;
}

Point3 TrakStarController::GetMockRecord()
{
	double radius = 100;
	int randomMax = 10;
	double randomMaxRadius = 200;
	
	if (mPrevMockRecord.x == 0 && mPrevMockRecord.y == 0 && mPrevMockRecord.z == 0)
	{
		//first mock value:
		mPrevMockRecord = Point3(radius, 0, 0);
		return mPrevMockRecord;
	}
	else {
		//add some random noise to the previous mockRecord;
		mPrevMockRecord = Point3(mPrevMockRecord.x + ((rand() % randomMax) - randomMax / 2), mPrevMockRecord.y + ((rand() % randomMax) - randomMax / 2), mPrevMockRecord.z + ((rand() % randomMax) - randomMax / 2));
		return mPrevMockRecord;
	}
}

void TrakStarController::ErrorHandler(int error)
{
	char			buffer[1024];
	char* pBuffer = &buffer[0];
	int				numberBytes;

	while (error != BIRD_ERROR_SUCCESS)
	{
		error = GetErrorText(error, pBuffer, sizeof(buffer), SIMPLE_MESSAGE);
		numberBytes = strlen(buffer);

		throw ex_trakStar(buffer, __func__, __FILE__);
		//printf("%s", buffer);
	}
}

const std::string TrakStarController::GetErrorString(int error)
{
	char			buffer[1024];
	char* pBuffer = &buffer[0];
	int				numberBytes;
	std::string errorString;

	if (error != BIRD_ERROR_SUCCESS)
	{
		error = GetErrorText(error, pBuffer, sizeof(buffer), SIMPLE_MESSAGE);
		errorString =  buffer[0];
		return errorString;
	}
}
