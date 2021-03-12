#include <chrono>
#include <iomanip>

#include "SmartScanService.h"
#include "Exceptions.h"

using namespace SmartScan;

SmartScanService::SmartScanService(bool useMockData) : mUseMockData{ useMockData }
{

}

SmartScanService::~SmartScanService()
{
	scans.clear();
}

void SmartScanService::Init()
{
	tSCtrl = new TrakStarController(mUseMockData);
	tSCtrl->Init();
	tSCtrl->Config();
	tSCtrl->AttachTransmitter();
}

void SmartScanService::NewScan()
{
	this->scans.emplace_back(std::make_shared<Scan>(FindNewScanId(), tSCtrl));
}

void SmartScanService::NewScan(SmartScan::ScanConfig* config, bool useSerials)
{
    if (mUseMockData)
    {
        this->scans.emplace_back(std::make_shared<Scan>(FindNewScanId(), tSCtrl));
    }
    else
    {
        if (useSerials)
        {
            if (config->useReferenceSensor)
            {
                config->referenceSensorId = tSCtrl->GetSensoridFromSerial(config->referenceSensorId);
    
                if(config->referenceSensorId < 0)
                {
                    throw ex_smartScan("Could not find reference sensor serial number", __func__, __FILE__);
                }
            }
            for(int i = 1; i < config->usedSensorIds.size(); i++)
            {
                config->usedSensorIds[i] = tSCtrl->GetSensoridFromSerial(config->usedSensorIds[i]);

                if(config->usedSensorIds[i] < 0)
                {
                    throw ex_smartScan("Could not find used sensor serial number", __func__, __FILE__);
                }
            }
        }
        this->scans.emplace_back(std::make_shared<Scan>(FindNewScanId(), tSCtrl, *config));
    }
}

void SmartScanService::DeleteScan()
{
	if (scans.size() > 0)
	{
		this->scans.erase(scans.end() - 1);
	}
	else {
		throw ex_smartScan("No scans left to delete", __func__, __FILE__);
	}
}

void SmartScanService::DeleteScan(int id)
{
	bool ok = false;
	for (int s = 0; s < scans.size(); s++)
	{
		if (scans[s]->mId == id) {
			this->scans.erase(scans.begin() + s);
			ok = true;
			break;
		}
	}
	if (!ok)
	{
		throw ex_smartScan("Scan id not found", __func__, __FILE__);
	}
}

void SmartScanService::StartScan()
{
    if (this->scans.size() == 0)
    {
        throw ex_smartScan("scan vector is empty", __func__, __FILE__);
    }

	// Check if reference points have been set;
	if (!this->scans.back()->IsAcquisitionOnly() && !scans.back()->GetReferences().size())
	{
		throw ex_smartScan("cannot start scan without reference points set.", __func__, __FILE__);
	}

	// Start the scan:
	try
	{
		// If UI callback is available, register it with this new Scan:
		if (mUICallback)
		{
			this->scans.back()->RegisterNewDataCallback(mUICallback);
		}

		scans.back()->Run();
	}
	catch (ex_scan e)
	{
		throw e;
	}
	catch (ex_trakStar e)
	{
		throw e;
	}
	catch (ex_smartScan e)
	{
		throw e;
	}
	catch (...)
	{
		throw "Cannot start scan";
	}
}

void SmartScanService::StartScan(int scanId)
{
	if (!IdExists(scanId))
	{
        throw ex_smartScan("scan id does not exist", __func__, __FILE__);
	}

	// Start the scan:
	try
	{
		// If UI callback is available, register it with this new Scan:
		if (mUICallback)
		{
			this->scans.back()->RegisterNewDataCallback(mUICallback);
		}
		scans.back()->Run(false);
	}
	catch (ex_scan e)
	{
		throw e;
	}
	catch (ex_trakStar e)
	{
		throw e;
	}
	catch (std::exception e)
	{
		throw e;
	}
	catch (...)
	{
		throw "Cannot start scan";
	}
}

void SmartScan::SmartScanService::CalibrateSingleRefPoint()
{
	if (scans.size() == 0)
	{
		throw ex_smartScan("No existing scan object found", __func__, __FILE__);
	}

	scans.back()->Run(true);

	ReferencePoint newRef;

	// Wait for values:
	while (scans.back()->mInBuff.size() < 2 || scans.back()->mRefBuff.size() < 1)
	{
	}

	std::vector<Point3>::const_iterator firstFingerIterator = scans.back()->mInBuff.cend() - scans.back()->NUsedSensors();
	// Add the referenceSensorPos:
	newRef.refSensorPos = scans.back()->mRefBuff.back();

	newRef.pos.x = ((firstFingerIterator[0].x + firstFingerIterator[1].x) / 2) - newRef.refSensorPos.x;
	newRef.pos.y = ((firstFingerIterator[0].y + firstFingerIterator[1].y) / 2) - newRef.refSensorPos.y;
	newRef.pos.z = ((firstFingerIterator[0].z + firstFingerIterator[1].z) / 2) - newRef.refSensorPos.z;

	scans.back()->AddReference(newRef);

	scans.back()->Stop(true);
}

void SmartScanService::CalibrateReferencePoints()
{
	int refCount;

	// Find how many calibration points are desired:
	std::cout << "[CALIBRATION] " << "Before starting the scan, reference points must be calibrated. Use the thumb and index finger to point out the knee, ankle and foot ecnter \n";
	std::cout << "[CALIBRATION] " << "Enter the number of desired reference points  (by default 3, as mentioned above): ";
	std::cin >> refCount;
	std::cin.get();

	if (!refCount)
	{
		throw ex_smartScan("No reference point.", __func__, __FILE__);
	}

	// Reset the Scan's reference points if some already exist:
	if (scans.back()->GetReferences().size() > 0)
	{
		scans.back()->ResetReferences();
	}

	// Start reading sensor data:
	std::cout << "[CALIBRATION] " << "A temporary scan will run for the duration of the calibration. The data will be deleted afterwards." << std::endl;
	// Acquisition only
	scans.back()->Run(true);
	// Do this for the given number of ref points:
	for (int i = 0; i < refCount; i++)
	{
		// Store the latest values:
		ReferencePoint newRef;

		// Wait for values:
		while (scans.back()->mInBuff.size() < 2 || scans.back()->mRefBuff.size() < 1)
		{
		}

		std::cout << "[CALIBRATION] " << "Position your fingers around the reference point and press any key to capture it" << std::endl;
		std::cin.get();

		std::vector<Point3>::const_iterator firstFingerIterator = scans.back()->mInBuff.cend() - scans.back()->NUsedSensors();
		// Add the referenceSensorPos:
		newRef.refSensorPos = scans.back()->mRefBuff.back();

		newRef.pos.x = ((firstFingerIterator[0].x + firstFingerIterator[1].x) / 2) - newRef.refSensorPos.x;
		newRef.pos.y = ((firstFingerIterator[0].y + firstFingerIterator[1].y) / 2) - newRef.refSensorPos.y;
		newRef.pos.z = ((firstFingerIterator[0].z + firstFingerIterator[1].z) / 2) - newRef.refSensorPos.z;

		// Reorientate the reference points to the 0-0-0 angles where all points are oriented to
		const double azimuth = newRef.refSensorPos.r.z;
		const double elevation = newRef.refSensorPos.r.y;
		const double roll = newRef.refSensorPos.r.x;
		// Declare new variables for new points
		double x_new = 0;
		double y_new = 0;
		double z_new = 0;
		const double pi = 3.14159265;

		// Use the azimuth to calculate the rotation around the z-axis
		const double azimuth_distance = sqrt(pow(newRef.pos.x, 2) + pow(newRef.pos.y, 2));
		const double a = (atan2(newRef.pos.y, newRef.pos.x) * 180 / pi) - azimuth;
		x_new = azimuth_distance * cos(a * pi / 180);
		y_new = azimuth_distance * sin(a * pi / 180);

		// Use the elevation to calculate the rotation around the y-axis
		const double elevation_distance = sqrt(pow(x_new, 2) + pow(newRef.pos.z, 2));
		const double b = (atan2(newRef.pos.z, x_new) * 180 / pi) + elevation;
		x_new = elevation_distance * cos(b * pi / 180);
		z_new = elevation_distance * sin(b * pi / 180);

		// Use the roll difference to calculate the rotation around the x-axis
		const double roll_distance = sqrt(pow(y_new, 2) + pow(z_new, 2));
		const double c = (atan2(z_new, y_new) * 180 / pi) - roll;
		y_new = roll_distance * cos(c * pi / 180);
		z_new = roll_distance * sin(c * pi / 180);

		newRef.pos.x = x_new;
		newRef.pos.y = y_new;
		newRef.pos.z = z_new;

		newRef.index = i;

		scans.back()->AddReference(newRef);
		std::cout << "[CALIBRATION] " << "Reference point at (" << newRef.pos.x << "," << newRef.pos.y << "," << newRef.pos.z << ") with index " << newRef.index << " set" << std::endl;
	}

	std::cout << "[CALIBRATION] " << "Done setting reference points" << std::endl;
	scans.back()->Stop(true);
}

void SmartScanService::SetReferencePoints(const std::vector<ReferencePoint> referencePoints)
{
	if (scans.size() < 0)
	{
		std::cout << "[SMART SCAN] " << "Cannot set reference points because no new scan has been created." << std::endl;
		return;
	}
	if (scans.back()->GetReferences().size() > 0)
	{
		// Reset the reference points:
		scans.back()->ResetReferences();
	}
	for (auto rp : referencePoints)
	{
		scans.back()->AddReference(rp);
	}
}

void SmartScanService::StopScan()
{
	scans.back()->Stop();
}

void SmartScanService::DumpScan() const
{
	scans.back()->DumpData();
}

void SmartScanService::SetUsedSensors(const std::vector<int> sensorIds)
{
	//scans.back()->SetUsedSensors(sensorIds);
}

void SmartScan::SmartScanService::SetFilteringPrecision(const double precision)
{
	mFilteringPrecision = precision;
}

// TODO: Optionally handle multiple simultaneous scans (i.e. some processing is being done on a previous scan
// while new data is acquired usign a different Scan object"
const std::shared_ptr<Scan> SmartScanService::GetScan() const
{
	return scans.back();
}

const std::shared_ptr<Scan> SmartScanService::GetScan(int id) const
{
	for (int s = 0; s < scans.size(); s++)
	{
		if (scans[s]->mId == id) {
			return scans[s];
		}
	}
	throw ex_smartScan("Scan id not found", __func__, __FILE__);
}

const std::vector<std::shared_ptr<Scan>>& SmartScanService::GetScansList() const
{
	return scans;
}

void SmartScanService::ExportCSV(const std::string filename, const bool raw)
{
	if (scans.empty())
	{
		throw "No measurement available for export";
	}
	if (raw)
	{
		csvExport.ExportPoint3(scans.back()->mInBuff, filename);
	}
	else
	{
		csvExport.ExportPoint3(scans.back()->mOutBuff, filename);
	}
}

void SmartScanService::ExportPointCloud(const std::string filename, const bool raw)
{
	if (scans.empty())
	{
		throw "No measurement available for export";
	}
	if (raw)
	{
		csvExport.ExportPC(scans.back()->mInBuff, filename);
	}
	else
	{
		csvExport.ExportPC(scans.back()->mOutBuff, filename);
	}
}

void SmartScanService::RegisterNewDataCallback(std::function<void(std::vector<Point3>&)> callback)
{
	mUICallback = callback;
	// Register this callback with all the existing Scans:
	for (auto& scan : scans)
	{
		scan->RegisterNewDataCallback(mUICallback);
	}
}

void SmartScanService::RegisterRawDataCallback(std::function<void(std::vector<Point3>&)> callback)
{
	mRawCallback = callback;
	// Register this callback with all the existing Scans:
	for (auto& scan : scans)
	{
		scan->RegisterRawDataCallback(mRawCallback);
	}
}

const int SmartScanService::FindNewScanId() const
{
	int newId = 0;
	for (int scn = 0; scn < scans.size(); scn++)
	{
		if (newId == scans[scn]->mId)
		{
			++newId;
		}
	}

	return newId;
}

const bool SmartScanService::IdExists(const int scanId) const
{
	for (int scn = 0; scn < scans.size(); scn++)
	{
		if (scanId == scans[scn]->mId)
		{
			return true;
		}
	}
	return false;
}