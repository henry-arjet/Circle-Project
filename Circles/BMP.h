#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string>




struct BMP {
#pragma pack(push, 1)
	struct FileHeader {
		uint16_t fileType{ 0x4D42 };
		uint32_t fileSize{ 0 };
		uint16_t reserved1{ 0 };
		uint16_t reserved2{ 0 };
		uint32_t offsetData{ 0 };
	};
#pragma pack(pop)
	struct InfoHeader {
		uint32_t size{ 0 }; //of the header
		int32_t width{ 0 };
		int32_t height{ 0 };
		uint16_t planes{ 1 };
		uint16_t bitCount{ 0 }; //bits per pixel
		uint32_t compression{ 0 };
		uint32_t sizeImage{ 0 };
		int32_t xPixelsPerInch{ 0 };
		int32_t yPixelsPerInch{ 0 };
		uint32_t colorsUsed{ 0 };
		uint32_t colorsImportant{ 0 };
	};

	struct ColorHeader {
		uint32_t alphaMask{ 0xff000000 };
		uint32_t redMask{ 0x00ff0000 };
		uint32_t greenMask{ 0x0000ff00 };
		uint32_t blueMask{ 0x000000ff };
		uint32_t colorSpaceType{ 0x73524742 }; //defaults to sRGB
		uint32_t unused[16]{ 0 };
	};

	FileHeader fileHeader;
	InfoHeader infoHeader;
	ColorHeader colorHeader;
	std::vector<uint8_t> data;
	uint32_t rowStride{ 0 };

	BMP(int32_t width, int32_t height, bool hasAlpha = false) {
		infoHeader.width = width;
		infoHeader.height = height;
		if (hasAlpha) {
			infoHeader.size = sizeof(InfoHeader) + sizeof(ColorHeader);
			fileHeader.offsetData = sizeof(FileHeader) + sizeof(InfoHeader) + sizeof(ColorHeader);

			infoHeader.bitCount = 32;
			infoHeader.compression = 3;
			rowStride = width * 4;
			data.resize(rowStride * abs(height));
			fileHeader.fileSize = fileHeader.offsetData + data.size();
		}

		else {
			infoHeader.size = sizeof(InfoHeader);
			fileHeader.offsetData = sizeof(FileHeader) + sizeof(InfoHeader);

			infoHeader.bitCount = 24;
			infoHeader.compression = 0;
			rowStride = width * 3;
			data.resize(rowStride * abs(height));

			uint32_t newStride = makeStrideAligned(4);
			fileHeader.fileSize = fileHeader.offsetData + data.size() + height * (newStride - rowStride);
		}
	};

	void write(std::string fname) {
		std::ofstream of{ fname, std::ios_base::binary };
		if (!of ) throw std::runtime_error("Unable to open output image file");
		else {
			if (infoHeader.bitCount == 32) {
				writeHeadersAndData(of);
			}
			else if (infoHeader.bitCount == 24) {
				if (infoHeader.width % 4 == 0) {
					writeHeadersAndData(of);
				}
				else {
					uint32_t newStride = makeStrideAligned(4);
					std::vector<uint8_t> paddingRow(newStride - rowStride);
					writeHeaders(of);

					
					for (int y = 0; y < abs(infoHeader.height); ++y) {
						of.write((const char*)(data.data() + rowStride * y), rowStride);
						of.write((const char*)paddingRow.data(), paddingRow.size());
					}
					
				}
			}
			else throw std::runtime_error("Must be 24 or 32 bits per pixel");
			

		}
	}
private:
	void writeHeaders(std::ofstream &of) {
		of.write((const char*)&fileHeader, sizeof(fileHeader));
		of.write((const char*)&infoHeader, sizeof(infoHeader));
		if (infoHeader.bitCount == 32) {
			of.write((const char*)&colorHeader, sizeof(colorHeader));
		}
	}

	void writeHeadersAndData(std::ofstream &of) {
		writeHeaders(of);
		of.write((const char*)data.data(), data.size());
	}


	uint32_t makeStrideAligned(uint32_t alignStride) {
		uint32_t newStride = rowStride;
		while (newStride % alignStride != 0) {
			newStride++;
		}
		return newStride;
	}
};