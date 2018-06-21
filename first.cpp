#include <vector> // Dynamic data storage
#include <iostream>
#include <math.h>
#include <future>
#include "pvm3.h" // Parallel API
#include "pixel.h"

const std::vector<std::vector<Pixel> > SubPicture(const std::vector<std::vector<Pixel> > &m, int rbegin, int cbegin)
{
    std::vector<std::vector<Pixel> > result(m.size()/2);
    for(unsigned int i=0; i < result.size(); i++)
        result[i].resize(result.size());

    int reinit = cbegin;

    for(unsigned int i=0; i < result.size(); i++)
    {
        for(unsigned int j=0; j < result.size(); j++)
        {
            result[i][j] = m[rbegin][cbegin];
            cbegin++;
        }
        rbegin++;
        cbegin = reinit;
    }

    return result;
}

const Pixel Average(const std::vector<Pixel>& v)
{
	int R, G, B;
	R = G = B = 0;
	for(unsigned int i=0; i<v.size(); i++)
	{
		R += v[i].R;
		G += v[i].G;
		B += v[i].B;
	}
	
	R = round(R / v.size());
	G = round(G / v.size());
	B = round(B / v.size());
	
	Pixel px(R, G, B);
	
	return px;
}

std::vector<std::vector<Pixel>> MergePictures(const std::vector<std::vector<Pixel> > &m1, const std::vector<std::vector<Pixel> > &m2, const std::vector<std::vector<Pixel> > &m3, const std::vector<std::vector<Pixel> > &m4)
{
	std::vector<std::vector<Pixel>> res(m1.size()*2);
    for(unsigned int i=0; i<res.size(); i++)
		res[i].resize(res.size());
	
	int x = res.size()/2;
    for(int i=0; i<x; i++)
        for(int j=0; j<x; j++)
        {
            res[i][j] = m1[i][j];
            res[i][x+j] = m2[i][j];
            res[x+i][j] = m3[i][j];
            res[x+i][x+j] = m4[i][j];
        }
		
	return res;
}

std::vector<std::vector<Pixel>> Compress(std::vector<std::vector<Pixel>> &v, int ratio)
{
	unsigned int newSize = v.size()/ratio;
	std::vector<std::vector<Pixel>> result(newSize);
	
	for(unsigned int i=0; i < newSize; i++)
        result[i].resize(newSize);
	
	for(unsigned int i=0; i<newSize; i++)
	{
		for(unsigned int j=0; j<newSize; j++)
		{
			std::vector<Pixel> avg;
			for(int k=0; k<ratio; k++)
			{
				for(int l=0; l<ratio; l++)
				{
					avg.push_back(v[i*ratio + k][j*ratio + l]);
				}
			}
			result[i][j] = Average(avg);
		}
	}
	
	return result;
}

std::vector<std::vector<Pixel>> RecCompress(std::vector<std::vector<Pixel>> &v, int ratio, bool flag)
{
	if(v.size() < 17 || flag)
	{
		return Compress(v, ratio);
	}else
	{
		std::vector<std::vector<Pixel>> finalImg;
		int subSize = v.size()/2;
		
		std::vector<std::vector<Pixel>> sub1 = SubPicture(v, 0, 0);
		std::vector<std::vector<Pixel>> sub2 = SubPicture(v, 0, subSize);
		std::vector<std::vector<Pixel>> sub3 = SubPicture(v, subSize, 0);
		std::vector<std::vector<Pixel>> sub4 = SubPicture(v, subSize, subSize);
		
		std::future<std::vector<std::vector<Pixel>>> res1(std::async(std::launch::async, RecCompress, std::ref(sub1), ratio, true));
		std::future<std::vector<std::vector<Pixel>>> res2(std::async(std::launch::async, RecCompress, std::ref(sub2), ratio, true));
		std::future<std::vector<std::vector<Pixel>>> res3(std::async(std::launch::async, RecCompress, std::ref(sub3), ratio, true));
		std::future<std::vector<std::vector<Pixel>>> res4(std::async(std::launch::async, RecCompress, std::ref(sub4), ratio, true));
		
		sub1 = res1.get();
		sub2 = res2.get();
		sub3 = res3.get();
		sub4 = res4.get();
		
		return MergePictures(sub1, sub2, sub3, sub4);
	}
}


// Main entry point of child
int main()
{	int parent_id = pvm_parent();	
//	std::vector<std::vector<std::vector<Pixel>>> proba;

	//receiving data
	pvm_recv(parent_id, 1);

	int from, to, numOfPics, ratio;
	
	pvm_upkint(&from, 1, 1);
	pvm_upkint(&to, 1, 1);
	pvm_upkint(&numOfPics, 1, 1);
	pvm_upkint(&ratio, 1, 1);
	
	//receiving pictures
	
	for(int i=0; i<numOfPics; i++)
	{
		pvm_recv(parent_id, 1);
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
		
		//resize
		picture = RecCompress(picture, ratio, false);
		
		//sending towards result
		pictureSize = picture.size();
		int red, green, blue;
		pvm_initsend(PvmDataDefault);
		pvm_pkint(&pictureSize, 1, 1);
		for(unsigned int k=0; k<picture.size(); k++)
		{
			for(unsigned int l=0; l<picture.size(); l++)
			{
				red = picture[k][l].R;
				green = picture[k][l].G;
				blue = picture[k][l].B;
			
				pvm_pkint(&red, 1, 1);
				pvm_pkint(&green, 1, 1);
				pvm_pkint(&blue, 1, 1);
			}
			
		}
		pvm_send(to, 4);	
	}
	
	
	pvm_exit();
	
	return 0;
}
