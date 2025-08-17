#include "main.h"
#include <iostream>

using namespace std;

unsigned int width = 1920;   // example, set properly in your code
unsigned int height = 1080;  // example, set properly in your code
string directory = "./";     // output folder

int generatePNG(vector<unsigned char> image, string outputName)
{
    const string outputPath = directory + outputName;

    // image = RGBA bytes (width * height * 4)
    cv::Mat rgba((int)height, (int)width, CV_8UC4, image.data());

    // OpenCV expects BGR or BGRA, convert RGBA -> BGRA
    cv::Mat bgra;
    cv::cvtColor(rgba, bgra, cv::COLOR_RGBA2BGRA);

    // Write PNG
    vector<int> params{ cv::IMWRITE_PNG_COMPRESSION, 3 };
    if (!cv::imwrite(outputPath, bgra, params)) {
        cout << "Error encoding PNG with OpenCV\n";
        return 1;
    }
    return 0;
}

int main()
{
    // test: create a dummy red image
    vector<unsigned char> img(width * height * 4, 255);
    for (size_t i = 0; i < img.size(); i += 4) {
        img[i + 0] = 0;   // R
        img[i + 1] = 0;   // G
        img[i + 2] = 255; // B
        img[i + 3] = 255; // A
    }

    generatePNG(img, "test.png");
    return 0;
}
