

#include <iostream>
#include <fstream>
#include <numeric>
#include <math.h>

#include "Terrain.h"

// constructor will initialise to safe values
Terrain::Terrain()
	:  
	HomogeneousFaceSurface(),
	xyScale(1)
	{ // constructor
	// terrain vector will default to empty
	// so no additional work required here
	} // constructor

// read routine returns true on success, failure otherwise
// xyScale gives the scale factor to use in the x-y directions
bool Terrain::ReadFileTerrainData(const char *fileName, float XYScale)
	{ // ReadFileTerrainData()
	// open a file stream
	std::ifstream inFile(fileName);
	if (inFile.bad())
		return false;

	// save the xy scale
	xyScale = XYScale;
	
	// now set a default height and width of the data
	long height = 0, width = 0;
	
	// and read those values in
	inFile >> height >> width;

	// now allocate the memory and read in the data values
	heightValues.resize(height);

	// the read / compute loop	
	for (int row = 0; row < height; row++)
		{ // per row
		// allocate the row
		heightValues[row].resize(width);

		// loop along the row
		for (int col = 0; col < width; col++)
			// read in a value
			inFile >> heightValues[row][col];
		} // per row
	
	// now, we want the triangles to be centred on the origin, but with the zero elevation set
	// at 0 z, so we have to juggle things somewhat
	// compute a temporary midpoint for the data so that it will end up centred on the or
	Cartesian3 midPoint;
	midPoint.x		= xyScale * (width / 2);
	midPoint.y		= xyScale * (height / 2);
	// we will set the z value to be the average value of the data
	midPoint.z		= 0.0;
	
	// each square of data is two triangles, but the end values don't have squares,
	// so we don't need quite as many vertices
	int nTriangles = (height-1)*(width-1) * 2;
	vertices.resize(3 * nTriangles);

	// add an extra loop counter for the vertex ID
	int vertex = 0;

	// now that we have read in all the data, we can create the triangles
	for (int row = 0; row < height-1; row++)
		for (int col = 0; col < width-1; col++)
			{ // loop through squares
			// first triangle
			vertices[vertex++] = Cartesian3(	(xyScale * col) 		- midPoint.x , 		(midPoint.y - (xyScale * row		)), 	heightValues[row]		[col]);
			vertices[vertex++] = Cartesian3(	(xyScale * (col+1)) 	- midPoint.x , 		(midPoint.y - (xyScale * (row+1)	)), 	heightValues[row+1]	[col+1]);
			vertices[vertex++] = Cartesian3(	(xyScale * (col+1))	- midPoint.x , 		(midPoint.y - (xyScale * row		)), 	heightValues[row]		[col+1]);

			// second triangle			
			vertices[vertex++] = Cartesian3(	(xyScale * col) 		- midPoint.x , 		(midPoint.y - (xyScale * row		)), 	heightValues[row]		[col]);
			vertices[vertex++] = Cartesian3(	(xyScale * col)	 	- midPoint.x , 		(midPoint.y - (xyScale * (row+1)	)), 	heightValues[row+1]	[col]);
			vertices[vertex++] = Cartesian3(	(xyScale * (col+1))	- midPoint.x , 		(midPoint.y - (xyScale * (row+1)	)), 	heightValues[row+1]	[col+1]);
			} // loop through squares

	// call the routine to compute normals
	ComputeUnitNormalVectors();
	
	// return success
	return true;
	} // ReadFileTerrainData()
	
// and a function to find the height at a known (x,y) coordinate
float Terrain::getHeight(float x, float y)
	{ // getHeight()
	float height = 0.0;

	// retrieve the number of rows and columns of the data
	long nRows = heightValues.size(), nColumns = heightValues[0].size();
	
	// Use this to compute the logical size of the map
	long totalHeight = (nRows - 1) * xyScale;

	// this returns the height at a known point.
	// we start by noting that (0,0) is in the dead centre, which is located at
	// x = nRows / 2 (integer), y = nRows / 2 (integer)
	// BUT that's only because of how I wrote the code to generate the data
	long arrayOrigin_i = nRows / 2;
	long arrayOrigin_j = nColumns / 2;

	// now correct x and y for this offset (note rows are y, columns are x)
	x = x + arrayOrigin_j * xyScale;
	y = y + arrayOrigin_i * xyScale;

	// we need to flip coordinates vertically because the rows start at the top
	y = totalHeight - y;

	// now divide by the x-y scale to get the index 
	long x_integer	=	x / xyScale;
	long y_integer 	= 	y / xyScale;

	// now work out the fractional parts
	float x_remainder	=	(float)(x - (xyScale * x_integer))/xyScale;
	float y_remainder	=	(float)(y - (xyScale * y_integer))/xyScale;

	// now we can find the row and column easily
	long row	=	y_integer;
	long column	=	x_integer; 

	// OK. There are two possibilities - above or below the TL-BR diagonal
	// Since this is the line x = y, it's easy to check
	if (x_remainder < y_remainder)
		{ // LL triangle
		// in theory, we want barycentric interpolation, but fortunately, it collapses for us because we have 
		// right triangles.  
		// y_remainder is alpha, the barycentric coordinate for the UL corner
		// (1.0 - y_remainder) * x_remainder is beta, the barycentric coordinate for the LR corner
		// (1.0 - y_remainder) * (1.0 - x_remainder) is gamma, the barycentric coordinate for the LL corner
		float alpha = y_remainder;
		float beta = (1.0 - y_remainder) * x_remainder;
		float gamma = 1.0 - alpha - beta;
		
		// compute and return
		height = alpha * heightValues[row][column] + beta * heightValues[row+1][column+1] + gamma * heightValues[row+1][column];
		} // LL triangle
	else
		{ // UR triangle
		// (1.0 - x_remainder) is alpha, the barycentric coordinate for the UL corner
		// x_remainder * y_remainder is beta, the barycentric coordinate for the LR corner
		// x_remainder * (1.0 - y_remainder) is gamma, the barycentric coordinate for the UR corner
		float alpha = 1.0 - y_remainder;
		float beta = x_remainder * y_remainder;
		float gamma = 1.0 - alpha - beta;
		
		// compute and return
		height = alpha * heightValues[row][column] + beta * heightValues[row+1][column+1] + gamma * heightValues[row][column+1];
		} // UR triangle

	// return the height
	return height;
	} // getHeight()