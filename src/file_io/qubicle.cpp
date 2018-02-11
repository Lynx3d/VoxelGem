/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../voxelscene.h"
#include "../voxelgrid.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <iostream>

union rgba_t
{
	uint32_t raw;
	struct
	{
		uint8_t r, g, b, a;
	};
	char bytes[4];
	rgba_t() {}
	rgba_t(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4):
		r(b1), g(b2), b(b3), a(b4) {}
};


class SceneOp
{
	public:
		SceneOp(VoxelScene &lscene): scene(lscene) {}
		virtual void operator()(rgba_t data, int x, int y, int z) = 0;
	protected:
		VoxelScene &scene;
};

class BaseColOp: public SceneOp
{
	public:
		BaseColOp(VoxelScene &lscene): SceneOp(lscene) {}
		void operator()(rgba_t data, int x, int y, int z) override
		{
			if (data.a)
			{
				VoxelEntry voxel(data.raw, Voxel::VF_NON_EMPTY);
				int pos[3] = { x, y, z };
				scene.setVoxel(pos, voxel);
			}
		}
};

void parse_file(QDataStream &fstream, SceneOp &dataOp)
{
	// Qubicle files are little endian, and this code currently only works on little endian arch too...
	fstream.setByteOrder(QDataStream::LittleEndian);
	const rgba_t CODEFLAG(2, 0, 0, 0);
	const rgba_t NEXTSLICEFLAG(6, 0, 0, 0);
	uint32_t version, colorFormat, zAxisOrientation, compressed, visibilityMaskEncoded, numMatrices;
	fstream >> version;
	fstream >> colorFormat;
	fstream >> zAxisOrientation;
	fstream >> compressed;
	fstream >> visibilityMaskEncoded;
	fstream >> numMatrices;
	rgba_t data;
	for (uint32_t i=0; i < numMatrices; ++i)
	{
		uint8_t nameLength;
		char nameBuff[257];
		fstream >> nameLength;
		fstream.readRawData(nameBuff, nameLength);
		nameBuff[nameLength] = '\0';
		std::cout << "loading layer '" << nameBuff << "' " << i+1 << "/" << numMatrices << std::endl;

		uint32_t sizeX, sizeY, sizeZ;
		fstream >> sizeX >>  sizeY >> sizeZ;

		uint32_t  posX, posY, posZ;
		fstream >> posX >> posY >> posZ;

		if (compressed == 0) // uncompressd
		{
			for (uint32_t z = 0; z < sizeZ; z++)
				for (uint32_t y = 0; y < sizeY; y++)
					for (uint32_t x = 0; x < sizeX; x++)
					{
						fstream.readRawData(data.bytes, 4);
						dataOp(data, x, y, z);
					}
		}
		else // RLE compressed
		{
			std::cout << "RLE compressed\n";
			uint32_t runLength, index;
			for (uint32_t z = 0; z < sizeZ; ++z)
			{
				index = 0;
				while (true)
				{
					fstream.readRawData(data.bytes, 4);
					if (data.raw == NEXTSLICEFLAG.raw)
						break;
					runLength = 1;
					if (data.raw == CODEFLAG.raw)
					{
						fstream >> runLength;
						fstream.readRawData(data.bytes, 4);
					}
					for (uint32_t j = 0; j < runLength; ++j)
					{
						uint32_t x = index % sizeX;
						uint32_t y = index / sizeX;
						++index;
						dataOp(data, posX + x, posY + y, posZ + z);
					}
				}
			}
		}
	}
}

void qubicle_import(const QString &filename, VoxelScene &scene)
{
	QFileInfo file_info(filename);
	std::cout << "importing " << filename.toStdString() << std::endl;
	if (!file_info.isReadable())
		return;
	QFile file(file_info.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return;
	QDataStream fstream(&file);
	BaseColOp colOp(scene);
	parse_file(fstream, colOp);
	// Trove specific:
	// check if we have files with _t, _a and _s suffix for material properties
	QString base = file_info.completeBaseName();
	QString suffix = file_info.suffix();
	QFileInfo typemap_info(file_info.dir(), base + "_t." + suffix);
	std::cout << "looking for " << typemap_info.filePath().toStdString() << std::endl;
	if (typemap_info.isReadable())
	{
		std::cout << "reading type map '" << typemap_info.filePath().toStdString() << "'\n";
	}

}
