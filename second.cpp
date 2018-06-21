#include <vector> // Dynamic data storage
#include <iostream>
#include <future>
#include "pvm3.h" // Parallel API
#include "pixel.h" // Parallel API


Pixel Conversion(const Pixel& px)
{
	Pixel result;
	
	(px.R < 128) ? result.R = 0 : result.R = 255;
	(px.G < 128) ? result.G = 0 : result.G = 255;
	(px.B < 128) ? result.B = 0 : result.B = 255;
	
	return result;
}

std::vector<Pixel> ColorCoder(const std::vector<Pixel> &v)
{
	std::vector<Pixel> res(v.size());
	for(unsigned int i=0; i<v.size(); i++)
	{
		res[i] = Conversion(v[i]);
	}
	
	return res;
}

// Main entry point of child
int main()
{	
	int parent_id = pvm_parent();
	
	//receiving data
	pvm_recv(parent_id, 2);

	int from, to, numOfPics;
	
	pvm_upkint(&from, 1, 1);
	pvm_upkint(&to, 1, 1);
	pvm_upkint(&numOfPics, 1, 1);
	
	//receiving pictures
	for(int i=0; i<numOfPics; i++)
	{
		pvm_recv(from, 4);
		int pictureSize;
		pvm_upkint(&pictureSize, 1, 1);
		
		int rowLength = pictureSize * 3;
		std::vector<std::vector<Pixel>> picture(pictureSize);
		
		for(int j=0; j<pictureSize; j++)
		{
			std::vector<int> pixRow(rowLength);
			pvm_upkint(pixRow.data(), rowLength, 1);
			std::vector<Pixel> tmp;
			for(unsigned int k=0; k<pixRow.size(); k += 3)
			{
				tmp.push_back(Pixel(pixRow[k], pixRow[k+1], pixRow[k+2]));
			}
			
			picture[j] = tmp;
		}
		
		//Transformating pixels by rows
		std::vector<std::future<std::vector<Pixel>>> results;
		for(unsigned int k=0; k<picture.size(); k++)
		{
			results.push_back(std::async(std::launch::async, ColorCoder, picture[k]));
		}
		
		for(unsigned int k=0; k<results.size(); k++)
		{
			picture[k] = results[k].get();
		}
		
		//sending results toward
		int red, green, blue;
		pvm_initsend(PvmDataDefault);
		pvm_pkint(&pictureSize, 1, 1);
		for(int y=0; y<pictureSize; y++)
		{
			for(int z=0; z<pictureSize; z++)
			{
				red = picture[y][z].R;
				green = picture[y][z].G;
				blue = picture[y][z].B;
			
				pvm_pkint(&red, 1, 1);
				pvm_pkint(&green, 1, 1);
				pvm_pkint(&blue, 1, 1);
			}
			
		}
		pvm_send(to, 5);
		
	}
	
	
	pvm_exit();
	
	return 0;
}
