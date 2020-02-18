#include "Main.h"


RasterProcessor rasterProc;
std::list<std::string> rasterList;
bool clampNegativeValuesToZero = true;

std::chrono::high_resolution_clock::time_point startTime;
std::chrono::high_resolution_clock::time_point rasterQuerryEnd;
std::chrono::high_resolution_clock::time_point valueExtractionEnd;
std::chrono::high_resolution_clock::time_point writeToFileEnd;

//int rasterCount = 0;

void QuerryRastersInDirectory(std::string path, std::string extension);
void ShowRasterList();
bool DoesFileExist(std::string filePath);
bool WriteResultsToFile(std::string outputPath, std::string outputExtension);
void ShowExecutionTime();

void main(int argc, char * argv[])
{
	
	//testing values
	float pointX = 35.486610f;
	float pointY = 12.830150f;


	//std::string path = "C:\\test";
	std::string path = "C:\\Users\\a\\Desktop\\ARC2\\Extracted\\1991_2000";


	startTime = std::chrono::high_resolution_clock::now();

	QuerryRastersInDirectory(path, "tif");
	rasterQuerryEnd = std::chrono::high_resolution_clock::now();
	//ShowRasterList();


	/*for (int i = 0; i < rasterList.size(); i++)
	{
		rasterProc.LoadRaster(rasterList[i]);
	}*/

	//rasterProc.LoadRaster(argv[1]);
	//rasterProc.UnloadRaster();


	for (std::list<std::string>::iterator it = rasterList.begin(); it != rasterList.end(); it++)
	{

		//std::string rasterPath = *it;
		
		rasterProc.LoadRaster(*it);

		std::string rowName = *it;
		rowName = rowName.substr(rowName.length() - 12);
		rasterProc.SampleAndAddPointToTimeSeries(pointX, pointY, rowName, InterpolationType::bicubic);

		//std::cout << rasterPath.c_str() << std::endl;
		rasterProc.UnloadRaster();
	}
	valueExtractionEnd = std::chrono::high_resolution_clock::now();

	//test
	/*std::cout << "TS:" << std::endl;

	std::vector<TimeSeriesEntry> * vec = rasterProc.GetTimeSeries();

	for (std::vector < TimeSeriesEntry>::iterator it = vec->begin(); it != vec->end(); it++)
		std::cout << it->rowName << " : " << it->rowValue << std::endl;
*/
	//end test

	WriteResultsToFile("output", "csv");
	writeToFileEnd = std::chrono::high_resolution_clock::now();

	ShowExecutionTime();

	std::cin.sync();
	std::cin.get();
}




void QuerryRastersInDirectory(std::string directoryPath, std::string extension)
{
	for (auto& dit : std::filesystem::directory_iterator(directoryPath))
	{
		//std::cout << dit.path().string().c_str() << std::endl;
		std::string qPath = dit.path().string();
		std::string qExtension = qPath.substr(qPath.length() - 3);

		if (qExtension == extension)
		{
			//std::cout << qPath.c_str() << std::endl;
			//std::cout << qExtension.c_str() << std::endl;
			rasterList.push_back(dit.path().string());
			//rasterCount++;
		}
	}
}

void ShowRasterList()
{
	for (std::string str : rasterList)
	{
		std::cout << str.c_str() << std::endl;
	}
}

bool DoesFileExist(std::string filePath)
{
	//std::cout << "Attempting to open " << filePath << std::endl;
	std::ifstream file_to_check;

	file_to_check.open(filePath);
	if (file_to_check.is_open())
	{
		file_to_check.close();
		return true;
	}
	else
	{
		file_to_check.close();
		return false;
	}
}

bool WriteResultsToFile(std::string outputName, std::string outputExtension)
{

	std::string outputPath = outputName + "." + outputExtension;
	
	int counter = 1;
	while (DoesFileExist(outputPath))
	{
		outputPath = outputName + "_" + std::to_string(counter) + "." + outputExtension;
		counter++;
	}


	std::ofstream outputStream;
	outputStream.open(outputPath);

	if (outputStream.is_open())
	{
		std::cout << "File creation is successfull\n";
	}
	else
	{
		std::cout << "Error: failed to create or open file!\n";
		return false;
	}

	std::cout << "\nWriting results to disk\n"; //test
	outputStream << "RowName,RowValue" << std::endl;

	std::vector<TimeSeriesEntry> * vec = rasterProc.GetTimeSeries();

	for (std::vector < TimeSeriesEntry>::iterator it = vec->begin(); it != vec->end(); it++)
	{
		//std::cout << it->rowName << " : " << it->rowValue << std::endl;
		outputStream << it->rowName << "," << it->rowValue << std::endl;
	}

	outputStream.close();
	
	std::cout << "\Finished writing results to disk in " << outputPath << "\n"; //test

	return true;
}

void ShowExecutionTime()
{
	std::chrono::duration _querry = std::chrono::duration_cast<std::chrono::milliseconds>(rasterQuerryEnd - startTime);
	std::chrono::duration _extract = std::chrono::duration_cast<std::chrono::milliseconds>(valueExtractionEnd - rasterQuerryEnd);
	std::chrono::duration _write = std::chrono::duration_cast<std::chrono::milliseconds>(writeToFileEnd - valueExtractionEnd);

	std::cout << "Raster Querrying: " << _querry.count() << " milliseconds" << std::endl;
	std::cout << "Value Extraction: " << _extract.count() << " milliseconds" << std::endl;
	std::cout << "Output Writing: " << _write.count() << " milliseconds" << std::endl;
	std::cout << "Total: " << _querry.count() + _extract.count() + _write.count() << " milliseconds" << std::endl;
}
