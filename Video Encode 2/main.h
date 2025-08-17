#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

using namespace std;

// global variables defined somewhere else
extern unsigned int width;
extern unsigned int height;
extern string directory;

// function declarations
int generatePNG(vector<unsigned char> image, string outputName);
