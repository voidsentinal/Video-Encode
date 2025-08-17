#include "main.h"
#include <iostream>
#include <vector>
#include "lodepng.h"
#include <fstream>
#include <bitset>
#include <opencv2/opencv.hpp>
#include <filesystem>
#ifdef _WIN32
#include "win_pick.h"   // ← file/folder pickers (Windows)
#endif

using namespace std;
using namespace cv;

// --- Paths & global state ---
const string directory = "images/";
const string outputDirectory = "output_images/";

// 4K frames
unsigned int width  = 3840;
unsigned int height = 2160;

// Grid size and derived capacity (unchanged logic)
unsigned int pixelSize = 4;
unsigned const int numBytes = ((width / pixelSize) * (height / pixelSize)) / 4;

bool endFile = false;
unsigned int numPNG = 0;

// runtime-selected paths (no extension restrictions)
static string g_inputFilePath;       // file to encode  OR encoded video to decode
static string g_outputVideoPath;     // where to save encoded .mp4
static string g_decodedOutputPath;   // where to save decoded bytes
const int framesPerImage = 1;
const int tolerance = 150;

// -------- prototypes --------
int generatePNG(vector<unsigned char> image, string outputName);
vector<unsigned char> generateImageArray(vector<unsigned char> bytes);
vector<unsigned char> getNthSet(unsigned int n, string fileName);
void generateVideo(const string& outputVideoPath);
void encode();
void decode();
void generatePNGSequence(string videoPath);
vector<unsigned char> PNGToData(string pngImagePath);
void appendBytesToFile(const vector<unsigned char>& bytes, const string& filename);

/**
* Main (keeps your e/d/b prompt; functions will pop pickers for paths)
*/
int main() {
    char input;
    cout << "Do you want to encode (e) or decode (d) or both (b): ";
    input = 'd';
    cin >> input;

    if (input == 'e') {
        encode();
    }
    else if (input == 'd') {
        decode();
    }
    else if (input == 'b') {
        encode();
        decode();
    }
    else {
        cout << "Invalid input" << endl;
    }
    return 0;
}

/**
* Generates a PNG from an image vector as outputName
*/
int generatePNG(vector<unsigned char> image, string outputName)
{
    const string outputPath = directory + outputName;
    if (lodepng::encode(outputPath, image, width, height) != 0) {
        cout << "Error encoding PNG" << endl;
        return 1;
    }
    return 0;
}

/**
* Generates an image array from a vector of bytes (your original 2-bits per cell coloring)
*/
vector<unsigned char> generateImageArray(vector<unsigned char> bytes) {
    vector<unsigned char> image(width * height * 4);
    int gridPos = 0;
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            unsigned int pixelIndex = (y * width + x) * 4;

            // position in logical grid (pixelSize x pixelSize cells)
            gridPos = (x / pixelSize) + (width / pixelSize) * (y / pixelSize);

            int bits = 0;
            if ((gridPos / 4) < bytes.size()) {
                bits = bytes[gridPos / 4] >> (6 - ((gridPos % 4) * 2)) & 3;
                switch (bits) {
                case 0: // black
                    image[pixelIndex] = 0;   image[pixelIndex + 1] = 0;   image[pixelIndex + 2] = 0;   image[pixelIndex + 3] = 255; break;
                case 1: // red
                    image[pixelIndex] = 255; image[pixelIndex + 1] = 0;   image[pixelIndex + 2] = 0;   image[pixelIndex + 3] = 255; break;
                case 2: // green
                    image[pixelIndex] = 0;   image[pixelIndex + 1] = 255; image[pixelIndex + 2] = 0;   image[pixelIndex + 3] = 255; break;
                case 3: // blue
                    image[pixelIndex] = 0;   image[pixelIndex + 1] = 0;   image[pixelIndex + 2] = 255; image[pixelIndex + 3] = 255; break;
                }
            } else {
                // white padding = end
                image[pixelIndex] = 255; image[pixelIndex + 1] = 255; image[pixelIndex + 2] = 255; image[pixelIndex + 3] = 255;
            }
        }
    }
    return image;
}

/**
* Returns the nth set of bytes of size numBytes (number of bytes that fit in one PNG grid)
*/
vector<unsigned char> getNthSet(unsigned int n, string fileName) {
    unsigned int bytesToUse = numBytes;
    vector<unsigned char> result;

    ifstream file(fileName, ios::binary);
    if (!file) {
        cerr << "Failed to open file: " << fileName << endl;
        return result;
    }

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    streampos startPos = n * bytesToUse;

    if (static_cast<streampos>(startPos) + static_cast<streampos>(bytesToUse) > fileSize) {
        bytesToUse = fileSize - startPos;
        endFile = true;
    }

    file.seekg(startPos, ios::beg);

    vector<unsigned char> buffer(bytesToUse);
    file.read(reinterpret_cast<char*>(buffer.data()), bytesToUse);
    if (!file) {
        cerr << "Failed to read bytes from file." << endl;
        return result;
    }

    result.assign(buffer.begin(), buffer.end());
    return result;
}

/**
* Stitches generated PNGs into a video at 30fps to the given output path
*/
void generateVideo(const string& outputVideoPath) {
    cv::VideoWriter video(outputVideoPath, cv::VideoWriter::fourcc('m','p','4','v'), 30, cv::Size(width, height));

    if (!video.isOpened()) {
        cerr << "Failed to create video file: " << outputVideoPath << endl;
        return;
    }

    cv::Mat frame;
    for (unsigned int i = 0; i < numPNG; i++) {
        string imagePath = directory + to_string(i) + ".png";
        frame = cv::imread(imagePath);
        if (frame.empty()) break;
        for (int j = 0; j < framesPerImage; j++) video.write(frame);
    }

    video.release();
    cout << "Video created successfully: " << outputVideoPath << endl;
}

/**
* Encode: choose input file (ANY type), then choose where to save encoded .mp4
*/
void encode() {
    // reset state per run
    endFile = false;
    numPNG = 0;

#ifdef _WIN32
    if (g_inputFilePath.empty()) {
        if (!pickOpenFile(g_inputFilePath, L"Select a file to ENCODE")) { cout << "Canceled.\n"; return; }
    }
    if (g_outputVideoPath.empty()) {
        string picked;
        if (pickSaveFile(picked, L"Save encoded video as", L"encoded.mp4")) g_outputVideoPath = picked;
        else g_outputVideoPath = g_inputFilePath + ".encoded.mp4";
    }
#endif

    // clean/prepare PNG output dir
    filesystem::remove_all(directory);
    filesystem::create_directory(directory);

    // chop input into PNGs
    while (!endFile) {
        string outputName = to_string(numPNG) + ".png";
        vector<unsigned char> bytes = getNthSet(numPNG, g_inputFilePath);
        vector<unsigned char> image = generateImageArray(bytes);
        generatePNG(image, outputName);
        numPNG++;
    }
    cout << numPNG << " images saved" << endl;

    // stitch to video
    generateVideo(g_outputVideoPath);
}

/**
* Decode: choose encoded video (.mp4), then choose where to save the recovered file (ANY extension)
*/
void decode() {
    // reset
    numPNG = 0;

#ifdef _WIN32
    if (g_inputFilePath.empty()) {
        if (!pickOpenFile(g_inputFilePath, L"Select encoded .mp4 to DECODE")) { cout << "Canceled.\n"; return; }
    }
    if (g_decodedOutputPath.empty()) {
        string picked;
        if (pickSaveFile(picked, L"Save decoded file as", L"decoded.bin")) g_decodedOutputPath = picked;
        else g_decodedOutputPath = g_inputFilePath + ".decoded.bin";
    }
#endif

    // remove any previous PNGs
    filesystem::remove_all(outputDirectory);

    // explode video into PNG sequence (updates numPNG)
    generatePNGSequence(g_inputFilePath);

    // reconstruct original bytes
    for (unsigned int i = 0; i < numPNG; i++) {
        string picName = outputDirectory + to_string(i) + ".png";
        vector<unsigned char> bytes = PNGToData(picName);
        appendBytesToFile(bytes, g_decodedOutputPath);
    }
}

/**
* Split a video into sequential PNG frames into outputDirectory
*/
void generatePNGSequence(string videoPath) {
    VideoCapture video(videoPath);
    if (!video.isOpened()) {
        cerr << "Error opening video file: " << videoPath << endl;
        return;
    }

    int frameCount = static_cast<int>(video.get(CAP_PROP_FRAME_COUNT));
    int frameNumber = 0;

    filesystem::create_directory(outputDirectory);

    while (frameNumber < frameCount) {
        Mat frame;
        if (!video.read(frame)) {
            cerr << "Error reading frame " << frameNumber << " from video." << endl;
            break;
        }
        string outputName = outputDirectory + to_string(frameNumber) + ".png";
        if (!imwrite(outputName, frame)) {
            cerr << "Error saving frame " << frameNumber << " as PNG." << endl;
        }
        frameNumber++;
    }

    video.release();
    cout << "PNG sequence generated successfully. Total frames: " << frameNumber << endl;
    numPNG = frameNumber;
}

/**
* Convert a single PNG back to bytes (your original color mapping)
*/
vector<unsigned char> PNGToData(string pngImagePath) {
    vector<unsigned char> bytes;
    unsigned char byte = 0;
    Mat image = imread(pngImagePath);
    if (image.empty()) {
        cerr << "Error reading image: " << pngImagePath << endl;
        return bytes;
    }

    Vec3b color;
    int byteCounter = 0;
    for (int y = pixelSize / 2; y < image.rows; y += pixelSize) {
        for (int x = pixelSize / 2; x < image.cols; x += pixelSize) {
            color = image.at<Vec3b>(y, x);
            byte = byte << 2;

            // White marks end-of-file
            if (static_cast<int>(color[0]) > tolerance &&
                static_cast<int>(color[1]) > tolerance &&
                static_cast<int>(color[2]) > tolerance) {
                return bytes;
            }
            // Red 01
            else if (static_cast<int>(color[2]) > tolerance) { byte |= 1; }
            // Green 11
            else if (static_cast<int>(color[0]) > tolerance) { byte |= 3; }
            // Blue 10
            else if (static_cast<int>(color[1]) > tolerance) { byte |= 2; }
            // Black 00 (implicit)

            if (byteCounter == 3) {
                bytes.push_back(byte);
                byteCounter = 0;
            } else {
                byteCounter++;
            }
        }
    }
    return bytes;
}

/**
* Append bytes to end of a file (create if missing)
*/
void appendBytesToFile(const vector<unsigned char>& bytes, const string& filename) {
    ofstream file(filename, ios::binary | ios::app);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!file) {
        cerr << "Error writing bytes to file: " << filename << endl;
    }
    file.close();
}
