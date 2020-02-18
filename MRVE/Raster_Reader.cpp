#include "Raster_Reader.h"

RasterProcessor::RasterProcessor()
{
	GDALAllRegister();
	timeSeries = new std::vector<TimeSeriesEntry>;
}

RasterProcessor::~RasterProcessor()
{
	UnloadRaster();
}



////test
//bool PFileIsExist(std::string location)
//{
//	std::cout << "Attempting to open " << location << std::endl;
//	std::ifstream file_to_check;
//
//	file_to_check.open(location);
//	if (file_to_check.is_open())
//	{
//		file_to_check.close();
//		return true;
//	}
//	else
//	{
//		file_to_check.close();
//		return false;
//	}
//}
////endtest


bool RasterProcessor::LoadRaster(std::string rasterPath)
{
	////test
	//if (!PFileIsExist(rasterPath))
	//{
	//	std::cout << "Error! Could not open DEM file: " << rasterPath << ". \nFile doesn't exist?\n\n";
	//	return false;
	//}
	////endtest

	const char * demloc = rasterPath.c_str();
	raster = (GDALDataset *)GDALOpen(demloc, GA_ReadOnly);

	if (raster == NULL)
	{
		std::cout << "Error! Could not load raster file.\n\n";
		return false;
	}

	rasterBand = raster->GetRasterBand(1); //Should consider checking that this is a greyscale (single band) dem first.

	SetRasterInfo();

	int demx = raster->GetRasterXSize();
	int demy = raster->GetRasterYSize();


	rasterPixels = new float*[demy];
	float * scanline;
	int demxsize = rasterBand->GetXSize();
	scanline = (float *)CPLMalloc(sizeof(float) * demxsize);

	for (int i = 0; i < demy; i++)
	{
		rasterPixels[i] = new float[demx];
		rasterBand->RasterIO(GF_Read, 0, i, demxsize, 1, scanline, demxsize, 1, GDT_Float32, 0, 0);

		for (int j = 0; j < demx; j++)
		{
			rasterPixels[i][j] = scanline[j];
		}
	}
	CPLFree(scanline);
	GDALClose(raster);
	//std::cout << "Successfully loaded raster file: " << rasterPath << "\n\n";

	return true;
}

void RasterProcessor::UnloadRaster()
{
	DeleteLoadedPixels();
}

float RasterProcessor::SampleAndAddPointToTimeSeries(float x, float y, std::string rowName, InterpolationType interpolationType)
{
	float pointValue = SamplePoint(x, y, interpolationType);
	
	TimeSeriesEntry newTSEntry;
	newTSEntry.rowName = rowName;
	newTSEntry.rowValue = pointValue;

	timeSeries->push_back(newTSEntry);

	return pointValue;
}

float RasterProcessor::SamplePoint(float x, float y, InterpolationType interpolationType)
{
	float pointValue = -9999.9f;

	int * maxCornerCellCoords = SeekMaxBoundingCellCoords(x, y);

	switch (interpolationType)
	{
	case nearestNeighbour:
		std::cout << "WARNING! Nearest Neighbour Interpolation is not yet implemented." << std::endl;
		//pointValue = NearestNeighbourInterpolation(maxCornerCellCoords[0], maxCornerCellCoords[1], x, y);
		break;
	case bilinear:
		std::cout << "WARNING! Bilinear Interpolation is not yet implemented." << std::endl;
		//pointValue = BilinearInterpolation(maxCornerCellCoords[0], maxCornerCellCoords[1], x, y);
		break;
	case bicubic:
		//std::cout << "WARNING! Bicubic Interpolation is not yet implemented." << std::endl;
		pointValue = BicubicInterpolation(maxCornerCellCoords[0], maxCornerCellCoords[1], x, y);
		break;
	default:
		pointValue = -9999.9f;
		break;
	}


	if (clampNegativeValuesToZero && pointValue < 0.0f)
		pointValue = 0.0f;

	return pointValue;
}

bool RasterProcessor::IsPointOOB(float x, float y)
{
	if (x < rasterInfo.NW_x || x > rasterInfo.SE_x)
	{
		return false;
	}
	if (y > rasterInfo.NW_y || y < rasterInfo.SE_y)
	{
		return false;
	}
}

std::vector<TimeSeriesEntry>* RasterProcessor::GetTimeSeries()
{
	return timeSeries;
}

//TODO consider making a lighter version of SetRasterInfo that only 
//sets important details (bounds, pixel size, corner coords, etc).
void RasterProcessor::SetRasterInfo()
{
	double temptransform[6];
	int min, max;
	double tempminmax[2];

	if (raster->GetProjectionRef() != NULL)
	{
		rasterInfo.ProjectionReference = raster->GetProjectionRef();
	}

	rasterInfo.x = rasterBand->GetXSize();
	rasterInfo.y = rasterBand->GetYSize();

	raster->GetGeoTransform(temptransform);
	rasterInfo.originx = temptransform[0];
	rasterInfo.originy = temptransform[3];
	rasterInfo.PixelSize_x = temptransform[1];
	rasterInfo.PixelSize_y = temptransform[5];
	rasterInfo.geotransform_2 = temptransform[2]; //TODO change these values when you fix the related name thingy.
	rasterInfo.geotransform_4 = temptransform[4];
	rasterInfo.z_min = rasterBand->GetMinimum(&min);
	rasterInfo.z_max = rasterBand->GetMaximum(&max);
	if (!(min && max))
	{
		GDALComputeRasterMinMax((GDALRasterBandH)rasterBand, true, tempminmax);
		rasterInfo.z_min = tempminmax[0];
		rasterInfo.z_max = tempminmax[1];
	}
	rasterBand->GetBlockSize(&rasterInfo.BlockSize_x, &rasterInfo.BlockSize_y);
	rasterInfo.RasterCount = raster->GetRasterCount();
	rasterInfo.RasterDataType = GDALGetDataTypeName(rasterBand->GetRasterDataType());
	rasterInfo.OverviewCount = rasterBand->GetOverviewCount();
	rasterInfo.ColorInterpretation = GDALGetColorInterpretationName(rasterBand->GetColorInterpretation());

	if (rasterBand->GetColorTable() != NULL)
	{
		rasterInfo.ColorEntryCount = rasterBand->GetColorTable()->GetColorEntryCount();
	}
	else
	{
		rasterInfo.ColorEntryCount = -1;
	}


	//setting corner coords
	//Necessary?
	rasterInfo.NW_x = rasterInfo.originx;
	rasterInfo.NW_y = rasterInfo.originy;

	rasterInfo.NE_x = rasterInfo.originx + rasterInfo.x * rasterInfo.PixelSize_x;
	rasterInfo.NE_y = rasterInfo.originy;

	rasterInfo.SE_x = rasterInfo.originx + rasterInfo.x * rasterInfo.PixelSize_x;
	rasterInfo.SE_y = rasterInfo.originy + rasterInfo.y * rasterInfo.PixelSize_y;

	rasterInfo.SW_x = rasterInfo.originx;
	rasterInfo.SW_y = rasterInfo.originy + rasterInfo.y * rasterInfo.PixelSize_y;

	//checking whether its geographic or utm
	//note this implementation is risky, based on a very simple and short observation

	std::string tempstring;

	tempstring = rasterInfo.ProjectionReference;

	//std::cout << "\n ********************* tempstring: " << tempstring.substr(0,6) << std::endl; //test
	if (tempstring.substr(0, 6) == "PROJCS")
	{
		rasterInfo.IsUTM = true;
		//rasterInfo.IsDecimal = false;
	}
	else
	{
		rasterInfo.IsUTM = false;
		//rasterInfo.IsDecimal = true;
	}

}

void RasterProcessor::DeleteLoadedPixels()
{
	if (rasterPixels != NULL)
	{
		for (int i = 0; i < rasterInfo.y; i++)
			if (rasterPixels[i] != NULL)
				delete[] rasterPixels[i];
		delete[] rasterPixels;
	}
}

int * RasterProcessor::SeekMaxBoundingCellCoords(float x, float y)
{
	int * cellCoords = new int[2];

	for (int j = 0; j < rasterInfo.y; j++)
	{
		if (y > rasterInfo.originy + j * rasterInfo.PixelSize_y)
		{
			cellCoords[1] = j;
			break;
		}
	}
	for (int k = 0; k < rasterInfo.x; k++)
	{
		if (x < rasterInfo.originx + k * rasterInfo.PixelSize_x)
		{
			cellCoords[0] = k;
			break;
		}
	}

	return cellCoords;
}

float RasterProcessor::NearestNeighbourInterpolation(int first_larger_x, int first_larger_y, float x, float y)
{
	return 0.0f;
}

float RasterProcessor::BilinearInterpolation(int first_larger_x, int first_larger_y, float x, float y)
{
	return 0.0f;
}

float RasterProcessor::BicubicInterpolation(int first_larger_x, int first_larger_y, float x, float y)
{
	//reference:
		//http://www.paulinternet.nl/?page=bicubic

	float result_depth;

	float boundingz[4][4];
	double boundingx[4];
	double boundingy[4];
	double temp_value[4];
	double pointx, pointy;

	for (int i = 0; i < 4; i++)
	{
		boundingx[i] = rasterInfo.originx + (first_larger_x - 2 + i) * rasterInfo.PixelSize_x;
		boundingy[i] = rasterInfo.originy + (first_larger_y - 2 + i) * rasterInfo.PixelSize_y;
		for (int j = 0; j < 4; j++)
		{
			boundingz[i][j] = rasterPixels[first_larger_y - 2 + j][first_larger_x - 2 + i];
		}
	}

	for (int i = 0; i < 4; i++)
	{
		pointx = abs((x - boundingx[i]) / boundingx[i]);
		temp_value[i] = boundingz[i][1] + 0.5 * pointx * (boundingz[i][2] - boundingz[i][0] + pointx * (2 * boundingz[i][0] - 5 * boundingz[i][1] + 4 * boundingz[i][2] - boundingz[i][3] + pointx * (3 * (boundingz[i][1] - boundingz[i][2]) + boundingz[i][3] - boundingz[i][0])));
	}

	pointy = abs((y - boundingy[1]) / boundingy[1]);
	
	
	result_depth = temp_value[1] + 0.5 * pointy * (temp_value[2] - temp_value[0] + pointy * (2 * temp_value[0] - 5 * temp_value[1] + 4 * temp_value[2] - temp_value[3] + pointy * (3 * (temp_value[1] - temp_value[2]) + temp_value[3] - temp_value[0])));

	return result_depth;
}
