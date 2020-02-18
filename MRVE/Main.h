#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <filesystem>
#include <fstream>
#include <chrono>


#include "gdal_priv.h"
#include "cpl_conv.h"
#include "KML_Parser.h"
#include "Raster_Reader.h"

//#include "CustomMath.h"


extern bool clampNegativeValuesToZero;