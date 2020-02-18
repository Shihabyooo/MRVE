#pragma once
#include "Main.h"


struct RasterInfo
{
	int x, y;
	int RasterCount;
	int BlockSize_x, BlockSize_y;
	int OverviewCount;
	int ColorEntryCount;

	std::string ProjectionReference;
	std::string RasterDataType;
	std::string ColorInterpretation;

	double originx, originy;
	double PixelSize_x, PixelSize_y;
	double geotransform_2, geotransform_4; //TODO find out what these figures stand for
	double z_max, z_min;

	double	NW_x, NW_y,
		NE_x, NE_y,
		SE_x, SE_y,
		SW_x, SW_y;
	bool IsUTM;
	//bool IsDecimal;
	bool WGS84;
};

enum InterpolationType 
{
	nearestNeighbour,
	bilinear,
	bicubic
};

class TimeSeriesEntry
{
public:
	TimeSeriesEntry()
	{
		rowName = "UnfilledName";
		rowValue = -9999.9f;
	};
	~TimeSeriesEntry() {};
	std::string rowName;
	float rowValue;
};

class RasterProcessor
{
public:
	RasterProcessor();
	~RasterProcessor();
	bool LoadRaster(std::string rasterPath);
	void UnloadRaster(); //should simply delete allocated rasterPixels

	float SampleAndAddPointToTimeSeries(float x, float y, std::string rowName, InterpolationType interpolationType = InterpolationType::nearestNeighbour);
	float SamplePoint(float x, float y, InterpolationType interpolationType = InterpolationType::nearestNeighbour);

	bool IsPointOOB(float x, float y);

	std::vector<TimeSeriesEntry> * GetTimeSeries();

private:
	void SetRasterInfo();
	void DeleteLoadedPixels();
	//float InterpolatePointValue(int x, int y, InterpolationType interpolationType);
	

	int * SeekMaxBoundingCellCoords(float x, float y);
	
	float NearestNeighbourInterpolation(int first_larger_x, int first_larger_y, float x, float y);
	float BilinearInterpolation(int first_larger_x, int first_larger_y, float x, float y);
	float BicubicInterpolation(int first_larger_x, int first_larger_y, float x, float y);



	float ** rasterPixels;
	GDALDataset * raster;
	GDALRasterBand * rasterBand;
	RasterInfo rasterInfo;

	std::vector<TimeSeriesEntry> * timeSeries;

};