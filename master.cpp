#include <fstream> // File operations
#include <vector> // Dynamic data storage
#include <stdlib.h>
#include <iostream>
#include "pvm3.h" // Parallel API
#include "pixel.h" //struct Pixel

//Main entry point
int main(int argc, char** argv)
{
	int ratio = 100/atoi(argv[1]);
	int masterTid = pvm_mytid();
	
	//spawn children
	int childrenTids[3];
	
	int started = pvm_spawn("first", (char**)0, 0, "", 1, &childrenTids[0]);
	started += pvm_spawn("second", (char**)0, 0, "", 1, &childrenTids[1]);
	started += pvm_spawn("third", (char**)0, 0, "", 1, &childrenTids[2]);
	
	if(started != 3)
	{
		for(int i=0; i<started; i++)
			pvm_kill(childrenTids[i]);
		
		pvm_exit();
		return -1;
	}
	
	//opening input + reading number of pictures	
	std::ifstream input(argv[2]);
	if(input.fail())
		return -1;

	int numOfPics;
	input >> numOfPics;
	
	//sending necessary data to children
	int from = masterTid;
	int to = childrenTids[1];
	
	//message to first
	pvm_initsend(PvmDataDefault);
	
	pvm_pkint(&from, 1, 1);
	pvm_pkint(&to, 1, 1);
	pvm_pkint(&numOfPics, 1, 1);
	pvm_pkint(&ratio, 1, 1);
	
	pvm_send(childrenTids[0], 1);
	
	//message to second
	from = childrenTids[0];
	to = childrenTids[2];
	
	pvm_initsend(PvmDataDefault);
	
	pvm_pkint(&from, 1, 1);
	pvm_pkint(&to, 1, 1);
	pvm_pkint(&numOfPics, 1, 1);
	
	pvm_send(childrenTids[1], 2);
	
	//message to third
	from = childrenTids[1];
	to = masterTid;
	
	pvm_initsend(PvmDataDefault);
	
	pvm_pkint(&from, 1, 1);
	pvm_pkint(&to, 1, 1);
	pvm_pkint(&numOfPics, 1, 1);
	
	pvm_send(childrenTids[2], 3);
	
	//reading input + sending to "first"
	
	for(int i=0; i<numOfPics; i++)
	{
		pvm_initsend(PvmDataDefault);
		
		int pictureSize;
		input >> pictureSize;
		pvm_pkint(&pictureSize, 1, 1);
		int rowLength = pictureSize * 3;
		
		for(int j=0; j < pictureSize; j++)
		{
			std::vector<int> pixRow(rowLength);
			for(int k=0; k<rowLength; k++)
			{
				input >> pixRow[k];			
			}
			pvm_pkint(pixRow.data(), rowLength, 1);
		}
		pvm_send(childrenTids[0], 1);
	}
	input.close();
	
	//receiving results and writing into file
	std::ofstream output(argv[3]);
	
	for(int i=0; i<numOfPics; i++)
	{
		pvm_recv(childrenTids[2], 6);
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
		
		std::vector<std::vector<int>> rowLabels(pictureSize);
		for(int k=0; k<pictureSize; k++)
		{
			int actSize;
			pvm_upkint(&actSize, 1, 1);
			rowLabels[k].resize(actSize);
			pvm_upkint(rowLabels[k].data(), actSize, 1);
		}
		
		std::vector<std::vector<int>> colLabels(pictureSize);
		for(int k=0; k<pictureSize; k++)
		{
			int actSize;
			pvm_upkint(&actSize, 1, 1);
			colLabels[k].resize(actSize);
			pvm_upkint(colLabels[k].data(), actSize, 1);
		}
		
		for(int k=0; k<picture.size(); k++)
		{
			for(int l=0; l<picture[k].size(); l++)
			{
				output<<"("<<picture[k][l].R<<","<<picture[k][l].G<<","<<picture[k][l].B<<")"<<" ";
			}
			output<<std::endl;
		}
		
		for(int k=0; k<rowLabels.size(); k++)
		{
			for(int l=0; l<rowLabels[k].size(); l++)
			{
				output << rowLabels[k][l] << " ";
			}
			output << std::endl;
		}
		
		for(int k=0; k<colLabels.size(); k++)
		{
			for(int l=0; l<colLabels[k].size(); l++)
			{
				output << colLabels[k][l] << " ";
			}
			output << std::endl;
		}
	}
	
	
	pvm_exit();
	
	return 0;
}