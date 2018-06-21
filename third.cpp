#include <vector> // Dynamic data storage
#include <iostream>
#include <future>
#include "pvm3.h" // Parallel API
#include "pixel.h" // Parallel API

//2 pixels equals?
bool EqualPixels(const Pixel &px1, const Pixel &px2)
{
	return(px1.R == px2.R && px1.G == px2.G && px1.B == px2.B);
}

//swapping mtx rows-columns
std::vector<std::vector<Pixel>> Transpose(const std::vector<std::vector<Pixel>> &mtx)
{
	std::vector<std::vector<Pixel>> result(mtx.size());
	for(int i=0; i<result.size(); i++)
		result[i].resize(mtx[i].size());
	
	for(int i=0; i<mtx.size(); i++)
	{
		for(int j=0; j<mtx[i].size(); j++)
		{
			result[j][i] = mtx[i][j];
		}
	}
	
	return result;
}

//making the labels of a row
std::vector<int> Labeling(const std::vector<Pixel> &v)
{
	std::vector<int> result;
	int count = 1;
	
	for(int i=0; i<v.size()-1; i++)
	{
		if(EqualPixels(v[i], v[i+1]))
		{
			count++;
		}else
		{
			result.push_back(count);
			count = 1;
		}
	}
	result.push_back(count);
	
	return result;
}

//only for test...
void Kiir(const std::vector<std::vector<int>> &v)
{
	for(int i=0; i<v.size(); i++)
	{
		for(int j=0; j<v[i].size(); j++)
		{
			std::cout<<v[i][j]<<" ";
		}
		std::cout<<std::endl;
	}
}

// Main entry point of child
int main()
{	
	int parent_id = pvm_parent();

	//receiving data from master
	pvm_recv(parent_id, 3);

	int from, to, numOfPics;
	
	pvm_upkint(&from, 1, 1);
	pvm_upkint(&to, 1, 1);
	pvm_upkint(&numOfPics, 1, 1);
	
	
	for(int i=0; i<numOfPics; i++)
	{
		//receiving a picture...
		pvm_recv(from, 5);
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
		}//picture received
		
		//counting labels of rows with pictureSize threads
		std::vector<std::future<std::vector<int>>> rowResults;
		for(unsigned int k=0; k<pictureSize; k++)
		{
			rowResults.push_back(std::async(std::launch::async, Labeling, picture[k]));
		}
		
		//get row labels
		std::vector<std::vector<int>> rowLabels(pictureSize);
		
		for(int k=0; k<pictureSize; k++)
		{
			rowLabels[k] = rowResults[k].get();
		}
		
		//counting labels of columns (or rows of transposed matrix) with pictureSize threads
		std::vector<std::vector<Pixel>> transposed = Transpose(picture);
		std::vector<std::future<std::vector<int>>> colResults;
		for(unsigned int k=0; k<pictureSize; k++)
		{
			colResults.push_back(std::async(std::launch::async, Labeling, transposed[k]));
		}
		
		//get columns labels
		std::vector<std::vector<int>> colLabels(pictureSize);
		for(int k=0; k<pictureSize; k++)
		{
			colLabels[k] = colResults[k].get();
		}
		
		//sending back to master...
		
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
		
		for(int y=0; y<pictureSize; y++)
		{
			int actSize = rowLabels[y].size();
			pvm_pkint(&actSize, 1, 1);
			pvm_pkint(rowLabels[y].data(), rowLabels[y].size(), 1);
		}
		
		for(int y=0; y<pictureSize; y++)
		{
			int actSize = colLabels[y].size();
			pvm_pkint(&actSize, 1, 1);
			pvm_pkint(colLabels[y].data(), colLabels[y].size(), 1);
		}
		pvm_send(to, 6);
	}
	
	pvm_exit();
	
	return 0;
}
